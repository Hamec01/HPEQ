#include "UI/HamloVerbPanel.h"

#include <cmath>

#include "DSP/HamloVerb.h"
#include "UI/BlackSteel.h"

namespace hpeq::ui
{
namespace
{
using namespace steel;
namespace verb = dsp::verb;

constexpr float kW = 1180.0f, kH = 760.0f; // design stage
constexpr float kPi = juce::MathConstants<float>::pi;

enum class Map { Linear, Log, Power };
struct MapInfo
{
    double min, max;
    Map kind;
    double skew;
};
constexpr std::array<MapInfo, HamloVerbPanel::NumKnobs> kMaps { {
    { 0.0, 500.0, Map::Power, 2.3 },   // PreDelay (ms)
    { 0.3, 1.5, Map::Linear, 1.0 },    // Size
    { 0.1, 30.0, Map::Log, 1.0 },      // Decay (s)
    { 0.0, 1.0, Map::Linear, 1.0 },    // Attack
    { 0.0, 1.0, Map::Linear, 1.0 },    // Distance
    { 0.0, 1.5, Map::Linear, 1.0 },    // Width
    { 0.0, 1.0, Map::Linear, 1.0 },    // EarlyDiff
    { 0.0, 1.0, Map::Linear, 1.0 },    // LateDiff
    { 0.01, 10.0, Map::Log, 1.0 },     // ModRate (Hz)
    { 0.0, 1.0, Map::Linear, 1.0 },    // ModDepth
    { 0.25, 4.0, Map::Log, 1.0 },      // BassMult
    { 50.0, 2000.0, Map::Log, 1.0 },   // BassXover (Hz)
    { 0.05, 1.0, Map::Log, 1.0 },      // HighMult
    { 1000.0, 16000.0, Map::Log, 1.0 },// HighXover (Hz)
    { 10.0, 1000.0, Map::Log, 1.0 },   // LowCut (Hz)
    { 1000.0, 20000.0, Map::Log, 1.0 },// HighCut (Hz)
    { 0.0, 1.0, Map::Linear, 1.0 },    // Duck
} };
constexpr std::array<const char*, HamloVerbPanel::NumKnobs> kLabels { "Pre-delay", "Size", "Decay", "Attack", "Distance", "Width",
                                                                      "Early diff", "Late diff", "Mod rate", "Mod depth", "Bass x",
                                                                      "Bass xover", "High x", "High xover", "Low cut", "High cut", "Duck" };
constexpr int kLarge = 6;

// Layout (design coordinates)
const juce::Rectangle<float> kPlate { 405.0f, 41.0f, 370.0f, 92.0f };
const juce::Rectangle<float> kLcd { 940.0f, 52.0f, 184.0f, 64.0f };
const juce::Point<float> kSelector { 174.0f, 302.0f };
constexpr float kSelectorLabelRadius = 104.0f;
const juce::Rectangle<float> kBezel { 350.0f, 156.0f, 512.0f, 290.0f };
const juce::Rectangle<float> kScreen { 368.0f, 172.0f, 476.0f, 238.0f };
const juce::Rectangle<float> kTabDecay { 520.0f, 418.0f, 84.0f, 20.0f }, kTabTime { 612.0f, 418.0f, 84.0f, 20.0f };
const juce::Rectangle<float> kMeterBezel { 884.0f, 156.0f, 240.0f, 180.0f };
const juce::Point<float> kLamp { 932.0f, 400.0f }, kFreezeLever { 1056.0f, 412.0f }, kFreezeLamp { 1104.0f, 412.0f };
constexpr float kColorX = 56.0f, kColorW = 250.0f, kColorTop = 46.0f;
constexpr float kLargeY = 535.0f, kLargeLabelY = 462.0f, kLargeReadoutY = 604.0f, kSyncY = 623.0f;
constexpr float kSmallY = 692.0f, kSmallLabelY = 646.0f, kSmallReadoutY = 733.0f;
const juce::Colour kPhosphor = phosphor, kPhosphorDim = phosphorDim;

float spaceAngle(int i) { return (-135.0f + 45.0f * static_cast<float>(i)) * kPi / 180.0f; }

// Where a space's name sits: beyond the dot, growing away from the knob on the sides.
juce::Rectangle<float> spaceLabelArea(int i)
{
    const float a = spaceAngle(i);
    const juce::Point<float> c(kSelector.x + std::sin(a) * kSelectorLabelRadius, kSelector.y - std::cos(a) * kSelectorLabelRadius);
    if (std::sin(a) > 0.5f)
        return { c.x - 6.0f, c.y - 10.0f, 96.0f, 20.0f };
    if (std::sin(a) < -0.5f)
        return { c.x - 90.0f, c.y - 10.0f, 96.0f, 20.0f };
    return { c.x - 46.0f, c.y - 10.0f, 92.0f, 20.0f };
}

float colorOptionX(int i) { return kColorX + kColorW * (0.12f + 0.76f * static_cast<float>(i) / 2.0f); }

juce::String hz(double v)
{
    return v >= 1000.0 ? juce::String(v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz";
}

double niceCeiling(double v)
{
    for (double n : { 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0, 12.0, 15.0, 20.0, 30.0, 40.0, 60.0, 90.0, 120.0 })
        if (v <= n)
            return n;
    return 120.0;
}
} // namespace

struct HamloVerbPanel::MeterState
{
    Meter meter;
};

HamloVerbPanel::HamloVerbPanel() : meter(std::make_unique<MeterState>())
{
    for (int i = 0; i < NumKnobs; ++i)
    {
        auto& k = knobs[static_cast<size_t>(i)];
        k.param = static_cast<Param>(i);
        if (i < kLarge)
        {
            k.centre = { 56.0f + (1068.0f / kLarge) * (static_cast<float>(i) + 0.5f), kLargeY };
            k.scale = 0.82f;
        }
        else
        {
            k.centre = { 56.0f + (1068.0f / (NumKnobs - kLarge)) * (static_cast<float>(i - kLarge) + 0.5f), kSmallY };
            k.scale = 0.5f;
        }
    }
    auto& m = meter->meter;
    m.bezel = kMeterBezel;
    m.min = -40.0f;
    m.max = 0.0f;
    m.redFrom = -3.0f;
    m.title = "OUTPUT";
    m.titleSize = 22.0f;
    for (float v : { -40.0f, -35.0f, -30.0f, -25.0f, -20.0f, -15.0f, -10.0f, -8.0f, -6.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f })
        m.ticks.push_back({ v, v == -40.0f || v == -30.0f || v == -20.0f || v == -10.0f || v == 0.0f });
    m.seed = 37;
    m.shown = -42.0f;
    setOpaque(false);
    startTimerHz(30);
}

HamloVerbPanel::~HamloVerbPanel() = default;

const char* HamloVerbPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "predelayMs", "size", "decaySeconds", "attack", "distance", "width", "earlyDiffusion",
                                          "lateDiffusion", "modRate", "modDepth", "bassMult", "bassXoverHz", "hfDecayRatio",
                                          "highXoverHz", "lowCutHz", "highCutHz", "duck" };
    return keys[p];
}

