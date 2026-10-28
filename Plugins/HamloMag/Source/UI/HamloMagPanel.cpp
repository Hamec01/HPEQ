#include "UI/HamloMagPanel.h"

#include <cmath>

#include "DSP/HamloMag.h"
#include "UI/BlackSteel.h"

namespace hamlo::ui
{
namespace
{
using namespace steel;
namespace mag = dsp::mag;

constexpr float kW = 1180.0f, kH = 820.0f; // design stage
constexpr float kPi = juce::MathConstants<float>::pi;

enum class Map { Linear, Log };
struct MapInfo
{
    double min, max;
    Map kind;
};
constexpr std::array<MapInfo, HamloMagPanel::NumParams> kMaps { {
    { 0.0, 48.0, Map::Linear },     // Drive (dB)
    { -1.0, 1.0, Map::Linear },     // Tone
    { 10.0, 1000.0, Map::Log },     // LowCut (Hz)
    { 1000.0, 20000.0, Map::Log },  // HighCut (Hz)
    { 0.0, 1.0, Map::Linear },      // Mix
    { -24.0, 24.0, Map::Linear },   // Output (dB)
    { 0.0, 1.0, Map::Linear },      // Harmonics
    { -1.0, 1.0, Map::Linear },     // EvenOdd
    { 0.0, 1.0, Map::Linear },      // Bias
    { 0.0, 1.0, Map::Linear },      // Wow
    { 0.0, 1.0, Map::Linear },      // Hiss
    { 0.0, 1.0, Map::Linear },      // Hum
    { 0.0, 1.0, Map::Linear },      // Crackle
} };
constexpr std::array<const char*, HamloMagPanel::NumParams> kLabels { "Drive", "Tone", "Low cut", "High cut", "Mix", "Output", "Harmonics",
                                                                      "Even / odd", "Bias", "Wow", "Hiss", "Hum 50", "Crackle" };

// Layout (design coordinates)
const juce::Rectangle<float> kPlate { 405.0f, 36.0f, 370.0f, 92.0f };
const juce::Rectangle<float> kLcd { 495.0f, 138.0f, 190.0f, 50.0f };
const juce::Point<float> kReelL { 160.0f, 152.0f }, kReelR { 1020.0f, 152.0f };
constexpr float kReelRadius = 96.0f;
const juce::Point<float> kRollerL { 300.0f, 236.0f }, kRollerR { 880.0f, 236.0f };
const juce::Rectangle<float> kHeads { 520.0f, 200.0f, 140.0f, 46.0f };
const juce::Point<float> kLamp { 718.0f, 163.0f };
// HamloMag (2026-10-28): SPEED moved right, under OVERSAMPLE - the top left is the preset screen on every Hamlo page
constexpr float kSpeedX = 800.0f, kOsX = 845.0f, kLeverTop = 52.0f, kSpeedTop = 146.0f;
float leverTop(float centre) { return centre == kSpeedX ? kSpeedTop : kLeverTop; }
const juce::Point<float> kSelector { 174.0f, 386.0f };
constexpr float kSelectorLabelRadius = 104.0f;
const juce::Rectangle<float> kBezel { 350.0f, 262.0f, 512.0f, 240.0f };
const juce::Rectangle<float> kScreen { 368.0f, 276.0f, 476.0f, 190.0f };
const juce::Rectangle<float> kTabCurve { 520.0f, 474.0f, 84.0f, 20.0f }, kTabHarm { 612.0f, 474.0f, 96.0f, 20.0f };
const juce::Rectangle<float> kMeterBezel { 884.0f, 262.0f, 240.0f, 160.0f };
constexpr std::array<float, 4> kToggleX { 914.0f, 974.0f, 1034.0f, 1094.0f };
constexpr float kToggleY = 474.0f;
constexpr float kLargeY = 598.0f, kLargeLabelY = 532.0f, kLargeReadoutY = 666.0f;
constexpr float kSmallY = 748.0f, kSmallLabelY = 702.0f, kSmallReadoutY = 790.0f;
const juce::Colour kOdd = phosphor.withRotatedHue(0.08f);

float modelAngle(int i) { return (-125.0f + 50.0f * static_cast<float>(i)) * kPi / 180.0f; }

juce::Rectangle<float> modelLabelArea(int i)
{
    const float a = modelAngle(i);
    const juce::Point<float> c(kSelector.x + std::sin(a) * kSelectorLabelRadius, kSelector.y - std::cos(a) * kSelectorLabelRadius);
    if (std::sin(a) > 0.5f)
        return { c.x - 6.0f, c.y - 10.0f, 110.0f, 20.0f };
    if (std::sin(a) < -0.5f)
        return { c.x - 104.0f, c.y - 10.0f, 110.0f, 20.0f };
    return { c.x - 46.0f, c.y - 10.0f, 92.0f, 20.0f };
}

float optionX(float centre, int i) { return centre + 42.0f * static_cast<float>(i - 1); }

juce::String hz(double v)
{
    return v >= 1000.0 ? juce::String(v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz";
}

juce::Point<float> toggleCentre(int i) { return { kToggleX[static_cast<size_t>(i)], kToggleY }; }
const juce::Point<float> kNoiseToggle { 1124.0f - 44.0f, kSmallY };
} // namespace

struct HamloMagPanel::MeterState
{
    Meter meter;
};

HamloMagPanel::HamloMagPanel() : meter(std::make_unique<MeterState>())
{
    for (int i = 0; i < NumParams; ++i)
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
            // seven small knobs and the NOISE toggle share the row
            k.centre = { 56.0f + (1068.0f / 8.0f) * (static_cast<float>(i - kLarge) + 0.5f), kSmallY };
            k.scale = 0.55f;
        }
    }
    auto& m = meter->meter;
    m.bezel = kMeterBezel;
    m.min = -40.0f;
    m.max = 6.0f;
    m.redFrom = 0.0f;
    m.title = "OUTPUT";
    m.titleSize = 22.0f;
    for (float v : { -40.0f, -30.0f, -20.0f, -15.0f, -10.0f, -6.0f, -3.0f, 0.0f, 3.0f, 6.0f })
        m.ticks.push_back({ v, v == -40.0f || v == -20.0f || v == -10.0f || v == 0.0f || v == 6.0f });
    m.seed = 53;
    m.shown = -42.0f;
    setOpaque(false);
    startTimerHz(30);
}

