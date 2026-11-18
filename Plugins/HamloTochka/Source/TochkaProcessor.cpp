#include "TochkaProcessor.h"
#include "TochkaEditor.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace hamlo::tochka
{
namespace
{
using hpeq::ChannelMode;
using hpeq::EqShape;
using Band = Processor::Band;

constexpr const char* stateType = "HamloTochkaState";
constexpr const char* bypassId = "bypass";
constexpr const char* matchId = "match_db";

struct FieldInfo { const char* id; const char* name; };
constexpr std::array<FieldInfo, Processor::FieldCount> fields {{
    { "on", "Enabled" }, { "shape", "Shape" }, { "slope", "Slope" }, { "channel", "Channel" },
    { "solo", "Solo" }, { "dynamic", "Dynamic" }, { "detect", "Detector" }, { "freq", "Frequency" },
    { "gain", "Gain" }, { "q", "Q" }, { "threshold", "Threshold" }, { "ratio", "Ratio" },
    { "range", "Range" }, { "attack", "Attack" }, { "release", "Release" },
    { "external_key", "External Key" }
}};

constexpr Band point(EqShape shape, float hz, float gain = 0.0f, float q = 0.707f,
                     int slope = 12, ChannelMode channel = ChannelMode::Stereo)
{
    Band band;
    band.active = true;
    band.shape = shape;
    band.frequencyHz = hz;
    band.gainDb = gain;
    band.q = q;
    band.slopeDbPerOct = slope;
    band.channel = channel;
    return band;
}

constexpr Band deEss()
{
    auto band = point(EqShape::Bell, 7000.0f, 0.0f, 0.8f);
    band.dynamic = true;
    band.detect = 1;
    band.thresholdDb = -28.0f;
    band.ratio = 4.0f;
    band.rangeDb = -9.0f;
    band.attackMs = 0.5f;
    band.releaseMs = 40.0f;
    return band;
}
} // namespace

const std::array<Preset, 10>& Processor::presets()
{
    static constexpr std::array<Preset, 10> bank {{
        { "Default", "General", "Empty EQ and unity output", {} },
        { "Sub Cleanup", "Utility", "Gentle 25 Hz low cut", { point(EqShape::LowCut, 25.0f) } },
        { "Kick Click", "Drums", "Add attack around 2.5 kHz", { point(EqShape::Bell, 2500.0f, 2.5f, 1.1f) } },
        { "Snare Body", "Drums", "Bring up the body near 180 Hz", { point(EqShape::Bell, 180.0f, 2.0f, 1.2f) } },
        { "Drum Bus Tilt", "Drums", "A gentle tilt toward the highs", { point(EqShape::Tilt, 850.0f, 2.0f) } },
        { "808 Pocket", "Bass", "A small cut for the kick. Tune to the mix.",
          { point(EqShape::Bell, 62.0f, -2.5f, 2.0f) } },
        { "Vocal Cleanup", "Vocal", "Low cut plus a small low-mid reduction",
          { point(EqShape::LowCut, 75.0f), point(EqShape::Bell, 280.0f, -2.0f, 1.15f) } },
        { "Vocal Presence", "Vocal", "A broad presence boost near 3 kHz", { point(EqShape::Bell, 3000.0f, 1.8f, 0.8f) } },
        { "Voice De-Ess", "Vocal", "Voice-relative dynamic cut near 7 kHz", { deEss() } },
        { "Side Low Cut", "Stereo", "High-pass the side channel at 100 Hz",
          { point(EqShape::LowCut, 100.0f, 0.0f, 0.707f, 12, ChannelMode::Side) } }
    }};
    return bank;
}

juce::String Processor::parameterId(int index, int field)
{
    return "band" + juce::String(index + 1).paddedLeft('0', 2) + "_" + fields[static_cast<size_t>(field)].id;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::parameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { bypassId, 1 }, "Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { matchId, 1 }, "Match Gain",
                                                        juce::NormalisableRange<float> { -12.0f, 12.0f }, 0.0f));
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto prefix = "Band " + juce::String(slot + 1).paddedLeft('0', 2) + " ";
        const auto key = [slot](int field) { return juce::ParameterID { parameterId(slot, field), 1 }; };
        const auto label = [&prefix](int field) { return prefix + fields[static_cast<size_t>(field)].name; };
        const auto toggle = [&](int field)
        {
            layout.add(std::make_unique<juce::AudioParameterBool>(key(field), label(field), false));
        };
        const auto choice = [&](int field, juce::StringArray values, int initial)
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(key(field), label(field), values, initial));
        };
        const auto value = [&](int field, float low, float high, float initial, bool skew = false)
        {
            juce::NormalisableRange<float> range(low, high);
            if (skew)
                range.setSkewForCentre(std::sqrt(low * high));
            layout.add(std::make_unique<juce::AudioParameterFloat>(key(field), label(field), range, initial));
        };

        toggle(On);
        choice(Shape, { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt", "All Pass" }, 0);
        choice(Slope, { "6", "12", "18", "24", "36", "48", "72", "96" }, 1);
        choice(Channel, { "Stereo", "Left", "Right", "Mid", "Side" }, 0);
        toggle(Solo);
        toggle(Dynamic);
        choice(Detect, { "Band", "Voice" }, 0);
        value(Frequency, 10.0f, 22000.0f, 1000.0f, true);
        value(Gain, -24.0f, 24.0f, 0.0f);
        value(Q, 0.1f, 18.0f, 0.707f, true);
        value(Threshold, -60.0f, 0.0f, -30.0f);
        value(Ratio, 1.0f, 20.0f, 3.0f, true);
        value(Range, -24.0f, 24.0f, -6.0f);
        value(Attack, 0.1f, 200.0f, 10.0f, true);
        value(Release, 5.0f, 2000.0f, 120.0f, true);
    }
    // Append key parameters after all 0.1 automation slots.
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto label = "Band " + juce::String(slot + 1).paddedLeft('0', 2) + " External Key";
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { parameterId(slot, ExternalKey), 1 },
                                                            label, false));
    }
    return layout;
}

