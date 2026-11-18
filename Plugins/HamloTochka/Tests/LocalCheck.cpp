#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "TochkaProcessor.h"
#include "UI/AnalysisModel.h"

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

void verifyGraph(hamlo::tochka::Processor& processor)
{
    processor.setCurrentProgram(0);
    auto band = processor.band(0);
    band.active = true;
    processor.editBand(0, band);
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    const float x = 69.0f + 1042.0f * std::log(1000.0f / 20.0f) / std::log(22000.0f / 20.0f);
    const juce::Point<float> centre(x, 290.0f);
    const auto event = [&](juce::Point<float> position, juce::Point<float> down, int mods, bool dragged = false)
    {
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position, juce::ModifierKeys(mods),
                                0, 0, 0, 0, 0, editor.get(), editor.get(), juce::Time::getCurrentTime(),
                                down, juce::Time::getCurrentTime(), 1, dragged);
    };
    const auto wheel = [&](float sign, int mods)
    {
        juce::MouseWheelDetails motion {};
        motion.deltaY = sign;
        editor->mouseWheelMove(event(centre, centre, mods), motion);
    };
    const auto before = processor.band(0).q;
    wheel(0.12f, 0);
    const auto widerQ = processor.band(0).q;
    wheel(-0.12f, 0);
    if (widerQ <= before || std::abs(processor.band(0).q - before) > 0.0001f)
        throw std::runtime_error("Graph wheel did not round-trip Q");
    wheel(0.12f, juce::ModifierKeys::shiftModifier);
    if (processor.band(0).q <= before || processor.band(0).q >= widerQ)
        throw std::runtime_error("Fine wheel did not apply a smaller Q change");

    wheel(0.12f, juce::ModifierKeys::ctrlModifier);
    if (std::abs(processor.band(0).gainDb - 0.75f) > 0.001f)
        throw std::runtime_error("Ctrl-wheel did not adjust gain");
    wheel(-0.12f, juce::ModifierKeys::altModifier);
    if (!processor.band(0).dynamic || std::abs(processor.band(0).rangeDb + 0.75f) > 0.001f)
        throw std::runtime_error("Alt-wheel did not enable and adjust dynamic range");

    band = processor.band(0);
    band.gainDb = 0.0f;
    band.rangeDb = -6.0f;
    processor.editBand(0, band);
    const juce::Point<float> handle(x, 324.5f);
    const juce::Point<float> lowered(x, 336.0f);
    editor->mouseDown(event(handle, handle, juce::ModifierKeys::leftButtonModifier));
    editor->mouseDrag(event(lowered, handle, juce::ModifierKeys::leftButtonModifier, true));
    editor->mouseUp(event(lowered, handle, 0, true));
    if (std::abs(processor.band(0).rangeDb + 8.0f) > 0.001f
        || std::abs(processor.band(0).gainDb) > 0.001f)
        throw std::runtime_error("Range-handle drag changed the wrong parameter");

    const int ctrlDrag = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::ctrlModifier;
    const juce::Point<float> up(x, 250.0f);
    const auto qBeforeDrag = processor.band(0).q;
    editor->mouseDown(event(centre, centre, ctrlDrag));
    editor->mouseDrag(event(up, centre, ctrlDrag, true));
    editor->mouseUp(event(up, centre, 0, true));
    if (processor.band(0).q <= qBeforeDrag || std::abs(processor.band(0).gainDb) > 0.001f)
        throw std::runtime_error("Ctrl-drag did not isolate Q");

    const auto q = processor.band(0).q;
    band = processor.band(0);
    band.shape = hpeq::EqShape::LowCut;
    processor.editBand(0, band);
    wheel(0.12f, 0);
    if (processor.band(0).slopeDbPerOct != 18 || processor.band(0).q != q)
        throw std::runtime_error("Cut wheel did not change slope independently");
}

void verifyDifference(hamlo::tochka::Processor& processor)
{
    processor.setCurrentProgram(0);
    processor.prepareToPlay(rate, frames);
    hpeq::ui::AnalysisModel analysis(processor.audioCapture());
    const auto count = analysis.fftSize();
    constexpr int bin = 170;
    auto band = processor.band(0);
    band.active = true;
    band.frequencyHz = static_cast<float>(bin * rate / count);
    band.gainDb = -6.0f;
    processor.editBand(0, band);
    processor.enableCapture(true);
    analysis.update();
    if (!std::isnan(analysis.differenceSpectrum()[bin]))
        throw std::runtime_error("Difference mode exposed a silent FFT bin");

    juce::AudioBuffer<float> buffer(2, frames);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < 4 * count; pos += frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            const auto x = static_cast<float>(0.15 * std::sin(juce::MathConstants<double>::twoPi * bin * (i + pos) / count));
            buffer.setSample(0, i, x);
            buffer.setSample(1, i, x);
        }
        processor.processBlock(buffer, midi);
    }
    analysis.update();
    processor.enableCapture(false);
    const auto delta = analysis.differenceSpectrum()[bin];
    if (!std::isfinite(delta) || std::abs(delta + 6.0f) > 0.15f)
        throw std::runtime_error("Measured difference did not show the -6 dB bell");

    juce::MemoryBlock saved;
    processor.showDifference(true);
    processor.getStateInformation(saved);
    processor.showDifference(false);
    processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    if (!processor.showsDifference())
        throw std::runtime_error("Difference view was lost on state restore");
    processor.showDifference(false);
    std::cout << "Measured spectrum difference: " << delta << " dB\n";
}