HamloMagPanel::~HamloMagPanel()
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

const char* HamloMagPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "driveDb", "tone", "satLowCutHz", "satHighCutHz", "mix", "trimDb", "harmonics", "evenOdd",
                                          "tapeBias", "wow", "hiss", "hum", "crackle" };
    return keys[p];
}

const char* HamloMagPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "model", "speed", "oversampling", "punish", "thump", "steep", "autoGain", "noiseFollow" };
    return keys[s];
}

double HamloMagPanel::defaultFor(Param p)
{
    return Values {}.knobs[static_cast<size_t>(p)];
}

bool HamloMagPanel::usedByClassic(Param p) noexcept
{
    return p == Drive || p == Mix || p == Output;
}

bool HamloMagPanel::isActive(Param p) const noexcept
{
    const int m = values.switches[Model];
    if (m == mag::Classic)
        return usedByClassic(p);
    if (p == Bias)
        return m == mag::Tape;
    if (p == Wow)
        return mag::hasWow(m);
    return true;
}

juce::String HamloMagPanel::format(Param p, double v)
{
    switch (p)
    {
        case Drive: case Output: return (v > 0.05 ? "+" : "") + juce::String(v, 1) + " dB";
        case Tone: case EvenOdd:
            return std::abs(v) < 0.005 ? juce::String("0") : (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100.0));
        case LowCut: return hz(v);
        case HighCut: return v >= 19900.0 ? juce::String("OFF") : hz(v);
        case Mix: case Harmonics: case Bias: case Wow: case Hiss: case Hum: case Crackle:
            return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case NumParams: break;
    }
    return {};
}

dsp::FxSlotParams HamloMagPanel::slotParams() const
{
    namespace s = dsp::slot;
    dsp::FxSlotParams p;
    p.active = values.on;
    p.type = dsp::FxType::Saturator;
    p.mix = static_cast<float>(values.knobs[Mix]);
    const auto& k = values.knobs;
    const auto& sw = values.switches;
    p.values[s::kDriveDb] = static_cast<float>(k[Drive]);
    p.values[s::kTrimDb] = static_cast<float>(k[Output]);
    p.values[s::kOversampling] = static_cast<float>(sw[Oversampling]);
    p.values[s::kAutoGain] = static_cast<float>(sw[Auto]);
    p.values[s::kSatModel] = static_cast<float>(sw[Model]);
    p.values[s::kSatPunish] = static_cast<float>(sw[Punish]);
    p.values[s::kSatTone] = static_cast<float>(k[Tone]);
    p.values[s::kSatLowCutHz] = static_cast<float>(k[LowCut]);
    p.values[s::kSatHighCutHz] = static_cast<float>(k[HighCut]);
    p.values[s::kSatThump] = static_cast<float>(sw[Thump]);
    p.values[s::kSatSteep] = static_cast<float>(sw[Steep]);
    p.values[s::kSatHarmonics] = static_cast<float>(k[Harmonics]);
    p.values[s::kSatEvenOdd] = static_cast<float>(k[EvenOdd]);
    p.values[s::kSatTapeBias] = static_cast<float>(k[Bias]);
    p.values[s::kSatSpeed] = static_cast<float>(sw[Speed]);
    p.values[s::kSatWow] = static_cast<float>(k[Wow]);
    p.values[s::kSatHiss] = static_cast<float>(k[Hiss]);
    p.values[s::kSatHum] = static_cast<float>(k[Hum]);
    p.values[s::kSatCrackle] = static_cast<float>(k[Crackle]);
    p.values[s::kSatNoiseFollow] = static_cast<float>(sw[NoiseFollow]);
    return p;
}

double HamloMagPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.kind == Map::Log ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloMagPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    return m.kind == Map::Log ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
}

void HamloMagPanel::setKnob(Param p, double v, bool notify)
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
    if (notify && onKnobChange)
        onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
    repaint();
}

void HamloMagPanel::setNorm(Param p, double n, bool notify)
{
    setKnob(p, valueFor(p, n), notify);
}

void HamloMagPanel::setSwitch(Switch s, int v)
{
    values.switches[static_cast<size_t>(s)] = v;
    if (onSwitchChange)
        onSwitchChange(s, v);
    repaint();
}

