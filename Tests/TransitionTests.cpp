// RISE / FILL (docs/RISE_FILL.md): the bar-aligned capture, RISE and FILL measured, determinism, the WAV, the
// preview, the page. HAMLO_RENDER=<folder>: also writes risers and fills for listening.

#include <cmath>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/TransitionRender.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionSimplePage.h"
#include "UI/HamloRevolutionTransitionPage.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace tr = hpeq::dsp::transition;
constexpr double kPi = 3.14159265358979323846;
constexpr int kBar = 96000; // 120 BPM, 48 kHz

class Head final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(120.0);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double ppq = 0.0;
};

void feed(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, 512);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        const int n = juce::jmin(512, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
}

juce::AudioBuffer<float> tone(int samples, double hz, double amp)
{
    juce::AudioBuffer<float> x(2, samples);
    for (int i = 0; i < samples; ++i)
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(amp * std::sin(2.0 * kPi * hz * i / kFs)));
    return x;
}

juce::AudioBuffer<float> pad(int samples)
{
    juce::AudioBuffer<float> x(2, samples);
    for (int i = 0; i < samples; ++i)
    {
        const double t = i / kFs;
        double v = 0.0;
        for (const double hz : { 220.0, 277.18, 329.63 })
            for (int k = 1; k <= 4; ++k)
                v += std::sin(2.0 * kPi * hz * k * t + k) / (k * 6.0);
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(0.25 * v));
    }
    return x;
}

double rmsDb(const juce::AudioBuffer<float>& b, int from, int n)
{
    double e = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = from; i < from + n; ++i)
            e += static_cast<double>(b.getSample(ch, i)) * b.getSample(ch, i);
    return 10.0 * std::log10(e / (2.0 * n) + 1.0e-20);
}

// the spectral centroid over [from, from + 8192)
double centroid(const juce::AudioBuffer<float>& b, int from)
{
    constexpr int order = 13, size = 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<float> data(2 * size, 0.0f);
    for (int i = 0; i < size; ++i)
        data[static_cast<size_t>(i)] = b.getSample(0, from + i) * static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPi * i / size));
    fft.performFrequencyOnlyForwardTransform(data.data());
    double num = 0.0, den = 0.0;
    for (int k = 1; k < size / 2; ++k)
    {
        num += k * kFs / size * data[static_cast<size_t>(k)];
        den += data[static_cast<size_t>(k)];
    }
    return num / std::max(1.0e-12, den);
}

double crossingHz(const juce::AudioBuffer<float>& y, int from, int n)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = from + 1; i < from + n; ++i)
    {
        const double a = y.getSample(0, i - 1), b = y.getSample(0, i);
        if (a < 0.0 && b >= 0.0)
        {
            const double t = (i - 1) + a / (a - b);
            if (first < 0.0)
                first = t;
            last = t;
            ++count;
        }
    }
    return count < 2 ? -1.0 : (count - 1) * kFs / (last - first);
}

tr::Settings settings(int kind, int bars, std::array<double, 4> knobs, std::uint64_t seed = 7)
{
    tr::Settings s;
    s.kind = kind;
    s.bars = bars;
    s.bpm = 120.0;
    s.sampleRate = kFs;
    s.knobs = knobs;
    s.seed = seed;
    return s;
}

void writeWav(const juce::AudioBuffer<float>& b, const juce::String& name)
{
    const auto folder = juce::SystemStats::getEnvironmentVariable("HAMLO_RENDER", {});
    if (folder.isEmpty())
        return;
    const juce::File dir(folder);
    dir.createDirectory();
    const auto file = dir.getChildFile(name + ".wav");
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream(file.createOutputStream().release());
    if (auto writer = wav.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(kFs).withNumChannels(2).withBitsPerSample(24)))
        writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}
} // namespace

class TransitionTests final : public juce::UnitTest
{
public:
    TransitionTests() : juce::UnitTest("Transition", "HamloRevolution") {}

