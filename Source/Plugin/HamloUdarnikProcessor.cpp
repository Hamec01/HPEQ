#include "Plugin/HamloUdarnikProcessor.h"

#include "Plugin/HamloUdarnikEditor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace hamlo::udarnik
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

HamloUdarnikProcessor::HamloUdarnikProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier(kStateTag), createLayout())
{
    const auto& specs = parameterSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        raw.push_back(parameters.getRawParameterValue(specs[i].id));
        jassert(raw.back() != nullptr);
        if (specs[i].slot == kMixSlot)
            mixIndex = static_cast<int>(i);
        if (juce::String(specs[i].id) == "protect")
            protectIndex = static_cast<int>(i);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
}

dsp::TransientSettings HamloUdarnikProcessor::currentSettings() const noexcept
{
    dsp::TransientSettings s;
    const auto& specs = parameterSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
        if (specs[i].slot >= 0) // MIX and PROTECT act in the plug-in
            s.values[static_cast<size_t>(specs[i].slot)] = transformed(raw[i]->load(std::memory_order_relaxed), specs[i].transform);
    return s;
}

void HamloUdarnikProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    transient.prepare(sampleRate);
    latency = dsp::UdarnikCore::lookaheadSamples(2, sampleRate); // SOYUZ, the largest look-ahead
    mixAlign.prepare(latency);
    wetAlign.prepare(latency);
    dryAlign.prepare(latency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    dryForMix.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    rampSteps = dsp::stepsFor(kRampSeconds, sampleRate);
    mixAmount.reset(juce::jlimit(0.0, 1.0, static_cast<double>(raw[static_cast<size_t>(mixIndex)]->load(std::memory_order_relaxed))));
    onAmount.reset(isOn() ? 1.0 : 0.0);
    setLatencySamples(latency);
}

bool HamloUdarnikProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void HamloUdarnikProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    const int slotLatency = dsp::TransientProcessor::lookaheadSamples(settings, currentSampleRate.load(std::memory_order_relaxed));
    const int extra = juce::jmax(0, latency - slotLatency);
    const bool protect = dsp::TransientProcessor::usesV2(settings) && raw[static_cast<size_t>(protectIndex)]->load(std::memory_order_relaxed) > 0.5f;
    mixAmount.setTarget(juce::jlimit(0.0, 1.0, static_cast<double>(raw[static_cast<size_t>(mixIndex)]->load(std::memory_order_relaxed))), rampSteps);
    onAmount.setTarget(isOn() ? 1.0 : 0.0, rampSteps);

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        float* dryPointers[dsp::kMaxChannels] {};
        float* mixDry[dsp::kMaxChannels] {};
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            dryPointers[channel] = dry.getWritePointer(channel);
            mixDry[channel] = dryForMix.getWritePointer(channel);
            juce::FloatVectorOperations::copy(dryPointers[channel], wet[channel], count);
        }
        // the slot's dry reference, delayed by the slot's own latency (HPEQ: FxNodeProcessor's slot align)
        mixAlign.process(dryPointers, mixDry, channels, count, slotLatency);
        transient.process(wet, channels, count, settings);
        for (int i = 0; i < count; ++i)
        {
            const auto m = static_cast<float>(mixAmount.next());
            for (int channel = 0; channel < channels; ++channel)
            {
                const float d = mixDry[channel][i];
                wet[channel][i] = m == 1.0f ? wet[channel][i] : (m == 0.0f ? d : d + m * (wet[channel][i] - d));
            }
        }
        // Always clock the aligners (also at zero delay), so a later look-ahead change reads continuous history.
        wetAlign.process(wet, wet, channels, count, extra);
        dryAlign.process(dryPointers, dryPointers, channels, count, latency);
        // v2 output protection (docs/UDARNIK_V2.md U7): only what the shaper added over the (aligned) input sample
        // and over -3 dBFS is soft-limited into the room up to 0 dBFS; a sample the shaper did not raise passes
        // exactly, and the output never passes 0 dBFS unless the input already did
        if (protect)
            for (int channel = 0; channel < channels; ++channel)
                for (int i = 0; i < count; ++i)
                {
                    const float v = wet[channel][i], a = std::abs(v);
                    constexpr float knee = 0.70794578f; // -3 dBFS
                    const float base = std::max(knee, std::abs(dryPointers[channel][i]));
                    if (a > base && base < 1.0f)
                    {
                        const float room = 1.0f - base;
                        wet[channel][i] = std::copysign(base + room * std::tanh((a - base) / room), v);
                    }
                }
        for (int i = 0; i < count; ++i)
        {
            const auto on = static_cast<float>(onAmount.next());
            if (on == 1.0f)
                continue; // exact at 1 (and at 0 below)
            for (int channel = 0; channel < channels; ++channel)
                wet[channel][i] = on == 0.0f ? dryPointers[channel][i] : dryPointers[channel][i] + on * (wet[channel][i] - dryPointers[channel][i]);
        }
    }
}