void HamloMagPanel::setValues(const Values& v)
{
    if (dragKnob >= 0 || dragSelector)
        return; // the drag owns the value until the gesture ends
    values = v;
    repaint();
}

void HamloMagPanel::setDisplayMode(DisplayMode m)
{
    mode = m;
    repaint();
}

juce::AffineTransform HamloMagPanel::toScreen() const
{
    return juce::AffineTransform::scale(scale).translated(offset);
}

juce::Point<float> HamloMagPanel::toDesign(juce::Point<float> p) const
{
    return (p - offset) / scale;
}

void HamloMagPanel::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    if (getWidth() > 0 && getHeight() > 0)
    {
        staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(staticLayer);
        g.addTransform(toScreen());
        renderStatic(g);
    }
    typing.reset();
}

// ---------------------------------------------------------------------------------------------------
// static plate

void HamloMagPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 2027);
    steel::scratches(g, { { 6, { 372.0f, 138.0f, 68.0f, 94.0f }, -4.0f }, { 13, { 762.0f, 138.0f, 68.0f, 94.0f }, 3.0f },
                          { 20, { 16.0f, 244.0f, 70.0f, 98.0f }, -6.0f } });
    nameplate(g, kPlate, "HamloMag", juce::String::fromUTF8("MODEL \xd0\x9c\xd0\x90\xd0\x93-4"), juce::String::fromUTF8("\xe2\x84\x96 0422"));
    lcdWindow(g, kLcd);

    // reel wells: a dark recess under each reel and the spindle plates
    for (auto c : { kReelL, kReelR })
    {
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(c.x - kReelRadius - 6.0f, c.y - kReelRadius + 4.0f, 2.0f * kReelRadius + 12.0f, 2.0f * kReelRadius + 12.0f);
        g.setColour(juce::Colour(0xff0b0b0a));
        g.fillEllipse(c.x - kReelRadius - 4.0f, c.y - kReelRadius - 4.0f, 2.0f * kReelRadius + 8.0f, 2.0f * kReelRadius + 8.0f);
    }
    // head block: brushed steel cover with the erase / record / play heads
    {
        const auto b = kHeads;
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRoundedRectangle(b.translated(0, 6).expanded(2), 6.0f);
        juce::ColourGradient cover(juce::Colour(0xffb9b9b2), b.getX(), b.getY(), juce::Colour(0xff5d5d58), b.getX(), b.getBottom(), false);
        cover.addColour(0.45, juce::Colour(0xff8e8e88));
        g.setGradientFill(cover);
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        for (float x = b.getX(); x < b.getRight(); x += 2.0f)
            g.drawVerticalLine(juce::roundToInt(x), b.getY() + 1, b.getBottom() - 1);
        for (int i = 0; i < 3; ++i)
        {
            const float x = b.getX() + 30.0f + 40.0f * static_cast<float>(i);
            juce::ColourGradient head(juce::Colour(0xff2b2b28), x - 8, b.getBottom() - 16, juce::Colour(0xff0b0b0a), x + 8, b.getBottom(), false);
            g.setGradientFill(head);
            g.fillRoundedRectangle(x - 9.0f, b.getBottom() - 16.0f, 18.0f, 14.0f, 3.0f);
            g.setColour(juce::Colour(0xffc9b26a));
            g.fillRect(x - 0.6f, b.getBottom() - 15.0f, 1.2f, 12.0f);
        }
        g.setFont(stencilFont(10.0f).withKerningFactor(0.2f));
        g.setColour(juce::Colour(0xff2a2926));
        g.drawText(juce::String::fromUTF8("\xd0\xa1\xd0\xa2  \xd0\x97\xd0\x9f  \xd0\x92\xd0\x9e\xd0\xa1\xd0\x9f"), b.withHeight(18.0f).translated(0, 3.0f),
                   juce::Justification::centred); // СТ  ЗП  ВОСП (erase, record, play)
    }
    // guide rollers
    for (auto c : { kRollerL, kRollerR })
    {
        hexagon(g, c, 26.0f, 26.0f, juce::Colour(0xff8e8e88), juce::Colour(0xff1c1c1a));
        juce::ColourGradient roller(juce::Colour(0xffeeeee6), c.x - 3, c.y - 4, juce::Colour(0xff4d4d48), c.x + 6, c.y + 6, true);
        g.setGradientFill(roller);
        g.fillEllipse(c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    }

    // lever labels
    engraved(g, "SPEED", juce::Rectangle<float>(kSpeedX - 60.0f, kSpeedTop - 14.0f, 120.0f, 20.0f), 14.0f);
    engraved(g, "OVERSAMPLE", juce::Rectangle<float>(kOsX - 60.0f, kLeverTop - 14.0f, 120.0f, 20.0f), 14.0f);

    // MODEL selector
    engraved(g, "MODEL", juce::Rectangle<float>(kSelector.x - 90.0f, 262.0f, 180.0f, 24.0f), 18.0f);
    for (int i = 0; i < mag::NumModels; ++i)
    {
        const float a = modelAngle(i);
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

    crtBezel(g, kBezel, kScreen, juce::String::fromUTF8("\xd0\xad\xd0\x9a\xd0\xa0\xd0\x90\xd0\x9d \xc2\xb7 \xd0\xa1\xd0\x9a\xd0\x9e"), 476.0f); // ЭКРАН · СКО
    meterFace(g, meter->meter);
    const std::array<const char*, 4> toggleLabels { "PUNISH", "THUMP", "STEEP", "AUTO" };
    for (int i = 0; i < 4; ++i)
        engraved(g, toggleLabels[static_cast<size_t>(i)], juce::Rectangle<float>(kToggleX[static_cast<size_t>(i)] - 34.0f, 432.0f, 68.0f, 18.0f), 12.5f);
    engraved(g, "ON", juce::Rectangle<float>(kLamp.x - 24.0f, kLamp.y + 18.0f, 48.0f, 16.0f), 12.0f);

    for (const auto& k : knobs)
    {
        const bool large = k.param < kLarge;
        const float labelY = large ? kLargeLabelY : kSmallLabelY;
        const float w = large ? 170.0f : 120.0f;
        engraved(g, juce::String(kLabels[static_cast<size_t>(k.param)]).toUpperCase(),
                 juce::Rectangle<float>(k.centre.x - w * 0.5f, labelY - 13.0f, w, 26.0f), large ? 20.0f : 14.0f, stencil, juce::Justification::centred, 0.1f);
        knobScale(g, k.centre, k.scale);
    }
    engraved(g, "NOISE", juce::Rectangle<float>(kNoiseToggle.x - 60.0f, kSmallLabelY - 13.0f, 120.0f, 26.0f), 14.0f);
    g.setColour(juce::Colours::black);
    g.drawHorizontalLine(684, 56.0f, kW - 56.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(683, 56.0f, kW - 56.0f);
}

// ---------------------------------------------------------------------------------------------------
// dynamic parts

void HamloMagPanel::drawReel(juce::Graphics& g, juce::Point<float> c, float pack, float angle) const
{
    const float r = kReelRadius;
    // the tape pack under the flange, seen through the windows
    juce::ColourGradient tape(juce::Colour(0xff4a3020), c.x, c.y, juce::Colour(0xff26170e), c.x + pack, c.y, true);
    g.setGradientFill(tape);
    g.fillEllipse(c.x - pack, c.y - pack, 2 * pack, 2 * pack);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    for (float rr = 26.0f; rr < pack; rr += 3.0f)
        g.drawEllipse(c.x - rr, c.y - rr, 2 * rr, 2 * rr, 0.6f);

    // the aluminium flange with three kidney windows, turning
    juce::Path flange;
    flange.addEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = angle + static_cast<float>(i) * 2.0f * kPi / 3.0f;
        juce::Path window;
        window.addCentredArc(c.x, c.y, r * 0.78f, r * 0.78f, 0.0f, a0 - 0.62f, a0 + 0.62f, true);
        window.addCentredArc(c.x, c.y, r * 0.34f, r * 0.34f, 0.0f, a0 + 0.95f, a0 - 0.95f, false);
        window.closeSubPath();
        flange.addPath(window);
    }
    flange.setUsingNonZeroWinding(false);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillPath(flange, juce::AffineTransform::translation(0.0f, 4.0f));
    juce::ColourGradient metal(juce::Colour(0xffe2e3df), c.x - r * 0.6f, c.y - r * 0.7f, juce::Colour(0xff6f716c), c.x + r * 0.7f, c.y + r * 0.8f, false);
    metal.addColour(0.45, juce::Colour(0xffaeb0aa));
    g.setGradientFill(metal);
    g.fillPath(flange);
    // concentric machining rings
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    for (float rr = r * 0.82f; rr < r; rr += 2.0f)
        g.drawEllipse(c.x - rr, c.y - rr, 2 * rr, 2 * rr, 0.5f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawEllipse(c.x - r, c.y - r, 2 * r, 2 * r, 1.5f);
    // hub with three driving spokes
    juce::ColourGradient hub(juce::Colour(0xfff5f5f0), c.x - 6, c.y - 8, juce::Colour(0xff3c3c38), c.x + 14, c.y + 14, true);
    g.setGradientFill(hub);
    g.fillEllipse(c.x - 22.0f, c.y - 22.0f, 44.0f, 44.0f);
    g.setColour(juce::Colour(0xff151514));
    g.fillEllipse(c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f);
    for (int i = 0; i < 3; ++i)
    {
        const float a = angle + 1.05f + static_cast<float>(i) * 2.0f * kPi / 3.0f;
        juce::Path spoke;
        spoke.addRoundedRectangle(-2.5f, -21.0f, 5.0f, 12.0f, 1.5f);
        g.setColour(juce::Colour(0xff1d1d1b));
        g.fillPath(spoke, juce::AffineTransform::rotation(a).translated(c));
    }
    // glare
    juce::ColourGradient glare(juce::Colours::white.withAlpha(0.22f), c.x - r * 0.5f, c.y - r * 0.6f, juce::Colours::transparentWhite, c.x, c.y, true);
    g.setGradientFill(glare);
    g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
}

void HamloMagPanel::drawTape(juce::Graphics& g) const
{
    // the tape leaves each pack tangentially at the bottom, runs over the rollers and the heads
    const float packL = 82.0f, packR = 52.0f;
    juce::Path tape;
    tape.startNewSubPath(kReelL.x + 10.0f, kReelL.y + packL);
    tape.lineTo(kRollerL.x, kRollerL.y + 8.0f);
    tape.lineTo(kHeads.getX() - 6.0f, kHeads.getBottom() - 4.0f);
    tape.lineTo(kHeads.getRight() + 6.0f, kHeads.getBottom() - 4.0f);
    tape.lineTo(kRollerR.x, kRollerR.y + 8.0f);
    tape.lineTo(kReelR.x - 10.0f, kReelR.y + packR);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.strokePath(tape, juce::PathStrokeType(5.0f), juce::AffineTransform::translation(0.0f, 3.0f));
    g.setColour(juce::Colour(0xff4b2f1d));
    g.strokePath(tape, juce::PathStrokeType(4.0f));
    g.setColour(juce::Colour(0xff8a6040).withAlpha(0.5f));
    g.strokePath(tape, juce::PathStrokeType(1.0f), juce::AffineTransform::translation(0.0f, -1.0f));
}

void HamloMagPanel::drawKnob(juce::Graphics& g, const Knob& k) const
{
    const bool active = isActive(k.param);
    const float angle = (-135.0f + static_cast<float>(normFor(k.param)) * 270.0f) * kPi / 180.0f;
    brassKnob(g, k.centre, angle, k.scale, active ? 1.0f : 0.45f, KnobAsset::ChippedIronDark, PointerAsset::WhiteEnamel);
    const bool large = k.param < kLarge;
    const auto r = juce::Rectangle<float>(k.centre.x - (large ? 80.0f : 60.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 12.0f,
                                          large ? 160.0f : 120.0f, 24.0f);
    const auto text = format(k.param, values.knobs[static_cast<size_t>(k.param)]);
    g.setFont(stencilFont(large ? 22.0f : 16.0f));
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0, 1), juce::Justification::centred);
    g.setColour(active ? stencil : stencilDim.withAlpha(0.6f));
    g.drawText(text, r, juce::Justification::centred);
}

void HamloMagPanel::drawSelector(juce::Graphics& g) const
{
    const int selected = juce::jlimit(0, mag::NumModels - 1, values.switches[Model]);
    for (int i = 0; i < mag::NumModels; ++i)
    {
        const float a = modelAngle(i);
        const auto r = modelLabelArea(i);
        const auto just = std::sin(a) > 0.5f ? juce::Justification::centredLeft
                          : std::sin(a) < -0.5f ? juce::Justification::centredRight : juce::Justification::centred;
        const auto text = juce::String(mag::modelName(i)).toUpperCase();
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
    const float a = modelAngle(selected);
    brassKnob(g, kSelector, a, 1.15f, 1.0f, KnobAsset::ChippedIronDark, PointerAsset::WhiteEnamel);
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

void HamloMagPanel::drawLevers(juce::Graphics& g) const
{
    const int model = values.switches[Model];
    const auto three = [&](float centre, int selected, const std::array<juce::String, 3>& names, bool usable)
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto r = juce::Rectangle<float>(optionX(centre, i) - 22.0f, leverTop(centre) + 4.0f, 44.0f, 16.0f);
            g.setFont(stencilFont(12.5f).withKerningFactor(0.05f));
            if (i == selected && usable)
            {
                g.setColour(juce::Colour(0x55ffe9a0));
                g.drawText(names[static_cast<size_t>(i)], r.expanded(1.0f), juce::Justification::centred);
                g.setColour(juce::Colour(0xfffff6d0));
            }
            else
                g.setColour(stencilDim.withAlpha(usable ? 1.0f : 0.4f));
            g.drawText(names[static_cast<size_t>(i)], r, juce::Justification::centred);
        }
        lever(g, { centre, leverTop(centre) + 64.0f }, (-36.0f + 36.0f * static_cast<float>(selected)) * kPi / 180.0f, 0.8f);
    };
    const bool vinyl = model == mag::Vinyl;
    three(kSpeedX, values.switches[Speed], { mag::speedName(model, 0), mag::speedName(model, 1), mag::speedName(model, 2) }, mag::hasWow(model));
    g.setFont(stencilFont(10.5f).withKerningFactor(0.15f));
    g.setColour(stencilDim);
    g.drawText(vinyl ? "RPM" : "CM/S", juce::Rectangle<float>(kSpeedX + 40.0f, kSpeedTop - 12.0f, 50.0f, 14.0f), juce::Justification::centredLeft);
    three(kOsX, values.switches[Oversampling], { "1x", "2x", "4x" }, true);
    lamp(g, kLamp, values.on, 0.42f);
}

void HamloMagPanel::drawToggles(juce::Graphics& g) const
{
    const bool classic = values.switches[Model] == mag::Classic;
    const std::array<Switch, 4> which { Punish, Thump, Steep, Auto };
    for (int i = 0; i < 4; ++i)
    {
        const bool on = values.switches[which[static_cast<size_t>(i)]] > 0;
        const auto c = toggleCentre(i);
        lever(g, c, on ? 0.0f : kPi, 0.62f, on ? LeverPosition::Up : LeverPosition::Down);
        const bool usable = !classic || which[static_cast<size_t>(i)] == Auto;
        if (on && usable)
        {
            g.setColour(juce::Colour(0xffff4a3a));
            g.fillEllipse(c.x + 16.0f, c.y - 22.0f, 5.0f, 5.0f);
        }
    }
    // NOISE: ALWAYS / FOLLOW
    const bool follow = values.switches[NoiseFollow] > 0;
    lever(g, kNoiseToggle, follow ? 0.0f : kPi, 0.62f, follow ? LeverPosition::Up : LeverPosition::Down);
    g.setFont(stencilFont(11.0f).withKerningFactor(0.1f));
    g.setColour(follow ? juce::Colour(0xfffff6d0) : stencilDim);
    g.drawText("FOLLOW", juce::Rectangle<float>(kNoiseToggle.x + 14.0f, kNoiseToggle.y - 26.0f, 60.0f, 14.0f), juce::Justification::centredLeft);
    g.setColour(!follow ? juce::Colour(0xfffff6d0) : stencilDim);
    g.drawText("ALWAYS", juce::Rectangle<float>(kNoiseToggle.x + 14.0f, kNoiseToggle.y + 12.0f, 60.0f, 14.0f), juce::Justification::centredLeft);
}

void HamloMagPanel::drawLcd(juce::Graphics& g) const
{
    const int model = values.switches[Model];
    juce::String line1 = juce::String(mag::modelName(model)).toUpperCase();
    if (mag::hasWow(model))
        line1 << " " << mag::speedName(model, values.switches[Speed]) << (model == mag::Vinyl ? "" : "");
    juce::String line2 = "DRV " + format(Drive, values.knobs[Drive] + (values.switches[Punish] > 0 && model != mag::Classic ? 20.0 : 0.0));
    if (outDb > -100.0f)
        line2 = "IN" + juce::String(juce::roundToInt(inDb)) + " OUT" + juce::String(juce::roundToInt(outDb));
    lcdText(g, kLcd.withHeight(kLcd.getHeight() * 0.5f).translated(0, 3.0f), line1, 18.0f);
    lcdText(g, kLcd.withTrimmedTop(kLcd.getHeight() * 0.5f).translated(0, -3.0f), line2, 17.0f);
}

// ---------------------------------------------------------------------------------------------------
// CRT

void HamloMagPanel::drawCurveView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    const auto area = s.reduced(30.0f, 22.0f).translated(0.0f, 4.0f);
    const float size = std::min(area.getWidth(), area.getHeight());
    const auto box = juce::Rectangle<float>(area.getCentreX() - size * 0.5f * 1.5f, area.getY(), size * 1.5f, size);
    const auto xFor = [&](double v) { return box.getCentreX() + static_cast<float>(v) * box.getWidth() * 0.5f; };
    const auto yFor = [&](double v) { return box.getCentreY() - static_cast<float>(juce::jlimit(-1.2, 1.2, v)) * box.getHeight() * 0.5f; };
    for (int i = -4; i <= 4; ++i)
    {
        g.setColour(phosphorDim.withAlpha(i == 0 ? 0.55f : 0.22f));
        g.drawVerticalLine(juce::roundToInt(xFor(i / 4.0)), box.getY(), box.getBottom());
        g.drawHorizontalLine(juce::roundToInt(yFor(i / 4.0)), box.getX(), box.getRight());
    }
    juce::Path unity;
    unity.startNewSubPath(xFor(-1.0), yFor(-1.0));
    unity.lineTo(xFor(1.0), yFor(1.0));
    g.setColour(phosphorDim.withAlpha(0.6f));
    const float dashes[] { 3.0f, 4.0f };
    juce::PathStrokeType(1.0f).createDashedStroke(unity, unity, dashes, 2);
    g.fillPath(unity);

    const auto p = slotParams();
    if (values.switches[Model] == mag::Classic)
    {
        crtText(g, "CLASSIC: SEE THE M6 CURVE (DRIVE / OUTPUT ONLY)", juce::Rectangle<float>(s.getX() + 20.0f, s.getY() + 6.0f, 420.0f, 14.0f), 11.0f,
                juce::Justification::centredLeft, 0.75f);
        return;
    }
    std::array<float, 256> in {}, out {};
    mag::transferCurve(p, in, out);
    juce::Path curve;
    for (size_t i = 0; i < in.size(); ++i)
    {
        const juce::Point<float> pt(xFor(in[i]), yFor(out[i]));
        if (i == 0)
            curve.startNewSubPath(pt);
        else
            curve.lineTo(pt);
    }
    const float live = juce::jlimit(0.0f, 1.0f, (inDb + 60.0f) / 54.0f);
    glow(g, curve, phosphor, 1.5f, 0.85f + 0.15f * live);
    crtText(g, "TRANSFER  IN -> OUT  @ DRIVE " + format(Drive, values.knobs[Drive] + (values.switches[Punish] > 0 ? 20.0 : 0.0)),
            juce::Rectangle<float>(s.getX() + 20.0f, s.getY() + 6.0f, 420.0f, 14.0f), 11.5f, juce::Justification::centredLeft, 0.75f);
    if (values.switches[Model] == mag::Tape)
        crtText(g, "HYSTERESIS LOOP: BIAS " + format(Bias, values.knobs[Bias]), juce::Rectangle<float>(s.getX() + 20.0f, s.getBottom() - 18.0f, 300.0f, 14.0f),
                11.0f, juce::Justification::centredLeft, 0.7f);
}

