#include "UI/VerbV2Panel.h"
#include "UI/BlackSteel.h"
#include "Standalone/VerbParameters.h"
#include "DSP/VerbV2Processor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace hpeq::ui
{
namespace
{
constexpr int stageWidth = 1180, stageHeight = 940;
const juce::Colour ink { 0xffe6d8a6 }, muted { 0xffaaa487 }, green { 0xff6dff9a };
enum Page { Always = -1, Main = 0, Equalizers = 1, Texture = 2, Effects = 3 };

juce::String helpFor(const juce::String& id)
{
    if (id == "mix") return "Insert dry/wet balance. With KEEP DRY, the source stays at unity and MIX adds reverb. Send is wet only. Mix Lock protects MIX when loading presets.";
    if (id == "keepDry") return "Keep the source at unity and add MIX times the reverb in modern INSERT. OUTPUT still trims the total. Remembered across presets; saved with the project. Turn off for the original dry/wet balance.";
    if (id == "routing") return "INSERT blends dry and wet. SEND outputs only reverb. Changing routing remembers the Insert Mix value.";
    if (id == "engine") return "LEGACY preserves old projects. The six new characters change the structure of the reverb, not just its brightness.";
    if (id == "space") return "Room geometry and reflection family. Classic in Legacy retains its original 250 ms / 20 s limits.";
    if (id == "color") return "Original Legacy bandwidth and colour. Available only in Legacy; new characters have their own sound.";
    if (id == "predelayMs") return "Time before the wet field begins. New characters support the full 0–500 ms range. Tempo Sync overrides the free-time value.";
    if (id == "predelaySync") return "Lock predelay to the host tempo, or choose Free to use the millisecond control.";
    if (id == "decaySeconds") return "Base reverb decay time. Decay EQ shapes frequency-dependent decay. Legacy Classic retains its original 20-second limit.";
    if (id == "size") return "Dimensions of the virtual space. Small spaces cluster reflections; larger spaces spread them apart.";
    if (id == "distance") return "Balance the direct early field against the developed tail to place the sound nearer or farther away.";
    if (id == "width") return "Wet stereo width. Zero collapses the wet field to mono. Values above 100% increase the side component.";
    if (id == "attack") return "Build-up of the late field. Increase for a softer arrival; lower for immediate percussive space.";
    if (id == "duck") return "Reverb ducking from the dry input. Keeps the source forward and reveals the tail between phrases.";
    if (id == "earlyDb") return "Independent early-reflection level; 0 dB is neutral. Available in the six new characters.";
    if (id == "lateDb") return "Independent late-tail level; 0 dB is neutral. Available in the six new characters.";
    if (id == "outputDb") return "Output trim after Insert mixing or Send wet output. Does not alter the tank input or decay.";
    if (id == "freeze") return "Hold the current field and close new input. In new characters Shimmer injection stops; output controls remain available.";
    if (id == "timeShape") return "GATED follows the input detector. REVERSE SWELL rises into an ending. REVERSE AUDIO reverses wet fragments and accumulates audio before playback.";
    if (id == "shapeTimeMs") return "Gate hold, swell length or reversed-fragment duration. Reverse Audio needs this wet accumulation time; the dry path remains immediate.";
    if (id == "shapeSync") return "Use host-tempo divisions for the time shape, or Free for milliseconds.";
    if (id == "gateThresholdDb") return "Input detection threshold for the gated/swell envelope. Use a lower threshold for quieter phrases.";
    if (id == "gateReleaseMs") return "Smooth release after the gate closes. Short releases give a sharp ending; longer releases soften it.";
    if (id == "shimmerAmount") return "Amount of pitch-shifted reverb injected into the tail. Zero disables Shimmer.";
    if (id == "shimmerPitch") return "Pitch shift in semitones: -12 is one octave down, +12 is one octave up, +24 is two octaves up.";
    if (id == "shimmerFeedback") return "Recirculation of pitch-shifted sound. Higher values produce a more developed harmonic cloud; Freeze disables injection.";
    if (id == "duckAttackMs") return "How quickly the dry input pushes down the reverb.";
    if (id == "duckReleaseMs") return "How quickly the reverb returns after a phrase or transient.";
    if (id == "earlyDiffusion") return "Density and scattering of the early reflections.";
    if (id == "lateDiffusion") return "Density of the late field. Lower values leave more audible structure.";
    if (id == "modRate") return "Speed of the subtle delay movement inside the reverb.";
    if (id == "modDepth") return "Depth of delay movement. Adds motion to sustained tails; Freeze settles modulation.";
    if (id == "bassMult") return "Decay-time multiplier below the bass crossover.";
    if (id == "bassXoverHz") return "Transition frequency of the bass decay multiplier.";
    if (id == "hfDecayRatio") return "High-frequency decay ratio. Lower values darken the tail as it decays.";
    if (id == "highXoverHz") return "Transition frequency of the high-frequency decay ratio.";
    if (id == "lowCutHz") return "Wet-output low cut. Removes rumble without removing bass from the dry signal.";
    if (id == "highCutHz") return "Wet-output high cut. Softens the reverb without filtering the dry signal.";
    if (id.startsWith("decayEq")) return "Decay EQ changes how long frequencies ring. Value is log2 ratio: -1 = half, +1 = double. Neutral is zero. Band controls are automatable.";
    if (id.startsWith("postEq")) return "Post EQ shapes the wet output after the reverb. Gain is in dB. Disabled bands are transparent; the dry input is unchanged.";
    return "Adjust " + id + ". Double-click a knob to restore its default. Hold Shift for precise dragging, or type a value.";
}

struct SteelLook final : juce::LookAndFeel_V4
{
    SteelLook()
    {
        setColour(juce::Slider::textBoxTextColourId, ink);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111915));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff565d48));
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0xff356a47));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff19211c));
        setColour(juce::ComboBox::textColourId, ink);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff6b715d));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff18201b));
        setColour(juce::PopupMenu::textColourId, ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff386046));
        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff17201b));
        setColour(juce::TooltipWindow::textColourId, ink);
        setColour(juce::TooltipWindow::outlineColourId, juce::Colour(0xff74927a));
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float value,
                          float, float, juce::Slider& slider) override
    {
        const auto c = juce::Point<float>(static_cast<float>(x + w / 2), static_cast<float>(y + h / 2));
        const float scale = juce::jmin(static_cast<float>(w), static_cast<float>(h)) / 132.0f;
        steel::knobScale(g, c, scale);
        steel::brassKnob(g, c, (-135.0f + value * 270.0f) * juce::MathConstants<float>::pi / 180.0f,
                         scale, slider.isEnabled() ? 1.0f : 0.35f);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        const bool lit = b.getToggleState();
        g.setColour(lit ? juce::Colour(0xff284835) : juce::Colour(0xff202622));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(lit ? green.withAlpha(0.85f) : ink.withAlpha(over || down ? 0.7f : 0.3f));
        g.drawRoundedRectangle(r, 3.0f, lit ? 1.5f : 1.0f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont(steel::monoFont(13.0f));
        g.setColour((b.getToggleState() ? green : ink).withAlpha(b.isEnabled() ? 1.0f : 0.35f));
        g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(5, 1), juce::Justification::centred, 2);
    }
};