const char* HamloVerbPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "space", "color", "predelaySync", "freeze" };
    return keys[s];
}

double HamloVerbPanel::defaultFor(Param p)
{
    return Values {}.knobs[static_cast<size_t>(p)]; // = defaultModule(Reverb) and the DSP's fallbacks
}

bool HamloVerbPanel::usedByClassic(Param p) noexcept
{
    return p == PreDelay || p == Size || p == Decay || p == Width || p == HighMult || p == LowCut;
}

bool HamloVerbPanel::isActive(Param p) const noexcept
{
    return values.switches[Space] != verb::Classic || usedByClassic(p);
}

juce::String HamloVerbPanel::format(Param p, double v)
{
    const auto times = juce::String::fromUTF8("\xc3\x97");
    switch (p)
    {
        case PreDelay: return juce::String(juce::roundToInt(v)) + " ms";
        case Decay: return juce::String(v, v < 10.0 ? 2 : 1) + " s";
        case Size: case Attack: case Distance: case Width: case EarlyDiff: case LateDiff: case ModDepth: case Duck:
            return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case ModRate: return juce::String(v, v < 1.0 ? 2 : 1) + " Hz";
        case BassMult: case HighMult: return times + juce::String(v, 2);
        case BassXover: case HighXover: case LowCut: return hz(v);
        case HighCut: return v >= 19900.0 ? juce::String("OFF") : hz(v);
        case NumKnobs: break;
    }
    return {};
}

dsp::FxSlotParams HamloVerbPanel::slotParams() const
{
    namespace s = dsp::slot;
    dsp::FxSlotParams p;
    p.active = values.on;
    p.type = FxType::Reverb;
    const auto& k = values.knobs;
    p.values[s::kPredelayMs] = static_cast<float>(k[PreDelay]);
    p.values[s::kDecaySeconds] = static_cast<float>(k[Decay]);
    p.values[s::kHfRatio] = static_cast<float>(k[HighMult]);
    p.values[s::kSize] = static_cast<float>(k[Size]);
    p.values[s::kWidth] = static_cast<float>(k[Width]);
    p.values[s::kLowCutHz] = static_cast<float>(k[LowCut]);
    p.values[s::kRevSpace] = static_cast<float>(values.switches[Space]);
    p.values[s::kRevColor] = static_cast<float>(values.switches[Color]);
    p.values[s::kRevSync] = static_cast<float>(values.switches[Sync]);
    p.values[s::kRevAttack] = static_cast<float>(k[Attack]);
    p.values[s::kRevEarlyDiff] = static_cast<float>(k[EarlyDiff]);
    p.values[s::kRevLateDiff] = static_cast<float>(k[LateDiff]);
    p.values[s::kRevModRate] = static_cast<float>(k[ModRate]);
    p.values[s::kRevModDepth] = static_cast<float>(k[ModDepth]);
    p.values[s::kRevBassMult] = static_cast<float>(k[BassMult]);
    p.values[s::kRevBassXover] = static_cast<float>(k[BassXover]);
    p.values[s::kRevHighXover] = static_cast<float>(k[HighXover]);
    p.values[s::kRevHighCut] = static_cast<float>(k[HighCut]);
    p.values[s::kRevDistance] = static_cast<float>(k[Distance]);
    p.values[s::kRevDuck] = static_cast<float>(k[Duck]);
    p.values[s::kRevFreeze] = static_cast<float>(values.switches[Freeze]);
    if (values.switches[Space] == verb::Classic)
    {
        // the M4 engine: one first-order damping filter, no bass band
        p.values[s::kRevBassMult] = 1.0f;
        p.values[s::kRevHighXover] = 1000.0f;
    }
    return p;
}

double HamloVerbPanel::normFor(Param p) const
{
    if (p == PreDelay && values.switches[Sync] > 0)
        return (values.switches[Sync] - 1) / static_cast<double>(verb::kSyncDivisions - 2);
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    switch (m.kind)
    {
        case Map::Log: return std::log(v / m.min) / std::log(m.max / m.min);
        case Map::Power: return std::pow((v - m.min) / (m.max - m.min), 1.0 / m.skew);
        case Map::Linear: break;
    }
    return (v - m.min) / (m.max - m.min);
}

double HamloVerbPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    switch (m.kind)
    {
        case Map::Log: return m.min * std::pow(m.max / m.min, n);
        case Map::Power: return m.min + (m.max - m.min) * std::pow(n, m.skew);
        case Map::Linear: break;
    }
    return m.min + (m.max - m.min) * n;
}

void HamloVerbPanel::setKnob(Param p, double v, bool notify)
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
    if (notify && onKnobChange)
        onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
    repaint();
}

void HamloVerbPanel::setNorm(Param p, double n, bool notify)
{
    if (p == PreDelay && values.switches[Sync] > 0)
    {
        const int division = 1 + juce::roundToInt(juce::jlimit(0.0, 1.0, n) * (verb::kSyncDivisions - 2));
        if (division != values.switches[Sync])
            setSwitch(Sync, division);
        return;
    }
    setKnob(p, valueFor(p, n), notify);
}

