#include "Harness/AudioFileIO.h"

#include <juce_cryptography/juce_cryptography.h>

namespace hpeq::test
{
bool writeFloatWav(const juce::File& file, const juce::AudioBuffer<float>& audio, double sampleRate)
{
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
                             .withSampleRate(sampleRate)
                             .withNumChannels(audio.getNumChannels())
                             .withBitsPerSample(32)
                             .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    auto writer = wav.createWriterFor(stream, options);
    if (writer == nullptr)
        return false;
    return writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());
}

bool readWav(const juce::File& file, juce::AudioBuffer<float>& audio, double& sampleRate)
{
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(file.createInputStream().release(), true));
    if (reader == nullptr)
        return false;
    audio.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
    sampleRate = reader->sampleRate;
    return reader->read(&audio, 0, audio.getNumSamples(), 0, true, true);
}

juce::String sha256Hex(const juce::File& file)
{
    return juce::SHA256(file).toHexString();
}

juce::String sha256Hex(const juce::AudioBuffer<float>& audio)
{
    juce::MemoryBlock bytes;
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        bytes.append(audio.getReadPointer(ch), sizeof(float) * static_cast<size_t>(audio.getNumSamples()));
    return juce::SHA256(bytes).toHexString();
}

bool writeJson(const juce::File& file, const juce::var& value)
{
    file.getParentDirectory().createDirectory();
    return file.replaceWithText(juce::JSON::toString(value, false, 6) + "\n");
}
} // namespace hpeq::test
