#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hpeq::test
{
// Fixture set `fx_addon_v1` (04_ACCEPTANCE_MATRIX.md §2). Every generator is a pure function of its
// arguments: same seed + rate + length -> bit-identical buffer on every run and machine.
constexpr std::uint32_t kFixtureSeed = 24681357u;
constexpr const char* kFixtureSetName = "fx_addon_v1";

// Highest frequency a generator may produce: 0.45 * fs. Above it a request is N/A, never aliased.
double maxValidFrequency(double sampleRate) noexcept;
bool isFrequencyValid(double frequencyHz, double sampleRate) noexcept;

struct SignalRequest
{
    double sampleRate = 48000.0;
    int numChannels = 2;
    double durationSeconds = 1.0;
    std::uint32_t seed = kFixtureSeed;
};

// Deterministic PRNG (splitmix64): independent of std:: / JUCE implementation details.
class DeterministicRandom
{
public:
    explicit DeterministicRandom(std::uint64_t seed) noexcept : state(seed) {}
    std::uint64_t nextU64() noexcept;
    double nextBipolar() noexcept; // uniform in [-1, 1)

private:
    std::uint64_t state;
};

juce::AudioBuffer<float> makeSilence(const SignalRequest&);
juce::AudioBuffer<float> makeDc(const SignalRequest&, float level);
juce::AudioBuffer<float> makeSubnormal(const SignalRequest&);
juce::AudioBuffer<float> makeImpulse(const SignalRequest&, int positionSamples, float amplitude);
juce::AudioBuffer<float> makeStep(const SignalRequest&, int positionSamples, float amplitude);
juce::AudioBuffer<float> makeSine(const SignalRequest&, double frequencyHz, double peakDbfs);
// Exponential (Farina) sweep from f0 to f1, constant peak level, 5 ms raised-cosine fades.
juce::AudioBuffer<float> makeExpSweep(const SignalRequest&, double f0, double f1, double peakDbfs);
juce::AudioBuffer<float> makeWhiteNoise(const SignalRequest&, double rmsDbfs);
juce::AudioBuffer<float> makePinkNoise(const SignalRequest&, double rmsDbfs);
// SMPTE IMD: 60 Hz + 7 kHz, 4:1 amplitude ratio, combined peak at peakDbfs.
juce::AudioBuffer<float> makeDualToneImd(const SignalRequest&, double peakDbfs);
// 1 kHz tone alternating loud/quiet every segmentSeconds: compressor attack/release fixture.
juce::AudioBuffer<float> makeGatedBurst(const SignalRequest&, double loudDbfs, double quietDbfs, double segmentSeconds);

enum class StereoPlacement { LeftOnly, RightOnly, MidOnly, SideOnly };
// Pink noise placed with M = (L+R)/2, S = (L-R)/2: MidOnly -> L == R, SideOnly -> L == -R.
juce::AudioBuffer<float> makeStereoPlacement(const SignalRequest&, StereoPlacement, double rmsDbfs);

// Synthetic kick/snare/hat loop at 120 BPM, one bar of 16ths, repeated to the requested length.
juce::AudioBuffer<float> makeSyntheticDrums(const SignalRequest&);

// One named fixture of the set. `valid == false` means N/A for this rate (e.g. 15 kHz at low fs).
struct Fixture
{
    std::string name;
    std::string description;
    bool valid = true;
    juce::AudioBuffer<float> audio;
};

// The full fx_addon_v1 signal list for one rate/channel/duration combination.
std::vector<Fixture> makeFixtureSet(const SignalRequest&);

const std::vector<double>& fixtureSineFrequencies();
} // namespace hpeq::test
