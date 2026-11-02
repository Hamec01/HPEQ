#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hamlo::echo
{
// Isolated candidate renderer. It is deliberately not yet attached to the plugin.
struct Reflection
{
    bool enabled = false, sync = false, reverse = false;
    int targetNote = 72;
    double timeMs = 125.0, beats = 0.5, gain = 0.125, pan = 0.0;
    double reverseMs = 125.0;
};

struct Score
{
    int sourceNote = 72;
    double bpm = 120.0;
    std::array<Reflection, 8> taps;
};

class ReflectionPrototype
{
public:
    void prepare(double sampleRate, int maximumBlockSize);
    void setScore(const Score&) noexcept;
    void process(const float* const* input, float* const* wet, int channels, int samples) noexcept;
    int pitchAccumulationSamples(int semitones) const noexcept;
    int reverseAccumulationSamples(int tap) const noexcept;
    size_t historyBytes() const noexcept;
    static size_t sharedTableBytes() noexcept;

private:
    struct Head { double read = 0.0; int phase = 0; bool active = false; };
    struct Voice
    {
        std::array<Head, 2> heads;
        int shift = 0, delay = 0, reverseLength = 0, windowOffset = 0;
        double ratio = 1.0, left = 1.0, right = 1.0;
        bool initialised = false;
    };
    float integerAt(int channel, int64_t position) const noexcept;
    double interpolate(int channel, double position, int shift) const noexcept;
    double matchedStart(double candidate, double continuing, int channels) const noexcept;
    void initialise(Voice&) noexcept;
    double forward(Voice&, int channel) const noexcept;
    double backwards(Voice&, int channel) const noexcept;

    Score score;
    std::array<Voice, 8> voices;
    std::array<std::vector<float>, 2> history;
    std::vector<double> window;
    double rate = 48000.0;
    int capacity = 0, write = 0, grain = 0, search = 0;
    int64_t clock = 0;
};
}