void HamloVerbPanel::setSwitch(Switch s, int v)
{
    values.switches[static_cast<size_t>(s)] = v;
    if (onSwitchChange)
        onSwitchChange(s, v);
    repaint();
}

void HamloVerbPanel::setValues(const Values& v)
{
    if (dragKnob >= 0 || dragHandle != NoHandle || dragSpace)
        return; // the drag owns the value until the gesture ends
    values = v;
    repaint();
}

void HamloVerbPanel::setDisplayMode(DisplayMode m)
{
    mode = m;
    phosphor.clear(phosphor.getBounds());
    repaint();
}

juce::AffineTransform HamloVerbPanel::toScreen() const
{
    return juce::AffineTransform::scale(scale).translated(offset);
}

juce::Point<float> HamloVerbPanel::toDesign(juce::Point<float> p) const
{
    return (p - offset) / scale;
}

void HamloVerbPanel::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    phosphor = {};
    if (getWidth() > 0 && getHeight() > 0)
    {
        staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(staticLayer);
        g.addTransform(toScreen());
        renderStatic(g);
        const auto screen = kScreen.transformedBy(toScreen()).getSmallestIntegerContainer();
        phosphor = juce::Image(juce::Image::ARGB, juce::jmax(1, screen.getWidth()), juce::jmax(1, screen.getHeight()), true);
    }
    typing.reset();
}

// ---------------------------------------------------------------------------------------------------
// static plate

void HamloVerbPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 2025);
    nameplate(g, kPlate, "HamloVerb", juce::String::fromUTF8("MODEL \xd0\xa0\xd0\x92-1"), juce::String::fromUTF8("\xe2\x84\x96 0420"));

    // COLOR switch label
    engraved(g, "COLOR", juce::Rectangle<float>(kColorX, kColorTop, kColorW, 22.0f), 16.0f);
    lcdWindow(g, kLcd);

    // SPACE selector: title, engraved dots and labels
    engraved(g, "SPACE", juce::Rectangle<float>(kSelector.x - 90.0f, 140.0f, 180.0f, 26.0f), 20.0f);
    for (int i = 0; i < verb::NumSpaces; ++i)
    {
        const float a = spaceAngle(i);
        for (float r : { 74.0f })
        {
            const juce::Point<float> d(kSelector.x + std::sin(a) * r, kSelector.y - std::cos(a) * r);
            g.setColour(juce::Colour(0xffefe3bd));
            g.fillEllipse(d.x - 3.0f, d.y - 3.0f, 6.0f, 6.0f);
        }
        g.setColour(juce::Colour(0xffefe3bd).withAlpha(0.6f));
        g.drawLine(kSelector.x + std::sin(a) * 80.0f, kSelector.y - std::cos(a) * 80.0f, kSelector.x + std::sin(a) * 90.0f,
                   kSelector.y - std::cos(a) * 90.0f, 1.6f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(kSelector.x - 62.0f, kSelector.y - 50.0f, 124.0f, 124.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(kSelector.x - 61.0f, kSelector.y - 61.0f, 122.0f, 122.0f);

    crtBezel(g, kBezel, kScreen, juce::String::fromUTF8("\xd0\xad\xd0\x9a\xd0\xa0\xd0\x90\xd0\x9d \xc2\xb7 \xd0\xa1\xd0\x9a\xd0\x9e"), 420.0f); // ЭКРАН · СКО

    // OUTPUT meter face, lamp and freeze labels
    meterFace(g, meter->meter);
    engraved(g, "ON", juce::Rectangle<float>(kLamp.x - 40.0f, 430.0f, 80.0f, 24.0f), 18.0f);
    engraved(g, "FREEZE", juce::Rectangle<float>(kFreezeLever.x - 60.0f, 350.0f, 120.0f, 22.0f), 16.0f);

    // knob labels and scales
    for (const auto& k : knobs)
    {
        const bool large = k.param < kLarge;
        const float labelY = large ? kLargeLabelY : kSmallLabelY;
        const float w = large ? 170.0f : 96.0f;
        engraved(g, juce::String(kLabels[static_cast<size_t>(k.param)]).toUpperCase(),
                 juce::Rectangle<float>(k.centre.x - w * 0.5f, labelY - 13.0f, w, 26.0f), large ? 20.0f : 13.0f, stencil,
                 juce::Justification::centred, 0.1f);
        knobScale(g, k.centre, k.scale);
    }
    // a groove between the two knob rows
    g.setColour(juce::Colours::black);
    g.drawHorizontalLine(632, 56.0f, kW - 56.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(631, 56.0f, kW - 56.0f);
}

// ---------------------------------------------------------------------------------------------------
// dynamic parts

void HamloVerbPanel::drawKnob(juce::Graphics& g, const Knob& k) const
{
    const bool active = isActive(k.param);
    const float angle = (-135.0f + static_cast<float>(normFor(k.param)) * 270.0f) * kPi / 180.0f;
    brassKnob(g, k.centre, angle, k.scale, active ? 1.0f : 0.45f);
    const bool large = k.param < kLarge;
    const auto r = juce::Rectangle<float>(k.centre.x - (large ? 80.0f : 48.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 12.0f,
                                          large ? 160.0f : 96.0f, 24.0f);
    juce::String text = format(k.param, values.knobs[static_cast<size_t>(k.param)]);
    if (k.param == PreDelay && values.switches[Sync] > 0)
        text = verb::syncName(values.switches[Sync]);
    g.setFont(stencilFont(large ? 22.0f : 15.0f));
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0, 1), juce::Justification::centred);
    g.setColour(active ? stencil : stencilDim.withAlpha(0.6f));
    g.drawText(text, r, juce::Justification::centred);

    if (k.param == PreDelay)
    {
        // FREE / SYNC selector under the pre-delay readout
        const bool sync = values.switches[Sync] > 0;
        const bool usable = values.switches[Space] != verb::Classic;
        for (int i = 0; i < 2; ++i)
        {
            const auto t = juce::Rectangle<float>(k.centre.x - 62.0f + static_cast<float>(i) * 64.0f, kSyncY - 8.0f, 60.0f, 16.0f);
            const bool lit = (i == 1) == sync;
            g.setFont(stencilFont(12.5f).withKerningFactor(0.12f));
            g.setColour(!usable ? stencilDim.withAlpha(0.35f) : lit ? juce::Colour(0xfffff6d0) : stencilDim);
            g.drawText(i == 0 ? "FREE" : "SYNC", t, juce::Justification::centred);
            if (lit && usable)
            {
                g.setColour(juce::Colour(0xffff4a3a));
                g.fillEllipse(t.getX() + (i == 0 ? 6.0f : 2.0f), t.getCentreY() - 2.5f, 5.0f, 5.0f);
            }
        }
    }
}

void HamloVerbPanel::drawSpaceSelector(juce::Graphics& g) const
{
    const int selected = juce::jlimit(0, verb::NumSpaces - 1, values.switches[Space]);
    for (int i = 0; i < verb::NumSpaces; ++i)
    {
        const float a = spaceAngle(i);
        const auto r = spaceLabelArea(i);
        const auto just = std::sin(a) > 0.5f ? juce::Justification::centredLeft
                          : std::sin(a) < -0.5f ? juce::Justification::centredRight : juce::Justification::centred;
        const auto text = juce::String(verb::spaceName(i)).toUpperCase();
        g.setFont(stencilFont(i == 0 ? 12.5f : 14.0f).withKerningFactor(0.08f));
        if (i == selected)
        {
            g.setColour(juce::Colour(0x55ffe9a0));
            g.drawText(text, r.expanded(1.0f), just);
            g.setColour(juce::Colour(0xfffff6d0));
        }
        else
            g.setColour(i == 0 ? stencilDim.withAlpha(0.7f) : stencilDim);
        g.drawText(text, r, just);
    }
    // the knob: a large bakelite chicken-head on a brass skirt
    const float a = spaceAngle(selected);
    brassKnob(g, kSelector, a, 1.15f);
    juce::Path head;
    head.startNewSubPath(-11.0f, 18.0f);
    head.lineTo(-6.0f, -52.0f);
    head.quadraticTo(0.0f, -60.0f, 6.0f, -52.0f);
    head.lineTo(11.0f, 18.0f);
    head.quadraticTo(0.0f, 30.0f, -11.0f, 18.0f);
    head.closeSubPath();
    const auto t = juce::AffineTransform::rotation(a).translated(kSelector);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillPath(head, t.translated(2.0f, 5.0f));
    juce::ColourGradient bakelite(juce::Colour(0xff3a3530), juce::Point<float>(-11, 0).transformedBy(t), juce::Colour(0xff0d0b0a),
                                  juce::Point<float>(11, 0).transformedBy(t), false);
    bakelite.addColour(0.35, juce::Colour(0xff5a524a));
    g.setGradientFill(bakelite);
    g.fillPath(head, t);
    juce::Path line;
    line.addRoundedRectangle(-1.6f, -54.0f, 3.2f, 34.0f, 1.5f);
    g.setColour(juce::Colour(0xfff4ead0));
    g.fillPath(line, t);
}

void HamloVerbPanel::drawColorSwitch(juce::Graphics& g) const
{
    const int selected = juce::jlimit(0, 2, values.switches[Color]);
    const bool usable = values.switches[Space] != verb::Classic;
    const float dialTop = kColorTop + 22.0f;
    for (int i = 0; i < 3; ++i)
    {
        const auto r = juce::Rectangle<float>(colorOptionX(i) - 40.0f, dialTop, 80.0f, 18.0f);
        g.setFont(stencilFont(13.5f).withKerningFactor(0.1f));
        if (i == selected && usable)
        {
            g.setColour(juce::Colour(0x55ffe9a0));
            g.drawText(verb::colorName(i), r.expanded(1.0f), juce::Justification::centred);
            g.setColour(juce::Colour(0xfffff6d0));
        }
        else
            g.setColour(stencilDim.withAlpha(usable ? 1.0f : 0.4f));
        g.drawText(verb::colorName(i), r, juce::Justification::centred);
    }
    const juce::Point<float> base(kColorX + kColorW * 0.5f, dialTop + 50.0f);
    lever(g, base, (-38.0f + 38.0f * static_cast<float>(selected)) * kPi / 180.0f, 0.9f);
}

void HamloVerbPanel::drawFreeze(juce::Graphics& g) const
{
    const bool on = values.switches[Freeze] > 0;
    lever(g, kFreezeLever, on ? 0.0f : kPi, 0.85f);
    g.setFont(stencilFont(11.0f).withKerningFactor(0.15f));
    g.setColour(stencilDim);
    g.drawText("ON", juce::Rectangle<float>(kFreezeLever.x - 70.0f, kFreezeLever.y - 30.0f, 40.0f, 14.0f), juce::Justification::centred);
    g.drawText("OFF", juce::Rectangle<float>(kFreezeLever.x - 70.0f, kFreezeLever.y + 16.0f, 40.0f, 14.0f), juce::Justification::centred);
    lamp(g, kFreezeLamp, on, 0.42f, juce::Colour(0xff57d8ff));
}

void HamloVerbPanel::drawLcd(juce::Graphics& g) const
{
    const int space = values.switches[Space];
    juce::String line1 = juce::String(verb::spaceName(space)).toUpperCase();
    if (space != verb::Classic)
        line1 << " " << verb::colorName(values.switches[Color]);
    juce::String line2 = format(Decay, values.knobs[Decay]);
    if (values.switches[Freeze] > 0 && space != verb::Classic)
        line2 = "FREEZE";
    if (duckDb > 0.05f)
        line2 << "  D-" << juce::String(duckDb, 1);
    lcdText(g, kLcd.withHeight(kLcd.getHeight() * 0.5f).translated(0, 3.0f), line1, 18.0f);
    lcdText(g, kLcd.withTrimmedTop(kLcd.getHeight() * 0.5f).translated(0, -3.0f), line2, 18.0f);
}

// ---------------------------------------------------------------------------------------------------
// CRT

float HamloVerbPanel::xForHz(double f) const
{
    const auto s = kScreen.reduced(30.0f, 0.0f);
    return s.getX() + s.getWidth() * static_cast<float>(std::log(juce::jlimit(20.0, 20000.0, f) / 20.0) / std::log(1000.0));
}

double HamloVerbPanel::hzForX(float x) const
{
    const auto s = kScreen.reduced(30.0f, 0.0f);
    return 20.0 * std::pow(1000.0, juce::jlimit(0.0, 1.0, static_cast<double>((x - s.getX()) / s.getWidth())));
}

double HamloVerbPanel::decayViewMax() const
{
    const double d = values.knobs[Decay];
    const bool classic = values.switches[Space] == verb::Classic;
    return niceCeiling(1.2 * d * std::max(1.0, classic ? 1.0 : values.knobs[BassMult]));
}

float HamloVerbPanel::yForSeconds(double s) const
{
    const float top = kScreen.getY() + 26.0f, bottom = kScreen.getBottom() - 26.0f;
    return bottom - (bottom - top) * static_cast<float>(juce::jlimit(0.0, 1.05, s / decayViewMax()));
}

double HamloVerbPanel::secondsForY(float y) const
{
    const float top = kScreen.getY() + 26.0f, bottom = kScreen.getBottom() - 26.0f;
    return juce::jmax(0.0, static_cast<double>((bottom - y) / (bottom - top))) * decayViewMax();
}

std::array<juce::Point<float>, 3> HamloVerbPanel::handlePositions() const
{
    const auto& k = values.knobs;
    const bool classic = values.switches[Space] == verb::Classic;
    const double bass = classic ? 1.0 : k[BassMult];
    const double mid = std::sqrt(k[BassXover] * k[HighXover]);
    return { juce::Point<float>(xForHz(k[BassXover] * 0.5), yForSeconds(k[Decay] * bass)),
             juce::Point<float>(xForHz(mid), yForSeconds(k[Decay])),
             juce::Point<float>(xForHz(k[HighXover] * 2.0), yForSeconds(k[Decay] * k[HighMult])) };
}

void HamloVerbPanel::drawDecayView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    const auto p = slotParams();
    const auto& k = values.knobs;
    const float top = s.getY() + 26.0f, bottom = s.getBottom() - 26.0f;
    // graticule
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = xForHz(f);
        g.setColour(kPhosphorDim.withAlpha(0.28f));
        g.drawVerticalLine(juce::roundToInt(x), top, bottom);
        crtText(g, f >= 1000.0 ? juce::String(juce::roundToInt(f / 1000.0)) + "k" : juce::String(juce::roundToInt(f)),
                juce::Rectangle<float>(x - 20.0f, bottom + 4.0f, 40.0f, 14.0f), 11.0f, juce::Justification::centred, 0.55f);
    }
    const double maxT = decayViewMax();
    for (int i = 0; i <= 4; ++i)
    {
        const double t = maxT * i / 4.0;
        const float y = yForSeconds(t);
        g.setColour(kPhosphorDim.withAlpha(i == 0 ? 0.5f : 0.28f));
        g.drawHorizontalLine(juce::roundToInt(y), xForHz(20.0), xForHz(20000.0));
        if (i > 0)
            crtText(g, juce::String(t, t < 10.0 && std::fmod(t, 1.0) != 0.0 ? 1 : 0) + "s",
                    juce::Rectangle<float>(s.getX() + 2.0f, y - 7.0f, 30.0f, 14.0f), 11.0f, juce::Justification::centredLeft, 0.55f);
    }
    crtText(g, "DECAY TIME / FREQUENCY", juce::Rectangle<float>(s.getX() + 34.0f, s.getY() + 6.0f, 260.0f, 14.0f), 11.5f,
            juce::Justification::centredLeft, 0.7f);

    // output filters: shaded where cut
    const float lowX = xForHz(k[LowCut]);
    g.setColour(juce::Colour(0x5502140a));
    g.fillRect(juce::Rectangle<float>(xForHz(20.0), top, lowX - xForHz(20.0), bottom - top));
    g.setColour(kPhosphorDim.withAlpha(0.7f));
    for (float y = top; y < bottom; y += 6.0f)
        g.drawLine(lowX, y, lowX, y + 3.0f, 1.0f);
    crtText(g, "LO " + hz(k[LowCut]), juce::Rectangle<float>(lowX + 3.0f, bottom - 16.0f, 80.0f, 13.0f), 10.5f, juce::Justification::centredLeft, 0.6f);
    if (k[HighCut] < 19900.0 && values.switches[Space] != verb::Classic)
    {
        const float hiX = xForHz(k[HighCut]);
        g.setColour(juce::Colour(0x5502140a));
        g.fillRect(juce::Rectangle<float>(hiX, top, xForHz(20000.0) - hiX, bottom - top));
        g.setColour(kPhosphorDim.withAlpha(0.7f));
        for (float y = top; y < bottom; y += 6.0f)
            g.drawLine(hiX, y, hiX, y + 3.0f, 1.0f);
        crtText(g, "HI " + hz(k[HighCut]), juce::Rectangle<float>(hiX - 83.0f, bottom - 16.0f, 80.0f, 13.0f), 10.5f, juce::Justification::centredRight, 0.6f);
    }

    // the decay curve, its fill breathing with the wet level
    juce::Path curve, fill;
    for (int i = 0; i <= 160; ++i)
    {
        const double f = 20.0 * std::pow(1000.0, i / 160.0);
        const float x = xForHz(f), y = yForSeconds(verb::decayAt(f, p));
        if (i == 0)
        {
            curve.startNewSubPath(x, y);
            fill.startNewSubPath(x, bottom);
        }
        else
            curve.lineTo(x, y);
        fill.lineTo(x, y);
    }
    fill.lineTo(xForHz(20000.0), bottom);
    fill.closeSubPath();
    const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
    juce::ColourGradient body(kPhosphor.withAlpha(0.10f + 0.30f * live), 0.0f, top, kPhosphor.withAlpha(0.02f), 0.0f, bottom, false);
    g.setGradientFill(body);
    g.fillPath(fill);
    glow(g, curve, kPhosphor, 1.6f);

    // the handles
    const auto h = handlePositions();
    const bool classic = values.switches[Space] == verb::Classic;
    const std::array<juce::String, 3> captions { "BASS " + format(BassMult, k[BassMult]), "DECAY " + format(Decay, k[Decay]),
                                                 "HIGH " + format(HighMult, k[HighMult]) };
    for (int i = 0; i < 3; ++i)
    {
        if (classic && i == BassHandle)
            continue;
        const bool hot = dragHandle == i;
        glowDot(g, h[static_cast<size_t>(i)], hot ? 8.0f : 6.5f, kPhosphor, hot ? 1.0f : 0.85f);
        const auto c = h[static_cast<size_t>(i)];
        crtText(g, captions[static_cast<size_t>(i)], juce::Rectangle<float>(c.x - 60.0f, c.y - 26.0f, 120.0f, 13.0f), 10.5f,
                juce::Justification::centred, 0.85f);
    }
    if (!classic)
    {
        for (double f : { k[BassXover], k[HighXover] })
        {
            const float x = xForHz(f);
            g.setColour(kPhosphor.withAlpha(0.35f));
            for (float y = top; y < bottom; y += 4.0f)
                g.fillRect(x - 0.5f, y, 1.0f, 1.5f);
        }
    }
}

