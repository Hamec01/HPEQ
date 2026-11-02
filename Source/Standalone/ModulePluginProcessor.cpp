#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"
#include "Core/RevolutionPresets.h"

#include "DSP/ProcessorsM6.h"
#include "DSP/ReverbProcessor.h"
#include "Standalone/ModulePluginEditor.h"

namespace hpeq::standalone
{
struct ModulePluginProcessor::Follower final : juce::Timer
{
    explicit Follower(ModulePluginProcessor& p) : owner(p) { startTimerHz(4); }
    ~Follower() override { stopTimer(); }
    void timerCallback() override { owner.followHpdg(); }
    ModulePluginProcessor& owner;
};

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
    {
        resetScenes({ revolution::generate({}, 1, revolution::Musical), {}, 1 });
        follower = std::make_unique<Follower>(*this);
    }
}

ModulePluginProcessor::~ModulePluginProcessor()
{
    follower.reset();
}

revolution::simple::Settings ModulePluginProcessor::simpleSettings() const
{
    revolution::simple::Settings s;
    s.genre = juce::roundToInt(parameterValue("simpleGenre"));
    s.intent = parameterValue("simpleIntent");
    s.amount = parameterValue("simpleAmount");
    s.variation = parameterValue("simpleVariation");
    s.fill = juce::roundToInt(parameterValue("simpleFill"));
    s.target = juce::roundToInt(parameterValue("simpleTarget"));
    return s;
}

revolution::simple::Result ModulePluginProcessor::lastSimple() const
{
    const juce::ScopedLock lock(revolutionLock);
    return simpleResult;
}

void ModulePluginProcessor::generateSimple(bool newSeed)
{
    namespace simple = revolution::simple;
    auto state = getRevolutionState();
    if (newSeed)
        state.seed = static_cast<juce::int64>(revolution::nextSeed(static_cast<std::uint64_t>(state.seed)));
    auto settings = simpleSettings();
    settings.seed = static_cast<std::uint64_t>(state.seed);
    // GLITCH 2: what the input is - drums (sharp attacks) or sustained (vocal, pad, keys); heard by LISTEN, kept in
    // the scene from the first generation that heard something (GENERATE listens again)
    {
        // percussive = the loudest 10 ms of the last second >= 9 dB above its median 10 ms (drums: hits and gaps); a pad
        // or a held vocal stays within a few dB (the energy novelty and the onset detector both read a chord's beating
        // as attacks)
        const float dyn = moduleNode().getRevolutionDynamics(0);
        if (dyn > 0.0f && (newSeed || state.material == 0))
            state.material = dyn >= 9.0f ? 1 : 2;
        settings.sustained = state.material == 2;
    }
    // the hits: HPDG's pattern when linked (the last one, also after LOST), else LISTEN's feel, else the genre's template
    simple::Hits hits;
    auto snapshot = std::make_unique<hamlo::link::Snapshot>();
    if (hpdgLink.isConnected() && hpdgLink.last(*snapshot))
        hits = simple::hitsFromSnapshot(*snapshot);
    else
    {
        // R5: GENERATE takes what the engine heard (its attack strengths and onset timing); knob moves reuse it
        if (newSeed)
        {
            std::array<float, revolution::kSteps> live {}, offsets {}, shares {};
            const auto& node = moduleNode();
            if (node.getRevolutionFeel(0, live) && std::any_of(live.begin(), live.end(), [](float f) { return f > 0.05f; })
                && node.getRevolutionTiming(0, offsets, shares))
            {
                state.feel = revolution::feelFrom(live);
                state.heardMask = 0;
                for (int s = 0; s < revolution::kSteps; ++s)
                {
                    const bool heard = shares[static_cast<size_t>(s)] >= 0.5f;
                    state.heard[static_cast<size_t>(s)] = heard ? offsets[static_cast<size_t>(s)] : 0.0f;
                    state.heardMask = static_cast<std::uint16_t>(state.heardMask | (heard ? 1u << s : 0u));
                }
            }
        }
        const int genre = simple::resolveGenre(settings.genre, {}, getHostBpm());
        hits = revolution::hasFeel(state.feel) ? simple::hitsFromFeel(state.feel, genre) : simple::templateHits(genre);
        if (revolution::hasFeel(state.feel))
            for (int s = 0; s < revolution::kSteps; ++s)
                if (((state.heardMask >> s) & 1) != 0)
                    hits.groove[static_cast<size_t>(s)] = state.heard[static_cast<size_t>(s)];
    }
    const auto result = simple::generate(settings, hits, getHostBpm());
    for (const auto& [id, value] : result.parameters)
        setParameterValue(id, value);
    state.pattern = result.pattern;
    state.lanes = result.lanes;
    state.groove = result.groove;
    setRevolutionState(state);
    const juce::ScopedLock lock(revolutionLock);
    simpleResult = result;
}

