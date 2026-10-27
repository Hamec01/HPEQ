#include "Plugin/HamloCompProcessor.h"

#include "Plugin/HamloCompEditor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace hamlo::comp
{
namespace
{
constexpr double kRampSeconds = 0.020; // MIX and ON fades (HPEQ: kMixRampSeconds and the bypass ramp)

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& p : parameterSpecs())
    {
        const juce::ParameterID id { p.id, 1 };
        if (!p.choices.isEmpty())
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(id, p.name, p.choices, juce::roundToInt(p.defaultValue)));
            continue;
        }
        juce::NormalisableRange<float> range(p.minimum, p.maximum, 0.0f, p.skew);
        layout.add(std::make_unique<juce::AudioParameterFloat>(id, p.name, range, p.defaultValue,
                                                               juce::AudioParameterFloatAttributes {}.withLabel(p.unit)));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}
} // namespace

HamloCompProcessor::HamloCompProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier(kStateTag), createLayout())
{
    for (const auto& p : parameterSpecs())
    {
        raw.push_back(parameters.getRawParameterValue(p.id));
        jassert(raw.back() != nullptr);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
}

dsp::CompressorSettings HamloCompProcessor::currentSettings() const noexcept
{
    const auto v = [this](int i) { return raw[static_cast<size_t>(i)]->load(std::memory_order_relaxed); };
    dsp::CompressorSettings s;
    s.thresholdDb = v(0);
    s.ratio = v(1);
    s.kneeDb = juce::jmin(24.0f, v(2)); // as HPEQ's HamloComp: the page shows 0..40, the law uses at most 24
    s.attackMs = v(3);
    s.releaseMs = v(4);
    s.makeupDb = v(5);
    s.detector = v(7);
    s.character = v(8);
    s.autoMakeup = v(9);
    return s;
}

void HamloCompProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    compressor.prepare(sampleRate);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    rampSteps = dsp::stepsFor(kRampSeconds, sampleRate);
    mixAmount.reset(juce::jlimit(0.0, 1.0, static_cast<double>(raw[6]->load(std::memory_order_relaxed))));
    onAmount.reset(isOn() ? 1.0 : 0.0);
    setLatencySamples(0);
}

bool HamloCompProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void HamloCompProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputs = getTotalNumInputChannels();
    const int outputs = getTotalNumOutputChannels();
    const int samples = buffer.getNumSamples();
    for (int channel = inputs; channel < outputs; ++channel)
        buffer.clear(channel, 0, samples);
    if (samples <= 0)
        return;

    const int channels = juce::jmin(inputs, outputs, buffer.getNumChannels(), dsp::kMaxChannels);
    const auto settings = currentSettings();
    mixAmount.setTarget(juce::jlimit(0.0, 1.0, static_cast<double>(raw[6]->load(std::memory_order_relaxed))), rampSteps);
    onAmount.setTarget(isOn() ? 1.0 : 0.0, rampSteps);

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        const float* dryPointers[dsp::kMaxChannels] {};
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            auto* d = dry.getWritePointer(channel);
            juce::FloatVectorOperations::copy(d, wet[channel], count);
            dryPointers[channel] = d;
        }
        compressor.process(wet, channels, count, settings, true);
        for (int i = 0; i < count; ++i)
        {
            const auto m = static_cast<float>(mixAmount.next());
            const auto on = static_cast<float>(onAmount.next());
            for (int channel = 0; channel < channels; ++channel)
            {
                const float d = dryPointers[channel][i];
                // MIX as HPEQ's module slot, then the ON crossfade; both exact at 0 and 1
                const float mixed = m == 1.0f ? wet[channel][i] : (m == 0.0f ? d : d + m * (wet[channel][i] - d));
                wet[channel][i] = on == 1.0f ? mixed : (on == 0.0f ? d : d + on * (mixed - d));
            }
        }
    }
}

juce::AudioProcessorEditor* HamloCompProcessor::createEditor()
{
    return new HamloCompEditor(*this);
}

void HamloCompProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void HamloCompProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::RangedAudioParameter* HamloCompProcessor::parameter(const juce::String& id) const noexcept
{
    return parameters.getParameter(id);
}

double HamloCompProcessor::parameterValue(const juce::String& id) const noexcept
{
    if (const auto* p = parameter(id))
        return p->convertFrom0to1(p->getValue());
    return 0.0;
}

void HamloCompProcessor::setParameterValue(const juce::String& id, double actualValue)
{
    if (auto* p = parameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(actualValue)));
}

int HamloCompProcessor::getNumPrograms()
{
    return 1 + static_cast<int>(factoryPresets().size());
}

const juce::String HamloCompProcessor::getProgramName(int index)
{
    const auto& list = factoryPresets();
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    return list[static_cast<size_t>(index - 1)].name;
}

void HamloCompProcessor::setCurrentProgram(int index)
{
    const auto& list = factoryPresets();
    currentProgram = juce::jlimit(0, static_cast<int>(list.size()), index);
    for (const auto& spec : parameterSpecs())
        setParameterValue(spec.id, currentProgram > 0 ? presetValue(list[static_cast<size_t>(currentProgram - 1)], spec.id, spec.presetBase)
                                                      : spec.defaultValue);
}
} // namespace hamlo::comp