    void runTest() override
    {
        const auto testFolder = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloRevolutionTransitionTests");

        beginTest("Capture: a click on every bar line at 120 BPM: the last 2 bars start on a click, exactly 2 bars long");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            juce::AudioBuffer<float> clicks(2, 6 * kBar);
            clicks.clear();
            for (int b = 0; b < 6; ++b)
                for (int ch = 0; ch < 2; ++ch)
                    clicks.setSample(ch, b * kBar, 1.0f);
            feed(*p, clicks);
            juce::AudioBuffer<float> out;
            double bpm = 0.0;
            bool aligned = false, complete = false;
            expect(p->captureLastBars(2, out, bpm, aligned, complete));
            int firstClick = -1, clicksFound = 0;
            for (int i = 0; i < out.getNumSamples(); ++i)
                if (out.getSample(0, i) > 0.5f)
                {
                    if (firstClick < 0)
                        firstClick = i;
                    ++clicksFound;
                }
            logMessage("  captured " + juce::String(out.getNumSamples()) + " samples (2 bars = " + juce::String(2 * kBar) + "), the first click at "
                       + juce::String(firstClick) + ", clicks " + juce::String(clicksFound) + ", aligned " + juce::String(static_cast<int>(aligned)));
            expectEquals(out.getNumSamples(), 2 * kBar);
            expectEquals(firstClick, 0);
            expectEquals(clicksFound, 2);
            expect(aligned && complete && std::abs(bpm - 120.0) < 1.0e-9);
        }

        const auto loop = drums(8.0);
        const auto lastBars = [&](const juce::AudioBuffer<float>& src, int bars)
        {
            juce::AudioBuffer<float> b(2, bars * kBar);
            for (int ch = 0; ch < 2; ++ch)
                b.copyFrom(ch, 0, src, ch, src.getNumSamples() - bars * kBar, bars * kBar);
            return b;
        };

        beginTest("RISE: N bars long, the last quarter >= 6 dB louder, the centroid rising, the fade to 0 at the bar line");
        for (const auto* source : { "drums", "pad" })
        {
            const juce::String label(source);
            const auto src = label == "drums" ? lastBars(loop, 2) : pad(2 * kBar);
            const auto out = tr::render(src, settings(tr::Rise, 2, tr::defaultKnobs(tr::Rise)));
            const int q = out.getNumSamples() / 4;
            const double rise = rmsDb(out, 3 * q, q) - rmsDb(out, 0, q);
            const double c0 = centroid(out, q / 2), c1 = centroid(out, out.getNumSamples() - 8192 - 400);
            logMessage("  " + label + ": " + juce::String(out.getNumSamples()) + " samples; last quarter " + juce::String(rise, 1)
                       + " dB above the first; centroid " + juce::String(c0, 0) + " -> " + juce::String(c1, 0) + " Hz");
            expectEquals(out.getNumSamples(), 2 * kBar);
            expect(rise >= 6.0, juce::String(rise));
            expect(c1 > c0, "brighter");
            expect(std::abs(out.getSample(0, out.getNumSamples() - 1)) < 1.0e-4f, "lands on the bar");
            writeWav(src, "rise_" + label + "_source");
            writeWav(out, "rise_" + label);
        }

        beginTest("RISE on a 440 Hz sine, PITCH 100 %: the end an octave up (within 3 %); grain edges click-free (<= -40 dB)");
        {
            const auto src = tone(2 * kBar, 440.0, 0.5);
            const auto out = tr::render(src, settings(tr::Rise, 2, { 1.0, 1.0, 0.0, 0.0 }));
            const double hz = crossingHz(out, out.getNumSamples() - 1600, 1200);
            double worst = 0.0;
            for (int i = 4; i < out.getNumSamples(); ++i)
                worst = std::max(worst, std::abs(static_cast<double>(out.getSample(0, i)) - 4.0 * out.getSample(0, i - 1) + 6.0 * out.getSample(0, i - 2)
                                                 - 4.0 * out.getSample(0, i - 3) + out.getSample(0, i - 4)));
            const double peak = out.getMagnitude(0, 0, out.getNumSamples());
            logMessage("  the end at " + juce::String(hz, 1) + " Hz (880 expected); the fourth difference " + juce::String(20.0 * std::log10(worst / peak), 1)
                       + " dB re the peak");
            expect(std::abs(hz / 880.0 - 1.0) <= 0.03, juce::String(hz));
            expect(20.0 * std::log10(worst / peak) <= -40.0);
        }