void ModulePluginProcessor::generateLane(int lane)
{
    namespace simple = revolution::simple;
    if (lane < revolution::kEngines || lane >= revolution::kLanes)
        return;
    auto state = getRevolutionState();
    auto settings = simpleSettings();
    settings.sustained = state.material == 2;
    simple::Hits hits;
    auto snapshot = std::make_unique<hamlo::link::Snapshot>();
    if (hpdgLink.isConnected() && hpdgLink.last(*snapshot))
        hits = simple::hitsFromSnapshot(*snapshot);
    else
    {
        const int genre = simple::resolveGenre(settings.genre, {}, getHostBpm());
        hits = revolution::hasFeel(state.feel) ? simple::hitsFromFeel(state.feel, genre) : simple::templateHits(genre);
    }
    // each press: a new seed, an INTENT zone and an AMOUNT drawn from it (the lane's rules hang on the hits more than on
    // the seed); a result that is empty or the same as now is drawn again (up to 12 times)
    const auto current = state.lanes[static_cast<size_t>(lane)].first;
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        settings.seed = revolution::mix64(static_cast<std::uint64_t>(state.seed) ^ (0x9E3779B97F4A7C15ull * static_cast<std::uint64_t>(lane + 1))
                                          ^ (static_cast<std::uint64_t>(++laneGenerations) << 20));
        settings.intent = (static_cast<double>(settings.seed % simple::kIntents) + 0.5) / simple::kIntents;
        settings.amount = 0.3 + 0.6 * revolution::unitOf(revolution::mix64(settings.seed));
        const auto r = simple::generate(settings, hits, getHostBpm());
        const auto& from = r.lanes[static_cast<size_t>(lane)];
        const bool empty = std::all_of(from.first.begin(), from.first.end(), [](auto v) { return v == 0; });
        if ((empty || from.first == current) && attempt < 11)
            continue;
        auto& to = state.lanes[static_cast<size_t>(lane)];
        to.first = from.first;
        to.extra = from.extra;
        to.probability = from.probability;
        to.length = from.length;
        to.rate = from.rate;
        break;
    }
    setRevolutionState(state);
}

void ModulePluginProcessor::simpleEdited()
{
    if (parameterValue("simpleLock") < 0.5)
        generateSimple(false);
}

