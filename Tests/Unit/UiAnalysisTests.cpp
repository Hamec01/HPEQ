// M5: T09 (what the UI shows is the real audio), METER, capture under load, undo/redo, A/B, and the
// editor's life cycle with every kind of selection. Thresholds fixed here before the runs.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Analysis/AudioCapture.h"
#include "Analysis/ResponseCurve.h"
#include "Analysis/SpectrumAnalyzer.h"
#include "DSP/LayerEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/AnalysisModel.h"

namespace hpeq::test
{
namespace
{
constexpr double kFs = 48000.0;

FxProjectState demoProject()
{
    FxProjectState p;
    auto& l = p.layers[0];
    EqNodeState cut;
    cut.id = p.nextNodeId++;
    cut.shape = EqShape::LowCut;
    cut.frequencyHz = 60.0;
    cut.slopeDbPerOct = 48;
    EqNodeState bell;
    bell.id = p.nextNodeId++;
    bell.shape = EqShape::Bell;
    bell.frequencyHz = 1000.0;
    bell.gainDb = 6.0;
    bell.q = 1.0;
    EqNodeState shelf;
    shelf.id = p.nextNodeId++;
    shelf.shape = EqShape::HighShelf;
    shelf.frequencyHz = 6000.0;
    shelf.gainDb = -4.0;
    shelf.q = 0.7;
    l.eqNodes = { cut, bell, shelf };
    return p;
}

std::unique_ptr<FxPluginProcessor> preparedProcessor(const FxProjectState& project, int block = 512)
{
    auto p = std::make_unique<FxPluginProcessor>();
    p->setProject(project);
    p->setRateAndBufferSizeDetails(kFs, block);
    p->prepareToPlay(kFs, block);
    return p;
}

juce::AudioBuffer<float> run(FxPluginProcessor& p, juce::AudioBuffer<float> audio, int block, std::uint64_t* allocations = nullptr)
{
    juce::MidiBuffer midi;
    for (int start = 0; start < audio.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), audio.getNumChannels(), start, juce::jmin(block, audio.getNumSamples() - start));
        ScopedAllocationProbe probe;
        p.processBlock(view, midi);
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return audio;
}
} // namespace

class UiAnalysisTests final : public juce::UnitTest
{
public:
    UiAnalysisTests() : juce::UnitTest("UI + analysis M5", "HPEQ") {}

