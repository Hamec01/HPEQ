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
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloVerbPanel.h"
#include "UI/HamloZastavaPanel.h"

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
        // Keep audio running so the needles show real values (they move with VU-like inertia).
        auto more = makeSyntheticDrums({ 48000.0, 2, 1.0, kFixtureSeed });
        for (int start = 0; start < more.getNumSamples(); start += 512)
        {
            juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
            processor->processBlock(view, midi);
            if ((start / 512) % 4 == 3)
                editor->updateAnalysisNow();
        }
        save("hpeq_d6_module_page.png");
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 2: the HamloVerb page (a reverb added from the UI = Hall), DECAY and TIME views,
    // then Cathedral 1970 with long bass and freeze, then an old project's reverb (Classic).
    {
        auto withVerb = processor->getProject();
        FxNodeState n;
        n.id = withVerb.nextNodeId++;
        n.lowHz = 300.0;
        n.highHz = 9000.0;
        n.chain = { defaultModule(FxType::Reverb) };
        auto& rv = n.chain[0].parameters;
        rv.set("bassMult", 1.4);
        rv.set("duck", 0.3);
        const auto verbId = n.id;
        withVerb.layers[0].fxNodes.push_back(n);
        processor->setProject(withVerb);
        processor->dispatchPendingMessages();
        editor->selectNode(true, verbId, 0);
        auto more = makeSyntheticDrums({ 48000.0, 2, 1.5, kFixtureSeed });
        for (int start = 0; start < more.getNumSamples(); start += 512)
        {
            juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
            processor->processBlock(view, midi);
        }
        const auto findPanel = [&editor]() -> ui::HamloVerbPanel*
        {
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* v = dynamic_cast<ui::HamloVerbPanel*>(c))
                    return v;
            }
            return nullptr;
        };
        editor->updateAnalysisNow();
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-14.0f, 3.2f);
            save("hpeq_hamloverb_decay.png");
            {
                const auto image = panel->createComponentSnapshot(panel->getLocalBounds(), true, 1.0f);
                auto file = out.getChildFile("hpeq_hamloverb_panel.png");
                file.deleteFile();
                juce::FileOutputStream stream(file);
                juce::PNGImageFormat().writeImageToStream(image, stream);
            }
            panel->setDisplayMode(ui::HamloVerbPanel::TimeView);
            save("hpeq_hamloverb_time.png");
        }
        auto cathedral = processor->getProject();
        auto& m = cathedral.layers[0].fxNodes.back().chain[0].parameters;
        m.set("space", 5);
        m.set("color", 0);
        m.set("decaySeconds", 7.5);
        m.set("bassMult", 1.8);
        m.set("hfDecayRatio", 0.3);
        m.set("predelaySync", 7);
        m.set("freeze", 1);
        m.set("highCutHz", 7000.0);
        processor->setProject(cathedral);
        processor->dispatchPendingMessages();
        editor->selectNode(true, verbId, 0);
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-9.0f, 0.0f);
            save("hpeq_hamloverb_cathedral_1970.png");
        }
        auto classic = processor->getProject();
        classic.layers[0].fxNodes.back().chain[0].parameters = rev; // a project saved before this pass
        processor->setProject(classic);
        processor->dispatchPendingMessages();
        editor->selectNode(true, verbId, 0);
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-24.0f, 0.0f);
            save("hpeq_hamloverb_classic.png");
        }
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 3: the HamloEcho page (a delay added from the UI = Tape 1/4), ECHO and FILTER views,
    // then BBD frozen with pitch, then an old project's delay (Classic).
    {
        auto withEcho = processor->getProject();
        FxNodeState n;
        n.id = withEcho.nextNodeId++;
        n.lowHz = 300.0;
        n.highHz = 9000.0;
        n.chain = { defaultModule(FxType::Delay) };
        auto& dv = n.chain[0].parameters;
        dv.set("feedback", 0.55);
        dv.set("link", 0);
        dv.set("syncR", 6);
        dv.set("cross", 0.35);
        dv.set("highRes", 0.45);
        dv.set("dampingHz", 6500.0);
        dv.set("bandHz", 1800.0);
        dv.set("bandGainDb", 4.0);
        dv.set("lowCutHz", 180.0);
        dv.set("lowRes", 0.2);
        const auto echoId = n.id;
        withEcho.layers[0].fxNodes.push_back(n);
        processor->setProject(withEcho);
        processor->dispatchPendingMessages();
        editor->selectNode(true, echoId, 0);
        auto more = makeSyntheticDrums({ 48000.0, 2, 1.5, kFixtureSeed });
        for (int start = 0; start < more.getNumSamples(); start += 512)
        {
            juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
            processor->processBlock(view, midi);
        }
        const auto findPanel = [&editor]() -> ui::HamloEchoPanel*
        {
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* v = dynamic_cast<ui::HamloEchoPanel*>(c))
                    return v;
            }
            return nullptr;
        };
        editor->updateAnalysisNow();
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-16.0f, 0.0f);
            save("hpeq_hamloecho_echo.png");
            {
                const auto image = panel->createComponentSnapshot(panel->getLocalBounds(), true, 1.0f);
                auto file = out.getChildFile("hpeq_hamloecho_panel.png");
                file.deleteFile();
                juce::FileOutputStream stream(file);
                juce::PNGImageFormat().writeImageToStream(image, stream);
            }
            panel->setDisplayMode(ui::HamloEchoPanel::FilterView);
            save("hpeq_hamloecho_filter.png");
        }
        auto bbd = processor->getProject();
        auto& m = bbd.layers[0].fxNodes.back().chain[0].parameters;
        m.set("character", 3);
        m.set("pitch", 7.0);
        m.set("pingPong", 1);
        m.set("cross", 1.0);
        m.set("diffusion", 0.4);
        m.set("timeMode", 1);
        processor->setProject(bbd);
        processor->dispatchPendingMessages();
        editor->selectNode(true, echoId, 0);
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-12.0f, 0.0f);
            save("hpeq_hamloecho_bbd_pitch.png");
        }
        auto classic = processor->getProject();
        juce::NamedValueSet old;
        old.set("timeMs", 320.0);
        old.set("sync", 0);
        old.set("feedback", 0.45);
        classic.layers[0].fxNodes.back().chain[0].parameters = old; // a project saved before this pass
        processor->setProject(classic);
        processor->dispatchPendingMessages();
        editor->selectNode(true, echoId, 0);
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-24.0f, 0.0f);
            save("hpeq_hamloecho_classic.png");
        }
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 4: the HamloMag page (a saturator added from the UI = TAPE 19 cm/s), CURVE and
    // HARMONICS views, then TUBE with hum and VINYL 78 rpm with crackle, then an old project (Classic).
    {
        auto withMag = processor->getProject();
        FxNodeState n;
        n.id = withMag.nextNodeId++;
        n.lowHz = 80.0;
        n.highHz = 6000.0;
        n.chain = { defaultModule(FxType::Saturator) };
        auto& sv = n.chain[0].parameters;
        sv.set("driveDb", 14.0);
        sv.set("tapeBias", 0.25);
        sv.set("hiss", 0.3);
        const auto magId = n.id;
        withMag.layers[0].fxNodes.push_back(n);
        processor->setProject(withMag);
        processor->dispatchPendingMessages();
        editor->selectNode(true, magId, 0);
        auto more = makeSyntheticDrums({ 48000.0, 2, 1.5, kFixtureSeed });
        for (int start = 0; start < more.getNumSamples(); start += 512)
        {
            juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
            processor->processBlock(view, midi);
        }
        const auto findPanel = [&editor]() -> ui::HamloMagPanel*
        {
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* v = dynamic_cast<ui::HamloMagPanel*>(c))
                    return v;
            }
            return nullptr;
        };
        editor->updateAnalysisNow();
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-14.0f, -9.0f);
            save("hpeq_hamlomag_curve.png");
            {
                const auto image = panel->createComponentSnapshot(panel->getLocalBounds(), true, 1.0f);
                auto file = out.getChildFile("hpeq_hamlomag_panel.png");
                file.deleteFile();
                juce::FileOutputStream stream(file);
                juce::PNGImageFormat().writeImageToStream(image, stream);
            }
            panel->setDisplayMode(ui::HamloMagPanel::HarmonicsView);
            save("hpeq_hamlomag_harmonics.png");
        }
        for (const auto& [model, file] : { std::pair { 2, "hpeq_hamlomag_tube_hum.png" }, std::pair { 5, "hpeq_hamlomag_vinyl78.png" } })
        {
            auto next = processor->getProject();
            auto& m = next.layers[0].fxNodes.back().chain[0].parameters;
            m.set("model", model);
            m.set("hum", model == 2 ? 0.5 : 0.0);
            m.set("crackle", model == 5 ? 0.6 : 0.0);
            m.set("speed", model == 5 ? 2 : 0);
            m.set("punish", model == 2 ? 1 : 0);
            m.set("evenOdd", model == 2 ? 0.6 : 0.0);
            processor->setProject(next);
            processor->dispatchPendingMessages();
            editor->selectNode(true, magId, 0);
            if (auto* panel = findPanel())
            {
                panel->settleMeters(-12.0f, -8.0f);
                panel->setDisplayMode(model == 2 ? ui::HamloMagPanel::HarmonicsView : ui::HamloMagPanel::CurveView);
                save(file);
            }
        }
        auto classic = processor->getProject();
        juce::NamedValueSet old;
        old.set("driveDb", 18.0);
        old.set("curve", 1);
        classic.layers[0].fxNodes.back().chain[0].parameters = old; // a project saved before this pass
        processor->setProject(classic);
        processor->dispatchPendingMessages();
        editor->selectNode(true, magId, 0);
        if (auto* panel = findPanel())
        {
            panel->settleMeters(-14.0f, -10.0f);
            save("hpeq_hamlomag_classic.png");
        }
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 5: HamloRedWall on a low region pushed 6 dB, and the layer's MIX chain (D7).
    {
        auto wall = processor->getProject();
        FxNodeState n;
        n.id = wall.nextNodeId++;
        n.lowHz = 40.0;
        n.highHz = 160.0;
        auto limiter = defaultModule(FxType::Limiter);
        limiter.parameters.set("thresholdDb", -6.0);
        limiter.parameters.set("style", 1);
        n.chain = { limiter };
        wall.layers[0].fxNodes.push_back(n);
        wall.layers[0].mixChain = { defaultModule(FxType::Compressor), defaultModule(FxType::Limiter) };
        processor->setProject(wall);
        processor->dispatchPendingMessages();
        editor->selectNode(true, n.id, 0);
        auto more = makeSyntheticDrums({ 48000.0, 2, 1.0, kFixtureSeed });
        for (int start = 0; start < more.getNumSamples(); start += 512)
        {
            juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
            processor->processBlock(view, midi);
            if ((start / 512) % 4 == 3)
                editor->updateAnalysisNow();
        }
        save("hpeq_hamloredwall_page.png");
        editor->selectMix(-1);
        save("hpeq_mix_chain_grid.png");
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 6: HamloUdarnik on the drums' click band, UDARNIK (peak-neutral punch) and PERESTROIKA.
    {
        auto punch = processor->getProject();
        FxNodeState n;
        n.id = punch.nextNodeId++;
        n.lowHz = 120.0;
        n.highHz = 6000.0;
        auto t = defaultModule(FxType::TransientShaper);
        t.parameters.set("style", 1);
        t.parameters.set("attackDb", 6.0);
        t.parameters.set("lookahead", 2);
        n.chain = { t };
        punch.layers[0].fxNodes.push_back(n);
        for (const auto& [style, file] : { std::pair { 1, "hpeq_hamloudarnik_udarnik.png" }, std::pair { 2, "hpeq_hamloudarnik_perestroika.png" } })
        {
            auto next = punch;
            auto& m = next.layers[0].fxNodes.back().chain[0].parameters;
            m.set("style", style);
            if (style == 2)
            {
                m.set("attackDb", 3.0);
                m.set("rise", 0.7);
                m.set("decay", 1.6);
            }
            processor->setProject(next);
            processor->dispatchPendingMessages();
            editor->selectNode(true, n.id, 0);
            auto more = makeSyntheticDrums({ 48000.0, 2, 1.0, kFixtureSeed });
            for (int start = 0; start < more.getNumSamples(); start += 512)
            {
                juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
                processor->processBlock(view, midi);
                if ((start / 512) % 3 == 2)
                    editor->updateAnalysisNow();
            }
            save(file);
        }
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Module pass 7: HamloZastava on the drums' low band, key HP 120 Hz, VOSTOK; and TREVOGA (duck).
    {
        auto gated = processor->getProject();
        FxNodeState n;
        n.id = gated.nextNodeId++;
        n.lowHz = 40.0;
        n.highHz = 400.0;
        auto gate = defaultModule(FxType::GateExpander);
        gate.parameters.set("thresholdDb", -30.0);
        gate.parameters.set("keyHpHz", 120.0);
        n.chain = { gate };
        gated.layers[0].fxNodes.push_back(n);
        for (const auto& [style, file] : { std::pair { 0, "hpeq_hamlozastava_zastava.png" }, std::pair { 2, "hpeq_hamlozastava_trevoga.png" } })
        {
            auto next = gated;
            next.layers[0].fxNodes.back().chain[0].parameters.set("style", style);
            if (style == 2)
                next.layers[0].fxNodes.back().chain[0].parameters.set("rangeDb", 12.0);
            processor->setProject(next);
            processor->dispatchPendingMessages();
            editor->selectNode(true, n.id, 0);
            auto more = makeSyntheticDrums({ 48000.0, 2, 1.0, kFixtureSeed });
            for (int start = 0; start < more.getNumSamples(); start += 512)
            {
                juce::AudioBuffer<float> view(more.getArrayOfWritePointers(), 2, start, juce::jmin(512, more.getNumSamples() - start));
                processor->processBlock(view, midi);
                if ((start / 512) % 3 == 2)
                    editor->updateAnalysisNow();
            }
            save(file);
        }
        processor->setProject(project);
        processor->dispatchPendingMessages();
    }

    // Every Black Steel page on its own at its design size (scratched pictures, panel layout checks).
    {
        const auto shoot = [&](juce::Component& c, int w, int h, const juce::String& name)
        {
            c.setSize(w, h);
            const auto image = c.createComponentSnapshot(c.getLocalBounds(), true, 1.0f);
            auto file = out.getChildFile(name);
            file.deleteFile();
            juce::FileOutputStream stream(file);
            juce::PNGImageFormat().writeImageToStream(image, stream);
            std::cout << file.getFullPathName() << "\n";
        };
        ui::HamloCompPanel compPanel;
        shoot(compPanel, 1180, 700, "hpeq_panel_hamlocomp.png");
        ui::HamloVerbPanel verb;
        shoot(verb, 1180, 760, "hpeq_panel_hamloverb.png");
        ui::HamloEchoPanel echo;
        shoot(echo, 1180, 760, "hpeq_panel_hamloecho.png");
        ui::HamloMagPanel mag;
        shoot(mag, 1180, 820, "hpeq_panel_hamlomag.png");
        ui::HamloRedWallPanel wall;
        shoot(wall, 1180, 700, "hpeq_panel_hamloredwall.png");
        ui::HamloUdarnikPanel udarnik;
        shoot(udarnik, 1180, 700, "hpeq_panel_hamloudarnik.png");
        ui::HamloZastavaPanel zastava;
        shoot(zastava, 1180, 780, "hpeq_panel_hamlozastava.png");
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
