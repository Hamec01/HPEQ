#include "Standalone/ModuleVariant.h"

#include <cmath>

#include "DSP/FxNodeProcessor.h"

namespace hpeq::standalone
{
namespace
{
using P = ParameterSpec;

P knob(const char* id, const char* name, float lo, float hi, float def, int target,
       float skew = 1.0f, const char* unit = "", ValueTransform transform = ValueTransform::Direct)
{
    P p;
    p.id = id;
    p.name = name;
    p.minimum = lo;
    p.maximum = hi;
    p.defaultValue = def;
    p.skew = skew;
    p.unit = unit;
    p.slotValueIndex = target;
    p.transform = transform;
    return p;
}

P choice(const char* id, const char* name, int def, int target, std::initializer_list<const char*> labels)
{
    P p;
    p.id = id;
    p.name = name;
    p.minimum = 0.0f;
    p.maximum = static_cast<float>(labels.size() - 1);
    p.defaultValue = static_cast<float>(def);
    p.interval = 1.0f;
    p.slotValueIndex = target;
    for (const auto* label : labels)
        p.choices.add(label);
    return p;
}

ModuleSpec makeComp()
{
    namespace s = dsp::slot;
    return { ModuleKind::Comp, FxType::Compressor, "HamloComp", 1180, 700, 7,
             { knob("thresholdDb", "Threshold", -60, 0, -30, s::kThresholdDb, 1, "dB"),
               knob("ratio", "Ratio", 1, 20, 4, s::kRatio, 0.5f, ":1"),
               knob("kneeDb", "Knee", 0, 40, 6, s::kKneeDb, 1, "dB"),
               knob("attackMs", "Attack", 0.1f, 100, 1, s::kAttackMs, 0.34f, "ms"),
               knob("releaseMs", "Release", 10, 1000, 80, s::kReleaseMs, 0.5f, "ms"),
               knob("makeupDb", "Makeup", 0, 24, 0, s::kMakeupDb, 1, "dB"),
               knob("mix", "Mix", 0, 1, 1, -1),
               choice("detector", "Detector", 0, s::kCompDetector, { "Peak", "RMS" }),
               choice("character", "Character", 1, s::kCompCharacter, { "Clean", "Punch", "Glue" }),
               choice("autoMakeup", "Auto Makeup", 1, s::kCompAutoMakeup, { "Off", "On" }) } };
}

ModuleSpec makeVerb()
{
    namespace s = dsp::slot;
    return { ModuleKind::Verb, FxType::Reverb, "HamloVerb", 1180, 760, 18,
             { knob("predelayMs", "Pre-delay", 0, 500, 20, s::kPredelayMs, 2.3f, "ms"),
               knob("size", "Size", 0.3f, 1.5f, 1, s::kSize),
               knob("decaySeconds", "Decay", 0.1f, 30, 2.4f, s::kDecaySeconds, 0.35f, "s"),
               knob("attack", "Attack", 0, 1, 0.2f, s::kRevAttack),
               knob("distance", "Distance", 0, 1, 0.4f, s::kRevDistance),
               knob("width", "Width", 0, 1.5f, 1, s::kWidth),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("earlyDiffusion", "Early Diffusion", 0, 1, 0.7f, s::kRevEarlyDiff),
               knob("lateDiffusion", "Late Diffusion", 0, 1, 0.7f, s::kRevLateDiff),
               knob("modRate", "Mod Rate", 0.01f, 10, 0.6f, s::kRevModRate, 0.3f, "Hz"),
               knob("modDepth", "Mod Depth", 0, 1, 0.4f, s::kRevModDepth),
               knob("bassMult", "Bass Mult", 0.25f, 4, 1.2f, s::kRevBassMult, 0.5f, "x"),
               knob("bassXoverHz", "Bass Xover", 50, 2000, 300, s::kRevBassXover, 0.38f, "Hz"),
               knob("hfDecayRatio", "High Mult", 0.05f, 1, 0.5f, s::kHfRatio, 0.44f, "x"),
               knob("highXoverHz", "High Xover", 1000, 16000, 4000, s::kRevHighXover, 0.5f, "Hz"),
               knob("lowCutHz", "Low Cut", 10, 1000, 80, s::kLowCutHz, 0.5f, "Hz"),
               knob("highCutHz", "High Cut", 1000, 20000, 12000, s::kRevHighCut, 0.5f, "Hz"),
               knob("duck", "Duck", 0, 1, 0, s::kRevDuck),
               choice("space", "Space", 3, s::kRevSpace, { "Classic", "Room", "Chamber", "Hall", "Plate", "Cathedral", "Ambience" }),
               choice("color", "Color", 2, s::kRevColor, { "1970", "1980", "Now" }),
               choice("predelaySync", "Pre-delay Sync", 0, s::kRevSync,
                      { "Free", "1/64", "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/2" }),
               choice("freeze", "Freeze", 0, s::kRevFreeze, { "Off", "On" }) } };
}

ModuleSpec makeEcho()
{
    namespace s = dsp::slot;
    return { ModuleKind::Echo, FxType::Delay, "HamloEcho", 1180, 760, 20,
             { knob("timeMs", "Time L", 1, 4000, 250, s::kDelayTimeMs, 0.32f, "ms"),
               knob("timeRMs", "Time R", 1, 4000, 375, s::kDlyTimeRMs, 0.32f, "ms"),
               knob("feedback", "Feedback", 0, 0.98f, 0.35f, s::kFeedback),
               knob("cross", "Cross", 0, 1, 0, s::kDlyCross),
               knob("driveDb", "Drive", 0, 24, 3, s::kDlyDriveDb, 1, "dB"),
               knob("width", "Width", 0, 1.5f, 1, s::kDelayWidth),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("modRate", "Mod Rate", 0.05f, 10, 0.5f, s::kDlyModRate, 0.36f, "Hz"),
               knob("modDepth", "Mod Depth", 0, 1, 0.3f, s::kDlyModDepth),
               knob("diffusion", "Diffusion", 0, 1, 0, s::kDlyDiffusion),
               knob("pitch", "Pitch", -12, 12, 0, s::kDlyPitch, 1, "st"),
               knob("lofi", "Lo-Fi", 0, 1, 0, s::kDlyLofi),
               knob("lowCutHz", "Low Cut", 10, 2000, 120, s::kDelayLowCutHz, 0.43f, "Hz"),
               knob("dampingHz", "High Cut", 500, 20000, 8000, s::kDampingHz, 0.5f, "Hz"),
               knob("bandHz", "Band Frequency", 100, 10000, 1000, s::kDlyBandHz, 0.5f, "Hz"),
               knob("bandGainDb", "Band Gain", -18, 12, 0, s::kDlyBandGainDb, 1, "dB"),
               knob("bandQ", "Band Q", 0.3f, 6, 1, s::kDlyBandQ, 0.5f),
               knob("duck", "Duck", 0, 1, 0, s::kDlyDuck),
               knob("lowRes", "Low Resonance", 0, 1, 0, s::kDlyLowRes),
               knob("highRes", "High Resonance", 0, 1, 0, s::kDlyHighRes),
               choice("character", "Character", 2, s::kDlyCharacter, { "Classic", "Clean", "Tape", "BBD", "Lo-Fi" }),
               choice("timeMode", "Time Mode", 0, s::kDlyTimeMode, { "Tape", "Fade" }),
               choice("sync", "Sync L", 8, s::kDelaySync,
                      { "Free", "1/32", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1/1" }),
               choice("syncR", "Sync R", 6, s::kDlySyncR,
                      { "Free", "1/32", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1/1" }),
               choice("link", "Link", 1, s::kDlyLink, { "Off", "On" }),
               choice("pingPong", "Ping-pong", 0, s::kPingPong, { "Off", "On" }),
               choice("freeze", "Freeze", 0, s::kDlyFreeze, { "Off", "On" }) } };
}

ModuleSpec makeMag()
{
    namespace s = dsp::slot;
    return { ModuleKind::Mag, FxType::Saturator, "HamloMag", 1180, 820, 13,
             { knob("driveDb", "Drive", 0, 48, 6, s::kDriveDb, 1, "dB"),
               knob("tone", "Tone", -1, 1, 0, s::kSatTone),
               knob("satLowCutHz", "Low Cut", 10, 1000, 20, s::kSatLowCutHz, 0.5f, "Hz"),
               knob("satHighCutHz", "High Cut", 1000, 20000, 20000, s::kSatHighCutHz, 0.5f, "Hz"),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("trimDb", "Output", -24, 24, 0, s::kTrimDb, 1, "dB"),
               knob("harmonics", "Harmonics", 0, 1, 0.5f, s::kSatHarmonics),
               knob("evenOdd", "Even / Odd", -1, 1, 0, s::kSatEvenOdd),
               knob("tapeBias", "Bias", 0, 1, 0.5f, s::kSatTapeBias),
               knob("wow", "Wow", 0, 1, 0.15f, s::kSatWow),
               knob("hiss", "Hiss", 0, 1, 0.15f, s::kSatHiss),
               knob("hum", "Hum", 0, 1, 0, s::kSatHum),
               knob("crackle", "Crackle", 0, 1, 0, s::kSatCrackle),
               choice("model", "Model", 1, s::kSatModel, { "Classic", "Tape", "Tube", "Iron", "Germanium", "Vinyl" }),
               choice("speed", "Speed", 0, s::kSatSpeed, { "1", "2", "3" }),
               choice("oversampling", "Oversampling", 1, s::kOversampling, { "1x", "2x", "4x" }),
               choice("punish", "Punish", 0, s::kSatPunish, { "Off", "On" }),
               choice("thump", "Thump", 0, s::kSatThump, { "Off", "On" }),
               choice("steep", "Steep", 0, s::kSatSteep, { "Off", "On" }),
               choice("autoGain", "Auto Gain", 1, s::kAutoGain, { "Off", "On" }),
               choice("noiseFollow", "Noise", 1, s::kSatNoiseFollow, { "Always", "Follow" }) } };
}

ModuleSpec makeRedWall()
{
    namespace s = dsp::slot;
    return { ModuleKind::RedWall, FxType::Limiter, "HamloRedWall", 1180, 700, 8,
             { knob("thresholdDb", "Loudness", -30, 0, 0, s::kLimThresholdDb, 1, "dB"),
               knob("ceilingDb", "Limit", -20, 0, -1, s::kCeilingDb, 1, "dB"),
               knob("releaseMs", "Release", 1, 1000, 60, s::kLimiterReleaseMs, 0.33f, "ms"),
               knob("lookaheadMs", "Lookahead", 0.1f, 10, 2, s::kLookaheadMs, 0.5f, "ms"),
               knob("link", "Link", 0, 1, 1, s::kLimLink),
               knob("outputDb", "Output", -24, 0, 0, s::kLimOutputDb, 1, "dB"),
               knob("mix", "Mix", 0, 1, 1, -1),
               choice("style", "Style", 0, s::kLimStyle, { "GLASNOST", "STAKHANOV", "OCTOBER" }),
               choice("truePeak", "True Peak", 1, s::kLimTruePeak, { "Off", "On" }),
               choice("autoRelease", "Release", 1, s::kLimAutoRelease, { "Manual", "Auto" }),
               choice("monitor", "Monitor", 0, s::kLimMonitor, { "Normal", "Unity", "Delta" }) } };
}

ModuleSpec makeUdarnik()
{
    namespace s = dsp::slot;
    return { ModuleKind::Udarnik, FxType::TransientShaper, "HamloUdarnik", 1180, 700, 10,
             { knob("attackDb", "Snap", -24, 24, 0, s::kAttackGainDb, 1, "dB"),
               knob("sustainDb", "Body", -24, 24, 0, s::kSustainGainDb, 1, "dB"),
               knob("speed", "Speed", 0, 1, 0.5f, s::kTransientSpeed),
               knob("rise", "Rise", 0.5f, 2, 1, s::kTrRiseLog2, 0.5f, "x", ValueTransform::Log2),
               knob("decay", "Decay", 0.5f, 2, 1, s::kTrDecayLog2, 0.5f, "x", ValueTransform::Log2),
               knob("sensitivity", "Sensitivity", 0, 1, 1, s::kTrStrictness, 1, "", ValueTransform::OneMinus),
               knob("preserve", "Preserve", 0, 1, 1, s::kTrPreserve),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("outputDb", "Output", -24, 12, 0, s::kTrOutputDb, 1, "dB"),
               choice("style", "Mode", 0, s::kTrStyle, { "PRAVDA", "UDARNIK", "PERESTROIKA" }),
               choice("lookahead", "Look-ahead", 1, s::kTrLookahead, { "Sputnik", "Vostok", "Soyuz" }),
               choice("listen", "Listen", 0, s::kTrListen, { "Full", "Attack", "Body" }) } };
}

ModuleSpec makeZastava()
{
    namespace s = dsp::slot;
    return { ModuleKind::Zastava, FxType::GateExpander, "HamloZastava", 1180, 780, 12,
             { knob("thresholdDb", "Threshold", -90, 0, -40, s::kGateThresholdDb, 1, "dB"),
               knob("rangeDb", "Range", 0, 90, 40, s::kGateRangeDb, 1, "dB"),
               knob("attackMs", "Attack", 0.05f, 100, 0.5f, s::kGateAttackMs, 0.35f, "ms"),
               knob("holdMs", "Hold", 0, 500, 20, s::kGateHoldMs, 1, "ms"),
               knob("releaseMs", "Release", 1, 2000, 100, s::kGateReleaseMs, 0.4f, "ms"),
               choice("style", "Mode", 0, s::kGateStyle, { "ZASTAVA", "PATRUL", "TREVOGA" }),
               knob("ratio", "Ratio", 1, 100, 4, s::kGateRatio, 0.5f, ":1"),
               knob("hysteresisDb", "Hysteresis", 0, 24, 4, s::kGateHysteresisDb, 1, "dB"),
               knob("keyHpHz", "Key HP", 20, 10000, 20, s::kGateKeyHpHz, 0.5f, "Hz", ValueTransform::HighPassOff),
               knob("keyLpHz", "Key LP", 200, 20000, 20000, s::kGateKeyLpHz, 0.5f, "Hz", ValueTransform::LowPassOff),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("outputDb", "Output", -24, 12, 0, s::kGateOutputDb, 1, "dB"),
               choice("lookahead", "Look-ahead", 1, s::kGateLookahead, { "Sputnik", "Vostok", "Soyuz" }),
               choice("keyListen", "Key Listen", 0, s::kGateKeyListen, { "Off", "On" }),
               choice("flip", "Flip", 0, s::kGateFlip, { "Off", "On" }) } };
}

ModuleSpec makeShirota()
{
    namespace s = dsp::slot;
    return { ModuleKind::Shirota, FxType::StereoProcessor, "HamloShirota", 1180, 780, 9,
             { knob("width", "Width", 0, 2, 1, s::kStereoWidth),
               knob("stereoize", "Stereoize", 0, 1, 0.5f, s::kStStereoize),
               knob("space", "Space", 0, 1, 0.5f, s::kStSpace),
               knob("monoBassHz", "Mono Bass", 10, 500, 10, s::kStMonoBassHz, 0.5f, "Hz", ValueTransform::MonoBassOff),
               knob("rotation", "Rotation", -45, 45, 0, s::kStRotation, 1, "deg"),
               choice("style", "Mode", 0, s::kStStyle, { "EDINSTVO", "PROSTOR", "KOSMOS" }),
               knob("balance", "Balance", -1, 1, 0, s::kStereoBalance),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("outputDb", "Output", -24, 12, 0, s::kStOutputDb, 1, "dB"),
               choice("safe", "Safe", 1, s::kStSafe, { "Off", "On" }),
               choice("monoListen", "Mono Check", 0, s::kStMonoListen, { "Off", "On" }) } };
}

// HamloFader: HPEQ's Gain slot with the ranges of LayerSnapshot.cpp (gain -60 .. +24 dB, delay 0 .. 20 ms, target
// -40 .. 0 dB, polarity 0 none / 1 left / 2 right / 3 both, mute, solo), knobs then switches as the page's enums
ModuleSpec makeFader()
{
    namespace s = dsp::slot;
    return { ModuleKind::Fader, FxType::Gain, "HamloFader", 1180, 700, 4,
             { knob("gainDb", "Gain", -60, 24, 0, s::kGainDb, 1, "dB"),
               knob("delayMs", "Delay", 0, 20, 0, s::kFadDelayMs, 1, "ms"),
               knob("targetDb", "Target", -40, 0, -18, s::kFadTargetDb, 1, "dB"),
               knob("mix", "Mix", 0, 1, 1, -1),
               choice("polarity", "Polarity", 0, s::kFadPolarity, { "Off", "Left", "Right", "Both" }),
               choice("mute", "Mute", 0, s::kFadMute, { "Off", "On" }),
               choice("solo", "Solo", 0, s::kFadSolo, { "Off", "On" }) } };
}

float transformed(float value, ValueTransform transform) noexcept
{
    switch (transform)
    {
        case ValueTransform::Direct: return value;
        case ValueTransform::Log2: return std::log2(juce::jlimit(0.5f, 2.0f, value));
        case ValueTransform::OneMinus: return 1.0f - juce::jlimit(0.0f, 1.0f, value);
        case ValueTransform::HighPassOff: return value <= 20.0f ? 0.0f : value;
        case ValueTransform::LowPassOff: return value >= 20000.0f ? 0.0f : value;
        case ValueTransform::MonoBassOff: return value < 20.0f ? 0.0f : value;
    }
    return value;
}
} // namespace

const ModuleSpec& moduleSpec(ModuleKind kind)
{
    static const ModuleSpec comp = makeComp();
    static const ModuleSpec verb = makeVerb();
    static const ModuleSpec echo = makeEcho();
    static const ModuleSpec mag = makeMag();
    static const ModuleSpec redWall = makeRedWall();
    static const ModuleSpec udarnik = makeUdarnik();
    static const ModuleSpec zastava = makeZastava();
    static const ModuleSpec shirota = makeShirota();
    static const ModuleSpec fader = makeFader();
    switch (kind)
    {
        case ModuleKind::Comp: return comp;
        case ModuleKind::Verb: return verb;
        case ModuleKind::Echo: return echo;
        case ModuleKind::Mag: return mag;
        case ModuleKind::RedWall: return redWall;
        case ModuleKind::Udarnik: return udarnik;
        case ModuleKind::Zastava: return zastava;
        case ModuleKind::Shirota: return shirota;
        case ModuleKind::Fader: return fader;
    }
    return comp;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(ModuleKind kind)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& p : moduleSpec(kind).parameters)
    {
        const juce::ParameterID id { p.id, 1 };
        if (!p.choices.isEmpty())
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(id, p.name, p.choices,
                                                                    juce::roundToInt(p.defaultValue)));
            continue;
        }
        juce::NormalisableRange<float> range(p.minimum, p.maximum, p.interval, p.skew);
        auto attributes = juce::AudioParameterFloatAttributes {}.withLabel(p.unit);
        layout.add(std::make_unique<juce::AudioParameterFloat>(id, p.name, range, p.defaultValue, attributes));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

dsp::FxSlotParams makeSlot(ModuleKind kind, const std::vector<std::atomic<float>*>& values, bool active) noexcept
{
    const auto& spec = moduleSpec(kind);
    dsp::FxSlotParams slot;
    slot.active = active;
    slot.type = spec.type;
    slot.mix = 1.0f;
    const auto count = juce::jmin(values.size(), spec.parameters.size());
    for (size_t i = 0; i < count; ++i)
    {
        const auto& p = spec.parameters[i];
        const float value = values[i]->load(std::memory_order_relaxed);
        if (p.slotValueIndex < 0)
            slot.mix = juce::jlimit(0.0f, 1.0f, value);
        else
            slot.values[static_cast<size_t>(p.slotValueIndex)] = transformed(value, p.transform);
    }
    if (kind == ModuleKind::Comp)
        slot.values[static_cast<size_t>(dsp::slot::kKneeDb)] = juce::jmin(24.0f, slot.values[static_cast<size_t>(dsp::slot::kKneeDb)]);
    return slot;
}

int maximumLatencySamples(ModuleKind kind, double sampleRate) noexcept
{
    dsp::FxSlotParams slot;
    slot.active = true;
    slot.type = moduleSpec(kind).type;
    switch (kind)
    {
        case ModuleKind::Mag:
            slot.values[dsp::slot::kOversampling] = 2.0f;
            slot.values[dsp::slot::kSatModel] = 1.0f;
            slot.values[dsp::slot::kSatWow] = 1.0f;
            break;
        case ModuleKind::RedWall:
            slot.values[dsp::slot::kLookaheadMs] = 10.0f;
            slot.values[dsp::slot::kLimTruePeak] = 1.0f;
            break;
        case ModuleKind::Udarnik:
            slot.values[dsp::slot::kTrStyle] = 1.0f;
            slot.values[dsp::slot::kTrLookahead] = 2.0f;
            break;
        case ModuleKind::Zastava:
            slot.values[dsp::slot::kGateStyle] = 1.0f;
            slot.values[dsp::slot::kGateLookahead] = 2.0f;
            break;
        case ModuleKind::Shirota:
            slot.values[dsp::slot::kStStereoize] = 0.5f;
            slot.values[dsp::slot::kStSafe] = 1.0f;
            break;
        case ModuleKind::Comp: case ModuleKind::Verb: case ModuleKind::Echo: case ModuleKind::Fader: break;
    }
    return dsp::FxNodeProcessor::slotLatency(slot, sampleRate);
}

} // namespace hpeq::standalone
