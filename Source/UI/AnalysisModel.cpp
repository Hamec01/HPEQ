#include "UI/AnalysisModel.h"

namespace hpeq::ui
{
AnalysisModel::AnalysisModel(AudioCapture& source)
    : capture(source),
      scratch(static_cast<size_t>(AudioCapture::kCapacity)),
      inputHistory(static_cast<size_t>(kHistory), 0.0f),
      outputHistory(static_cast<size_t>(kHistory), 0.0f)
{
    frameIn.resize(static_cast<size_t>(analyzer.size()));
    frameOut.resize(static_cast<size_t>(analyzer.size()));
}

float AnalysisModel::viewSample(float l, float r) const noexcept
{
    switch (view)
    {
        case View::Mid: return 0.5f * (l + r);
        case View::Left: return l;
        case View::Right: return r;
        case View::Side: return 0.5f * (l - r);
    }
    return l;
}

void AnalysisModel::update()
{
    const int n = capture.pull(scratch.data(), static_cast<int>(scratch.size()));
    if (frozen)
        return;
    for (int i = 0; i < n; ++i)
    {
        const auto& f = scratch[static_cast<size_t>(i)];
        inputHistory[static_cast<size_t>(writePos)] = viewSample(f.inL, f.inR);
        outputHistory[static_cast<size_t>(writePos)] = viewSample(f.outL, f.outR);
        writePos = (writePos + 1) & (kHistory - 1);
    }

    const int size = analyzer.size();
    for (int i = 0; i < size; ++i)
    {
        frameIn[static_cast<size_t>(i)] = input(kHistory - size + i);
        frameOut[static_cast<size_t>(i)] = output(kHistory - size + i);
    }
    analyzer.analyse(frameIn.data(), instantIn);
    analyzer.analyse(frameOut.data(), instantOut);
    SpectrumAnalyzer::average(inputDb, instantIn, 0.35f);
    SpectrumAnalyzer::average(outputDb, instantOut, 0.35f);
    SpectrumAnalyzer::difference(outputDb, inputDb, kDifferenceMaskDb, diffDb);
}
} // namespace hpeq::ui
