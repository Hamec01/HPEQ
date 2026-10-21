#pragma once

#include <vector>

#include "Analysis/AudioCapture.h"
#include "Analysis/SpectrumAnalyzer.h"

namespace hpeq::ui
{
// UI-thread side of the analysers: drains the capture queue into history rings (real input and
// output PCM, sample-aligned) and computes averaged spectra of the selected channel view. Freeze
// keeps draining the queue but stops updating history and spectra.
class AnalysisModel
{
public:
    enum class View { Mid, Left, Right, Side };
    static constexpr int kHistory = 1 << 17;

    explicit AnalysisModel(AudioCapture& source);

    // Pull everything available; recompute spectra when not frozen.
    void update();

    bool frozen = false;
    View view = View::Mid;

    // Waveform history of the view signal: index 0 = oldest, kHistory - 1 = newest.
    float input(int index) const noexcept { return inputHistory[static_cast<size_t>((writePos + index) & (kHistory - 1))]; }
    float output(int index) const noexcept { return outputHistory[static_cast<size_t>((writePos + index) & (kHistory - 1))]; }

    const std::vector<float>& inputSpectrum() const noexcept { return inputDb; }
    const std::vector<float>& outputSpectrum() const noexcept { return outputDb; }
    const std::vector<float>& differenceSpectrum() const noexcept { return diffDb; }
    int fftSize() const noexcept { return analyzer.size(); }

    static constexpr float kDifferenceMaskDb = -80.0f; // fixed; bins with less input are not compared

private:
    float viewSample(float l, float r) const noexcept;

    AudioCapture& capture;
    SpectrumAnalyzer analyzer;
    std::vector<AudioCapture::Frame> scratch;
    std::vector<float> inputHistory, outputHistory, frameIn, frameOut, instantIn, instantOut;
    std::vector<float> inputDb, outputDb, diffDb;
    int writePos = 0;
};
} // namespace hpeq::ui
