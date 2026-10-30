#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"
#include "Standalone/VerbParameters.h"
#include <cmath>

#include "DSP/ReverbProcessor.h"
#include "Standalone/ModulePluginEditor.h"

namespace hpeq::standalone
{
namespace
{
constexpr double kBypassRampSeconds = 0.02;
}

ModulePluginProcessor::ModulePluginProcessor(ModuleKind moduleKind)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      kind(moduleKind),
      parameters(*this, nullptr, juce::Identifier(moduleSpec(kind).productName), createParameterLayout(kind))
{
    rawValues.reserve(getSpec().parameters.size());
    for (const auto& spec : getSpec().parameters)
    {
        auto* value = parameters.getRawParameterValue(spec.id);
        jassert(value != nullptr);
        rawValues.push_back(value);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
    defaults = parameters.copyState();
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);
    modern.prepare(sampleRate, maximumBlockSize);
    wetBuffer.setSize(2, maximumBlockSize);
    insertMix.reset(rawValues[6]->load());
    outputGain.reset(std::pow(10.0, rawValues[Output]->load() / 20.0));
    lastEngine = juce::roundToInt(rawValues[Engine]->load());

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    setLatencySamples(fixedLatency);
    // M0 acquired its reverb slot during prepare, before the first host block.
    // In particular, MIX automation between prepare and process must ramp from
    // that initial value rather than starting at the first block's value.
    engine.setTargets(currentSlot(isModuleOn()));
}

bool ModulePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

dsp::FxSlotParams ModulePluginProcessor::currentSlot(bool active) const noexcept
{
    return makeSlot(kind, rawValues, active);
}

dsp::Transport ModulePluginProcessor::currentTransport() noexcept
{
    dsp::Transport transport;
    transport.bpm = hostBpm.load(std::memory_order_relaxed);
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
            {
                transport.bpm = *bpm;
                hostBpm.store(*bpm, std::memory_order_relaxed);
            }
            transport.playing = position->getIsPlaying();
            if (const auto ppq = position->getPpqPosition(); ppq.hasValue())
            {
                transport.hasPosition = true;
                transport.ppqAtBlockStart = *ppq;
            }
        }
    return transport;
}

void ModulePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    if (channels <= 0)
        return;
    const bool on = isModuleOn();
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    const auto slot = currentSlot(on);
    const auto settings = readVerbSettings(rawValues, slot);
    const bool isModern = settings.engine > 0;
    if ((lastEngine == 0) != !isModern) { if (isModern) modern.reset(); else { engine.reset(); engine.setTargets(slot); } }
    lastEngine = settings.engine;
    insertMix.setTarget(rawValues[6]->load(), bypassRampSteps);
    outputGain.setTarget(std::pow(10.0, rawValues[Output]->load() / 20.0), bypassRampSteps);
    const bool send = rawValues[Routing]->load() >= .5f;
    const auto transport = currentTransport();

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        float* dryPointers[dsp::kMaxChannels] {};
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            dryPointers[channel] = dry.getWritePointer(channel);
            juce::FloatVectorOperations::copy(dryPointers[channel], wet[channel], count);
        }

        auto chunkTransport = transport;
        if (chunkTransport.hasPosition && start > 0)
            chunkTransport.ppqAtBlockStart += static_cast<double>(start) * chunkTransport.bpm
                                               / (60.0 * currentSampleRate.load(std::memory_order_relaxed));
        if (isModern)
        {
            const float* input[] { dryPointers[0], dryPointers[1] };
            float* result[] { wetBuffer.getWritePointer(0), wetBuffer.getWritePointer(1) };
            modern.process(input, result, channels, count, settings, chunkTransport);
            for (int i = 0; i < count; ++i)
            {
                const float m = send ? (insertMix.next(), 1.0f) : static_cast<float>(insertMix.next());
                const float gain = static_cast<float>(outputGain.next());
                for (int ch = 0; ch < channels; ++ch)
                    wet[ch][i] = gain * (m == 1 ? result[ch][i] : m == 0 ? dryPointers[ch][i] : (1-m)*dryPointers[ch][i] + m*result[ch][i]);
            }
        }
        else engine.process(wet, channels, count, slot, chunkTransport);
        for (int i = 0; i < count; ++i)
        {
            const float amount = static_cast<float>(wetAmount.next());
            for (int channel = 0; channel < channels; ++channel)
                wet[channel][i] = dryPointers[channel][i] + amount * (wet[channel][i] - dryPointers[channel][i]);
        }
    }
}