        beginTest("FILL: N bars long; the bars before the last >= 75 % the source; the last bar re-cut; the last beat rolled");
        {
            const auto src = lastBars(loop, 2);
            const auto out = tr::render(src, settings(tr::Fill, 2, tr::defaultKnobs(tr::Fill), 11));
            // the bars before the last: the source itself, up to one gain (the clip is kept under -0.1 dBFS)
            double sxy = 0.0, sxx = 0.0;
            for (int i = 0; i < kBar; ++i)
            {
                sxy += static_cast<double>(out.getSample(0, i)) * src.getSample(0, i);
                sxx += static_cast<double>(src.getSample(0, i)) * src.getSample(0, i);
            }
            const double gain = sxy / std::max(1.0e-20, sxx);
            int same = 0;
            for (int i = 0; i < kBar; ++i)
                same += std::abs(out.getSample(0, i) - gain * src.getSample(0, i)) < 1.0e-6;
            double diff = 0.0, ref = 0.0;
            for (int i = kBar; i < 2 * kBar; ++i)
            {
                diff += std::pow(static_cast<double>(out.getSample(0, i)) - src.getSample(0, i), 2.0);
                ref += std::pow(static_cast<double>(src.getSample(0, i)), 2.0);
            }
            // the roll: the last beat's first half is its first 1/16 twice (grains from the beat's start)
            const int beat = kBar / 4, sixteenth = kBar / 16, start = kBar + 3 * beat;
            double num = 0.0, ea = 0.0, eb = 0.0;
            for (int i = 200; i < sixteenth - 200; ++i)
            {
                const double a = out.getSample(0, start + i), b = out.getSample(0, start + sixteenth + i);
                num += a * b;
                ea += a * a;
                eb += b * b;
            }
            const double correlation = num / std::sqrt(std::max(1.0e-20, ea * eb));
            logMessage("  bar 1 identical " + juce::String(100.0 * same / kBar, 1) + " %; bar 2 differs " + juce::String(10.0 * std::log10(diff / ref), 1)
                       + " dB; the roll's repeats correlate " + juce::String(correlation, 3));
            expectEquals(out.getNumSamples(), 2 * kBar);
            expect(same >= 0.75 * kBar);
            expect(10.0 * std::log10(diff / ref) > -20.0);
            expect(correlation > 0.9, juce::String(correlation));
            writeWav(src, "fill_drums_source");
            writeWav(out, "fill_drums");
            writeWav(tr::render(lastBars(loop, 4), settings(tr::Fill, 4, tr::defaultKnobs(tr::Fill), 23)), "fill_drums_4bars");
            writeWav(tr::render(lastBars(loop, 1), settings(tr::Rise, 1, tr::defaultKnobs(tr::Rise), 5)), "rise_drums_1bar");
            writeWav(tr::render(lastBars(loop, 4), settings(tr::Rise, 4, tr::defaultKnobs(tr::Rise), 9)), "rise_drums_4bars");
        }

        beginTest("Determinism: the same source, settings and seed = the same samples; a new seed = another clip");
        {
            const auto src = lastBars(loop, 2);
            for (int kind : { static_cast<int>(tr::Rise), static_cast<int>(tr::Fill) })
            {
                const auto a = tr::render(src, settings(kind, 2, tr::defaultKnobs(kind), 3)), b = tr::render(src, settings(kind, 2, tr::defaultKnobs(kind), 3));
                expect(bitExact(a, b), "same");
                bool differs = false;
                for (std::uint64_t seed = 4; seed < 12 && !differs; ++seed)
                    differs = !bitExact(a, tr::render(src, settings(kind, 2, tr::defaultKnobs(kind), seed)));
                expect(differs, "a new seed");
            }
        }

