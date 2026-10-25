#include "UI/HamloEchoPanel.h"

#include <cmath>

#include "DSP/HamloEcho.h"
#include "UI/BlackSteel.h"

namespace hpeq::ui
{
namespace
{
using namespace steel;
namespace echo = dsp::echo;

constexpr float kW = 1180.0f, kH = 760.0f; // design stage
constexpr float kPi = juce::MathConstants<float>::pi;

enum class Map { Linear, Log };
struct MapInfo
{
    double min, max;
    Map kind;
};
constexpr std::array<MapInfo, HamloEchoPanel::NumParams> kMaps { {
    { 1.0, 4000.0, Map::Log },      // TimeL (ms)
    { 1.0, 4000.0, Map::Log },      // TimeR (ms)
    { 0.0, 0.98, Map::Linear },     // Feedback
    { 0.0, 1.0, Map::Linear },      // Cross
    { 0.0, 24.0, Map::Linear },     // Drive (dB)
    { 0.0, 1.5, Map::Linear },      // Width
    { 0.0, 1.0, Map::Linear },      // Mix
    { 0.05, 10.0, Map::Log },       // ModRate (Hz)
    { 0.0, 1.0, Map::Linear },      // ModDepth
    { 0.0, 1.0, Map::Linear },      // Diffusion
    { -12.0, 12.0, Map::Linear },   // Pitch (st)
    { 0.0, 1.0, Map::Linear },      // Lofi
    { 10.0, 2000.0, Map::Log },     // LowCut (Hz)
    { 500.0, 20000.0, Map::Log },   // HighCut (Hz)
    { 100.0, 10000.0, Map::Log },   // BandFreq (Hz)
    { -18.0, 12.0, Map::Linear },   // BandGain (dB)
    { 0.3, 6.0, Map::Log },         // BandQ
    { 0.0, 1.0, Map::Linear },      // Duck
    { 0.0, 1.0, Map::Linear },      // LowRes
    { 0.0, 1.0, Map::Linear },      // HighRes
} };
constexpr std::array<const char*, HamloEchoPanel::kNumKnobs> kLabels { "Time L", "Time R", "Feedback", "Cross", "Drive", "Width", "Mix",
                                                                       "Mod rate", "Mod depth", "Diffusion", "Pitch", "Lo-Fi", "Low cut",
                                                                       "High cut", "Band freq", "Band gain", "Band Q", "Duck" };
constexpr int kLarge = 7;

// Layout (design coordinates): the same frame as HamloVerb.
const juce::Rectangle<float> kPlate { 405.0f, 41.0f, 370.0f, 92.0f };
const juce::Rectangle<float> kLcd { 940.0f, 52.0f, 184.0f, 64.0f };
const juce::Point<float> kSelector { 174.0f, 302.0f };
constexpr float kSelectorLabelRadius = 104.0f;
const juce::Rectangle<float> kBezel { 350.0f, 156.0f, 512.0f, 290.0f };
const juce::Rectangle<float> kScreen { 368.0f, 172.0f, 476.0f, 238.0f };
const juce::Rectangle<float> kTabEcho { 520.0f, 418.0f, 84.0f, 20.0f }, kTabFilter { 612.0f, 418.0f, 84.0f, 20.0f };
const juce::Rectangle<float> kMeterBezel { 884.0f, 156.0f, 240.0f, 180.0f };
const juce::Point<float> kLamp { 918.0f, 404.0f }, kPingLever { 1000.0f, 412.0f }, kFreezeLever { 1078.0f, 412.0f },
    kFreezeLamp { 1110.0f, 440.0f };
constexpr float kModeX = 56.0f, kModeW = 250.0f, kModeTop = 46.0f;
constexpr float kLargeY = 535.0f, kLargeLabelY = 462.0f, kLargeReadoutY = 604.0f, kSyncY = 623.0f;
constexpr float kSmallY = 692.0f, kSmallLabelY = 646.0f, kSmallReadoutY = 733.0f;
const juce::Colour kRight = phosphor.withRotatedHue(0.08f);

float characterAngle(int i) { return (-120.0f + 60.0f * static_cast<float>(i)) * kPi / 180.0f; }

juce::Rectangle<float> characterLabelArea(int i)
{
    const float a = characterAngle(i);
    const juce::Point<float> c(kSelector.x + std::sin(a) * kSelectorLabelRadius, kSelector.y - std::cos(a) * kSelectorLabelRadius);
    if (std::sin(a) > 0.5f)
        return { c.x - 6.0f, c.y - 10.0f, 96.0f, 20.0f };
    if (std::sin(a) < -0.5f)
        return { c.x - 90.0f, c.y - 10.0f, 96.0f, 20.0f };
    return { c.x - 46.0f, c.y - 10.0f, 92.0f, 20.0f };
}

float modeOptionX(int i) { return kModeX + kModeW * (0.2f + 0.6f * static_cast<float>(i)); }

juce::String hz(double v)
{
    return v >= 1000.0 ? juce::String(v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz";
}

juce::String ms(double seconds)
{
    return seconds < 1.0 ? juce::String(juce::roundToInt(seconds * 1000.0)) + " ms" : juce::String(seconds, 2) + " s";
}

double niceCeiling(double v)
{
    for (double n : { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 24.0, 30.0 })
        if (v <= n)
            return n;
    return 30.0;
}

juce::Rectangle<float> syncTag(juce::Point<float> knob, int i)
{
    return { knob.x - 62.0f + static_cast<float>(i) * 64.0f, kSyncY - 8.0f, 60.0f, 16.0f };
}
} // namespace

struct HamloEchoPanel::MeterState
{
    Meter meter;
};

HamloEchoPanel::HamloEchoPanel() : meter(std::make_unique<MeterState>())
{
    for (int i = 0; i < kNumKnobs; ++i)
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
            k.centre = { 56.0f + (1068.0f / (kNumKnobs - kLarge)) * (static_cast<float>(i - kLarge) + 0.5f), kSmallY };
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
    m.seed = 41;
    m.shown = -42.0f;
    setOpaque(false);
    startTimerHz(30);
}

HamloEchoPanel::~HamloEchoPanel()
{
    stopTimer();
    if (typing != nullptr)
    {
        typing->onReturnKey = {};
        typing->onFocusLost = {};
        typing->onEscapeKey = {};
        typing.reset();
    }
}

const char* HamloEchoPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "timeMs", "timeRMs", "feedback", "cross", "driveDb", "width", "mix", "modRate", "modDepth",
                                          "diffusion", "pitch", "lofi", "lowCutHz", "dampingHz", "bandHz", "bandGainDb", "bandQ",
                                          "duck", "lowRes", "highRes" };
    return keys[p];
}