void HamloVerbPanel::drawTimeView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    const auto& k = values.knobs;
    const int space = values.switches[Space];
    const bool classic = space == verb::Classic;
    const auto p = slotParams();
    const double pre = dsp::HamloVerb::predelaySeconds(p, 120.0);
    const double bass = classic ? 1.0 : k[BassMult];
    const double span = niceCeiling(pre + k[Decay] * std::max(1.0, bass) * 0.8 + 0.05);
    const float left = s.getX() + 34.0f, right = s.getRight() - 12.0f, top = s.getY() + 26.0f, bottom = s.getBottom() - 26.0f;
    // square-root time axis: the early reflections (tens of ms) get room next to a tail of seconds
    const auto xFor = [&](double t) { return left + (right - left) * static_cast<float>(std::sqrt(juce::jlimit(0.0, 1.0, t / span))); };
    const auto yFor = [&](double db) { return top + (bottom - top) * static_cast<float>(juce::jlimit(0.0, 1.0, -db / 60.0)); };

    for (int i = 0; i <= 6; ++i)
    {
        const float y = yFor(-10.0 * i);
        g.setColour(kPhosphorDim.withAlpha(i == 0 ? 0.45f : 0.25f));
        g.drawHorizontalLine(juce::roundToInt(y), left, right);
        crtText(g, juce::String(-10 * i), juce::Rectangle<float>(s.getX() + 2.0f, y - 7.0f, 30.0f, 14.0f), 10.5f, juce::Justification::centredLeft, 0.5f);
    }
    float lastLabel = -1.0e9f;
    for (double t : { 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 8.0, 12.0, 20.0, 30.0, 45.0, 60.0 })
    {
        if (t > span * 1.0001)
            break;
        const float x = xFor(t);
        g.setColour(kPhosphorDim.withAlpha(0.22f));
        g.drawVerticalLine(juce::roundToInt(x), top, bottom);
        if (x - lastLabel < 46.0f)
            continue;
        lastLabel = x;
        crtText(g, t < 1.0 ? juce::String(juce::roundToInt(t * 1000.0)) + "ms" : juce::String(t, 0) + "s",
                juce::Rectangle<float>(x - 30.0f, bottom + 4.0f, 60.0f, 14.0f), 10.0f, juce::Justification::centred, 0.55f);
    }
    crtText(g, "IMPULSE: EARLY REFLECTIONS + LATE DECAY (dB)", juce::Rectangle<float>(left, s.getY() + 6.0f, 360.0f, 14.0f), 11.5f,
            juce::Justification::centredLeft, 0.7f);

    // pre-delay marker
    g.setColour(kPhosphor.withAlpha(0.45f));
    for (float y = top; y < bottom; y += 5.0f)
        g.fillRect(xFor(pre) - 0.5f, y, 1.0f, 2.0f);
    crtText(g, "PRE " + juce::String(juce::roundToInt(pre * 1000.0)) + "ms", juce::Rectangle<float>(xFor(pre) + 4.0f, top + 2.0f, 80.0f, 13.0f),
            10.5f, juce::Justification::centredLeft, 0.65f);

    // late envelopes per band (build-up from the attack, then the band's T60)
    const double size = k[Size];
    const double buildUp = classic ? 0.02 : 0.015 + k[Attack] * verb::kMaxAttackSeconds * size * 0.45;
    const double distance = classic ? 0.5 : k[Distance];
    const auto& def = verb::spaceDef(space);
    const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
    const double lateDb = classic ? -10.0 : 20.0 * std::log10(std::max(1.0e-3, def.lateLevel * (0.6 + 0.8 * distance))) - 9.0;
    struct Band { double t60; float alpha; const char* name; };
    const std::array<Band, 3> bands { Band { k[Decay] * bass, 0.55f, "LOW" }, Band { k[Decay], 1.0f, "MID" },
                                      Band { k[Decay] * k[HighMult], 0.55f, "HIGH" } };
    for (const auto& b : bands)
    {
        if (classic && b.name[0] == 'L')
            continue;
        juce::Path env;
        bool started = false;
        float lastX = 0.0f, lastY = 0.0f;
        for (int i = 0; i <= 220; ++i)
        {
            const double t = pre + (span - pre) * i / 220.0;
            const double u = t - pre;
            const double build = 1.0 - std::exp(-u / buildUp);
            const double db = lateDb + 20.0 * std::log10(std::max(1.0e-6, build)) - 60.0 * u / std::max(0.05, b.t60);
            if (db < -62.0 && started)
                break;
            const float x = xFor(t), y = yFor(db);
            if (!started)
                env.startNewSubPath(x, y);
            else
                env.lineTo(x, y);
            started = true;
            lastX = x;
            lastY = y;
        }
        glow(g, env, kPhosphor, b.alpha > 0.9f ? 1.5f : 1.0f, b.alpha * (0.75f + 0.25f * live));
        crtText(g, b.name, juce::Rectangle<float>(juce::jmin(lastX + 4.0f, right - 36.0f), juce::jmin(lastY, bottom - 14.0f) - 12.0f, 36.0f, 12.0f),
                10.0f, juce::Justification::centredLeft, 0.7f * b.alpha);
    }

    // early reflections: left microphone up, right a shade bluer
    if (!classic)
    {
        const double earlyDb = 20.0 * std::log10(std::max(1.0e-3, def.earlyLevel * (1.6 - 1.2 * distance)));
        for (int mic = 0; mic < 2; ++mic)
        {
            const auto taps = verb::earlyTaps(space, mic);
            const auto col = mic == 0 ? kPhosphor : kPhosphor.withRotatedHue(0.06f);
            for (int i = 0; i < taps.count; ++i)
            {
                const double t = pre + taps.seconds[static_cast<size_t>(i)] * size;
                const double db = earlyDb + 20.0 * std::log10(std::max(1.0e-4, taps.gain[static_cast<size_t>(i)]));
                const float x = xFor(t) + (mic == 0 ? -0.8f : 0.8f);
                juce::Path spike;
                spike.startNewSubPath(x, bottom);
                spike.lineTo(x, yFor(db));
                glow(g, spike, col, 0.9f, 0.75f);
            }
        }
    }

    crtText(g, "LIVE " + (outDb > -100.0f ? juce::String(outDb, 1) + " dB" : juce::String("--")),
            juce::Rectangle<float>(right - 110.0f, s.getY() + 6.0f, 110.0f, 14.0f), 11.0f, juce::Justification::centredRight, 0.6f);
}

void HamloVerbPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kScreen;
    crtBegin(g, s);

    // persistence of the previous frames (screen pixels), then this frame's traces
    if (phosphor.isValid())
    {
        g.saveState();
        g.addTransform(toScreen().inverted());
        const auto onScreen = s.transformedBy(toScreen()).getSmallestIntegerContainer();
        g.drawImageAt(phosphor, onScreen.getX(), onScreen.getY());
        g.restoreState();
    }
    if (mode == DecayView)
        drawDecayView(g, s);
    else
        drawTimeView(g, s);

    crtEnd(g, s);

    // mode buttons on the bezel
    crtButton(g, kTabDecay, "DECAY", mode == DecayView);
    crtButton(g, kTabTime, "TIME", mode == TimeView);
}

void HamloVerbPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    drawCrt(g);
    drawColorSwitch(g);
    drawSpaceSelector(g);
    drawFreeze(g);
    lamp(g, kLamp, values.on, 0.75f);
    meterNeedle(g, meter->meter);
    drawLcd(g);
    for (const auto& k : knobs)
        drawKnob(g, k);
}

void HamloVerbPanel::timerCallback()
{
    if (!isShowing())
        return;
    float level = -120.0f, duck = 0.0f;
    if (readMeters)
        std::tie(level, duck) = readMeters();
    if (!values.on)
    {
        level = -120.0f;
        duck = 0.0f;
    }
    auto& m = meter->meter;
    const float target = juce::jlimit(-60.0f, 6.0f, level);
    m.shown += (target - m.shown) * (target > m.shown ? 0.55f : 0.14f);
    outDb += (level - outDb) * 0.3f;
    duckDb += (duck - duckDb) * 0.3f;

    // phosphor persistence: fade the old image, burn in a sweep dot that rides the live trace
    if (phosphor.isValid())
    {
        juce::Image::BitmapData data(phosphor, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < data.height; ++y)
            for (int x = 0; x < data.width; ++x)
            {
                auto c = data.getPixelColour(x, y);
                if (c.getAlpha() != 0)
                    data.setPixelColour(x, y, c.withMultipliedAlpha(0.80f));
            }
        juce::Graphics pg(phosphor);
        const auto onScreen = kScreen.transformedBy(toScreen()).getSmallestIntegerContainer().toFloat();
        pg.addTransform(toScreen().translated(-onScreen.getX(), -onScreen.getY())); // design -> image pixels
        sweep = std::fmod(sweep + 1.0 / 60.0, 1.0);
        const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
        if (live > 0.02f)
        {
            juce::Point<float> dot;
            if (mode == DecayView)
            {
                const double f = 20.0 * std::pow(1000.0, sweep);
                dot = { xForHz(f), yForSeconds(verb::decayAt(f, slotParams())) };
            }
            else
            {
                const float right = kScreen.getRight() - 12.0f;
                const float top = kScreen.getY() + 26.0f, bottom = kScreen.getBottom() - 26.0f;
                dot = { right, top + (bottom - top) * juce::jlimit(0.0f, 1.0f, -level / 60.0f) };
            }
            glowDot(pg, dot, 3.0f + 3.0f * live, kPhosphor, 0.5f + 0.5f * live);
        }
    }
    repaint();
}