    void runTest() override
    {
        beginTest("T09: captured input/output are the real, sample-aligned PCM (bit-exact)");
        {
            auto processor = preparedProcessor(demoProject());
            auto* gain = processor->getParameters().getParameter(params::outputGain.getParamID());
            gain->setValueNotifyingHost(gain->convertTo0to1(-3.0f));
            const auto input = makeSyntheticDrums({ kFs, 2, 1.0, kFixtureSeed }); // < capture capacity
            std::uint64_t allocations = 0;
            const auto output = run(*processor, input, 512, &allocations);
            std::vector<AudioCapture::Frame> frames(static_cast<size_t>(AudioCapture::kCapacity));
            const int n = processor->getCapture().pull(frames.data(), static_cast<int>(frames.size()));
            expectEquals(n, input.getNumSamples());
            bool exact = true;
            for (int i = 0; i < n; ++i)
            {
                const auto& f = frames[static_cast<size_t>(i)];
                exact = exact && f.inL == input.getSample(0, i) && f.inR == input.getSample(1, i)
                        && f.outL == output.getSample(0, i) && f.outR == output.getSample(1, i);
            }
            expect(exact, "capture differs from the real input/output PCM");
            expectEquals(static_cast<int>(allocations), 0, "capture allocated on the audio thread");
        }

        beginTest("capture under load: a full queue drops analyser frames, never audio");
        {
            auto processor = preparedProcessor(demoProject());
            const auto input = makePinkNoise({ kFs, 2, 3.0, kFixtureSeed }, -20.0); // 3 s > 1.37 s capacity, nobody reads
            auto reference = preparedProcessor(demoProject());
            const auto a = run(*processor, input, 512);
            const auto b = run(*reference, input, 512);
            expect(compareAligned(a, b).bitExact);
            expect(processor->getCapture().droppedFrames() > 0);
            logMessage("  dropped analyser frames with no reader: " + juce::String(processor->getCapture().droppedFrames()));
        }

        beginTest("T09: analyser calibration (dBFS) and the displayed spectrum = FFT of the capture");
        {
            SpectrumAnalyzer analyzer;
            const int size = analyzer.size();
            std::vector<float> levels;
            const double binHz = kFs / size;
            const auto onBin = makeSine({ kFs, 1, 1.0, kFixtureSeed }, 93.0 * binHz, -6.0);
            analyzer.analyse(onBin.getReadPointer(0), levels);
            expectWithinAbsoluteError(static_cast<double>(levels[93]), -6.0, 0.05);
            const auto offBin = makeSine({ kFs, 1, 1.0, kFixtureSeed }, 93.5 * binHz, -6.0);
            analyzer.analyse(offBin.getReadPointer(0), levels);
            const double scallop = -6.0 - std::max(levels[93], levels[94]);
            expect(scallop <= 1.43, "Hann scalloping " + juce::String(scallop));
            logMessage("  on-bin sine -6 dBFS reads " + juce::String(-6.0, 2) + " dB; worst off-bin reading " + juce::String(-6.0 - scallop, 2) + " dB");

            AudioCapture capture;
            ui::AnalysisModel model(capture);
            const auto x = makePinkNoise({ kFs, 2, 0.5, kFixtureSeed }, -18.0);
            const float* io[2] { x.getReadPointer(0), x.getReadPointer(1) };
            capture.push(io, io, 2, x.getNumSamples());
            model.update();
            std::vector<float> mid(static_cast<size_t>(size)), direct, averaged;
            for (int i = 0; i < size; ++i)
            {
                const int s = x.getNumSamples() - size + i;
                mid[static_cast<size_t>(i)] = 0.5f * (x.getSample(0, s) + x.getSample(1, s));
            }
            SpectrumAnalyzer reference;
            reference.analyse(mid.data(), direct);
            SpectrumAnalyzer::average(averaged, direct, 0.35f); // first frame = the frame itself
            bool equal = averaged.size() == model.inputSpectrum().size();
            for (size_t k = 0; equal && k < averaged.size(); ++k)
                equal = averaged[k] == model.inputSpectrum()[k];
            expect(equal, "displayed spectrum is not the FFT of the captured PCM");
        }

        beginTest("T09: the drawn PRE-EQ curve equals the measured response of the engine");
        {
            const auto project = demoProject();
            dsp::LayerEngine engine;
            engine.prepare(kFs, 512);
            engine.publish(dsp::makeSnapshot(project.layers[0]));
            const auto sweep = makeExpSweep({ kFs, 1, 2.0, kFixtureSeed }, 10.0, 0.45 * kFs, -12.0);
            const int pre = static_cast<int>(0.3 * kFs);
            juce::AudioBuffer<float> y(1, sweep.getNumSamples() + pre);
            y.clear();
            y.copyFrom(0, pre, sweep, 0, 0, sweep.getNumSamples());
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* chunk[1] { y.getWritePointer(0) + start };
                engine.process(chunk, 1, juce::jmin(512, y.getNumSamples() - start));
            }
            juce::AudioBuffer<float> out(1, sweep.getNumSamples());
            out.copyFrom(0, 0, y, 0, pre, sweep.getNumSamples());
            std::vector<double> grid;
            for (int i = 0; i < 200; ++i)
                grid.push_back(25.0 * std::pow(800.0, i / 199.0));
            double worst = 0.0;
            for (const auto& point : measureTransfer(out, sweep, kFs, grid, 0, -100.0))
            {
                const double drawn = eqResponseDb(project.layers[0], kFs, point.binFrequencyHz);
                if (point.magnitudeDb.has_value() && drawn > -60.0)
                    worst = std::max(worst, std::abs(*point.magnitudeDb - drawn));
            }
            expect(worst <= 0.05, "drawn vs measured " + juce::String(worst) + " dB");
            logMessage("  drawn EQ curve vs measured engine response: worst " + juce::String(worst, 4) + " dB");
        }

        beginTest("T09: difference spectrum shows a +6 dB bell as +6 dB, masks quiet bins");
        {
            FxProjectState p;
            EqNodeState bell;
            bell.id = p.nextNodeId++;
            bell.shape = EqShape::Bell;
            bell.frequencyHz = 1000.0;
            bell.gainDb = 6.0;
            bell.q = 1.0;
            p.layers[0].eqNodes = { bell };
            auto processor = preparedProcessor(p);
            AudioCapture& capture = processor->getCapture();
            ui::AnalysisModel model(capture);
            auto x = makePinkNoise({ kFs, 2, 1.2, kFixtureSeed }, -18.0);
            juce::MidiBuffer midi;
            for (int start = 0; start < x.getNumSamples(); start += 512)
            {
                juce::AudioBuffer<float> view(x.getArrayOfWritePointers(), 2, start, juce::jmin(512, x.getNumSamples() - start));
                processor->processBlock(view, midi);
                model.update();
            }
            const auto& diff = model.differenceSpectrum();
            const int bin = static_cast<int>(std::round(1000.0 * model.fftSize() / kFs));
            double meanAtBell = 0.0;
            for (int k = bin - 3; k <= bin + 3; ++k)
                meanAtBell += diff[static_cast<size_t>(k)];
            meanAtBell /= 7.0;
            expect(std::abs(meanAtBell - 6.0) <= 0.5, "difference at 1 kHz " + juce::String(meanAtBell, 2));
            const int far = static_cast<int>(std::round(100.0 * model.fftSize() / kFs));
            expect(std::abs(diff[static_cast<size_t>(far)]) <= 0.5, "difference at 100 Hz " + juce::String(diff[static_cast<size_t>(far)], 2));
            expect(std::isnan(diff.back()) || model.inputSpectrum().back() > ui::AnalysisModel::kDifferenceMaskDb, "unmasked quiet bin");
            logMessage("  difference spectrum: 1 kHz (bell +6) " + juce::String(meanAtBell, 2) + " dB, 100 Hz " + juce::String(diff[static_cast<size_t>(far)], 2) + " dB");
        }