double read(const VerbV2Panel& owner, const juce::String& key, double fallback = 0.0)
{
    return owner.readValue ? owner.readValue(key) : fallback;
}
}

struct VerbV2Panel::Impl
{
    struct Control
    {
        juce::String id;
        Page page;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> choice;
        std::unique_ptr<juce::TextButton> toggle;
        std::unique_ptr<juce::Label> label;
        juce::Component* component() const { return slider ? static_cast<juce::Component*>(slider.get()) : choice ? static_cast<juce::Component*>(choice.get()) : toggle.get(); }
    };

    struct EqGraph final : juce::Component, juce::SettableTooltipClient
    {
        Impl& ui;
        bool decay;
        int dragging = -1;
        EqGraph(Impl& u, bool d) : ui(u), decay(d)
        {
            setName(d ? "Decay EQ graph" : "Post EQ graph");
            setTooltip("Select a numbered band. Drag left/right for frequency and up/down for value. Use the labelled controls below for precise values, shape, Q and enable.");
        }
        juce::String key(int band, const char* suffix) const { return juce::String(decay ? "decayEq" : "postEq") + juce::String(band + 1) + suffix; }
        float x(double hz) const { return 32.0f + static_cast<float>(std::log(juce::jlimit(20.0, 20000.0, hz) / 20.0) / std::log(1000.0)) * (getWidth() - 48.0f); }
        float y(double value) const { const double limit = decay ? 2.0 : 18.0; return 25.0f + static_cast<float>((limit - value) / (2.0 * limit)) * (getHeight() - 54.0f); }
        juce::Point<float> point(int i) const { return { x(read(ui.owner, key(i, "Hz"), 1000.0)), y(read(ui.owner, key(i, "Value"))) }; }
        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xff0c1811)); g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(green.withAlpha(0.3f)); g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
            g.setFont(steel::monoFont(11.0f));
            for (double hz : { 20.0, 100.0, 1000.0, 10000.0, 20000.0 })
            {
                g.setColour(green.withAlpha(0.12f)); g.drawVerticalLine(juce::roundToInt(x(hz)), 24.0f, getHeight() - 29.0f);
                g.setColour(muted); g.drawText(hz >= 1000 ? juce::String(hz / 1000.0, 0) + "k" : juce::String(hz, 0), juce::roundToInt(x(hz)) - 22, getHeight() - 24, 44, 16, juce::Justification::centred);
            }
            const double limit = decay ? 2.0 : 18.0;
            for (double value : { -limit, 0.0, limit })
            {
                g.setColour(green.withAlpha(value == 0.0 ? 0.3f : 0.12f));
                g.drawHorizontalLine(juce::roundToInt(y(value)), 30.0f, getWidth() - 15.0f);
            }
            // Use the DSP's own model. Decay is a target, never a measured RT60.
            juce::Path curve;
            const double baseTime = std::max(0.1, read(ui.owner, "decaySeconds", 2.4));
            const double fs = ui.owner.readSampleRate ? std::max(8000.0, ui.owner.readSampleRate()) : 48000.0;
            for (int step = 0; step <= 200; ++step)
            {
                const double hz = 20.0 * std::pow(1000.0, step / 200.0);
                const double value = decay ? std::log2(dsp::VerbV2Processor::decaySecondsAt(ui.viewSettings, hz) / baseTime)
                                          : 20.0 * std::log10(std::max(1.0e-12, dsp::VerbV2Processor::postEqMagnitude(ui.viewSettings, hz, fs)));
                const auto p = juce::Point<float>(x(hz), y(juce::jlimit(-limit, limit, value)));
                if (step == 0) curve.startNewSubPath(p); else curve.lineTo(p);
            }
            steel::glow(g, curve, green, 1.25f, 0.7f);
            g.setColour(muted); g.drawText(decay ? "DECAY EQ  /  target ratio" : "POST EQ  /  response dB", 12, 5, getWidth() - 24, 17, juce::Justification::centredLeft);
            for (int i = 0; i < 6; ++i)
            {
                const auto p = point(i); const bool enabled = read(ui.owner, key(i, "On")) >= 0.5;
                g.setColour((i == ui.selectedBand[decay ? 0 : 1] ? ink : green).withAlpha(enabled ? 1.0f : 0.4f));
                g.drawLine(p.x, y(0.0), p.x, p.y, 1.0f);
                g.fillEllipse(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
                g.setColour(juce::Colour(0xff0c1811));
                g.drawText(juce::String(i + 1), juce::Rectangle<float>(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f), juce::Justification::centred);
            }
        }
        void mouseDown(const juce::MouseEvent& e) override
        {
            float distance = 22.0f;
            for (int i = 0; i < 6; ++i) if (point(i).getDistanceFrom(e.position) < distance) { distance = point(i).getDistanceFrom(e.position); dragging = i; }
            if (dragging >= 0)
            {
                ui.selectBand(decay, dragging);
                if (ui.owner.onGestureStart) ui.owner.onGestureStart(key(dragging, "Hz"));
                if (ui.owner.onGestureStart) ui.owner.onGestureStart(key(dragging, "Value"));
            }
        }
        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (dragging < 0 || !ui.owner.onChange) return;
            const double nx = juce::jlimit(0.0, 1.0, static_cast<double>((e.position.x - 32.0f) / (getWidth() - 48.0f)));
            const double ny = juce::jlimit(0.0, 1.0, static_cast<double>((e.position.y - 25.0f) / (getHeight() - 54.0f)));
            ui.owner.onChange(key(dragging, "Hz"), 20.0 * std::pow(1000.0, nx));
            ui.owner.onChange(key(dragging, "Value"), (decay ? 2.0 : 18.0) * (1.0 - 2.0 * ny));
            ui.refreshControls(); repaint();
        }
        void mouseUp(const juce::MouseEvent&) override { if (dragging >= 0 && ui.owner.onGestureEnd) ui.owner.onGestureEnd(); dragging = -1; }
    };

    VerbV2Panel& owner;
    const standalone::ModuleSpec& spec;
    SteelLook look;
    juce::Component stage;
    std::vector<Control> controls;
    std::array<juce::TextButton, 7> engines;
    std::array<juce::TextButton, 6> materials;
    std::array<juce::TextButton, 3> sections;
    std::array<std::array<juce::TextButton, 6>, 2> bands;
    juce::TextButton main { "MAIN" }, advanced { "ADVANCED" }, loadStart { "LOAD START" }, mixLock { "MIX LOCK" };
    juce::Label help, character, status;
    EqGraph decayGraph, postGraph;
    juce::Image background;
    int page = Main;
    std::array<int, 2> selectedBand { 0, 0 };
    std::array<std::atomic<float>, standalone::VerbParameterCount> graphValues {};
    std::vector<std::atomic<float>*> graphPointers;
    dsp::VerbV2Settings viewSettings;
    juce::String material { "All" };
    float output = -120.0f, duck = 0.0f;
    bool updating = false;

    Impl(VerbV2Panel& p, const standalone::ModuleSpec& s) : owner(p), spec(s), decayGraph(*this, true), postGraph(*this, false)
    {
        owner.setLookAndFeel(&look);
        for (auto& value : graphValues) graphPointers.push_back(&value);
        owner.addAndMakeVisible(stage);
        stage.setInterceptsMouseClicks(false, true);
        stage.setBounds(0, 0, stageWidth, stageHeight);
        background = juce::Image(juce::Image::RGB, stageWidth, stageHeight, true);
        { juce::Graphics g(background); steel::panel(g, stageWidth, stageHeight, 37); steel::nameplate(g, { 420, 27, 414, 91 }, "HamloVerb", "SPATIAL INSTRUMENT", "HV-002"); }
        for (auto* label : { &help, &character, &status })
        {
            stage.addAndMakeVisible(*label); label->setFont(juce::Font(steel::monoFont(13.0f)));
            label->setColour(juce::Label::textColourId, muted); label->setJustificationType(juce::Justification::centredLeft);
            label->setInterceptsMouseClicks(false, false);
        }
        help.setBounds(56, 865, 1068, 48);
        help.setText("Choose a material to browse. LOAD START changes the sound. Hover over a control for help.", juce::dontSendNotification);
        character.setBounds(56, 255, 1068, 30);
        status.setBounds(854, 113, 270, 26);
        static const char* materialNames[] { "ALL", "VOCAL", "DRUMS", "SAMPLES / KEYS", "SYNTH", "ATMOSPHERE / FX" };
        static const char* materialGroups[] { "All", "Vocal", "Drums", "Samples / Keys", "Synth", "Atmosphere / FX" };
        const int widths[] { 76, 116, 116, 180, 116, 190 };
        int x = 56;
        for (int i = 0; i < 6; ++i)
        {
            auto& b = materials[static_cast<size_t>(i)]; stage.addAndMakeVisible(b); b.setButtonText(materialNames[i]); b.setBounds(x, 157, widths[i], 34); x += widths[i] + 10;
            b.setTooltip("Filter the preset browser by material. This button does not change any sound parameter.");
            b.onClick = [this, i] { owner.selectMaterial(materialGroups[i]); if (owner.onMaterial) owner.onMaterial(material); };
        }
        stage.addAndMakeVisible(loadStart); loadStart.setBounds(990, 157, 134, 34);
        loadStart.setTooltip("Load a starting preset from the selected material and current genre. Mix Lock and routing are preserved.");
        loadStart.onClick = [this] { if (owner.onLoadStart) owner.onLoadStart(material); };
        static const char* engineNames[] { "LEGACY", "CLEAN", "WARM", "VELVET", "CRISP", "DEEP", "BLOOM" };
        for (int i = 0; i < 7; ++i)
        {
            auto& b = engines[static_cast<size_t>(i)]; stage.addAndMakeVisible(b); b.setButtonText(engineNames[i]); b.setBounds(56 + i * 154, 211, 144, 36);
            b.setTooltip(helpFor("engine")); b.onClick = [this, i] { immediate("engine", i); };
        }
        stage.addAndMakeVisible(main); main.setBounds(56, 300, 128, 30); main.onClick = [this] { page = Main; updateVisibility(); };
        stage.addAndMakeVisible(advanced); advanced.setBounds(196, 300, 150, 30); advanced.onClick = [this] { page = Equalizers; updateVisibility(); };
        const char* sectionNames[] { "EQ", "TEXTURE", "EFFECTS" };
        for (int i = 0; i < 3; ++i)
        {
            auto& b = sections[static_cast<size_t>(i)]; stage.addAndMakeVisible(b); b.setButtonText(sectionNames[i]); b.setBounds(578 + i * 184, 300, 174, 30);
            b.onClick = [this, i] { page = i + 1; updateVisibility(); };
        }
        addChoice("routing", "ROUTING", Always, { 856, 47, 144, 30 });
        addToggle("bypass", "BYPASS", Always, { 1014, 47, 110, 30 });
        stage.addAndMakeVisible(mixLock); mixLock.setBounds(856, 84, 128, 26); mixLock.setTooltip(helpFor("mix"));
        mixLock.onClick = [this] { if (owner.onMixLock) owner.onMixLock(!mixLock.getToggleState()); };
        addToggle("keepDry", "KEEP DRY", Always, { 996, 84, 128, 26 });

        const char* mainIds[] { "decaySeconds", "predelayMs", "size", "distance", "width", "mix", "duck", "earlyDb", "lateDb", "outputDb", "attack", "shimmerAmount" };
        const char* mainLabels[] { "DECAY", "PREDELAY", "SIZE", "DISTANCE", "WIDTH", "MIX", "DUCK", "EARLY", "TAIL", "OUTPUT", "BUILD", "SHIMMER" };
        for (int i = 0; i < 12; ++i) addKnob(mainIds[i], mainLabels[i], Main, { 56 + (i % 6) * 180, 354 + (i / 6) * 166, 168, 144 });
        addChoice("space", "SPACE", Main, { 56, 724, 220, 32 });
        addChoice("predelaySync", "PREDELAY SYNC", Main, { 296, 724, 220, 32 });
        addChoice("timeShape", "TIME SHAPE", Main, { 536, 724, 272, 32 });
        addToggle("freeze", "FREEZE", Main, { 830, 724, 132, 32 });
        addChoice("color", "LEGACY COLOR", Main, { 982, 724, 142, 32 });
        const char* textureIds[] { "earlyDiffusion", "lateDiffusion", "modRate", "modDepth", "duckAttackMs", "duckReleaseMs", "bassMult", "bassXoverHz", "hfDecayRatio", "highXoverHz", "lowCutHz", "highCutHz" };
        const char* textureLabels[] { "EARLY DIFF", "TAIL DIFF", "MOD RATE", "MOD DEPTH", "DUCK ATTACK", "DUCK RELEASE", "BASS DECAY", "BASS CROSS", "HIGH DECAY", "HIGH CROSS", "LOW CUT", "HIGH CUT" };
        for (int i = 0; i < 12; ++i) addKnob(textureIds[i], textureLabels[i], Texture, { 56 + (i % 6) * 180, 378 + (i / 6) * 190, 168, 152 });
        addChoice("timeShape", "TIME SHAPE", Effects, { 56, 372, 318, 32 });
        addChoice("shapeSync", "SHAPE SYNC", Effects, { 404, 372, 318, 32 });
        addToggle("freeze", "FREEZE", Effects, { 752, 372, 372, 32 });
        const char* effectIds[] { "shapeTimeMs", "gateThresholdDb", "gateReleaseMs", "shimmerAmount", "shimmerPitch", "shimmerFeedback" };
        const char* effectLabels[] { "SHAPE TIME", "GATE THRESHOLD", "GATE RELEASE", "SHIMMER", "PITCH", "FEEDBACK" };
        for (int i = 0; i < 6; ++i) addKnob(effectIds[i], effectLabels[i], Effects, { 56 + i * 180, 462, 168, 168 });

        stage.addAndMakeVisible(decayGraph); decayGraph.setBounds(56, 358, 520, 230);
        stage.addAndMakeVisible(postGraph); postGraph.setBounds(604, 358, 520, 230);
        for (int side = 0; side < 2; ++side)
        {
            const int left = side == 0 ? 56 : 604;
            for (int band = 0; band < 6; ++band)
            {
                auto& b = bands[static_cast<size_t>(side)][static_cast<size_t>(band)]; stage.addAndMakeVisible(b); b.setButtonText("BAND " + juce::String(band + 1)); b.setBounds(left + band * 88, 600, 80, 28);
                b.onClick = [this, side, band] { selectBand(side == 0, band); };
                const auto prefix = juce::String(side == 0 ? "decayEq" : "postEq") + juce::String(band + 1);
                addKnob(prefix + "Hz", "FREQUENCY", Equalizers, { left, 644, 164, 132 });
                addKnob(prefix + "Value", side == 0 ? "DECAY RATIO" : "GAIN dB", Equalizers, { left + 178, 644, 164, 132 });
                addKnob(prefix + "Q", "Q", Equalizers, { left + 356, 644, 164, 132 });
                addChoice(prefix + "Type", "SHAPE", Equalizers, { left, 816, 320, 30 });
                addToggle(prefix + "On", "BAND ENABLED", Equalizers, { left + 340, 816, 180, 30 });
            }
        }
        materials[0].setToggleState(true, juce::dontSendNotification); updateVisibility();
    }

    ~Impl() { owner.setLookAndFeel(nullptr); }
    const standalone::ParameterSpec* find(const juce::String& id) const
    {
        for (const auto& p : spec.parameters) if (p.id == id) return &p;
        return nullptr;
    }
    void immediate(const juce::String& id, double value)
    {
        if (owner.onGestureStart) owner.onGestureStart(id);
        if (owner.onChange) owner.onChange(id, value);
        if (owner.onGestureEnd) owner.onGestureEnd();
    }
    void label(Control& c, const juce::String& name, juce::Rectangle<int> bounds)
    {
        c.label = std::make_unique<juce::Label>(); c.label->setText(name, juce::dontSendNotification);
        c.label->setFont(juce::Font(steel::monoFont(12.0f))); c.label->setJustificationType(juce::Justification::centred);
        c.label->setColour(juce::Label::textColourId, ink); c.label->setBounds(bounds);
        c.label->setInterceptsMouseClicks(false, false); stage.addAndMakeVisible(*c.label);
    }
    void addKnob(const juce::String& id, const juce::String& name, Page target, juce::Rectangle<int> bounds)
    {
        const auto* p = find(id); if (p == nullptr) return;
        Control c; c.id = id; c.page = target; c.slider = std::make_unique<juce::Slider>(); auto& slider = *c.slider;
        slider.setName(name); slider.setTitle(name); slider.setDescription(helpFor(id)); slider.setTooltip(helpFor(id));
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 118, 24);
        slider.setRange(p->minimum, p->maximum, p->interval); slider.setSkewFactor(p->skew);
        slider.setDoubleClickReturnValue(true, p->defaultValue); slider.setValue(p->defaultValue, juce::dontSendNotification);
        slider.setNumDecimalPlacesToDisplay(p->interval >= 1.0f ? 0 : (id.endsWith("Hz") || id.endsWith("Ms") ? 1 : 2));
        slider.setTextValueSuffix(p->unit.isEmpty() ? "" : " " + p->unit);
        if (id.startsWith("decayEq") && id.endsWith("Value"))
        {
            slider.textFromValueFunction = [](double v) { return juce::String(std::pow(2.0, v), 2) + "x"; };
            slider.valueFromTextFunction = [](const juce::String& text) { return std::log2(juce::jlimit(0.25, 4.0, text.getDoubleValue())); };
        }
        slider.onDragStart = [this, id] { if (owner.onGestureStart) owner.onGestureStart(id); };
        slider.onValueChange = [this, id, ptr = &slider] { if (!updating && owner.onChange) owner.onChange(id, ptr->getValue()); };
        slider.onDragEnd = [this] { if (owner.onGestureEnd) owner.onGestureEnd(); };
        label(c, name, bounds.withHeight(20)); slider.setBounds(bounds.withTrimmedTop(23)); stage.addAndMakeVisible(slider); controls.push_back(std::move(c));
    }
    void addChoice(const juce::String& id, const juce::String& name, Page target, juce::Rectangle<int> bounds)
    {
        const auto* p = find(id); if (p == nullptr) return;
        Control c; c.id = id; c.page = target; c.choice = std::make_unique<juce::ComboBox>(); auto& choice = *c.choice;
        choice.setName(name); choice.setTooltip(helpFor(id)); choice.addItemList(p->choices, 1); choice.setSelectedItemIndex(juce::roundToInt(p->defaultValue), juce::dontSendNotification);
        choice.onChange = [this, id, ptr = &choice] { if (!updating) immediate(id, ptr->getSelectedItemIndex()); };
        label(c, name, bounds.translated(0, -23).withHeight(20)); choice.setBounds(bounds); stage.addAndMakeVisible(choice); controls.push_back(std::move(c));
    }
    void addToggle(const juce::String& id, const juce::String& name, Page target, juce::Rectangle<int> bounds)
    {
        Control c; c.id = id; c.page = target; c.toggle = std::make_unique<juce::TextButton>(name); auto& b = *c.toggle;
        b.setName(name); b.setTooltip(helpFor(id)); b.setBounds(bounds);
        b.onClick = [this, id] { immediate(id, read(owner, id) >= 0.5 ? 0.0 : 1.0); };
        stage.addAndMakeVisible(b); controls.push_back(std::move(c));
    }
    bool bandVisible(const juce::String& id) const
    {
        for (int side = 0; side < 2; ++side)
        {
            const juce::String prefix = side == 0 ? "decayEq" : "postEq";
            if (id.startsWith(prefix)) return id.substring(prefix.length(), prefix.length() + 1).getIntValue() == selectedBand[static_cast<size_t>(side)] + 1;
        }
        return true;
    }
    void selectBand(bool decay, int index)
    {
        selectedBand[decay ? 0 : 1] = index; updateVisibility(); decayGraph.repaint(); postGraph.repaint();
    }
    void updateVisibility()
    {
        main.setToggleState(page == Main, juce::dontSendNotification); advanced.setToggleState(page != Main, juce::dontSendNotification);
        for (int i = 0; i < 3; ++i) { sections[static_cast<size_t>(i)].setVisible(page != Main); sections[static_cast<size_t>(i)].setToggleState(page == i + 1, juce::dontSendNotification); }
        for (auto& c : controls)
        {
            const bool visible = (c.page == Always || c.page == page) && bandVisible(c.id);
            c.component()->setVisible(visible); if (c.label) c.label->setVisible(visible);
        }
        decayGraph.setVisible(page == Equalizers); postGraph.setVisible(page == Equalizers);
        for (int side = 0; side < 2; ++side) for (int band = 0; band < 6; ++band)
        {
            auto& b = bands[static_cast<size_t>(side)][static_cast<size_t>(band)]; b.setVisible(page == Equalizers); b.setToggleState(selectedBand[static_cast<size_t>(side)] == band, juce::dontSendNotification);
        }
        owner.repaint();
    }
    void refreshControls()
    {
        const int engine = juce::jlimit(0, 6, juce::roundToInt(read(owner, "engine")));
        const bool legacy = engine == 0, send = read(owner, "routing") >= 0.5;
        updating = true;
        for (size_t i = 0; i < std::min(graphValues.size(), spec.parameters.size()); ++i)
            graphValues[i].store(static_cast<float>(read(owner, spec.parameters[i].id, spec.parameters[i].defaultValue)), std::memory_order_relaxed);
        viewSettings = standalone::readVerbSettings(graphPointers, standalone::makeSlot(standalone::ModuleKind::Verb, graphPointers, true));
        for (auto& c : controls)
        {
            const double value = c.id == "mix" && send ? 1.0 : read(owner, c.id);
            if (c.slider && !c.slider->isMouseButtonDown()) c.slider->setValue(value, juce::dontSendNotification);
            if (c.choice) c.choice->setSelectedItemIndex(juce::roundToInt(value), juce::dontSendNotification);
            if (c.toggle) c.toggle->setToggleState(value >= 0.5, juce::dontSendNotification);
            bool enabled = true;
            if (c.id == "space" || c.id == "routing") enabled = c.id == "space" ? legacy : !legacy;
            if (c.label && c.id == "space") c.label->setText("LEGACY SPACE", juce::dontSendNotification);
            if (c.label && c.id == "mix") c.label->setText(legacy ? "LEGACY MIX" : "MIX", juce::dontSendNotification);
            if (c.id == "mix") enabled = !send;
            if (c.id == "keepDry") enabled = !legacy && !send;
            if (c.id == "color") enabled = legacy;
            if (c.id.startsWith("decayEq") || c.id.startsWith("postEq") || c.id == "earlyDb" || c.id == "lateDb" || c.id == "outputDb"
                || c.id.startsWith("shimmer") || c.id.startsWith("shape") || c.id.startsWith("gate") || c.id == "timeShape"
                || c.id == "duckAttackMs" || c.id == "duckReleaseMs") enabled = !legacy;
            c.component()->setEnabled(enabled); if (c.label) c.label->setAlpha(enabled ? 1.0f : 0.4f);
        }
        updating = false;
        for (int i = 0; i < 7; ++i) engines[static_cast<size_t>(i)].setToggleState(engine == i, juce::dontSendNotification);
        static const char* descriptions[] {
            "LEGACY  /  Original sound and project recall. Classic retains its 250 ms / 20 s limits.",
            "CLEAN  /  Clear spatial field. Natural separation, controlled reflections and an open tail.",
            "WARM  /  Cross-coupled diffusion and gentle colour. Rounded depth with a vintage glow.",
            "VELVET  /  Nested diffusion. A dense, soft field around vocals, keys and sustained sounds.",
            "CRISP  /  Fast, dense plate. Immediate definition for snare, percussion and sharp accents.",
            "DEEP  /  Sparse early space and a developed distant tail. Place sound beyond the speakers.",
            "BLOOM  /  Cascaded build-up. An expanding field for melodic layers and atmosphere."
        };
        character.setText(descriptions[engine], juce::dontSendNotification);
        decayGraph.setEnabled(!legacy); postGraph.setEnabled(!legacy); decayGraph.setAlpha(legacy ? 0.4f : 1.0f); postGraph.setAlpha(legacy ? 0.4f : 1.0f);
        decayGraph.repaint(); postGraph.repaint();
    }
};