const char* HamloEchoPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "character", "timeMode", "sync", "syncR", "link", "pingPong", "freeze" };
    return keys[s];
}

double HamloEchoPanel::defaultFor(Param p)
{
    return Values {}.knobs[static_cast<size_t>(p)]; // = defaultModule(Delay) and the DSP's fallbacks
}

bool HamloEchoPanel::usedByClassic(Param p) noexcept
{
    return p == TimeL || p == Feedback || p == Width || p == Mix || p == LowCut || p == HighCut;
}

bool HamloEchoPanel::isActive(Param p) const noexcept
{
    if (values.switches[Character] == echo::Classic)
        return usedByClassic(p);
    if (p == TimeR)
        return values.switches[Link] == 0;
    return true;
}

bool HamloEchoPanel::synced(Param p) const noexcept
{
    return (p == TimeL && values.switches[SyncL] > 0) || (p == TimeR && values.switches[SyncR] > 0);
}

double HamloEchoPanel::tempo() const
{
    return readTempo ? juce::jlimit(20.0, 999.0, readTempo()) : 120.0;
}

juce::String HamloEchoPanel::format(Param p, double v)
{
    switch (p)
    {
        case TimeL: case TimeR: return v < 1000.0 ? juce::String(juce::roundToInt(v)) + " ms" : juce::String(v / 1000.0, 2) + " s";
        case Feedback: case Cross: case Width: case Mix: case ModDepth: case Diffusion: case Lofi: case Duck: case LowRes: case HighRes:
            return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case Drive: case BandGain: return (v > 0.05 ? "+" : "") + juce::String(v, 1) + " dB";
        case ModRate: return juce::String(v, v < 1.0 ? 2 : 1) + " Hz";
        case Pitch: return (v > 0.05 ? "+" : "") + juce::String(v, 1) + " st";
        case LowCut: case BandFreq: return hz(v);
        case HighCut: return v >= 19900.0 ? juce::String("OFF") : hz(v);
        case BandQ: return "Q " + juce::String(v, 2);
        case NumParams: break;
    }
    return {};
}

dsp::FxSlotParams HamloEchoPanel::slotParams() const
{
    namespace s = dsp::slot;
    dsp::FxSlotParams p;
    p.active = values.on;
    p.type = FxType::Delay;
    const auto& k = values.knobs;
    const auto& sw = values.switches;
    p.values[s::kDelayTimeMs] = static_cast<float>(k[TimeL]);
    p.values[s::kDelaySync] = static_cast<float>(sw[SyncL]);
    p.values[s::kFeedback] = static_cast<float>(k[Feedback]);
    p.values[s::kPingPong] = static_cast<float>(sw[PingPong]);
    p.values[s::kDampingHz] = static_cast<float>(k[HighCut]);
    p.values[s::kDelayLowCutHz] = static_cast<float>(k[LowCut]);
    p.values[s::kDelayWidth] = static_cast<float>(k[Width]);
    p.mix = static_cast<float>(k[Mix]);
    p.values[s::kDlyCharacter] = static_cast<float>(sw[Character]);
    p.values[s::kDlyTimeRMs] = static_cast<float>(k[TimeR]);
    p.values[s::kDlySyncR] = static_cast<float>(sw[SyncR]);
    p.values[s::kDlyLink] = static_cast<float>(sw[Link]);
    p.values[s::kDlyCross] = static_cast<float>(k[Cross]);
    p.values[s::kDlyDriveDb] = static_cast<float>(k[Drive]);
    p.values[s::kDlyModRate] = static_cast<float>(k[ModRate]);
    p.values[s::kDlyModDepth] = static_cast<float>(k[ModDepth]);
    p.values[s::kDlyDiffusion] = static_cast<float>(k[Diffusion]);
    p.values[s::kDlyPitch] = static_cast<float>(k[Pitch]);
    p.values[s::kDlyLowRes] = static_cast<float>(k[LowRes]);
    p.values[s::kDlyHighRes] = static_cast<float>(k[HighRes]);
    p.values[s::kDlyBandHz] = static_cast<float>(k[BandFreq]);
    p.values[s::kDlyBandGainDb] = static_cast<float>(k[BandGain]);
    p.values[s::kDlyBandQ] = static_cast<float>(k[BandQ]);
    p.values[s::kDlyDuck] = static_cast<float>(k[Duck]);
    p.values[s::kDlyFreeze] = static_cast<float>(sw[Freeze]);
    p.values[s::kDlyLofi] = static_cast<float>(k[Lofi]);
    p.values[s::kDlyTimeMode] = static_cast<float>(sw[TimeMode]);
    if (sw[Character] == echo::Classic)
    {
        // the M6 engine: one time, no resonance, no band, no cross
        p.values[s::kDlyLink] = 1.0f;
        p.values[s::kDlyCross] = 0.0f;
        p.values[s::kDlyLowRes] = p.values[s::kDlyHighRes] = 0.0f;
        p.values[s::kDlyBandGainDb] = 0.0f;
    }
    return p;
}

double HamloEchoPanel::normFor(Param p) const
{
    if (p == TimeL && values.switches[SyncL] > 0)
        return (values.switches[SyncL] - 1) / static_cast<double>(echo::kSyncDivisions - 2);
    if (p == TimeR && values.switches[SyncR] > 0)
        return (values.switches[SyncR] - 1) / static_cast<double>(echo::kSyncDivisions - 2);
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.kind == Map::Log ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloEchoPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    return m.kind == Map::Log ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
}