void HamloMagPanel::drawHarmonicsView(juce::Graphics& g, juce::Rectangle<float> s) const
{
    const auto p = slotParams();
    const int level = inDb > -90.0f ? 2 * juce::roundToInt(inDb * 0.5f) : -12;
    bool same = level == profileLevel;
    for (size_t i = 0; same && i < p.values.size(); ++i)
        same = p.values[i] == profileFor.values[i];
    if (!same && values.switches[Model] != mag::Classic)
    {
        mag::harmonicProfile(p, static_cast<double>(level), profile);
        profileFor = p;
        profileLevel = level;
    }
    const float left = s.getX() + 40.0f, right = s.getRight() - 16.0f, top = s.getY() + 28.0f, bottom = s.getBottom() - 24.0f;
    const auto yFor = [&](double db) { return top + (bottom - top) * static_cast<float>(juce::jlimit(0.0, 1.0, -db / 100.0)); };
    for (int db = 0; db >= -100; db -= 20)
    {
        g.setColour(phosphorDim.withAlpha(db == 0 ? 0.5f : 0.22f));
        g.drawHorizontalLine(juce::roundToInt(yFor(db)), left, right);
        crtText(g, juce::String(db), juce::Rectangle<float>(s.getX() + 4.0f, yFor(db) - 7.0f, 32.0f, 14.0f), 10.5f, juce::Justification::centredLeft, 0.5f);
    }
    crtText(g, "HARMONICS @ " + juce::String(level) + " dBFS IN  (HamloLaw + " + juce::String(mag::modelName(values.switches[Model])).toUpperCase() + ")",
            juce::Rectangle<float>(left, s.getY() + 6.0f, 400.0f, 14.0f), 11.5f, juce::Justification::centredLeft, 0.75f);
    if (values.switches[Model] == mag::Classic)
        return;
    const float slot = (right - left) / 9.0f;
    for (int h = 0; h < 9; ++h)
    {
        const double db = profile[static_cast<size_t>(h)];
        const float x0 = left + slot * static_cast<float>(h) + slot * 0.22f, x1 = left + slot * static_cast<float>(h + 1) - slot * 0.22f;
        const float y = yFor(db);
        const auto col = h == 0 ? phosphor.brighter(0.3f) : (h % 2 == 1 ? phosphor : kOdd); // H2, H4 … even
        juce::ColourGradient bar(col.withAlpha(0.55f), 0.0f, y, col.withAlpha(0.08f), 0.0f, bottom, false);
        g.setGradientFill(bar);
        g.fillRect(juce::Rectangle<float>(x0, y, x1 - x0, bottom - y));
        juce::Path cap;
        cap.startNewSubPath(x0, y);
        cap.lineTo(x1, y);
        glow(g, cap, col, 1.4f);
        crtText(g, "H" + juce::String(h + 1), juce::Rectangle<float>(x0 - 6.0f, bottom + 4.0f, x1 - x0 + 12.0f, 14.0f), 10.5f, juce::Justification::centred, 0.65f);
        crtText(g, juce::String(juce::roundToInt(db)), juce::Rectangle<float>(x0 - 8.0f, y - 16.0f, x1 - x0 + 16.0f, 13.0f), 10.0f, juce::Justification::centred, 0.8f);
    }
}

