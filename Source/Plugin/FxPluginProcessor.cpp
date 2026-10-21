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
}

void FxPluginProcessor::prepareToPlay(double sampleRate, int)
{
    gain.reset(sampleRate, kGainRampSeconds);
    gain.setCurrentAndTargetValue(targetGain(outputGainDb->load(), bypassParameter->get()));
    setLatencySamples(0);
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

    gain.setTargetValue(targetGain(outputGainDb->load(std::memory_order_relaxed), bypassParameter->get()));

    if (gain.isSmoothing())
    {
        // One gain value per sample frame, identical for every channel.
        auto* const* data = buffer.getArrayOfWritePointers();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = gain.getNextValue();
            for (int ch = 0; ch < channels; ++ch)
                data[ch][i] *= g;
        }
    }
    else if (const float g = gain.getTargetValue(); g != 1.0f)
    {
        for (int ch = 0; ch < channels; ++ch)
            buffer.applyGain(ch, 0, numSamples, g);
    }
    // g == 1 and not ramping: the buffer is untouched, so pass-through and bypass are bit-exact.

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