VerbV2Panel::VerbV2Panel(const standalone::ModuleSpec& spec) : impl(std::make_unique<Impl>(*this, spec)) {}
VerbV2Panel::~VerbV2Panel() = default;
void VerbV2Panel::refresh(bool locked, float outputDb, float duckDb)
{
    impl->output = outputDb; impl->duck = duckDb; impl->mixLock.setToggleState(locked, juce::dontSendNotification);
    impl->status.setText("WET " + juce::String(outputDb, 1) + " dB   DUCK " + juce::String(duckDb, 1) + " dB", juce::dontSendNotification);
    impl->refreshControls();
    const auto mouse = getMouseXYRelative();
    const auto key = helpKeyAt(mouse.toFloat());
    if (key.isNotEmpty()) impl->help.setText(helpFor(key), juce::dontSendNotification);
}
void VerbV2Panel::showAdvanced(bool advanced) { impl->page = advanced ? Equalizers : Main; impl->updateVisibility(); }
bool VerbV2Panel::isAdvanced() const noexcept { return impl->page != Main; }
void VerbV2Panel::showAdvancedSection(int section) { impl->page = juce::jlimit(1, 3, section + 1); impl->updateVisibility(); }
void VerbV2Panel::selectMaterial(const juce::String& material)
{
    impl->material = material;
    static const char* names[] { "All", "Vocal", "Drums", "Samples / Keys", "Synth", "Atmosphere / FX" };
    for (int i = 0; i < 6; ++i) impl->materials[static_cast<size_t>(i)].setToggleState(material == names[i], juce::dontSendNotification);
}
const juce::String& VerbV2Panel::selectedMaterial() const noexcept { return impl->material; }
juce::Rectangle<float> VerbV2Panel::contentBounds() const
{
    const float scale = juce::jmin(static_cast<float>(getWidth()) / stageWidth, static_cast<float>(getHeight()) / stageHeight);
    return { (getWidth() - stageWidth * scale) * 0.5f, (getHeight() - stageHeight * scale) * 0.5f, stageWidth * scale, stageHeight * scale };
}
juce::String VerbV2Panel::helpKeyAt(juce::Point<float> local) const
{
    const auto bounds = contentBounds(); const float scale = bounds.getWidth() / stageWidth;
    const auto p = (local - bounds.getPosition()) / juce::jmax(0.001f, scale);
    for (const auto& c : impl->controls) if (c.component()->isVisible() && c.component()->getBounds().toFloat().contains(p)) return c.id;
    return {};
}
void VerbV2Panel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101410)); const auto bounds = contentBounds();
    g.drawImage(impl->background, bounds);
    g.addTransform(juce::AffineTransform::scale(bounds.getWidth() / stageWidth).translated(bounds.getX(), bounds.getY()));
    g.setColour(ink.withAlpha(0.18f)); g.drawHorizontalLine(340, 56.0f, 1124.0f); g.drawHorizontalLine(858, 56.0f, 1124.0f);
    g.setFont(steel::monoFont(13.0f)); g.setColour(muted);
    if (impl->page == Main)
        g.drawFittedText("SOURCE  >  SPACE & CHARACTER  >  TIME SHAPE + SHIMMER  >  TONE  >  OUTPUT\nAdvanced opens decay/post EQ, texture and effect details. Material filters never change the sound.", { 56, 785, 1068, 56 }, juce::Justification::centred, 2);
    if (impl->page == Texture)
        g.drawFittedText("DECAY controls the time profile. LOW CUT and HIGH CUT shape the wet output.\nFreeze settles modulation and holds the field; the output controls remain available.", { 56, 775, 1068, 60 }, juce::Justification::centred, 2);
    if (impl->page == Effects)
        g.drawFittedText("REVERSE SWELL shapes the arrival. REVERSE AUDIO reverses accumulated wet fragments.\nReverse Audio adds wet build-up time; dry/host latency stays zero. Freeze closes new input and Shimmer injection.", { 56, 687, 1068, 105 }, juce::Justification::centred, 3);
    g.setFont(steel::monoFont(11.0f)); g.drawText("HAMLOPROD  /  ALGORITHMIC REVERBERATION  /  0.2", 56, 916, 1068, 17, juce::Justification::centred);
}
void VerbV2Panel::resized()
{
    const auto b = contentBounds(); impl->stage.setTransform(juce::AffineTransform::scale(b.getWidth() / stageWidth).translated(b.getX(), b.getY()));
}
}