void HamloMagPanel::drawCrt(juce::Graphics& g) const
{
    crtBegin(g, kScreen);
    if (mode == CurveView)
        drawCurveView(g, kScreen);
    else
        drawHarmonicsView(g, kScreen);
    crtEnd(g, kScreen);
    crtButton(g, kTabCurve, "CURVE", mode == CurveView);
    crtButton(g, kTabHarm, "HARMONICS", mode == HarmonicsView);
}

void HamloMagPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    drawReel(g, kReelL, 82.0f, static_cast<float>(reelLeft));
    drawReel(g, kReelR, 52.0f, static_cast<float>(reelRight));
    drawTape(g);
    drawCrt(g);
    drawLevers(g);
    drawSelector(g);
    drawToggles(g);
    meterNeedle(g, meter->meter);
    drawLcd(g);
    for (const auto& k : knobs)
        drawKnob(g, k);
}

void HamloMagPanel::timerCallback()
{
    if (!isShowing())
        return;
    float in = -120.0f, out = -120.0f;
    if (readMeters)
        std::tie(in, out) = readMeters();
    if (!values.on)
        in = out = -120.0f;
    auto& m = meter->meter;
    const float target = juce::jlimit(-60.0f, 9.0f, out);
    m.shown += (target - m.shown) * (target > m.shown ? 0.55f : 0.14f);
    inDb += (in - inDb) * 0.3f;
    outDb += (out - outDb) * 0.3f;
    // the reels turn while there is signal: the take-up (smaller pack) faster, at the speed set
    if (values.on && in > -70.0f && values.switches[Model] != mag::Classic)
    {
        const double speed = std::array<double, 3> { 1.0, 0.5, 0.25 }[static_cast<size_t>(juce::jlimit(0, 2, values.switches[Speed]))];
        reelLeft += 0.09 * speed;
        reelRight += 0.09 * speed * 82.0 / 52.0;
    }
    repaint();
}