bool ModulePluginProcessor::followHpdg()
{
    if (kind != ModuleKind::Revolution || !hpdgLink.isConnected())
        return false;
    auto snapshot = std::make_unique<hamlo::link::Snapshot>();
    if (!hpdgLink.takeIfNew(*snapshot))
        return false;
    if (parameterValue("simpleLock") >= 0.5)
        return true; // LOCK keeps everything
    // R5: TONAL in HPDG's key, on either page
    const int key = juce::jlimit(0, 11, static_cast<int>(snapshot->keyRoot));
    setParameterValue("throwANote", key);
    setParameterValue("throwBNote", key);
    if (getPage() == Page::Simple)
        generateSimple(false);
    return true;
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
    for (int i = 0; i < revolution::kSteps; ++i)
        out[static_cast<size_t>(kSceneGroove + i)].store(state.groove[static_cast<size_t>(i)], std::memory_order_relaxed);
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
            const int to = dsp::slot::laneSettings(e);
            for (int f = 0; f < revolution::kLaneFloats; ++f)
                slot.values[static_cast<size_t>(to + f)] = scene[static_cast<size_t>(kSceneLanes + e * revolution::kLaneFloats + f)].load(std::memory_order_relaxed);
            slot.values[static_cast<size_t>(dsp::slot::laneBand(e))] = scene[static_cast<size_t>(kSceneBands + e)].load(std::memory_order_relaxed);
        }
        for (int i = 0; i < revolution::kSteps; ++i)
            slot.values[static_cast<size_t>(dsp::slot::kGrooveBase + i)] = scene[static_cast<size_t>(kSceneGroove + i)].load(std::memory_order_relaxed);
        for (int i = 0; i < revolution::kLanes - revolution::kEngines; ++i)
        {
            const int to = i < revolution::kShapes ? dsp::slot::kShapeBase + i * dsp::slot::kShapeStride + dsp::slot::kShapePattern
                                                   : dsp::slot::kThrowBase + (i - revolution::kShapes) * dsp::slot::kThrowStride + dsp::slot::kThrowPattern;
            for (int q = 0; q < 4; ++q)
                slot.values[static_cast<size_t>(to + q)] = scene[static_cast<size_t>(kSceneShapes + 4 * i + q)].load(std::memory_order_relaxed);
        }
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
                if (state.material != 0)
                    to.setAttribute("material", state.material);
                if (state.heardMask != 0)
                {
                    juce::StringArray heard;
                    for (auto h : state.heard)
                        heard.add(juce::String(h, 6));
                    to.setAttribute("heard", heard.joinIntoString(" "));
                    to.setAttribute("heardMask", static_cast<int>(state.heardMask));
                }
                if (std::any_of(state.groove.begin(), state.groove.end(), [](float g) { return g != 0.0f; }))
                {
                    juce::StringArray groove;
                    for (auto g : state.groove)
                        groove.add(juce::String(g, 6));
                    to.setAttribute("groove", groove.joinIntoString(" "));
                }
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
            e->setAttribute("mode", getPage() == Page::Simple ? "simple" : "advanced");
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
                            if (!name.startsWith("hpdg") && !name.startsWith("lane") && name != "index" && name != "groove" && name != "mode"
                                && !name.startsWith("heard") && name != "material")
                                set.set(juce::Identifier(name), juce::var(from.getAttributeValue(i).getLargeIntValue()));
                        }
                        const auto* seed = set.getVarPointer("revSeed");
                        RevolutionState state { revolution::readPattern(set), revolution::readFeel(set), seed != nullptr ? static_cast<juce::int64>(*seed) : 1 };
                        for (int l = 0; l < revolution::kLanes; ++l)
                            if (from.hasAttribute("lane" + juce::String(l)))
                                state.lanes[static_cast<size_t>(l)] = revolution::laneFromText(from.getStringAttribute("lane" + juce::String(l)));
                        const auto groove = juce::StringArray::fromTokens(from.getStringAttribute("groove"), " ", "");
                        for (int i = 0; i < revolution::kSteps && i < groove.size(); ++i)
                            state.groove[static_cast<size_t>(i)] = groove[i].getFloatValue();
                        const auto heard = juce::StringArray::fromTokens(from.getStringAttribute("heard"), " ", "");
                        for (int i = 0; i < revolution::kSteps && i < heard.size(); ++i)
                            state.heard[static_cast<size_t>(i)] = heard[i].getFloatValue();
                        state.heardMask = static_cast<std::uint16_t>(from.getIntAttribute("heardMask", 0));
                        state.material = juce::jlimit(0, 2, from.getIntAttribute("material", 0));
                        return state;
                    };
                    // a state from before R4 has no mode: its pattern is hand-made, ADVANCED shows it
                    setPage(e->getStringAttribute("mode", "advanced") == "simple" ? Page::Simple : Page::Advanced);
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
            // GLITCH 2: a state saved before it has no glitchEngine: its GLITCH steps are the legacy actions
            const bool knowsGlitchEngine = kind == ModuleKind::Revolution && xml->getChildByAttribute("id", "glitchEngine") != nullptr;
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            if (kind == ModuleKind::Revolution && !knowsGlitchEngine)
                setParameterValue("glitchEngine", 0);
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
    const int genre = kind == ModuleKind::Revolution ? static_cast<int>(revolution::genrePresets().size()) : 0;
    return 1 + static_cast<int>(presets::factoryModulePresets(getSpec().type).size()) + genre;
}

const revolution::GenrePreset* ModulePluginProcessor::genrePresetFor(int program) const
{
    const int first = 1 + static_cast<int>(presets::factoryModulePresets(getSpec().type).size());
    if (kind != ModuleKind::Revolution || program < first || program >= first + static_cast<int>(revolution::genrePresets().size()))
        return nullptr;
    return &revolution::genrePresets()[static_cast<size_t>(program - first)];
}

const juce::String ModulePluginProcessor::getProgramName(int index)
{
    if (const auto* g = genrePresetFor(index))
        return g->name;
    const auto& list = presets::factoryModulePresets(getSpec().type);
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    return list[static_cast<size_t>(index - 1)].name;
}

void ModulePluginProcessor::setCurrentProgram(int index)
{
    if (const auto* g = genrePresetFor(index))
    {
        // R6: a SIMPLE recipe - defaults, the recipe, SIMPLE shown, the generator run (HPDG's beat when linked)
        currentProgram = index;
        for (const auto& spec : getSpec().parameters)
            setParameterValue(spec.id, spec.defaultValue);
        setParameterValue("simpleGenre", g->simpleGenre);
        setParameterValue("simpleIntent", g->intent);
        setParameterValue("simpleAmount", g->amount);
        setParameterValue("simpleVariation", g->variation);
        setParameterValue("simpleGroove", g->groove);
        setParameterValue("simpleFill", g->fill);
        setParameterValue("simpleTarget", g->target);
        setParameterValue("mix", g->mix);
        setPage(Page::Simple);
        RevolutionState first;
        first.seed = static_cast<juce::int64>(g->seed);
        resetScenes(first);
        generateSimple(false);
        return;
    }
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