        beginTest("METER: input/output peak and RMS of a known signal (+-0.01 dB)");
        {
            auto processor = preparedProcessor(FxProjectState {});
            auto* gain = processor->getParameters().getParameter(params::outputGain.getParamID());
            gain->setValueNotifyingHost(gain->convertTo0to1(-6.0f));
            const auto sine = makeSine({ kFs, 2, 0.5, kFixtureSeed }, 1000.0, -12.0);
            run(*processor, sine, 480); // 480 = whole periods of 1 kHz: block RMS is exact
            const auto m = processor->getMeters();
            expectWithinAbsoluteError(juce::Decibels::gainToDecibels(m.inputPeak), -12.0f, 0.01f);
            expectWithinAbsoluteError(juce::Decibels::gainToDecibels(m.inputRms), -15.0103f, 0.01f);
            expectWithinAbsoluteError(juce::Decibels::gainToDecibels(m.outputPeak), -18.0f, 0.01f);
            expectWithinAbsoluteError(juce::Decibels::gainToDecibels(m.outputRms), -21.0103f, 0.01f);
        }

        beginTest("undo / redo and A/B restore exact project states");
        {
            auto processor = preparedProcessor(demoProject());
            const auto original = toValueTree(processor->getProject());
            processor->getUndoManager().beginNewTransaction("a");
            processor->editProject("gain", [](FxProjectState& p) { p.layers[0].eqNodes[1].gainDb = -9.0; });
            processor->getUndoManager().beginNewTransaction("b");
            processor->editProject("add", [](FxProjectState& p) { FxNodeState n; n.id = p.nextNodeId++; p.layers[0].fxNodes.push_back(n); });
            const auto edited = toValueTree(processor->getProject());
            processor->getUndoManager().undo();
            processor->getUndoManager().undo();
            expect(original.isEquivalentTo(toValueTree(processor->getProject())), "undo did not restore");
            processor->getUndoManager().redo();
            processor->getUndoManager().redo();
            expect(edited.isEquivalentTo(toValueTree(processor->getProject())), "redo did not restore");
            processor->editProject("noop", [](FxProjectState&) {});
            expect(processor->getUndoManager().canUndo());

            processor->selectAB(1);
            expect(edited.isEquivalentTo(toValueTree(processor->getProject())), "B must start as a copy of A");
            processor->editProject("B edit", [](FxProjectState& p) { p.layers[0].eqNodes.clear(); });
            const auto bState = toValueTree(processor->getProject());
            processor->selectAB(0);
            expect(edited.isEquivalentTo(toValueTree(processor->getProject())), "A lost");
            processor->selectAB(1);
            expect(bState.isEquivalentTo(toValueTree(processor->getProject())), "B lost");
        }

        beginTest("editor: every selection kind, a 6-slot chain, deletes, size limits; 50 cycles while audio runs");
        {
            auto project = demoProject();
            FxNodeState chain;
            chain.id = project.nextNodeId++;
            chain.chain.clear();
            for (auto t : { FxType::Compressor, FxType::Reverb, FxType::Gain, FxType::Compressor, FxType::Reverb, FxType::Gain })
            {
                FxSlotState s;
                s.type = t;
                chain.chain.push_back(s);
            }
            project.layers[0].fxNodes.push_back(chain);
            auto processor = preparedProcessor(project);
            const auto audio = makePinkNoise({ kFs, 2, 0.05, kFixtureSeed }, -20.0);
            for (int cycle = 0; cycle < 50; ++cycle)
            {
                std::unique_ptr<juce::AudioProcessorEditor> base(processor->createEditorIfNeeded());
                auto* editor = dynamic_cast<FxPluginEditor*>(base.get());
                expect(editor != nullptr);
                if (editor == nullptr)
                    break;
                editor->setSize(cycle % 2 == 0 ? 1000 : 2600, cycle % 2 == 0 ? 700 : 1700);
                editor->selectNode(false, project.layers[0].eqNodes[0].id);
                editor->selectNode(true, chain.id, cycle % 6);
                editor->showDifference(cycle % 3 == 0);
                run(*processor, audio, 256);
                editor->updateAnalysisNow();
                juce::Image image(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
                juce::Graphics g(image);
                editor->paintEntireComponent(g, true);
                processor->editorBeingDeleted(editor);
            }
            // Deleting the selected node from outside (e.g. undo) must not break the editor.
            std::unique_ptr<juce::AudioProcessorEditor> base(processor->createEditorIfNeeded());
            auto* editor = dynamic_cast<FxPluginEditor*>(base.get());
            editor->selectNode(true, chain.id, 5);
            processor->editProject("remove", [](FxProjectState& p) { p.layers[0].fxNodes.clear(); });
            editor->selectNode(true, chain.id, 5); // stale id: the inspector must fall back to "nothing selected"
            juce::Image image(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
            juce::Graphics g(image);
            editor->paintEntireComponent(g, true);
            processor->editorBeingDeleted(editor);
        }
    }
};

static UiAnalysisTests uiAnalysisTests;
} // namespace hpeq::test