void HamloMagPanel::settleMeters(float inputDb, float outputDb)
{
    meter->meter.shown = juce::jlimit(-42.0f, 7.0f, outputDb);
    inDb = inputDb;
    outDb = outputDb;
    reelLeft = 0.4;
    reelRight = 1.3;
    repaint();
}

// ---------------------------------------------------------------------------------------------------
// mouse

int HamloMagPanel::knobAt(juce::Point<float> d) const
{
    for (const auto& k : knobs)
        if (d.getDistanceFrom(k.centre) <= 62.0f * k.scale)
            return k.param;
    return -1;
}

int HamloMagPanel::modelLabelAt(juce::Point<float> d) const
{
    for (int i = 0; i < mag::NumModels; ++i)
        if (modelLabelArea(i).expanded(0.0f, 1.0f).contains(d))
            return i;
    return -1;
}

void HamloMagPanel::mouseDown(const juce::MouseEvent& e)
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
        dragSelector = true;
        dragSelectorStart = values.switches[Model];
        dragStartY = e.position.y;
        return;
    }
    if (const int m = modelLabelAt(d); m >= 0)
    {
        setSwitch(Model, m);
        return;
    }
    if (kTabCurve.contains(d))
    {
        setDisplayMode(CurveView);
        return;
    }
    if (kTabHarm.contains(d))
    {
        setDisplayMode(HarmonicsView);
        return;
    }
    for (const auto& [centre, which] : { std::pair { kSpeedX, Speed }, std::pair { kOsX, Oversampling } })
        if (juce::Rectangle<float>(centre - 70.0f, leverTop(centre), 140.0f, 90.0f).contains(d))
        {
            int option = -1;
            if (d.y <= leverTop(centre) + 24.0f)
                for (int i = 0; i < 3; ++i)
                    if (std::abs(d.x - optionX(centre, i)) <= 21.0f)
                        option = i;
            setSwitch(which, option >= 0 ? option : (values.switches[which] + 1) % 3);
            return;
        }
    const std::array<Switch, 4> toggles { Punish, Thump, Steep, Auto };
    for (int i = 0; i < 4; ++i)
        if (juce::Rectangle<float>(toggleCentre(i).x - 22.0f, toggleCentre(i).y - 30.0f, 44.0f, 52.0f).contains(d))
        {
            const auto which = toggles[static_cast<size_t>(i)];
            setSwitch(which, values.switches[which] > 0 ? 0 : 1);
            return;
        }
    if (juce::Rectangle<float>(kNoiseToggle.x - 22.0f, kNoiseToggle.y - 30.0f, 100.0f, 60.0f).contains(d))
    {
        setSwitch(NoiseFollow, values.switches[NoiseFollow] > 0 ? 0 : 1);
        return;
    }
    if (d.getDistanceFrom(kLamp) <= 16.0f)
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    for (const auto& k : knobs) // a click on a readout types a value
    {
        const bool large = k.param < kLarge;
        const auto r = juce::Rectangle<float>(k.centre.x - (large ? 80.0f : 60.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 12.0f,
                                              large ? 160.0f : 120.0f, 24.0f);
        if (r.contains(d))
        {
            editReadout(k.param);
            return;
        }
    }
}