double ModulePluginProcessor::getTailLengthSeconds() const
{
    const auto slot = currentSlot(true);
    const double bpm = hostBpm.load(std::memory_order_relaxed);
    const auto settings = readVerbSettings(rawValues, slot);
    return settings.engine == 0 ? dsp::ReverbProcessor::tailSeconds(slot, bpm) : dsp::VerbV2Processor::tailSeconds(settings, bpm);
}

juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", 2, nullptr);
    state.setProperty("program", getCurrentProgram(), nullptr);
    const auto& bank = presets::factoryModulePresets(getSpec().type);
    const int selected = getCurrentProgram();
    if (selected > 0 && selected <= static_cast<int>(bank.size()))
        state.setProperty("presetId", bank[static_cast<size_t>(selected-1)].id, nullptr);
    state.setProperty("mixLock", getMixLock(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}

void ModulePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            const auto incoming = juce::ValueTree::fromXml(*xml);
            auto merged = defaults.createCopy();
            for (auto child : merged)
            {
                const auto id = child.getProperty("id").toString();
                const auto saved = incoming.getChildWithProperty("id", id);
                if (saved.isValid())
                {
                    const double value = saved.getProperty("value");
                    if (std::isfinite(value))
                        if (auto* param = parameter(id))
                            child.setProperty("value", param->getNormalisableRange().snapToLegalValue(static_cast<float>(value)), nullptr);
                }
            }
            parameters.replaceState(merged);
            currentProgram.store(juce::jlimit(0, getNumPrograms()-1, static_cast<int>(incoming.getProperty("program", 0))));
            const auto savedId = incoming.getProperty("presetId").toString();
            if (savedId.isNotEmpty())
            {
                const auto& bank = presets::factoryModulePresets(getSpec().type);
                for (size_t i = 0; i < bank.size(); ++i)
                    if (savedId == bank[i].id) { currentProgram.store(static_cast<int>(i+1)); break; }
            }
            setMixLock(static_cast<bool>(incoming.getProperty("mixLock", false)));
        }
}

juce::RangedAudioParameter* ModulePluginProcessor::parameter(const juce::String& id) const noexcept
{
    return parameters.getParameter(id);
}

double ModulePluginProcessor::parameterValue(const juce::String& id) const noexcept
{
    if (const auto* p = parameter(id))
        return p->convertFrom0to1(p->getValue());
    return 0.0;
}

void ModulePluginProcessor::setParameterValue(const juce::String& id, double actualValue)
{
    if (auto* p = parameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(actualValue)));
}

int ModulePluginProcessor::getNumPrograms()
{
    return 1 + static_cast<int>(presets::factoryModulePresets(getSpec().type).size());
}

const juce::String ModulePluginProcessor::getProgramName(int index)
{
    const auto& list = presets::factoryModulePresets(getSpec().type);
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    return list[static_cast<size_t>(index - 1)].name;
}

void ModulePluginProcessor::setCurrentProgram(int index)
{
    const auto& list = presets::factoryModulePresets(getSpec().type);
    currentProgram = juce::jlimit(0, static_cast<int>(list.size()), index);
    const auto selected = currentProgram.load();
    for (const auto& spec : getSpec().parameters)
    {
        if (spec.id == "routing" || (spec.id == "mix" && getMixLock())) continue;
        double value = spec.defaultValue;
        if (selected > 0)
            value = presets::presetValue(list[static_cast<size_t>(selected - 1)], spec.id, value);
        setParameterValue(spec.id, value);
    }
}

bool ModulePluginProcessor::isProgramModified() const
{
    const auto& list = presets::factoryModulePresets(getSpec().type);
    const int index = currentProgram.load();
    for (const auto& spec : getSpec().parameters)
    {
        if (spec.id == "routing" || (spec.id == "mix" && getMixLock())) continue;
        const double expected = index > 0 && index <= static_cast<int>(list.size()) ? presets::presetValue(list[static_cast<size_t>(index-1)], spec.id, spec.defaultValue) : spec.defaultValue;
        if (const auto* p = parameter(spec.id))
            if (std::abs(p->getValue()-p->convertTo0to1(static_cast<float>(expected))) > 1e-6f) return true;
    }
    return false;
}
} // namespace hpeq::standalone