double HamloUdarnikProcessor::getTailLengthSeconds() const
{
    return static_cast<double>(latency) / currentSampleRate.load(std::memory_order_relaxed);
}

juce::AudioProcessorEditor* HamloUdarnikProcessor::createEditor()
{
    return new HamloUdarnikEditor(*this);
}

void HamloUdarnikProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("program", currentProgram, nullptr); // the preset screen shows it again after a reload
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void HamloUdarnikProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            // a state from before the v2 engine is CLASSIC without protection (it sounds as it did)
            bool hasEngine = false;
            for (auto* e : xml->getChildWithTagNameIterator("PARAM"))
                hasEngine = hasEngine || e->getStringAttribute("id") == "engine";
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            currentProgram = juce::jlimit(0, static_cast<int>(factoryPresets().size()), static_cast<int>(xml->getIntAttribute("program", 0)));
            if (!hasEngine)
            {
                setParameterValue("engine", 0.0);
                setParameterValue("protect", 0.0);
            }
        }
}

juce::RangedAudioParameter* HamloUdarnikProcessor::parameter(const juce::String& id) const noexcept
{
    return parameters.getParameter(id);
}

double HamloUdarnikProcessor::parameterValue(const juce::String& id) const noexcept
{
    if (const auto* p = parameter(id))
        return p->convertFrom0to1(p->getValue());
    return 0.0;
}

void HamloUdarnikProcessor::setParameterValue(const juce::String& id, double actualValue)
{
    if (auto* p = parameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(actualValue)));
}

bool HamloUdarnikProcessor::programModified() const
{
    const auto& list = factoryPresets();
    const Preset* preset = currentProgram > 0 && currentProgram <= static_cast<int>(list.size()) ? &list[static_cast<size_t>(currentProgram - 1)] : nullptr;
    for (const auto& spec : parameterSpecs())
    {
        const double expected = preset != nullptr ? presetValue(*preset, spec.id, spec.defaultValue) : spec.defaultValue;
        const double tolerance = 1.0e-3 * (spec.maximum - spec.minimum);
        if (std::abs(parameterValue(spec.id) - juce::jlimit(static_cast<double>(spec.minimum), static_cast<double>(spec.maximum), expected)) > tolerance)
            return true;
    }
    return false;
}

int HamloUdarnikProcessor::getNumPrograms()
{
    return 1 + static_cast<int>(factoryPresets().size());
}

const juce::String HamloUdarnikProcessor::getProgramName(int index)
{
    const auto& list = factoryPresets();
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    const auto& preset = list[static_cast<size_t>(index - 1)];
    // the genre presets carry their genre and group in the host's program list (the preset screen shows them apart)
    return juce::String(preset.genre).isEmpty() ? juce::String(preset.name) : juce::String(preset.genre) + " / " + preset.group + " / " + preset.name;
}

void HamloUdarnikProcessor::setCurrentProgram(int index)
{
    const auto& list = factoryPresets();
    currentProgram = juce::jlimit(0, static_cast<int>(list.size()), index);
    for (const auto& spec : parameterSpecs())
        setParameterValue(spec.id, currentProgram > 0 ? presetValue(list[static_cast<size_t>(currentProgram - 1)], spec.id, spec.defaultValue)
                                                      : spec.defaultValue);
}
} // namespace hamlo::udarnik