void HamloMagPanel::mouseDrag(const juce::MouseEvent& e)
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
        const int m = juce::jlimit(0, mag::NumModels - 1, dragSelectorStart + steps);
        if (m != values.switches[Model])
            setSwitch(Model, m);
    }
}

void HamloMagPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
    dragSelector = false;
}

void HamloMagPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        const auto p = static_cast<Param>(k);
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setKnob(p, defaultFor(p), true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloMagPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        const auto p = static_cast<Param>(k);
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setNorm(p, normFor(p) + (e.mods.isShiftDown() ? 0.004 : 0.02) * (wheel.deltaY > 0 ? 1.0 : -1.0), true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        return;
    }
    if (d.getDistanceFrom(kSelector) <= 60.0f)
        setSwitch(Model, juce::jlimit(0, mag::NumModels - 1, values.switches[Model] + (wheel.deltaY > 0 ? 1 : -1)));
}

void HamloMagPanel::editReadout(Param p)
{
    const auto& k = knobs[static_cast<size_t>(p)];
    const bool large = p < kLarge;
    const auto area = juce::Rectangle<float>(k.centre.x - (large ? 70.0f : 56.0f), (large ? kLargeReadoutY : kSmallReadoutY) - 13.0f,
                                             large ? 140.0f : 112.0f, 26.0f)
                          .transformedBy(toScreen())
                          .toNearestInt();
    const bool percent = p == Mix || p == Harmonics || p == Bias || p == Wow || p == Hiss || p == Hum || p == Crackle || p == Tone || p == EvenOdd;
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(stencilFont((large ? 20.0f : 15.0f) * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const double v = values.knobs[static_cast<size_t>(p)];
    typing->setText(percent ? juce::String(juce::roundToInt(v * 100.0)) : juce::String(v, std::abs(v) < 10.0 ? 2 : 0), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloMagPanel> self(this);
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
juce::String HamloMagPanel::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (const int k = knobAt(d); k >= 0)
        return keyFor(static_cast<Param>(k));
    return {};
}

juce::Rectangle<float> HamloMagPanel::contentBounds() const
{
    return juce::Rectangle<float>(0.0f, 0.0f, kW, kH).transformedBy(toScreen());
}

} // namespace hamlo::ui
