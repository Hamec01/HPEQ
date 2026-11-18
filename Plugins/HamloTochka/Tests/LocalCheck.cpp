#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "TochkaProcessor.h"

namespace
{
constexpr double rate = 48000.0;
constexpr int frames = 256;

void play(hamlo::tochka::Processor& processor, int blocks, hpeq::LoudnessMeter* measured = nullptr)
{
    juce::AudioBuffer<float> buffer(2, frames);
    juce::MidiBuffer midi;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < frames; ++i)
        {
            const float x = 0.16f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi
                                                               * (i + b * frames) * 1000.0 / rate));
            buffer.setSample(0, i, x);
            buffer.setSample(1, i, x);
        }
        processor.processBlock(buffer, midi);
        if (measured != nullptr)
            measured->process(buffer.getArrayOfReadPointers(), 2, frames);
    }
}

std::vector<float> renderAutomation(int blockSize)
{
    auto processor = std::make_unique<hamlo::tochka::Processor>();
    processor->prepareToPlay(rate, frames);
    auto band = processor->band(0);
    band.active = true;
    band.dynamic = true;
    band.gainDb = 6.0f;
    processor->editBand(0, band);

    std::vector<float> input(16384);
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = 0.2f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi
                                                   * static_cast<double>(i) * 1000.0 / rate));
    auto output = input;
    juce::MidiBuffer midi;
    for (int pos = 0; pos < static_cast<int>(input.size());)
    {
        if (pos == 8192)
        {
            band.gainDb = -6.0f;
            processor->editBand(0, band);
        }
        const auto end = juce::jmin(pos + blockSize, pos < 8192 ? 8192 : static_cast<int>(input.size()));
        float* channels[] { input.data() + pos, output.data() + pos };
        juce::AudioBuffer<float> buffer(channels, 2, end - pos);
        processor->processBlock(buffer, midi);
        pos = end;
    }
    return output;
}
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        const auto normal = renderAutomation(256);
        for (const auto blockSize : { 1, 64, 127, 512, 4096 })
        {
            const auto render = renderAutomation(blockSize);
            if (std::memcmp(normal.data(), render.data(), render.size() * sizeof(float)) != 0)
                throw std::runtime_error("Native automation depends on the block size");
        }

        auto processor = std::make_unique<hamlo::tochka::Processor>();
        processor->prepareToPlay(rate, frames);
        processor->startLearn();
        if (processor->stopLearn())
            throw std::runtime_error("Empty capture produced a match gain");

        hpeq::LoudnessMeter dry, matched;
        dry.prepare(rate, false);
        matched.prepare(rate, false);
        play(*processor, 450, &dry);

        auto band = processor->band(0);
        band.active = true;
        band.gainDb = 6.0f;
        processor->editBand(0, band);
        play(*processor, 40);
        processor->startLearn();
        play(*processor, 450);
        if (!processor->stopLearn() || std::abs(processor->matchDb() + 6.0f) > 0.3f)
            throw std::runtime_error("Learn Match did not compensate the 6 dB bell boost");
        play(*processor, 40);
        play(*processor, 450, &matched);
        const auto error = matched.integratedLufs() - dry.integratedLufs();
        if (std::abs(error) > 0.3)
            throw std::runtime_error("Held loudness difference exceeds 0.3 LU");

        processor->setCurrentProgram(8);
        const auto deEss = processor->band(0);
        if (!deEss.active || !deEss.dynamic || deEss.detect != 1 || processor->matchDb() != 0.0f)
            throw std::runtime_error("Factory de-ess did not set the voice detector and clear match gain");

        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor->createEditorIfNeeded());
        if (editor == nullptr || editor->getWidth() != 1180 || editor->getHeight() != 820)
            throw std::runtime_error("Unexpected editor size");
        if (argc == 2)
        {
            auto screenshot = editor->createComponentSnapshot(editor->getLocalBounds());
            juce::FileOutputStream file { juce::File(juce::String::fromUTF8(argv[1])) };
            if (!file.openedOk() || !juce::PNGImageFormat().writeImageToStream(screenshot, file))
                throw std::runtime_error("Could not write editor screenshot");
        }

        std::cout << "HamloTochka: native block sizes passed; held match difference " << error
                  << " LU; factory de-ess and editor passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