void HamloEchoPanel::setKnob(Param p, double v, bool notify)
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
    if (notify && onKnobChange)
        onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
    repaint();
}

void HamloEchoPanel::setNorm(Param p, double n, bool notify)
{
    if (synced(p))
    {
        const auto which = p == TimeL ? SyncL : SyncR;
        const int division = 1 + juce::roundToInt(juce::jlimit(0.0, 1.0, n) * (echo::kSyncDivisions - 2));
        if (division != values.switches[which])
            setSwitch(which, division);
        return;
    }
    setKnob(p, valueFor(p, n), notify);
}

void HamloEchoPanel::setSwitch(Switch s, int v)
{
    values.switches[static_cast<size_t>(s)] = v;
    if (onSwitchChange)
        onSwitchChange(s, v);
    repaint();
}

void HamloEchoPanel::setValues(const Values& v)
{
    if (dragKnob >= 0 || dragHandle != NoHandle || dragSelector)
        return; // the drag owns the value until the gesture ends
    values = v;
    repaint();
}

void HamloEchoPanel::setDisplayMode(DisplayMode m)
{
    mode = m;
    if (phosphorImage.isValid())
        phosphorImage.clear(phosphorImage.getBounds());
    repaint();
}

juce::AffineTransform HamloEchoPanel::toScreen() const
{
    return juce::AffineTransform::scale(scale).translated(offset);
}

juce::Point<float> HamloEchoPanel::toDesign(juce::Point<float> p) const
{
    return (p - offset) / scale;
}

void HamloEchoPanel::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    phosphorImage = {};
    if (getWidth() > 0 && getHeight() > 0)
    {
        staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(staticLayer);
        g.addTransform(toScreen());
        renderStatic(g);
        const auto screen = kScreen.transformedBy(toScreen()).getSmallestIntegerContainer();
        phosphorImage = juce::Image(juce::Image::ARGB, juce::jmax(1, screen.getWidth()), juce::jmax(1, screen.getHeight()), true);
    }
    typing.reset();
}

// ---------------------------------------------------------------------------------------------------
// static plate

void HamloEchoPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 2026);
    steel::scratches(g, { { 9, { 304.0f, 14.0f, 82.0f, 122.0f }, -4.0f }, { 22, { 836.0f, 14.0f, 82.0f, 122.0f }, 5.0f } });
    nameplate(g, kPlate, "HamloEcho", juce::String::fromUTF8("MODEL \xd0\xad\xd0\xa5\xd0\x9e-2"), juce::String::fromUTF8("\xe2\x84\x96 0421"));
    engraved(g, "TIME MODE", juce::Rectangle<float>(kModeX, kModeTop, kModeW, 22.0f), 16.0f);
    lcdWindow(g, kLcd);

    engraved(g, "CHARACTER", juce::Rectangle<float>(kSelector.x - 90.0f, 140.0f, 180.0f, 26.0f), 20.0f);
    for (int i = 0; i < echo::NumCharacters; ++i)
    {
        const float a = characterAngle(i);
        const juce::Point<float> d(kSelector.x + std::sin(a) * 74.0f, kSelector.y - std::cos(a) * 74.0f);
        g.setColour(juce::Colour(0xffefe3bd));
        g.fillEllipse(d.x - 3.0f, d.y - 3.0f, 6.0f, 6.0f);
        g.setColour(juce::Colour(0xffefe3bd).withAlpha(0.6f));
        g.drawLine(kSelector.x + std::sin(a) * 80.0f, kSelector.y - std::cos(a) * 80.0f, kSelector.x + std::sin(a) * 90.0f,
                   kSelector.y - std::cos(a) * 90.0f, 1.6f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(kSelector.x - 62.0f, kSelector.y - 50.0f, 124.0f, 124.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(kSelector.x - 61.0f, kSelector.y - 61.0f, 122.0f, 122.0f);

    crtBezel(g, kBezel, kScreen, juce::String::fromUTF8("\xd0\xad\xd0\x9a\xd0\xa0\xd0\x90\xd0\x9d \xc2\xb7 \xd0\xa1\xd0\x9a\xd0\x9e"), 420.0f); // ЭКРАН · СКО

    meterFace(g, meter->meter);
    engraved(g, "ON", juce::Rectangle<float>(kLamp.x - 40.0f, 430.0f, 80.0f, 24.0f), 18.0f);
    engraved(g, juce::String::fromUTF8("PING\xc2\xb7PONG"), juce::Rectangle<float>(kPingLever.x - 46.0f, 350.0f, 92.0f, 22.0f), 14.0f);
    engraved(g, "FREEZE", juce::Rectangle<float>(kFreezeLever.x - 34.0f, 350.0f, 80.0f, 22.0f), 14.0f);

    for (const auto& k : knobs)
    {
        const bool large = k.param < kLarge;
        const float labelY = large ? kLargeLabelY : kSmallLabelY;
        const float w = large ? 148.0f : 96.0f;
        engraved(g, juce::String(kLabels[static_cast<size_t>(k.param)]).toUpperCase(),
                 juce::Rectangle<float>(k.centre.x - w * 0.5f, labelY - 13.0f, w, 26.0f), large ? 20.0f : 13.0f, stencil,
                 juce::Justification::centred, 0.1f);
        knobScale(g, k.centre, k.scale);
    }
    g.setColour(juce::Colours::black);
    g.drawHorizontalLine(632, 56.0f, kW - 56.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(631, 56.0f, kW - 56.0f);
}

// ---------------------------------------------------------------------------------------------------
// dynamic parts

void HamloEchoPanel::drawKnob(juce::Graphics& g, const Knob& k) const
{
    const bool active = isActive(k.param);
    const bool linkedR = k.param == TimeR && values.switches[Link] > 0 && values.switches[Character] != echo::Classic;
    const auto shown = linkedR ? TimeL : k.param; // a linked right time follows the left one
    const float angle = (-135.0f + static_cast<float>(normFor(shown)) * 270.0f) * kPi / 180.0f;
    brassKnob(g, k.centre, angle, k.scale, active ? 1.0f : 0.45f, KnobAsset::LatheSteelRust, PointerAsset::WhiteEnamel);
    const bool large = k.param < kLarge;
    const auto r = juce::Rectangle<float>(k.centre.x - (large ? 80.0f : 48.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 12.0f,
                                          large ? 160.0f : 96.0f, 24.0f);
    juce::String text = format(shown, values.knobs[static_cast<size_t>(shown)]);
    if (synced(shown))
        text = echo::syncName(values.switches[shown == TimeL ? SyncL : SyncR]);
    g.setFont(stencilFont(large ? 22.0f : 15.0f));
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0, 1), juce::Justification::centred);
    g.setColour(active || linkedR ? stencil.withAlpha(linkedR ? 0.55f : 1.0f) : stencilDim.withAlpha(0.6f));
    g.drawText(text, r, juce::Justification::centred);

    if (k.param == TimeL || k.param == TimeR)
    {
        const bool classic = values.switches[Character] == echo::Classic;
        const bool usable = k.param == TimeL || (!classic && values.switches[Link] == 0);
        const bool sync = values.switches[k.param == TimeL ? SyncL : SyncR] > 0;
        for (int i = 0; i < 2; ++i)
        {
            const auto t = syncTag(k.centre, i);
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
    if (k.param == TimeR)
    {
        // LINK between the two time knobs: a chain glyph and a lamp
        const juce::Point<float> c((knobs[TimeL].centre.x + k.centre.x) * 0.5f, kLargeY);
        const bool linked = values.switches[Link] > 0;
        const bool usable = values.switches[Character] != echo::Classic;
        juce::Path chain;
        chain.addRoundedRectangle(c.x - 15.0f, c.y - 5.0f, 17.0f, 10.0f, 5.0f);
        chain.addRoundedRectangle(c.x - 2.0f, c.y - 5.0f, 17.0f, 10.0f, 5.0f);
        g.setColour(juce::Colours::black);
        g.strokePath(chain, juce::PathStrokeType(3.6f), juce::AffineTransform::translation(0.0f, 1.0f));
        g.setColour(linked && usable ? juce::Colour(0xfffff6d0) : stencilDim.withAlpha(usable ? 1.0f : 0.4f));
        g.strokePath(chain, juce::PathStrokeType(2.2f));
        g.setFont(stencilFont(12.5f).withKerningFactor(0.15f));
        g.drawText("LINK", juce::Rectangle<float>(c.x - 30.0f, c.y + 12.0f, 60.0f, 16.0f), juce::Justification::centred);
        if (linked && usable)
        {
            g.setColour(juce::Colour(0xffff4a3a));
            g.fillEllipse(c.x - 3.0f, c.y - 22.0f, 6.0f, 6.0f);
        }
    }
}

void HamloEchoPanel::drawSelector(juce::Graphics& g) const
{
    const int selected = juce::jlimit(0, echo::NumCharacters - 1, values.switches[Character]);
    for (int i = 0; i < echo::NumCharacters; ++i)
    {
        const float a = characterAngle(i);
        const auto r = characterLabelArea(i);
        const auto just = std::sin(a) > 0.5f ? juce::Justification::centredLeft
                          : std::sin(a) < -0.5f ? juce::Justification::centredRight : juce::Justification::centred;
        const auto text = juce::String(echo::characterName(i)).toUpperCase();
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
    const float a = characterAngle(selected);
    brassKnob(g, kSelector, a, 1.15f, 1.0f, KnobAsset::LatheSteelRust, PointerAsset::WhiteEnamel);
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

void HamloEchoPanel::drawTimeMode(juce::Graphics& g) const
{
    const int selected = juce::jlimit(0, 1, values.switches[TimeMode]);
    const bool usable = values.switches[Character] != echo::Classic;
    const float dialTop = kModeTop + 22.0f;
    for (int i = 0; i < 2; ++i)
    {
        const auto r = juce::Rectangle<float>(modeOptionX(i) - 40.0f, dialTop, 80.0f, 18.0f);
        const auto text = juce::String(i == 0 ? "TAPE" : "FADE");
        g.setFont(stencilFont(13.5f).withKerningFactor(0.1f));
        if (i == selected && usable)
        {
            g.setColour(juce::Colour(0x55ffe9a0));
            g.drawText(text, r.expanded(1.0f), juce::Justification::centred);
            g.setColour(juce::Colour(0xfffff6d0));
        }
        else
            g.setColour(stencilDim.withAlpha(usable ? 1.0f : 0.4f));
        g.drawText(text, r, juce::Justification::centred);
    }
    lever(g, { kModeX + kModeW * 0.5f, dialTop + 50.0f }, (selected == 0 ? -34.0f : 34.0f) * kPi / 180.0f, 0.9f);
}

void HamloEchoPanel::drawLevers(juce::Graphics& g) const
{
    lamp(g, kLamp, values.on, 0.62f);
    for (const auto& [centre, on] : { std::pair { kPingLever, values.switches[PingPong] > 0 }, std::pair { kFreezeLever, values.switches[Freeze] > 0 } })
    {
        lever(g, centre, on ? 0.0f : kPi, 0.8f, on ? LeverPosition::Up : LeverPosition::Down);
        g.setFont(stencilFont(10.5f).withKerningFactor(0.15f));
        g.setColour(stencilDim);
        g.drawText("ON", juce::Rectangle<float>(centre.x - 52.0f, centre.y - 30.0f, 30.0f, 13.0f), juce::Justification::centred);
        g.drawText("OFF", juce::Rectangle<float>(centre.x - 52.0f, centre.y + 16.0f, 30.0f, 13.0f), juce::Justification::centred);
    }
    lamp(g, kFreezeLamp, values.switches[Freeze] > 0, 0.32f, juce::Colour(0xff57d8ff));
}

void HamloEchoPanel::drawLcd(juce::Graphics& g) const
{
    const auto& sw = values.switches;
    const int character = sw[Character];
    juce::String line1 = juce::String(echo::characterName(character)).toUpperCase();
    if (character != echo::Classic && sw[TimeMode] == echo::FadeTime)
        line1 << " FADE";
    const auto p = slotParams();
    const double t = tempo();
    const auto timeText = [&](bool right)
    {
        const int s = right ? sw[SyncR] : sw[SyncL];
        return s > 0 ? juce::String(echo::syncName(s)) : juce::String(juce::roundToInt(echo::timeSeconds(p, right, t) * 1000.0));
    };
    juce::String line2 = timeText(false);
    if (character != echo::Classic && sw[Link] == 0)
        line2 << "|" << timeText(true);
    line2 << " FB" << juce::roundToInt(values.knobs[Feedback] * 100.0);
    if (sw[Freeze] > 0 && character != echo::Classic)
        line2 = "FREEZE";
    if (duckDb > 0.05f)
        line2 << " D-" << juce::String(duckDb, 1);
    lcdText(g, kLcd.withHeight(kLcd.getHeight() * 0.5f).translated(0, 3.0f), line1, 18.0f);
    lcdText(g, kLcd.withTrimmedTop(kLcd.getHeight() * 0.5f).translated(0, -3.0f), line2, line2.length() > 15 ? 15.0f : 18.0f);
}

// ---------------------------------------------------------------------------------------------------
// CRT

float HamloEchoPanel::xForHz(double f) const
{
    const auto s = kScreen.reduced(30.0f, 0.0f);
    return s.getX() + s.getWidth() * static_cast<float>(std::log(juce::jlimit(20.0, 20000.0, f) / 20.0) / std::log(1000.0));
}

double HamloEchoPanel::hzForX(float x) const
{
    const auto s = kScreen.reduced(30.0f, 0.0f);
    return 20.0 * std::pow(1000.0, juce::jlimit(0.0, 1.0, static_cast<double>((x - s.getX()) / s.getWidth())));
}

float HamloEchoPanel::yForDb(double db) const
{
    const float top = kScreen.getY() + 26.0f, bottom = kScreen.getBottom() - 26.0f;
    return top + (bottom - top) * static_cast<float>(juce::jlimit(0.0, 1.0, (15.0 - db) / 45.0)); // +15 .. -30 dB
}

double HamloEchoPanel::dbForY(float y) const
{
    const float top = kScreen.getY() + 26.0f, bottom = kScreen.getBottom() - 26.0f;
    return 15.0 - 45.0 * static_cast<double>((y - top) / (bottom - top));
}

std::array<juce::Point<float>, 3> HamloEchoPanel::handlePositions() const
{
    const auto& k = values.knobs;
    return { juce::Point<float>(xForHz(k[LowCut]), yForDb(-3.0 + 15.0 * k[LowRes])),
             juce::Point<float>(xForHz(k[BandFreq]), yForDb(k[BandGain])),
             juce::Point<float>(xForHz(k[HighCut]), yForDb(-3.0 + 15.0 * k[HighRes])) };
}

std::vector<HamloEchoPanel::Echo> HamloEchoPanel::echoTrain(double& span) const
{
    // The echo train of an impulse into both lines (ping-pong: the left line only), from the feedback,
    // the cross amount and the loop's mean gain over 300 Hz .. 3 kHz.
    const auto p = slotParams();
    const double bpm = tempo();
    const std::array<double, 2> time { echo::timeSeconds(p, false, bpm), echo::timeSeconds(p, true, bpm) };
    const bool classic = values.switches[Character] == echo::Classic;
    double meanDb = 0.0;
    for (int i = 0; i < 9; ++i)
        meanDb += echo::loopResponseDb(300.0 * std::pow(10.0, i / 8.0), p) / 9.0;
    const double loopDb = classic ? 0.0 : meanDb;
    const double fb = values.knobs[Feedback], cross = classic ? (values.switches[PingPong] > 0 ? 1.0 : 0.0) : values.knobs[Cross];
    const bool frozen = values.switches[Freeze] > 0 && !classic;
    span = niceCeiling(std::max(time[0], time[1]) * (fb > 0.05 || frozen ? 8.0 : 2.0));
    std::vector<Echo> out;
    std::vector<Echo> pending;
    const bool ping = values.switches[PingPong] > 0;
    pending.push_back({ time[0], loopDb, 0, 1 });
    if (!ping)
        pending.push_back({ time[1], loopDb, 1, 1 });
    while (!pending.empty() && out.size() < 160)
    {
        const auto e = pending.front();
        pending.erase(pending.begin());
        if (e.seconds > span || e.db < -48.0)
            continue;
        out.push_back(e);
        const double same = frozen ? 1.0 : fb * (1.0 - cross), other = frozen ? 0.0 : fb * cross;
        const double next = frozen ? 0.0 : loopDb;
        if (same > 1.0e-3)
            pending.push_back({ e.seconds + time[static_cast<size_t>(e.side)], e.db + 20.0 * std::log10(same) + next, e.side, e.repeat + 1 });
        if (other > 1.0e-3)
            pending.push_back({ e.seconds + time[static_cast<size_t>(1 - e.side)], e.db + 20.0 * std::log10(other) + next, 1 - e.side, e.repeat + 1 });
    }
    return out;
}

void HamloEchoPanel::drawEchoView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    double span = 1.0;
    const auto train = echoTrain(span);
    const float left = s.getX() + 34.0f, right = s.getRight() - 12.0f, top = s.getY() + 26.0f, bottom = s.getBottom() - 26.0f;
    const float mid = 0.5f * (top + bottom), half = 0.5f * (bottom - top) - 4.0f;
    const auto xFor = [&](double t) { return left + (right - left) * static_cast<float>(juce::jlimit(0.0, 1.0, t / span)); };
    const auto height = [&](double db) { return half * static_cast<float>(juce::jlimit(0.0, 1.0, (db + 48.0) / 48.0)); };

    for (double db : { 0.0, -12.0, -24.0, -36.0 })
    {
        g.setColour(phosphorDim.withAlpha(db == 0.0 ? 0.35f : 0.2f));
        g.drawHorizontalLine(juce::roundToInt(mid - height(db)), left, right);
        g.drawHorizontalLine(juce::roundToInt(mid + height(db)), left, right);
        if (db > -40.0)
            crtText(g, juce::String(juce::roundToInt(db)), juce::Rectangle<float>(s.getX() + 2.0f, mid - height(db) - 7.0f, 30.0f, 14.0f), 10.0f,
                    juce::Justification::centredLeft, 0.5f);
    }
    g.setColour(phosphorDim.withAlpha(0.6f));
    g.drawHorizontalLine(juce::roundToInt(mid), left, right);
    for (int i = 0; i <= 8; ++i)
    {
        const double t = span * i / 8.0;
        const float x = xFor(t);
        g.setColour(phosphorDim.withAlpha(0.22f));
        g.drawVerticalLine(juce::roundToInt(x), top, bottom);
        if (i % 2 == 0)
            crtText(g, ms(t), juce::Rectangle<float>(x - 30.0f, bottom + 4.0f, 60.0f, 14.0f), 10.0f, juce::Justification::centred, 0.55f);
    }
    crtText(g, "ECHO TRAIN: LEFT UP, RIGHT DOWN", juce::Rectangle<float>(left, s.getY() + 6.0f, 260.0f, 14.0f), 11.5f, juce::Justification::centredLeft, 0.7f);

    // decay guides through the tops, one per side
    for (int side = 0; side < 2; ++side)
    {
        juce::Path guide;
        bool started = false;
        for (const auto& e : train)
            if (e.side == side)
            {
                const juce::Point<float> pt(xFor(e.seconds), side == 0 ? mid - height(e.db) : mid + height(e.db));
                if (!started)
                    guide.startNewSubPath(pt);
                else
                    guide.lineTo(pt);
                started = true;
            }
        if (started)
        {
            g.setColour((side == 0 ? phosphor : kRight).withAlpha(0.3f));
            const float dashes[] { 3.0f, 4.0f };
            juce::PathStrokeType(1.0f).createDashedStroke(guide, guide, dashes, 2);
            g.fillPath(guide);
        }
    }
    // the echoes
    const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
    const double pitch = values.switches[Character] == echo::Classic ? 0.0 : values.knobs[Pitch];
    for (const auto& e : train)
    {
        const float x = xFor(e.seconds), h = height(e.db);
        juce::Path spike;
        spike.startNewSubPath(x, mid);
        spike.lineTo(x, e.side == 0 ? mid - h : mid + h);
        glow(g, spike, e.side == 0 ? phosphor : kRight, 1.3f, 0.8f + 0.2f * live);
        if (std::abs(pitch) > 0.05 && e.repeat <= 4)
        {
            const double total = pitch * e.repeat;
            const float labelY = e.side == 0 ? juce::jmax(top, mid - h - 16.0f) : juce::jmin(bottom - 13.0f, mid + h + 2.0f);
            crtText(g, (total > 0 ? "+" : "") + juce::String(total, 0), juce::Rectangle<float>(x + 3.0f, labelY, 40.0f, 13.0f),
                    10.0f, juce::Justification::centredLeft, 0.75f);
        }
    }
    crtText(g, "LIVE " + (outDb > -100.0f ? juce::String(outDb, 1) + " dB" : juce::String("--")),
            juce::Rectangle<float>(right - 110.0f, s.getY() + 6.0f, 110.0f, 14.0f), 11.0f, juce::Justification::centredRight, 0.6f);
    if (values.switches[Freeze] > 0 && values.switches[Character] != echo::Classic)
        crtText(g, "FREEZE: THE LOOP HOLDS", juce::Rectangle<float>(left, bottom - 16.0f, 240.0f, 14.0f), 11.0f, juce::Justification::centredLeft, 0.85f);
}

void HamloEchoPanel::drawFilterView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    const auto p = slotParams();
    const auto& k = values.knobs;
    const float top = s.getY() + 26.0f, bottom = s.getBottom() - 26.0f;
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = xForHz(f);
        g.setColour(phosphorDim.withAlpha(0.28f));
        g.drawVerticalLine(juce::roundToInt(x), top, bottom);
        crtText(g, f >= 1000.0 ? juce::String(juce::roundToInt(f / 1000.0)) + "k" : juce::String(juce::roundToInt(f)),
                juce::Rectangle<float>(x - 20.0f, bottom + 4.0f, 40.0f, 14.0f), 11.0f, juce::Justification::centred, 0.55f);
    }
    for (double db : { 12.0, 6.0, 0.0, -6.0, -12.0, -18.0, -24.0 })
    {
        const float y = yForDb(db);
        g.setColour(phosphorDim.withAlpha(db == 0.0 ? 0.55f : 0.25f));
        g.drawHorizontalLine(juce::roundToInt(y), xForHz(20.0), xForHz(20000.0));
        crtText(g, (db > 0 ? "+" : "") + juce::String(juce::roundToInt(db)), juce::Rectangle<float>(s.getX() + 2.0f, y - 7.0f, 30.0f, 14.0f), 10.5f,
                juce::Justification::centredLeft, 0.5f);
    }
    crtText(g, "FEEDBACK FILTER  1x  2x  3x  4x", juce::Rectangle<float>(s.getX() + 34.0f, s.getY() + 6.0f, 300.0f, 14.0f), 11.5f,
            juce::Justification::centredLeft, 0.7f);

    std::array<double, 161> response {};
    for (size_t i = 0; i < response.size(); ++i)
        response[i] = echo::loopResponseDb(20.0 * std::pow(1000.0, static_cast<double>(i) / 160.0), p);
    for (int pass = 4; pass >= 1; --pass)
    {
        juce::Path curve;
        for (size_t i = 0; i < response.size(); ++i)
        {
            const float x = xForHz(20.0 * std::pow(1000.0, static_cast<double>(i) / 160.0)), y = yForDb(response[i] * pass);
            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }
        if (pass == 1)
        {
            const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
            juce::Path fill(curve);
            fill.lineTo(xForHz(20000.0), bottom);
            fill.lineTo(xForHz(20.0), bottom);
            fill.closeSubPath();
            juce::ColourGradient body(phosphor.withAlpha(0.08f + 0.25f * live), 0.0f, top, phosphor.withAlpha(0.02f), 0.0f, bottom, false);
            g.setGradientFill(body);
            g.fillPath(fill);
            glow(g, curve, phosphor, 1.6f);
        }
        else
            glow(g, curve, phosphor, 0.8f, 0.55f - 0.12f * static_cast<float>(pass));
    }

    if (values.switches[Character] == echo::Classic)
    {
        crtText(g, "CLASSIC: LOW CUT AND DAMPING ONLY", juce::Rectangle<float>(s.getX() + 34.0f, bottom - 18.0f, 300.0f, 14.0f), 11.0f,
                juce::Justification::centredLeft, 0.8f);
        return;
    }
    const auto h = handlePositions();
    const std::array<juce::String, 3> captions { "LOW " + hz(k[LowCut]) + "  R" + juce::String(juce::roundToInt(k[LowRes] * 100.0)),
                                                 "BAND " + format(BandGain, k[BandGain]) + "  " + format(BandQ, k[BandQ]),
                                                 "HIGH " + format(HighCut, k[HighCut]) + "  R" + juce::String(juce::roundToInt(k[HighRes] * 100.0)) };
    for (int i = 0; i < 3; ++i)
    {
        const bool hot = dragHandle == i;
        const auto c = h[static_cast<size_t>(i)];
        glowDot(g, c, hot ? 8.0f : 6.5f, phosphor, hot ? 1.0f : 0.85f);
        const float cx = juce::jlimit(s.getX() + 70.0f, s.getRight() - 70.0f, c.x);
        crtText(g, captions[static_cast<size_t>(i)], juce::Rectangle<float>(cx - 70.0f, c.y + (i == 1 ? 12.0f : -26.0f), 140.0f, 13.0f), 10.5f,
                juce::Justification::centred, 0.85f);
    }
}

void HamloEchoPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kScreen;
    crtBegin(g, s);
    if (phosphorImage.isValid())
    {
        g.saveState();
        g.addTransform(toScreen().inverted());
        const auto onScreen = s.transformedBy(toScreen()).getSmallestIntegerContainer();
        g.drawImageAt(phosphorImage, onScreen.getX(), onScreen.getY());
        g.restoreState();
    }
    if (mode == EchoView)
        drawEchoView(g, s);
    else
        drawFilterView(g, s);
    crtEnd(g, s);
    crtButton(g, kTabEcho, "ECHO", mode == EchoView);
    crtButton(g, kTabFilter, "FILTER", mode == FilterView);
}

void HamloEchoPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    drawCrt(g);
    drawTimeMode(g);
    drawSelector(g);
    drawLevers(g);
    meterNeedle(g, meter->meter);
    drawLcd(g);
    for (const auto& k : knobs)
        drawKnob(g, k);
}

void HamloEchoPanel::timerCallback()
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

    // phosphor persistence: fade the old image, burn in a play head running at real time
    if (phosphorImage.isValid())
    {
        juce::Image::BitmapData data(phosphorImage, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < data.height; ++y)
            for (int x = 0; x < data.width; ++x)
            {
                auto c = data.getPixelColour(x, y);
                if (c.getAlpha() != 0)
                    data.setPixelColour(x, y, c.withMultipliedAlpha(0.82f));
            }
        juce::Graphics pg(phosphorImage);
        const auto onScreen = kScreen.transformedBy(toScreen()).getSmallestIntegerContainer().toFloat();
        pg.addTransform(toScreen().translated(-onScreen.getX(), -onScreen.getY())); // design -> image pixels
        const float live = juce::jlimit(0.0f, 1.0f, (outDb + 60.0f) / 54.0f);
        if (live > 0.02f)
        {
            juce::Point<float> dot;
            if (mode == EchoView)
            {
                double span = 1.0;
                echoTrain(span);
                playhead = std::fmod(playhead + 1.0 / 30.0, span);
                const float left = kScreen.getX() + 34.0f, right = kScreen.getRight() - 12.0f;
                dot = { left + (right - left) * static_cast<float>(playhead / span), kScreen.getCentreY() };
            }
            else
            {
                playhead = std::fmod(playhead + 1.0 / 60.0, 1.0);
                const double f = 20.0 * std::pow(1000.0, playhead);
                dot = { xForHz(f), yForDb(echo::loopResponseDb(f, slotParams())) };
            }
            glowDot(pg, dot, 3.0f + 3.0f * live, phosphor, 0.5f + 0.5f * live);
        }
    }
    repaint();
}