void verifyFactoryBank(hamlo::tochka::Processor& processor)
{
    std::map<juce::String, int> genreCounts;
    std::set<juce::String> names;
    const auto& programs = processor.presets();
    float peak = 0.0f;
    int dynamic = 0;
    for (int program = 0; program < processor.getNumPrograms(); ++program)
    {
        processor.setCurrentProgram(program);
        const auto& preset = programs[static_cast<size_t>(program)];
        if (!names.insert(processor.getProgramName(program)).second || juce::String(preset.description).isEmpty())
            throw std::runtime_error("Duplicate or undocumented preset");
        ++genreCounts[preset.genre];
        if (program < 10)
            continue;
        const auto band = processor.band(0);
        const bool external = processor.usesExternalKey(0);
        if (!band.active || external != preset.firstBandExternal)
            throw std::runtime_error("Preset failed to load its band or key mode");

        auto layout = processor.getBusesLayout();
        layout.inputBuses.set(1, external ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::disabled());
        if (!processor.setBusesLayout(layout))
            throw std::runtime_error("Factory test could not set sidechain layout");
        processor.prepareToPlay(rate, frames);
        juce::AudioBuffer<float> buffer(external ? 3 : 2, frames);
        juce::MidiBuffer midi;
        const auto iter = band.dynamic ? 150 : 35;
        for (int n = 0; n < iter; ++n)
        {
            for (int i = 0; i < frames; ++i)
            {
                const auto t = juce::MathConstants<double>::twoPi * (i + n * frames) / rate;
                const auto probe = static_cast<float>(0.178 * std::sin(band.frequencyHz * t));
                const auto body = static_cast<float>(0.043 * std::sin(1000.0 * t));
                buffer.setSample(0, i, probe + body);
                buffer.setSample(1, i, band.channel == hpeq::ChannelMode::Side ? -probe + body : probe + body);
                if (external)
                    buffer.setSample(2, i, probe);
            }
            processor.processBlock(buffer, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < frames; ++i)
                {
                    const auto x = buffer.getSample(ch, i);
                    if (!std::isfinite(x))
                        throw std::runtime_error("Non-finite factory preset: " + preset.name.toStdString());
                    peak = std::max(peak, std::abs(x));
                }
        }
        if (band.dynamic && band.shape == hpeq::EqShape::Bell)
        {
            const auto applied = processor.dynamicGain(0);
            if (applied * band.rangeDb <= 0.0f || std::abs(applied) < 0.1f
                || std::abs(applied) > std::abs(band.rangeDb) + 0.01f)
                throw std::runtime_error("Dynamic preset probe out of range: " + preset.name.toStdString()
                                         + " (" + std::to_string(applied) + " dB)");
            ++dynamic;
        }
    }
    if (names.size() != 210 || genreCounts["Factory"] != 10 || genreCounts["Boom Bap"] != 50
        || genreCounts["Trap"] != 50 || genreCounts["Techno"] != 50 || genreCounts["DnB"] != 50
        || peak > 0.70795f)
        throw std::runtime_error("Factory bank failed count or synthetic headroom checks");
    std::cout << "210 factory programs, 200 synthetic renders, " << dynamic
              << " dynamic bell probes; maximum sample peak " << juce::Decibels::gainToDecibels(peak) << " dBFS\n";

    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(1, juce::AudioChannelSet::disabled());
    if (!processor.setBusesLayout(layout))
        throw std::runtime_error("Could not restore factory test bus layout");
    processor.setCurrentProgram(0);
}

void runPreview(hamlo::tochka::Processor& processor, const juce::File& output)
{
    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(1, juce::AudioChannelSet::mono());
    if (!processor.setBusesLayout(layout))
        throw std::runtime_error("Native mono key layout failed");
    processor.setCurrentProgram(0);
    auto band = processor.band(0);
    band.active = true;
    band.dynamic = true;
    band.frequencyHz = 125.0f;
    band.thresholdDb = -24.0f;
    band.q = 4.0f;
    processor.editBand(0, band);
    processor.setExternalKey(0, true);
    processor.showDifference(true);
    processor.prepareToPlay(rate, frames);
    processor.enableCapture(true);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer(3, frames);
    for (int b = 0; b < 128; ++b)
    {
        for (int i = 0; i < frames; ++i)
        {
            const auto t = juce::MathConstants<double>::twoPi * (i + b * frames) / rate;
            const auto x = static_cast<float>(0.15 * std::sin(125.0 * t) + 0.06 * std::sin(2000.0 * t));
            buffer.setSample(0, i, x);
            buffer.setSample(1, i, x);
            buffer.setSample(2, i, static_cast<float>(0.145 * std::sin(125.0 * t)));
        }
        processor.processBlock(buffer, midi);
    }
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    auto screenshot = editor->createComponentSnapshot(editor->getLocalBounds());
    juce::FileOutputStream file(output);
    if (!file.openedOk() || !file.setPosition(0) || file.truncate().failed()
        || !juce::PNGImageFormat().writeImageToStream(screenshot, file))
        throw std::runtime_error("Could not write editor screenshot");
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
        verifyGraph(*processor);
        verifyDifference(*processor);
        verifyFactoryBank(*processor);
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

        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor->createEditor());
        if (editor == nullptr || editor->getWidth() != 1180 || editor->getHeight() != 820)
            throw std::runtime_error("Unexpected editor size");
        if (argc == 2)
        {
            editor.reset();
            runPreview(*processor, juce::File(juce::String::fromUTF8(argv[1])));
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