        beginTest("The WAV and the preview: written, reads back (24-bit), the preview plays the clip through the output");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setRendersFolder(testFolder);
            feed(*p, loop);
            const auto t = p->makeTransition(tr::Rise, 2, tr::defaultKnobs(tr::Rise), true, true);
            expect(t.valid && t.file.existsAsFile(), t.file.getFullPathName());
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(t.file.createInputStream().release(), true));
            expect(reader != nullptr);
            if (reader == nullptr)
                return;
            const auto clip = p->transitionClip();
            juce::AudioBuffer<float> back(2, static_cast<int>(reader->lengthInSamples));
            reader->read(&back, 0, back.getNumSamples(), 0, true, true);
            double worst = 0.0;
            int worstAt = 0, worstCh = 0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < clip.getNumSamples(); ++i)
                    if (std::abs(back.getSample(ch, i) - clip.getSample(ch, i)) > worst)
                    {
                        worst = std::abs(back.getSample(ch, i) - clip.getSample(ch, i));
                        worstAt = i;
                        worstCh = ch;
                    }
            juce::ignoreUnused(worstAt, worstCh);
            logMessage("  " + t.file.getFileName() + ": " + juce::String(back.getNumSamples()) + " samples, largest difference " + juce::String(worst, 9));
            expectEquals(back.getNumSamples(), clip.getNumSamples());
            expect(clip.getMagnitude(0, 0, clip.getNumSamples()) <= 0.98855f && clip.getMagnitude(1, 0, clip.getNumSamples()) <= 0.98855f, "<= -0.1 dBFS");
            expect(worst <= std::pow(2.0, -22.0));
            // preview
            p->setPreview(true);
            juce::AudioBuffer<float> block(2, 512);
            block.clear();
            juce::MidiBuffer midi;
            p->processBlock(block, midi);
            bool played = true;
            for (int i = 0; i < 512; ++i)
                played = played && block.getSample(0, i) == clip.getSample(0, i);
            expect(played, "the clip through the output");
            p->setPreview(false);
            testFolder.deleteRecursively();
        }

        beginTest("A sample dropped on the page: the source (resampled 44.1 -> 48 kHz, repeated to the bars); USE INPUT goes back");
        {
            testFolder.createDirectory();
            // a 0.8 s, 44.1 kHz mono WAV of 1 kHz
            const auto sample = testFolder.getChildFile("dropped_1k.wav");
            {
                juce::AudioBuffer<float> b(1, static_cast<int>(0.8 * 44100.0));
                for (int i = 0; i < b.getNumSamples(); ++i)
                    b.setSample(0, i, static_cast<float>(0.5 * std::sin(2.0 * kPi * 1000.0 * i / 44100.0)));
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::OutputStream> stream(sample.createOutputStream().release());
                if (auto writer = wav.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(44100.0).withNumChannels(1).withBitsPerSample(24)))
                    writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
            }
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setRendersFolder(testFolder.getChildFile("renders"));
            feed(*p, loop);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            editor->getSimplePage()->pressTransition(1); // FILL
            auto* page = editor->getTransitionPage();
            page->setKnob(0, 0.0, false);
            page->setKnob(1, 0.0, false);
            page->setKnob(2, 0.0, false);
            page->setKnob(3, 0.0, true); // no chops, no roll, no ending: the source itself
            expect(page->isInterestedInFileDrag({ sample.getFullPathName() }));
            expect(!page->isInterestedInFileDrag({ testFolder.getChildFile("notes.txt").getFullPathName() }));
            page->filesDropped({ sample.getFullPathName() }, 10, 10);
            const auto t = p->lastTransition();
            expect(t.sourceFile == sample, t.sourceFile.getFullPathName());
            const auto clip = p->transitionClip();
            const double hz = crossingHz(clip, 1000, 24000);
            // the repeat: 0.8 s at 48 kHz later, the same samples (up to one gain)
            const int period = static_cast<int>(std::floor(0.8 * 44100.0 / (44100.0 / kFs)));
            double num = 0.0, ea = 0.0, eb = 0.0;
            for (int i = 2000; i < 6000; ++i)
            {
                const double a = clip.getSample(0, i), b = clip.getSample(0, i + period);
                num += a * b;
                ea += a * a;
                eb += b * b;
            }
            const double repeat = num / std::sqrt(std::max(1.0e-20, ea * eb));
            logMessage("  from " + t.sourceFile.getFileName() + ": " + juce::String(hz, 1) + " Hz at 48 kHz (1000 expected); repeated after "
                       + juce::String(period) + " samples (correlation " + juce::String(repeat, 4) + ")");
            expect(std::abs(hz - 1000.0) < 2.0, juce::String(hz));
            expect(repeat > 0.999, juce::String(repeat));
            page->pressUseInput();
            expect(p->lastTransition().sourceFile == juce::File(), "USE INPUT: the input again");
            expect(!p->useSampleForTransition(testFolder.getChildFile("missing.wav")), "an unreadable file is refused");
            testFolder.deleteRecursively();
        }

        beginTest("Page: SIMPLE's [RISE] opens the transition page with a fresh clip; BACK returns to SIMPLE");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setRendersFolder(testFolder);
            feed(*p, loop);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            editor->getSimplePage()->pressTransition(0);
            auto* page = editor->getTransitionPage();
            expect(page != nullptr && page->isVisible() && !editor->getSimplePage()->isVisible());
            expect(p->lastTransition().valid && p->lastTransition().settings.kind == tr::Rise);
            page->chooseKind(tr::Fill);
            expect(p->lastTransition().settings.kind == tr::Fill);
            page->chooseBars(4);
            expectEquals(p->lastTransition().settings.bars, 4);
            page->pressBack();
            expect(editor->getSimplePage()->isVisible() && !page->isVisible());
            testFolder.deleteRecursively();
        }
    }
};

static TransitionTests transitionTests;
} // namespace hamlo::test