Processor::Processor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, stateType, parameterLayout())
{
    bypassParameter = state.getParameter(bypassId);
    matchParameter = state.getParameter(matchId);
    bypassValue = state.getRawParameterValue(bypassId);
    matchValue = state.getRawParameterValue(matchId);
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
        for (int field = 0; field < FieldCount; ++field)
        {
            const auto id = parameterId(slot, field);
            bandParameters[static_cast<size_t>(slot)][static_cast<size_t>(field)] = state.getParameter(id);
            bandValues[static_cast<size_t>(slot)][static_cast<size_t>(field)] = state.getRawParameterValue(id);
        }
}

bool Processor::isBusesLayoutSupported(const BusesLayout& buses) const
{
    const auto out = buses.getMainOutputChannelSet();
    const auto key = buses.getChannelSet(true, 1);
    return buses.getMainInputChannelSet() == out
           && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && (key.isDisabled() || key == juce::AudioChannelSet::mono() || key == juce::AudioChannelSet::stereo());
}

Processor::Band Processor::band(int index) const noexcept
{
    const auto& values = bandValues[static_cast<size_t>(index)];
    const auto get = [&values](Field f) { return values[static_cast<size_t>(f)]->load(std::memory_order_relaxed); };
    Band result;
    result.active = get(On) >= 0.5f;
    result.shape = static_cast<EqShape>(static_cast<int>(get(Shape)));
    result.slopeDbPerOct = hpeq::dsp::kEqSlopes[static_cast<size_t>(get(Slope))];
    result.channel = static_cast<ChannelMode>(static_cast<int>(get(Channel)));
    result.solo = get(Solo) >= 0.5f;
    result.dynamic = get(Dynamic) >= 0.5f;
    result.detect = static_cast<int>(get(Detect));
    result.frequencyHz = get(Frequency);
    result.gainDb = get(Gain);
    result.q = get(Q);
    result.thresholdDb = get(Threshold);
    result.ratio = get(Ratio);
    result.rangeDb = get(Range);
    result.attackMs = get(Attack);
    result.releaseMs = get(Release);
    return result;
}

bool Processor::hasBand(int index) const noexcept
{
    const auto mask = std::uint32_t { 1 } << index;
    if ((occupied.load(std::memory_order_relaxed) & mask) != 0)
        return true;
    const auto& params = bandParameters[static_cast<size_t>(index)];
    return std::any_of(params.begin(), params.end(), [](auto* param)
    {
        return std::abs(param->getValue() - param->getDefaultValue()) > 1.0e-6f;
    });
}

bool Processor::usesExternalKey(int index) const noexcept
{
    return bandValues[static_cast<size_t>(index)][ExternalKey]->load(std::memory_order_relaxed) >= 0.5f;
}

void Processor::setExternalKey(int index, bool enabled)
{
    writeParameter(*bandParameters[static_cast<size_t>(index)][ExternalKey], enabled ? 1.0f : 0.0f, false);
    modified.store(true);
}

void Processor::writeParameter(juce::RangedAudioParameter& parameter, float value, bool inGesture)
{
    const auto normalized = parameter.convertTo0to1(value);
    if (parameter.getValue() == normalized)
        return;
    if (!inGesture)
        parameter.beginChangeGesture();
    parameter.setValueNotifyingHost(normalized);
    if (!inGesture)
        parameter.endChangeGesture();
}

