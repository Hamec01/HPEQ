#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace hpeq::params
{
// Host-automatable parameters. IDs are permanent: renaming or reusing one breaks saved projects
// and automation. New parameters get new IDs and `kParameterVersion` of the release adding them.
constexpr int kParameterVersion = 1;

inline const juce::ParameterID outputGain { "outputGain", kParameterVersion };
inline const juce::ParameterID bypass { "bypass", kParameterVersion };

constexpr float kOutputGainMinDb = -24.0f;
constexpr float kOutputGainMaxDb = 24.0f;

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        outputGain,
        "Output Gain",
        juce::NormalisableRange<float> { kOutputGainMinDb, kOutputGainMaxDb, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterBool>(bypass, "Bypass", false));
    return layout;
}
} // namespace hpeq::params