void HamloEchoPanel::settleMeters(float outputDb, float duck)
{
    meter->meter.shown = juce::jlimit(-42.0f, 1.0f, outputDb);
    outDb = outputDb;
    duckDb = duck;
    repaint();
}

// ---------------------------------------------------------------------------------------------------
// mouse

int HamloEchoPanel::knobAt(juce::Point<float> d) const
{
    for (const auto& k : knobs)
        if (d.getDistanceFrom(k.centre) <= 62.0f * k.scale)
            return k.param;
    return -1;
}

int HamloEchoPanel::characterLabelAt(juce::Point<float> d) const
{
    for (int i = 0; i < echo::NumCharacters; ++i)
        if (characterLabelArea(i).expanded(0.0f, 1.0f).contains(d))
            return i;
    return -1;
}

HamloEchoPanel::Handle HamloEchoPanel::handleAt(juce::Point<float> d) const
{
    if (mode != FilterView || !kScreen.contains(d) || values.switches[Character] == echo::Classic)
        return NoHandle;
    const auto h = handlePositions();
    Handle best = NoHandle;
    float bestDistance = 16.0f;
    for (int i = 0; i < 3; ++i)
    {
        const float dist = d.getDistanceFrom(h[static_cast<size_t>(i)]);
        if (dist < bestDistance)
        {
            bestDistance = dist;
            best = static_cast<Handle>(i);
        }
    }
    return best;
}

void HamloEchoPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        auto p = static_cast<Param>(k);
        if (p == TimeR && values.switches[Link] > 0)
            return; // linked: the right time follows the left one
        dragKnob = k;
        dragStartNorm = normFor(p);
        dragStartY = e.position.y;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        return;
    }
    if (d.getDistanceFrom(kSelector) <= 60.0f)
    {
        dragSelector = true;
        dragSelectorStart = values.switches[Character];
        dragStartY = e.position.y;
        return;
    }
    if (const int c = characterLabelAt(d); c >= 0)
    {
        setSwitch(Character, c);
        return;
    }
    if (const auto h = handleAt(d); h != NoHandle)
    {
        dragHandle = h;
        if (onKnobGestureStart)
            onKnobGestureStart(h == LowHandle ? LowCut : h == BandHandle ? BandFreq : HighCut);
        return;
    }
    if (kTabEcho.contains(d))
    {
        setDisplayMode(EchoView);
        return;
    }
    if (kTabFilter.contains(d))
    {
        setDisplayMode(FilterView);
        return;
    }
    const float dialTop = kModeTop + 22.0f;
    if (d.x >= kModeX && d.x <= kModeX + kModeW && d.y >= dialTop && d.y <= dialTop + 74.0f)
    {
        int option = -1;
        if (d.y <= dialTop + 20.0f)
            for (int i = 0; i < 2; ++i)
                if (std::abs(d.x - modeOptionX(i)) <= 40.0f)
                    option = i;
        setSwitch(TimeMode, option >= 0 ? option : 1 - values.switches[TimeMode]);
        return;
    }
    if (juce::Rectangle<float>(kPingLever.x - 26.0f, kPingLever.y - 44.0f, 52.0f, 70.0f).contains(d))
    {
        setSwitch(PingPong, values.switches[PingPong] > 0 ? 0 : 1);
        return;
    }
    if (juce::Rectangle<float>(kFreezeLever.x - 26.0f, kFreezeLever.y - 44.0f, 52.0f, 70.0f).contains(d) || d.getDistanceFrom(kFreezeLamp) <= 12.0f)
    {
        setSwitch(Freeze, values.switches[Freeze] > 0 ? 0 : 1);
        return;
    }
    if (d.getDistanceFrom(kLamp) <= 24.0f)
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    const bool classic = values.switches[Character] == echo::Classic;
    const juce::Point<float> link((knobs[TimeL].centre.x + knobs[TimeR].centre.x) * 0.5f, kLargeY);
    if (!classic && juce::Rectangle<float>(link.x - 28.0f, link.y - 26.0f, 56.0f, 56.0f).contains(d))
    {
        setSwitch(Link, values.switches[Link] > 0 ? 0 : 1);
        return;
    }
    for (const auto param : { TimeL, TimeR })
    {
        if (param == TimeR && (classic || values.switches[Link] > 0))
            continue;
        const auto which = param == TimeL ? SyncL : SyncR;
        for (int i = 0; i < 2; ++i)
            if (syncTag(knobs[param].centre, i).expanded(0.0f, 1.0f).contains(d))
            {
                setSwitch(which, i == 0 ? 0 : (values.switches[which] > 0 ? values.switches[which] : 8)); // 1/4 to start
                return;
            }
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

void HamloEchoPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob >= 0)
    {
        const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
        setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
        return;
    }
    if (dragSelector)
    {
        const int steps = juce::roundToInt((dragStartY - e.position.y) / (28.0f * scale));
        const int c = juce::jlimit(0, echo::NumCharacters - 1, dragSelectorStart + steps);
        if (c != values.switches[Character])
            setSwitch(Character, c);
        return;
    }
    if (dragHandle != NoHandle)
    {
        const auto d = toDesign(e.position);
        const double db = dbForY(d.y);
        if (dragHandle == BandHandle)
        {
            setKnob(BandFreq, hzForX(d.x), true);
            setKnob(BandGain, db, true);
        }
        else
        {
            const auto freq = dragHandle == LowHandle ? LowCut : HighCut;
            setKnob(freq, hzForX(d.x), true);
            setKnob(dragHandle == LowHandle ? LowRes : HighRes, (db + 3.0) / 15.0, true);
        }
    }
}

