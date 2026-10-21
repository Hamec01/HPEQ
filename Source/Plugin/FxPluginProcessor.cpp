#include "Plugin/FxPluginProcessor.h"

#include <cmath>

#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginEditor.h"

namespace hpeq
{
namespace
{
const juce::Identifier kStateRoot { "HPEQ" };
const juce::Identifier kStateSchema { "schemaVersion" };
const juce::Identifier kParametersTree { "Parameters" };

constexpr double kGainRampSeconds = 0.02;

float targetGain(float gainDb, bool bypassed) noexcept
{
    return bypassed ? 1.0f : juce::Decibels::decibelsToGain(gainDb);
}
} // namespace

FxPluginProcessor::FxPluginProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, kParametersTree, params::createLayout())
{
    outputGainDb = parameters.getRawParameterValue(params::outputGain.getParamID());
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter(params::bypass.getParamID()));
    jassert(outputGainDb != nullptr && bypassParameter != nullptr);
    engine.publish(dsp::makeSnapshot(project.layers[0]));
}

void FxPluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    const int maxBlock = juce::jmax(1, maximumExpectedSamplesPerBlock);
    engine.prepare(sampleRate, maxBlock);
    dry.setSize(dsp::kMaxChannels, maxBlock);
    bypassRampSteps = dsp::stepsFor(dsp::kMixRampSeconds, sampleRate);
    wetAmount.reset(bypassParameter->get() ? 0.0 : 1.0);
    gain.reset(sampleRate, kGainRampSeconds);
    gain.setCurrentAndTargetValue(targetGain(outputGainDb->load(), bypassParameter->get()));
    setLatencySamples(engine.getLatencySamples());
}

void FxPluginProcessor::releaseResources() {}

bool FxPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    // Mono -> mono and stereo -> stereo only. Mono -> stereo needs a defined upmix rule first (01 §10).
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void FxPluginProcessor::measure(const juce::AudioBuffer<float>& buffer,
                                int numChannels,
                                std::atomic<float>& peak,
                                std::atomic<float>& rms) noexcept
{
    float blockPeak = 0.0f;
    double sumSquares = 0.0;
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float v = data[i];
            blockPeak = std::max(blockPeak, std::abs(v));
            sumSquares += static_cast<double>(v) * v;
        }
    }
    const auto count = static_cast<double>(numSamples) * juce::jmax(1, numChannels);
    peak.store(blockPeak, std::memory_order_relaxed);
    rms.store(static_cast<float>(std::sqrt(sumSquares / count)), std::memory_order_relaxed);
}

void FxPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputs = getTotalNumInputChannels();
    const int numOutputs = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    for (int ch = numInputs; ch < numOutputs; ++ch)
        buffer.clear(ch, 0, numSamples);
    if (numSamples <= 0)
        return;

    const int channels = juce::jmin(numInputs, numOutputs, buffer.getNumChannels());
    measure(buffer, channels, inputPeak, inputRms);

    const bool bypassed = bypassParameter->get();
    gain.setTargetValue(targetGain(outputGainDb->load(std::memory_order_relaxed), bypassed));
    wetAmount.setTarget(bypassed ? 0.0 : 1.0, bypassRampSteps);

    // The layer keeps running while bypassed (warm filter/detector state, no click on return); bypass
    // crossfades to the dry copy and, once settled, outputs the dry samples exactly.
    auto* const* data = buffer.getArrayOfWritePointers();
    const int capacity = dry.getNumSamples();
    if (capacity <= 0)
        return; // not prepared yet: leave the input untouched
    for (int start = 0; start < numSamples; start += capacity)
    {
        const int n = juce::jmin(capacity, numSamples - start);
        float* chunk[dsp::kMaxChannels] {};
        for (int ch = 0; ch < channels; ++ch)
        {
            chunk[ch] = data[ch] + start;
            dry.copyFrom(ch, 0, chunk[ch], n);
        }

        engine.process(chunk, channels, n);

        if (wetAmount.isRamping())
        {
            for (int i = 0; i < n; ++i)
            {
                const auto w = static_cast<float>(wetAmount.next());
                for (int ch = 0; ch < channels; ++ch)
                    chunk[ch][i] = dry.getSample(ch, i) + w * (chunk[ch][i] - dry.getSample(ch, i));
            }
        }
        else if (wetAmount.value() == 0.0)
        {
            for (int ch = 0; ch < channels; ++ch)
                std::copy(dry.getReadPointer(ch), dry.getReadPointer(ch) + n, chunk[ch]);
        }

        if (gain.isSmoothing())
        {
            // One gain value per sample frame, identical for every channel.
            for (int i = 0; i < n; ++i)
            {
                const float g = gain.getNextValue();
                for (int ch = 0; ch < channels; ++ch)
                    chunk[ch][i] *= g;
            }
        }
        else if (const float g = gain.getTargetValue(); g != 1.0f)
        {
            for (int ch = 0; ch < channels; ++ch)
                juce::FloatVectorOperations::multiply(chunk[ch], g, n);
        }
        // g == 1 and not ramping: untouched, so pass-through and bypass are bit-exact.
    }

    measure(buffer, channels, outputPeak, outputRms);
}

FxPluginProcessor::MeterSnapshot FxPluginProcessor::getMeters() const noexcept
{
    return { inputPeak.load(std::memory_order_relaxed),
             inputRms.load(std::memory_order_relaxed),
             outputPeak.load(std::memory_order_relaxed),
             outputRms.load(std::memory_order_relaxed) };
}

FxProjectState FxPluginProcessor::getProject() const
{
    const juce::ScopedLock lock(projectLock);
    return project;
}

void FxPluginProcessor::setProject(FxProjectState newProject)
{
    sanitize(newProject);
    const juce::ScopedLock lock(projectLock);
    project = std::move(newProject);
    const auto snapshot = dsp::makeSnapshot(project.layers[0]);
    engine.publish(snapshot);

    double tail = 0.0;
    for (const auto& node : snapshot.fx)
        for (int k = 0; node.active && k < node.numSlots; ++k)
            if (const auto& slot = node.slots[static_cast<size_t>(k)]; slot.active && slot.type == FxType::Reverb)
                tail = std::max(tail, dsp::ReverbProcessor::tailSeconds(slot));
    tailSeconds.store(tail, std::memory_order_relaxed);
}

void FxPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree root(kStateRoot);
    root.setProperty(kStateSchema, kStateSchemaVersion, nullptr);
    root.appendChild(parameters.copyState(), nullptr);
    root.appendChild(toValueTree(getProject()), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary(*xml, destData);
}

void FxPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr)
        return;
    const auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType(kStateRoot))
        return;
    const int version = root.getProperty(kStateSchema, 0);
    if (version < 1 || version > kStateSchemaVersion)
        return; // a newer or broken state: keep the current one rather than half-load it

    const auto projectTree = root.getChildWithName("HpeqProject");
    auto restored = fromValueTree(projectTree);
    if (!restored.has_value())
        return;

    const auto parametersTree = root.getChildWithName(kParametersTree);
    if (parametersTree.isValid())
        parameters.replaceState(parametersTree);
    setProject(std::move(*restored));
}

juce::AudioProcessorEditor* FxPluginProcessor::createEditor()
{
    return new FxPluginEditor(*this);
}
} // namespace hpeq

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hpeq::FxPluginProcessor();
}
