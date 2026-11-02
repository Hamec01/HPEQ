// V1 against the M0 build itself: the output of every program and of the clock / stretch variants, playing 120 BPM
// (then a tempo ramp and a jump), hashed bit for bit. The table was recorded from R0 (5239801) before any lane code;
// a later stage must reproduce it exactly while every new control is at its default (the Legacy scene).
// A 0 in the table = record mode: the test prints the hash and fails.

#include <cstring>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;

class ScriptedHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(bpm);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double bpm = 120.0, ppq = 0.0;
};

std::uint64_t hashOf(const juce::AudioBuffer<float>& y)
{
    std::uint64_t h = 1469598103934665603ULL;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        for (int i = 0; i < y.getNumSamples(); ++i)
        {
            std::uint32_t bits = 0;
            const float v = y.getSample(ch, i);
            std::memcpy(&bits, &v, sizeof(bits));
            h = (h ^ bits) * 1099511628211ULL;
        }
    return h;
}

// 6 s: 120 BPM for 3 s, a ramp to 140 over 1 s, a jump back to bar 3 at 4.5 s.
juce::AudioBuffer<float> renderScripted(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block)
{
    ScriptedHead head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, block);
    auto y = x;
    juce::MidiBuffer midi;
    double ppq = 0.0;
    bool jumped = false;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        const double t = start / kFs;
        head.bpm = t < 3.0 ? 120.0 : t < 4.0 ? 120.0 + 20.0 * (t - 3.0) : 140.0;
        if (!jumped && t >= 4.5)
        {
            ppq = 8.0;
            jumped = true;
        }
        head.ppq = ppq;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
        ppq += n * head.bpm / (60.0 * kFs);
    }
    p.setPlayHead(nullptr);
    return y;
}

struct Variant
{
    const char* name;
    int program;
    std::vector<std::pair<const char*, double>> values;
};

const std::vector<Variant>& variants()
{
    static const std::vector<Variant> v {
        { "Default", 0, {} },
        { "Default swing 0.6 humanize 0.5", 0, { { "swing", 0.6 }, { "humanize", 0.5 } } },
        { "Default triplet 1/8", 0, { { "grid", 1 }, { "triplet", 1 } } },
        { "Default 1/32 steps 12", 0, { { "grid", 3 }, { "steps", 12 } } },
        { "Default free 180 ms", 0, { { "free", 1 }, { "freeMs", 180 } } },
        { "Half-Time Tape, rhythm stretch", 3, { { "timeStretch", 1 } } },
        { "Half-Time Tape, clean stretch", 3, { { "timeStretch", 2 } } },
        { "Double-Time Rush, clean stretch", 11, { { "timeStretch", 2 } } },
        { "Default order M-G-T, motion width", 0, { { "order", 5 }, { "motionTarget", 3 } } },
    };
    return v;
}

// Recorded from R0 (5239801): programs 1 .. 24, then the variants above.
constexpr std::uint64_t kPrograms[] { 0x958d48bbd97f7530ULL, 0xab0af8a709d3a3dbULL, 0xd781752a25bc90e6ULL, 0x4039b4406aa1816fULL, 0x761cf584edd718ebULL, 0xee1a9f85fa843638ULL, 0xd9a3c8294de725e7ULL, 0xc33856a02a25c2b5ULL, 0xfb380d7c8546ec94ULL, 0x8b88a323499c867ULL, 0x5e031e130f40b522ULL, 0xc245ba756766fbfaULL, 0x600be73bfd70bfdbULL, 0x63fbdc1490a040f1ULL, 0x8b7ca1acbf59121cULL, 0x8b45e8b6e5df98dULL, 0x3bd26839f0550486ULL, 0x15049fb3889efee6ULL, 0xd133067a7045f74fULL, 0xdb0961dc050699dfULL, 0xb72cf749c490f287ULL, 0x3f35639432be7e6eULL, 0xd7dcb363e22f8532ULL, 0xdf91f743144b1492ULL };
constexpr std::uint64_t kVariants[] { 0xd344d5fd840362caULL, 0x491e081d01664a4eULL, 0x1fba947f870d334ULL, 0xaf272aaaac35a713ULL, 0x580c6b50e63e3af6ULL, 0x2f82ea027b461f39ULL, 0x59363d0f7148fe02ULL, 0x51169fb8b5a5e7c0ULL, 0xe320f21136a788eULL };
} // namespace

class GoldenTests final : public juce::UnitTest
{
public:
    GoldenTests() : juce::UnitTest("Golden", "HamloRevolution") {}

    void runTest() override
    {
        const auto loop = drums(6.0);
        beginTest("V1: every program and the clock / stretch variants = the R0 output, bit for bit (blocks 256)");
        auto probe = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
        // HPEQ's programs (R6's genre presets after them are SIMPLE recipes: PresetTests)
        int programs = 0;
        while (programs + 1 < probe->getNumPrograms() && probe->genrePresetFor(programs + 1) == nullptr)
            ++programs;
        const bool recording = sizeof(kPrograms) / sizeof(kPrograms[0]) != static_cast<size_t>(programs);
        juce::String table;
        for (int i = 1; i <= programs; ++i)
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setCurrentProgram(i);
            const auto h = hashOf(renderScripted(*p, loop, 256));
            table << "0x" << juce::String::toHexString(static_cast<juce::int64>(h)) << "ULL, ";
            if (!recording)
                expect(h == kPrograms[i - 1], p->getProgramName(i));
        }
        logMessage("  programs: " + table);
        table.clear();
        const auto& vs = variants();
        const bool recordingVariants = sizeof(kVariants) / sizeof(kVariants[0]) != vs.size();
        for (size_t i = 0; i < vs.size(); ++i)
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setCurrentProgram(vs[i].program);
            for (const auto& [id, value] : vs[i].values)
                p->setParameterValue(id, value);
            const auto h = hashOf(renderScripted(*p, loop, 256));
            table << "0x" << juce::String::toHexString(static_cast<juce::int64>(h)) << "ULL, ";
            if (!recordingVariants)
                expect(h == kVariants[i], vs[i].name);
        }
        logMessage("  variants: " + table);
        expect(!recording && !recordingVariants, "record mode: paste the hashes above into the tables");
    }
};

static GoldenTests goldenTests;
} // namespace hamlo::test