void HamloEchoPanel::mouseUp(const juce::MouseEvent&)
{
    if ((dragKnob >= 0 || dragHandle != NoHandle) && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
    dragHandle = NoHandle;
    dragSelector = false;
}

void HamloEchoPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        const auto p = static_cast<Param>(k);
        if (synced(p) || (p == TimeR && values.switches[Link] > 0))
            return;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setKnob(p, defaultFor(p), true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloEchoPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    const double direction = wheel.deltaY > 0 ? 1.0 : -1.0;
    if (const int k = knobAt(d); k >= 0)
    {
        const auto p = static_cast<Param>(k);
        if (p == TimeR && values.switches[Link] > 0)
            return;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setNorm(p, normFor(p) + (e.mods.isShiftDown() ? 0.004 : 0.02) * direction, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        return;
    }
    if (handleAt(d) == BandHandle) // the wheel on the band handle sets its Q
    {
        if (onKnobGestureStart)
            onKnobGestureStart(BandQ);
        setNorm(BandQ, normFor(BandQ) + 0.03 * direction, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        return;
    }
    if (d.getDistanceFrom(kSelector) <= 60.0f)
        setSwitch(Character, juce::jlimit(0, echo::NumCharacters - 1, values.switches[Character] + (wheel.deltaY > 0 ? 1 : -1)));
}

void HamloEchoPanel::editReadout(Param p)
{
    if (synced(p) || (p == TimeR && values.switches[Link] > 0))
        return;
    const auto& k = knobs[static_cast<size_t>(p)];
    const bool large = p < kLarge;
    const auto area = juce::Rectangle<float>(k.centre.x - (large ? 70.0f : 46.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 13.0f,
                                             large ? 140.0f : 92.0f, 26.0f)
                          .transformedBy(toScreen())
                          .toNearestInt();
    const bool percent = p == Feedback || p == Cross || p == Width || p == Mix || p == ModDepth || p == Diffusion || p == Lofi || p == Duck;
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(stencilFont((large ? 20.0f : 15.0f) * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const double v = values.knobs[static_cast<size_t>(p)];
    typing->setText(percent ? juce::String(juce::roundToInt(v * 100.0)) : juce::String(v, std::abs(v) < 10.0 ? 2 : 0), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloEchoPanel> self(this);
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
        if ((p == TimeL || p == TimeR) && text.containsChar('s') && !text.contains("ms"))
            entered *= 1000.0; // "1.5 s"
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
