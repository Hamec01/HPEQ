// HPEQ_Lab ui --out <dir>
//
// Renders the real editor to PNG after real audio has gone through the plugin: a demo project close
// to the owner's mock-up, built only from processors that exist (EQ shapes, Compressor, Reverb, Gain).

#include <chrono>
#include <iostream>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"

using namespace hpeq;
using namespace hpeq::test;

int runUiSnapshots(const juce::File& out)
{
    juce::ScopedJuceInitialiser_GUI gui;
    auto processor = std::make_unique<FxPluginProcessor>();

    FxProjectState project;
    auto& layer = project.layers[0];
    const auto eq = [&](EqShape shape, double hz, double db, double q, int slope)
    {
        EqNodeState n;
        n.id = project.nextNodeId++;
        n.shape = shape;
        n.frequencyHz = hz;
        n.gainDb = db;
        n.q = q;
        n.slopeDbPerOct = slope;
        layer.eqNodes.push_back(n);
        return n.id;
    };
    const auto fx = [&](double lo, double hi, std::initializer_list<std::pair<FxType, juce::NamedValueSet>> chain)
    {
        FxNodeState n;
        n.id = project.nextNodeId++;
        n.lowHz = lo;
        n.highHz = hi;
        n.chain.clear();
        for (const auto& [type, params] : chain)
        {
            FxSlotState s;
            s.type = type;
            s.parameters = params;
            n.chain.push_back(s);
        }
        layer.fxNodes.push_back(n);
        return n.id;
    };
    eq(EqShape::LowCut, 45.0, 0.0, 0.7, 96);
    const auto bellId = eq(EqShape::Bell, 2500.0, -3.5, 1.4, 12);
    eq(EqShape::HighShelf, 9000.0, 2.5, 0.7, 12);
    juce::NamedValueSet comp;
    comp.set("thresholdDb", -32.0);
    comp.set("ratio", 4.0);
    comp.set("attackMs", 5.0);
    comp.set("releaseMs", 120.0);
    juce::NamedValueSet rev;
    rev.set("decaySeconds", 1.6);
    rev.set("predelayMs", 18.0);
    juce::NamedValueSet gain;
    gain.set("gainDb", -4.0);
    const auto compId = fx(100.0, 250.0, { { FxType::Compressor, comp } });
    fx(450.0, 1200.0, { { FxType::Gain, gain }, { FxType::Compressor, comp } });
    fx(3000.0, 8000.0, { { FxType::Reverb, rev } });
    processor->setProject(project);
    processor->setRateAndBufferSizeDetails(48000.0, 512);
    processor->prepareToPlay(48000.0, 512);

    auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(processor->createEditorIfNeeded()));
    editor->setSize(1400, 900);

    auto audio = makeSyntheticDrums({ 48000.0, 2, 2.0, kFixtureSeed });
    const auto noise = makePinkNoise({ 48000.0, 2, 2.0, kFixtureSeed }, -24.0);
    for (int ch = 0; ch < 2; ++ch)
        audio.addFrom(ch, 0, noise, ch, 0, audio.getNumSamples());
    juce::MidiBuffer midi;
    for (int start = 0, block = 0; start < audio.getNumSamples(); start += 512, ++block)
    {
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, juce::jmin(512, audio.getNumSamples() - start));
        processor->processBlock(view, midi);
        if (block % 8 == 7)
            editor->updateAnalysisNow(); // the timer does this at 30 Hz in a host
    }
    editor->updateAnalysisNow();

    const auto save = [&](const juce::String& name)
    {
        const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
        auto file = out.getChildFile(name);
        file.getParentDirectory().createDirectory();
        file.deleteFile();
        juce::FileOutputStream stream(file);
        juce::PNGImageFormat png;
        png.writeImageToStream(image, stream);
        std::cout << file.getFullPathName() << "\n";
    };

    editor->selectNode(true, compId);
    save("hpeq_m5_compressor_selected.png");
    editor->selectNode(false, bellId);
    save("hpeq_m5_eq_selected.png");
    editor->showDifference(true);
    save("hpeq_m5_difference.png");

    // M7: more layers, layer 2 selected (a copy of layer 1 with width / pan, Side mode), 3 muted, 4 bypassed.
    {
        auto layered = processor->getProject();
        copyLayerInto(layered, layered.layers[0], 1);
        layered.layers[1].width = 1.4f;
        layered.layers[1].pan = -0.2f;
        layered.layers[1].mix = 0.7f;
        layered.layers[1].channel = ChannelMode::Side;
        copyLayerInto(layered, layered.layers[0], 2);
        layered.layers[2].mute = true;
        copyLayerInto(layered, layered.layers[0], 3);
        layered.layers[3].enabled = false;
        layered.routing = LayerRouting::Serial;
        processor->setProject(layered);
        processor->selectLayer(1);
        processor->dispatchPendingMessages(); // change message -> editor
        editor->showDifference(false);
        editor->updateAnalysisNow();
        save("hpeq_m7_layers.png");
        processor->setProject(project);
        processor->selectLayer(0);
        processor->dispatchPendingMessages();
    }

    // D6: modules on an EQ point (grid), a region's grid, one module's page.
    {
        auto withModules = processor->getProject();
        for (auto& n : withModules.layers[0].eqNodes)
            if (n.id == bellId)
            {
                FxSlotState a, b;
                a.type = FxType::Saturator;
                b.type = FxType::Compressor;
                n.chain = { a, b };
            }
        processor->setProject(withModules);
        processor->dispatchPendingMessages();
        editor->updateAnalysisNow();
        editor->selectNode(false, bellId, -1);
        save("hpeq_d6_point_modules_grid.png");
        editor->selectNode(true, compId, -1);
        save("hpeq_d6_region_modules_grid.png");
        editor->selectNode(true, compId, 0);
        save("hpeq_d6_module_page.png");
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // UI frame cost: analysis update (FFT x2) + full repaint at this size, as the 30 Hz timer does.
    editor->showDifference(false);
    editor->selectNode(true, compId);
    std::vector<double> frameMs;
    juce::Image frame(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    for (int i = 0; i < 60; ++i)
    {
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, (i * 1600) % (audio.getNumSamples() - 1600), 1600);
        processor->processBlock(view, midi);
        const auto t0 = std::chrono::steady_clock::now();
        editor->updateAnalysisNow();
        juce::Graphics g(frame);
        editor->paintEntireComponent(g, true);
        frameMs.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    }
    const auto t = summarise(frameMs);
    std::cout << "UI frame (update + full repaint " << editor->getWidth() << "x" << editor->getHeight() << "): p50 " << t.p50
              << " ms, p95 " << t.p95 << " ms, max " << t.max << " ms (30 Hz budget 33.3 ms)\n";

    processor->editorBeingDeleted(editor.get());
    editor.reset();
    return 0;
}
