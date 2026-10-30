#pragma once
#include "Standalone/ModuleVariant.h"
#include "DSP/ScoreSettings.h"

namespace hpeq::standalone
{
namespace scoreparam
{
enum Global
{
    Mode, Source, Octaves, Mix, Output, Routing, MixLock, Width, Duck, Freeze,
    Feedback, FeedbackMs, FeedbackSync, FeedbackPitch, Color, Drive, LowCut, HighCut,
    Bands, SplitLow, SplitHigh, SpaceMix, SpaceDecay, SpaceSize, SpaceDamping, SpaceType, SpaceRoute,
    GlitchTarget, Glitch, GlitchMs, GlitchAmount, GlobalCount
};
enum Band { BandTime, BandFeedback, BandGain, BandPan, BandEnabled, BandCount };
enum Tap { On, Note, Time, Sync, Gain, Pan, Shape, Fragment, Repeats, Low, High, TapCount };
constexpr int bandIndex(int band, Band field) { return GlobalCount + band * BandCount + field; }
constexpr int tapIndex(int tap, Tap field) { return GlobalCount + 3 * BandCount + tap * TapCount + field; }
constexpr int Swing = GlobalCount + 3 * BandCount + 8 * TapCount;
constexpr int SpaceSend = Swing+1;
constexpr int Count = SpaceSend+8;
}
const std::vector<ParameterSpec>& scoreParameterSpecs();
hamlo::echo::ScoreSettings readScoreSettings(const std::vector<std::atomic<float>*>&, double bpm) noexcept;
juce::String noteName(int midi, bool flStudio);
int parseNote(const juce::String&, bool flStudio);
}