void Processor::writeBand(int index, const Band& pointToWrite, bool inGesture)
{
    const auto& b = pointToWrite;
    const auto& params = bandParameters[static_cast<size_t>(index)];
    const auto first = std::begin(hpeq::dsp::kEqSlopes);
    const auto last = std::end(hpeq::dsp::kEqSlopes);
    const auto slope = std::find(first, last, b.slopeDbPerOct);
    const std::array<float, ExternalKey> values {
        b.active ? 1.0f : 0.0f, static_cast<float>(b.shape), slope == last ? 1.0f : static_cast<float>(slope - first),
        static_cast<float>(b.channel), b.solo ? 1.0f : 0.0f, b.dynamic ? 1.0f : 0.0f, static_cast<float>(b.detect),
        b.frequencyHz, b.gainDb, b.q, b.thresholdDb, b.ratio, b.rangeDb, b.attackMs, b.releaseMs
    };
    // Set the controls before enabling a previously inactive slot.
    for (size_t i = 1; i < values.size(); ++i)
        writeParameter(*params[i], values[i], inGesture);
    writeParameter(*params[On], values[On], inGesture);
}

void Processor::editBand(int index, const Band& edited, bool inGesture)
{
    occupied.fetch_or(std::uint32_t { 1 } << index, std::memory_order_relaxed);
    if (edited.solo)
        for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
            if (slot != index)
                writeParameter(*bandParameters[static_cast<size_t>(slot)][Solo], 0.0f, false);
    writeBand(index, edited, inGesture);
    modified.store(true);
}

void Processor::deleteBand(int index)
{
    writeParameter(*bandParameters[static_cast<size_t>(index)][On], 0.0f, false);
    writeBand(index, {}, false);
    setExternalKey(index, false);
    occupied.fetch_and(~(std::uint32_t { 1 } << index), std::memory_order_relaxed);
    modified.store(true);
}

void Processor::beginBandGesture(int index, int field)
{
    bandParameters[static_cast<size_t>(index)][static_cast<size_t>(field)]->beginChangeGesture();
}

void Processor::endBandGesture(int index, int field)
{
    bandParameters[static_cast<size_t>(index)][static_cast<size_t>(field)]->endChangeGesture();
}

void Processor::prepareToPlay(double rate, int)
{
    sampleRate.store(rate);
    eq.prepare(rate);
    dryMeter.prepare(rate, false);
    wetMeter.prepare(rate, false);
    wetMix.reset(rate, 0.01);
    matchGain.reset(rate, 0.02);
    reset();
}

void Processor::reset()
{
    eq.reset();
    controlPhase = 0;
    wetMix.setCurrentAndTargetValue(isDry() ? 0.0f : 1.0f);
    matchGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(matchDb()));
    activeLearnId = 0;
    learnReady.store(0);
    keyActive.store(false, std::memory_order_relaxed);
}

juce::AudioProcessorParameter* Processor::getBypassParameter() const { return bypassParameter; }

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) { process(buffer, isDry()); }
void Processor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) { process(buffer, true); }

void Processor::process(juce::AudioBuffer<float>& buffer, bool forceDry)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getMainBusNumInputChannels();
    if (channels == 0 || buffer.getNumSamples() == 0)
        return;

    auto keyBuffer = getBusBuffer(buffer, true, 1);
    const auto keyChannels = keyBuffer.getNumChannels();
    keyActive.store(keyChannels > 0, std::memory_order_relaxed);
    Bands bands;
    hpeq::dsp::KeyedEq::ExternalBands external {};
    for (size_t i = 0; i < bands.size(); ++i)
    {
        bands[i] = band(static_cast<int>(i));
        external[i] = usesExternalKey(static_cast<int>(i));
    }
    eq.setTargets(bands);
    wetMix.setTargetValue(forceDry ? 0.0f : 1.0f);
    matchGain.setTargetValue(juce::Decibels::decibelsToGain(matchDb()));

    const auto request = learnRequest.load(std::memory_order_acquire);
    if (request != activeLearnId)
    {
        activeLearnId = request;
        if (activeLearnId != 0)
        {
            dryMeter.reset();
            wetMeter.reset();
        }
    }
    const auto showSpectrum = captureEnabled.load(std::memory_order_relaxed);
    const auto count = buffer.getNumSamples();
    for (int offset = 0; offset < count;)
    {
        const auto size = juce::jmin(count - offset, static_cast<int>(dryStorage.front().size()));
        std::array<float*, 2> out {};
        std::array<const float*, 2> dry {};
        std::array<const float*, 2> key {};
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<size_t>(ch);
            out[c] = buffer.getWritePointer(ch, offset);
            std::copy_n(out[c], size, dryStorage[c].data());
            dry[c] = dryStorage[c].data();
        }
        for (int ch = 0; ch < keyChannels; ++ch)
            key[static_cast<size_t>(ch)] = keyBuffer.getReadPointer(ch, offset);

        eq.process(out.data(), channels, size, controlPhase, external, key.data(), keyChannels);
        controlPhase = (controlPhase + size) % hpeq::dsp::kControlInterval;
        if (activeLearnId != 0)
        {
            dryMeter.process(dry.data(), channels, size);
            wetMeter.process(out.data(), channels, size);
        }
        if (wetMix.isSmoothing() || wetMix.getCurrentValue() != 1.0f
            || matchGain.isSmoothing() || matchGain.getCurrentValue() != 1.0f)
        {
            for (int frame = 0; frame < size; ++frame)
            {
                const auto mix = wetMix.getNextValue();
                const auto gain = matchGain.getNextValue();
                for (int ch = 0; ch < channels; ++ch)
                {
                    const auto c = static_cast<size_t>(ch);
                    const float wet = gain * out[c][frame];
                    const float input = dry[c][frame];
                    out[c][frame] = mix == 1.0f ? wet : (mix == 0.0f ? input : input + mix * (wet - input));
                }
            }
        }
        if (showSpectrum)
        {
            capture.push(dry.data(), out.data(), channels, size);
            if (keyChannels > 0)
                keyCapture.push(key.data(), key.data(), keyChannels, size);
        }
        offset += size;
    }

    if (activeLearnId != 0 && dryMeter.integratedLufs() > -70.0 && wetMeter.integratedLufs() > -70.0)
    {
        const auto difference = static_cast<float>(dryMeter.integratedLufs() - wetMeter.integratedLufs());
        learnedCorrection.store(juce::jlimit(-12.0f, 12.0f, difference), std::memory_order_relaxed);
        learnReady.store(activeLearnId, std::memory_order_release);
    }
}

