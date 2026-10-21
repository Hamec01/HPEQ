#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace hpeq::test
{
// 32-bit float WAV: the fixtures and renders must not be re-quantised.
bool writeFloatWav(const juce::File& file, const juce::AudioBuffer<float>& audio, double sampleRate);
bool readWav(const juce::File& file, juce::AudioBuffer<float>& audio, double& sampleRate);

juce::String sha256Hex(const juce::File& file);
juce::String sha256Hex(const juce::AudioBuffer<float>& audio); // over raw float sample bytes

bool writeJson(const juce::File& file, const juce::var& value);
} // namespace hpeq::test
