#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"

#include "DSP/ProcessorsM6.h"
#include "DSP/ReverbProcessor.h"
#include "Standalone/ModulePluginEditor.h"

namespace hpeq::standalone
{
namespace
{
constexpr double kBypassRampSeconds = 0.02;
const juce::Identifier kRevolutionTag { "REVOLUTION" };
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
    for (size_t i = 0; i < getSpec().parameters.size(); ++i)
        if (getSpec().parameters[i].id == "scene")
            sceneParameterIndex = static_cast<int>(i);
    if (kind == ModuleKind::Revolution) // as presetBaseModule: a Musical pattern from seed 1
        resetScenes({ revolution::generate({}, 1, revolution::Musical), {}, 1 });
}

int ModulePluginProcessor::currentScene() const noexcept
{
    if (sceneParameterIndex < 0)
        return 0;
    return juce::jlimit(0, revolution::kScenes - 1, juce::roundToInt(rawValues[static_cast<size_t>(sceneParameterIndex)]->load(std::memory_order_relaxed)));
}

ModulePluginProcessor::RevolutionState ModulePluginProcessor::getRevolutionState() const
{
    return getScene(currentScene());
}

void ModulePluginProcessor::setRevolutionState(const RevolutionState& state)
{
    setScene(currentScene(), state);
}

ModulePluginProcessor::RevolutionState ModulePluginProcessor::getScene(int index) const
{
    const juce::ScopedLock lock(revolutionLock);
    const auto i = static_cast<size_t>(juce::jlimit(0, revolution::kScenes - 1, index));
    return scenes[sceneUsed[i] ? i : 0];
}

bool ModulePluginProcessor::isSceneUsed(int index) const
{
    const juce::ScopedLock lock(revolutionLock);
    return sceneUsed[static_cast<size_t>(juce::jlimit(0, revolution::kScenes - 1, index))];
}

void ModulePluginProcessor::setScene(int index, const RevolutionState& state)
{
    const juce::ScopedLock lock(revolutionLock);
    const int i = juce::jlimit(0, revolution::kScenes - 1, index);
    scenes[static_cast<size_t>(i)] = state;
    sceneUsed[static_cast<size_t>(i)] = true;
    if (i == 0)
        for (int s = 0; s < revolution::kScenes; ++s)
            publishScene(s); // the scenes that follow scene 1 change with it
    else
        publishScene(i);
}

void ModulePluginProcessor::clearScene(int index)
{
    const juce::ScopedLock lock(revolutionLock);
    const int i = juce::jlimit(0, revolution::kScenes - 1, index);
    if (i == 0)
        return;
    sceneUsed[static_cast<size_t>(i)] = false;
    publishScene(i);
}

void ModulePluginProcessor::resetScenes(const RevolutionState& first)
{
    const juce::ScopedLock lock(revolutionLock);
    scenes = {};
    sceneUsed = { true };
    scenes[0] = first;
    for (int s = 0; s < revolution::kScenes; ++s)
        publishScene(s);
}

void ModulePluginProcessor::publishScene(int index) noexcept
{
    const auto& state = scenes[sceneUsed[static_cast<size_t>(index)] ? static_cast<size_t>(index) : 0];
    auto& out = sceneSlots[static_cast<size_t>(index)];
    for (int e = 0; e < revolution::kEngines; ++e)
        for (int q = 0; q < 4; ++q)
            out[static_cast<size_t>(4 * e + q)].store(revolution::packQuad(state.pattern.steps[static_cast<size_t>(e)], q), std::memory_order_relaxed);
    // LayerSnapshot.cpp: the seed's low 24 bits (exact in a float)
    out[12].store(static_cast<float>(static_cast<std::uint64_t>(state.seed) & 0xffffffu), std::memory_order_relaxed);
    for (int e = 0; e < revolution::kLanes; ++e)
    {
        const auto& source = state.lanes[static_cast<size_t>(e)];
        float lane[revolution::kLaneFloats] {};
        revolution::packLane(source, lane);
        for (int f = 0; f < revolution::kLaneFloats; ++f)
            out[static_cast<size_t>(kSceneLanes + e * revolution::kLaneFloats + f)].store(lane[f], std::memory_order_relaxed);
        out[static_cast<size_t>(kSceneBands + e)].store(static_cast<float>(source.band), std::memory_order_relaxed);
        if (e >= revolution::kEngines)
            for (int q = 0; q < 4; ++q)
                out[static_cast<size_t>(kSceneShapes + 4 * (e - revolution::kEngines) + q)].store(revolution::packQuad(source.first, q),
                                                                                                  std::memory_order_relaxed);
    }
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    wetAlign.prepare(fixedLatency);
    dryAlign.prepare(fixedLatency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    setLatencySamples(fixedLatency);

    dsp::LayerSnapshot snapshot;
    snapshot.mix.active = true;
    snapshot.mix.channel = ChannelMode::Stereo;
    snapshot.mix.numSlots = 1;
    snapshot.mix.slots[0] = currentSlot(isModuleOn());
    engine.publish(snapshot);
    engine.update();
}

bool ModulePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

dsp::FxSlotParams ModulePluginProcessor::currentSlot(bool active) const noexcept
{
    auto slot = makeSlot(kind, rawValues, active);
    if (kind == ModuleKind::Revolution)
    {
        static constexpr int base[revolution::kEngines] { dsp::slot::kTimeOn, dsp::slot::kGlitchOn, dsp::slot::kMotionOn };
        const auto& scene = sceneSlots[static_cast<size_t>(currentScene())];
        for (int e = 0; e < revolution::kEngines; ++e)
            for (int q = 0; q < 4; ++q)
                slot.values[static_cast<size_t>(base[e] + 3 + q)] = scene[static_cast<size_t>(4 * e + q)].load(std::memory_order_relaxed);
        slot.values[dsp::slot::kRevHumanSeed] = scene[12].load(std::memory_order_relaxed);
        for (int e = 0; e < revolution::kLanes; ++e)
        {
            const int to = e < revolution::kEngines ? dsp::slot::kLaneBase + e * dsp::slot::kLaneStride
                                                    : dsp::slot::kShapeLaneBase + (e - revolution::kEngines) * dsp::slot::kLaneStride;
            for (int f = 0; f < revolution::kLaneFloats; ++f)
                slot.values[static_cast<size_t>(to + f)] = scene[static_cast<size_t>(kSceneLanes + e * revolution::kLaneFloats + f)].load(std::memory_order_relaxed);
            slot.values[static_cast<size_t>(dsp::slot::kBandBase + e)] = scene[static_cast<size_t>(kSceneBands + e)].load(std::memory_order_relaxed);
        }
        for (int i = 0; i < revolution::kShapes; ++i)
            for (int q = 0; q < 4; ++q)
                slot.values[static_cast<size_t>(dsp::slot::kShapeBase + i * dsp::slot::kShapeStride + dsp::slot::kShapePattern + q)]
                    = scene[static_cast<size_t>(kSceneShapes + 4 * i + q)].load(std::memory_order_relaxed);
    }
    return slot;
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
    const bool on = isModuleOn();
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    dsp::LayerSnapshot snapshot;
    snapshot.mix.active = true;
    snapshot.mix.channel = ChannelMode::Stereo;
    snapshot.mix.numSlots = 1;
    snapshot.mix.slots[0] = currentSlot(on);
    engine.publish(snapshot);
    engine.update();
    const int extraWetDelay = juce::jmax(0, fixedLatency - engine.getLatencySamples());
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
        engine.process(wet, channels, count, chunkTransport);
        // Always clock the aligner, including at zero delay, so a later automated latency change
        // can draw from continuous history instead of newly-cleared memory.
        wetAlign.process(wet, wet, channels, count, extraWetDelay);
        dryAlign.process(dryPointers, dryPointers, channels, count, fixedLatency);

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
    double tail = 0.0;
    if (kind == ModuleKind::Verb)
        tail = dsp::ReverbProcessor::tailSeconds(slot, bpm);
    else if (kind == ModuleKind::Echo)
        tail = dsp::DelayProcessor::tailSeconds(slot, bpm);
    return tail + static_cast<double>(fixedLatency) / currentSampleRate.load(std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
    {
        if (kind == ModuleKind::Revolution)
        {
            // The pattern state as HPEQ keeps it in the module (writePattern / writeFeel / revSeed), one element.
            while (auto* old = xml->getChildByName(kRevolutionTag))
                xml->removeChildElement(old, true);
            // Scene 1 on the element itself (as R0 saved it), the lanes as lane0 .. lane2, edited scenes 2 .. 8 as children.
            const auto writeScene = [](juce::XmlElement& to, const RevolutionState& state)
            {
                juce::NamedValueSet set;
                revolution::writePattern(set, state.pattern);
                revolution::writeFeel(set, state.feel);
                set.set("revSeed", state.seed);
                for (const auto& value : set)
                    to.setAttribute(value.name, value.value.toString());
                for (int l = 0; l < revolution::kLanes; ++l)
                    if (state.lanes[static_cast<size_t>(l)] != revolution::Lane {})
                        to.setAttribute("lane" + juce::String(l), revolution::laneToText(state.lanes[static_cast<size_t>(l)]));
            };
            auto* e = xml->createNewChildElement(kRevolutionTag);
            writeScene(*e, getScene(0));
            for (int s = 1; s < revolution::kScenes; ++s)
                if (isSceneUsed(s))
                {
                    auto* child = e->createNewChildElement("SCENE");
                    child->setAttribute("index", s);
                    writeScene(*child, getScene(s));
                }
            // HPDG Link: on / off and which HPDG (its instance id, kept in HPDG's project too)
            e->setAttribute("hpdgLink", hpdgLink.isConnected() ? 1 : 0);
            e->setAttribute("hpdgId", juce::String::toHexString(static_cast<juce::int64>(hpdgLink.getInstanceId())));
        }
        copyXmlToBinary(*xml, destination);
    }
}

void ModulePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            if (kind == ModuleKind::Revolution)
                if (const auto* e = xml->getChildByName(kRevolutionTag))
                {
                    const auto readScene = [](const juce::XmlElement& from)
                    {
                        juce::NamedValueSet set;
                        for (int i = 0; i < from.getNumAttributes(); ++i)
                        {
                            const auto name = from.getAttributeName(i);
                            if (!name.startsWith("hpdg") && !name.startsWith("lane") && name != "index")
                                set.set(juce::Identifier(name), juce::var(from.getAttributeValue(i).getLargeIntValue()));
                        }
                        const auto* seed = set.getVarPointer("revSeed");
                        RevolutionState state { revolution::readPattern(set), revolution::readFeel(set), seed != nullptr ? static_cast<juce::int64>(*seed) : 1 };
                        for (int l = 0; l < revolution::kLanes; ++l)
                            if (from.hasAttribute("lane" + juce::String(l)))
                                state.lanes[static_cast<size_t>(l)] = revolution::laneFromText(from.getStringAttribute("lane" + juce::String(l)));
                        return state;
                    };
                    if (e->getIntAttribute("hpdgLink", 0) != 0)
                        hpdgLink.connect(static_cast<std::uint64_t>(e->getStringAttribute("hpdgId").getHexValue64()));
                    else
                        hpdgLink.disconnect();
                    resetScenes(readScene(*e));
                    for (auto* child : e->getChildWithTagNameIterator("SCENE"))
                    {
                        const int s = child->getIntAttribute("index", 0);
                        if (s > 0 && s < revolution::kScenes)
                            setScene(s, readScene(*child));
                    }
                }
            while (auto* old = xml->getChildByName(kRevolutionTag))
                xml->removeChildElement(old, true);
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
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
    for (const auto& spec : getSpec().parameters)
    {
        double value = spec.defaultValue;
        if (currentProgram > 0)
            value = presets::presetValue(list[static_cast<size_t>(currentProgram - 1)], spec.id, value);
        setParameterValue(spec.id, value);
    }
    if (kind == ModuleKind::Revolution)
    {
        // As HPEQ applies a Revolution preset (Plugin/Presets.cpp makeModule): the pattern from its seed and character.
        juce::int64 seed = 1;
        if (currentProgram > 0)
            seed = static_cast<juce::int64>(presets::presetValue(list[static_cast<size_t>(currentProgram - 1)], "revSeed", 1.0));
        const int character = juce::roundToInt(parameterValue("character"));
        resetScenes({ revolution::generate({}, static_cast<std::uint64_t>(seed), character), {}, seed });
    }
}

} // namespace hpeq::standalone