void HamloVerbPanel::settleMeters(float outputDb, float duck)
{
    meter->meter.shown = juce::jlimit(-42.0f, 1.0f, outputDb);
    outDb = outputDb;
    duckDb = duck;
    repaint();
}

// ---------------------------------------------------------------------------------------------------
// mouse

int HamloVerbPanel::knobAt(juce::Point<float> d) const
{
    for (const auto& k : knobs)
        if (d.getDistanceFrom(k.centre) <= 62.0f * k.scale)
            return k.param;
    return -1;
}

int HamloVerbPanel::spaceLabelAt(juce::Point<float> d) const
{
    for (int i = 0; i < verb::NumSpaces; ++i)
    {
        if (spaceLabelArea(i).expanded(0.0f, 1.0f).contains(d))
            return i;
    }
    return -1;
}

HamloVerbPanel::Handle HamloVerbPanel::handleAt(juce::Point<float> d) const
{
    if (mode != DecayView || !kScreen.contains(d))
        return NoHandle;
    const auto h = handlePositions();
    Handle best = NoHandle;
    float bestDistance = 16.0f;
    for (int i = 0; i < 3; ++i)
    {
        if (i == BassHandle && values.switches[Space] == verb::Classic)
            continue;
        const float dist = d.getDistanceFrom(h[static_cast<size_t>(i)]);
        if (dist < bestDistance)
        {
            bestDistance = dist;
            best = static_cast<Handle>(i);
        }
    }
    return best;
}

void HamloVerbPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        dragKnob = k;
        dragStartNorm = normFor(static_cast<Param>(k));
        dragStartY = e.position.y;
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        return;
    }
    if (d.getDistanceFrom(kSelector) <= 60.0f)
    {
        dragSpace = true;
        dragSpaceStart = values.switches[Space];
        dragStartY = e.position.y;
        return;
    }
    if (const int s = spaceLabelAt(d); s >= 0)
    {
        setSwitch(Space, s);
        return;
    }
    if (const auto h = handleAt(d); h != NoHandle)
    {
        dragHandle = h;
        if (onKnobGestureStart)
            onKnobGestureStart(h == BassHandle ? BassMult : h == MidHandle ? Decay : HighMult);
        return;
    }
    if (kTabDecay.contains(d))
    {
        setDisplayMode(DecayView);
        return;
    }
    if (kTabTime.contains(d))
    {
        setDisplayMode(TimeView);
        return;
    }
    const float dialTop = kColorTop + 22.0f;
    if (d.x >= kColorX && d.x <= kColorX + kColorW && d.y >= dialTop && d.y <= dialTop + 74.0f)
    {
        int option = -1;
        if (d.y <= dialTop + 20.0f)
            for (int i = 0; i < 3; ++i)
                if (std::abs(d.x - colorOptionX(i)) <= 40.0f)
                    option = i;
        if (option < 0)
            option = (values.switches[Color] + 1) % 3;
        setSwitch(Color, option);
        return;
    }
    if (juce::Rectangle<float>(kFreezeLever.x - 30.0f, kFreezeLever.y - 44.0f, 60.0f, 70.0f).contains(d) || d.getDistanceFrom(kFreezeLamp) <= 16.0f)
    {
        setSwitch(Freeze, values.switches[Freeze] > 0 ? 0 : 1);
        return;
    }
    if (d.getDistanceFrom(kLamp) <= 28.0f)
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    const auto& pre = knobs[PreDelay];
    for (int i = 0; i < 2; ++i)
        if (juce::Rectangle<float>(pre.centre.x - 62.0f + static_cast<float>(i) * 64.0f, kSyncY - 9.0f, 60.0f, 18.0f).contains(d))
        {
            if (values.switches[Space] != verb::Classic)
                setSwitch(Sync, i == 0 ? 0 : (values.switches[Sync] > 0 ? values.switches[Sync] : 7)); // 1/8 to start
            return;
        }
    for (const auto& k : knobs) // a click on a readout types a value
    {
        const bool large = k.param < kLarge;
        const auto r = juce::Rectangle<float>(k.centre.x - (large ? 80.0f : 48.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 12.0f,
                                              large ? 160.0f : 96.0f, 24.0f);
        if (r.contains(d))
        {
            editReadout(k.param);
            return;
        }
    }
}

void HamloVerbPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob >= 0)
    {
        const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
        setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
        return;
    }
    if (dragSpace)
    {
        const int steps = juce::roundToInt((dragStartY - e.position.y) / (28.0f * scale));
        const int s = juce::jlimit(0, verb::NumSpaces - 1, dragSpaceStart + steps);
        if (s != values.switches[Space])
            setSwitch(Space, s);
        return;
    }
    if (dragHandle != NoHandle)
    {
        const auto d = toDesign(e.position);
        const double seconds = juce::jmax(0.05, secondsForY(d.y));
        const double decay = values.knobs[Decay];
        if (dragHandle == MidHandle)
            setKnob(Decay, seconds, true);
        else if (dragHandle == BassHandle)
        {
            setKnob(BassMult, seconds / decay, true);
            setKnob(BassXover, hzForX(d.x) * 2.0, true);
        }
        else
        {
            setKnob(HighMult, seconds / decay, true);
            setKnob(HighXover, hzForX(d.x) * 0.5, true);
        }
    }
}

void HamloVerbPanel::mouseUp(const juce::MouseEvent&)
{
    if ((dragKnob >= 0 || dragHandle != NoHandle) && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
    dragHandle = NoHandle;
    dragSpace = false;
}

void HamloVerbPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        const auto p = static_cast<Param>(k);
        if (p == PreDelay && values.switches[Sync] > 0)
            return;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setKnob(p, defaultFor(p), true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloVerbPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        const double step = (e.mods.isShiftDown() ? 0.004 : 0.02) * (wheel.deltaY > 0 ? 1.0 : -1.0);
        setNorm(static_cast<Param>(k), normFor(static_cast<Param>(k)) + step, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        return;
    }
    if (d.getDistanceFrom(kSelector) <= 60.0f)
        setSwitch(Space, juce::jlimit(0, verb::NumSpaces - 1, values.switches[Space] + (wheel.deltaY > 0 ? 1 : -1)));
}

void HamloVerbPanel::editReadout(Param p)
{
    if (p == PreDelay && values.switches[Sync] > 0)
        return;
    const auto& k = knobs[static_cast<size_t>(p)];
    const bool large = p < kLarge;
    const auto area = juce::Rectangle<float>(k.centre.x - (large ? 70.0f : 46.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 13.0f,
                                             large ? 140.0f : 92.0f, 26.0f)
                          .transformedBy(toScreen())
                          .toNearestInt();
    const bool percent = p == Size || p == Attack || p == Distance || p == Width || p == EarlyDiff || p == LateDiff || p == ModDepth || p == Duck;
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(stencilFont((large ? 20.0f : 15.0f) * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const double v = values.knobs[static_cast<size_t>(p)];
    typing->setText(percent ? juce::String(juce::roundToInt(v * 100.0)) : juce::String(v, v < 10.0 ? 2 : 0), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloVerbPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, percent, closeEditor]
    {
        if (typing == nullptr)
            return;
        const auto text = typing->getText().toLowerCase();
        double entered = text.retainCharacters("-0123456789.").getDoubleValue();
        if (percent)
            entered /= 100.0;
        if (text.containsChar('k'))
            entered *= 1000.0;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setKnob(p, entered, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        closeEditor();
    };
    typing->onReturnKey = commit;
    typing->onFocusLost = commit;
    typing->onEscapeKey = closeEditor;
    addAndMakeVisible(*typing);
    typing->grabKeyboardFocus();
}
} // namespace hpeq::ui