void Processor::setDry(bool dry) { writeParameter(*bypassParameter, dry ? 1.0f : 0.0f, false); }
void Processor::setMatch(float db) { writeParameter(*matchParameter, db, false); }
void Processor::clearMatch() { setMatch(0.0f); }

void Processor::startLearn() noexcept
{
    if (++nextLearnId == 0)
        ++nextLearnId;
    learnRequest.store(nextLearnId, std::memory_order_release);
}

bool Processor::stopLearn()
{
    const auto request = learnRequest.exchange(0);
    if (request == 0 || request != learnReady.load(std::memory_order_acquire))
        return false;
    setMatch(learnedCorrection.load());
    return true;
}

int Processor::getNumPrograms() { return static_cast<int>(presets().size()); }

const juce::String Processor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? presets()[static_cast<size_t>(index)].name : "";
}

void Processor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, getNumPrograms()))
        return;
    const auto& preset = presets()[static_cast<size_t>(index)];
    std::uint32_t used = 0;
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto n = static_cast<size_t>(slot);
        const auto next = n < preset.bands.size() ? preset.bands[n] : Band {};
        if (next.active)
            used |= std::uint32_t { 1 } << slot;
        else
            writeParameter(*bandParameters[n][On], 0.0f, false);
        writeBand(slot, next, false);
        writeParameter(*bandParameters[n][ExternalKey], 0.0f, false);
    }
    occupied.store(used);
    learnRequest.store(0);
    clearMatch();
    program.store(index);
    modified.store(false);
}

void Processor::getStateInformation(juce::MemoryBlock& target)
{
    auto saved = state.copyState();
    saved.setProperty("schema", 1, nullptr);
    saved.setProperty("program", getCurrentProgram(), nullptr);
    saved.setProperty("edited", presetEdited(), nullptr);
    saved.setProperty("occupied", static_cast<int>(occupied.load()), nullptr);
    if (auto xml = saved.createXml())
        copyXmlToBinary(*xml, target);
}

void Processor::setStateInformation(const void* bytes, int size)
{
    if (auto xml = getXmlFromBinary(bytes, size); xml && xml->hasTagName(stateType))
    {
        auto restored = juce::ValueTree::fromXml(*xml);
        if (static_cast<int>(restored.getProperty("schema", 1)) != 1)
            return;
        program.store(juce::jlimit(0, getNumPrograms() - 1, static_cast<int>(restored.getProperty("program", 0))));
        modified.store(static_cast<bool>(restored.getProperty("edited", false)));
        occupied.store(static_cast<std::uint32_t>(static_cast<int>(restored.getProperty("occupied", 0))));
        learnRequest.store(0);
        // A 0.1 state has no key controls. Clear any previously selected keys
        // when loading that state into an existing plug-in instance.
        for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
        {
            const auto id = parameterId(slot, ExternalKey);
            if (!restored.getChildWithProperty("id", id).isValid())
            {
                juce::ValueTree parameter("PARAM");
                parameter.setProperty("id", id, nullptr);
                parameter.setProperty("value", 0.0f, nullptr);
                restored.appendChild(parameter, nullptr);
            }
        }
        state.replaceState(restored);
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
} // namespace hamlo::tochka

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hamlo::tochka::Processor();
}
