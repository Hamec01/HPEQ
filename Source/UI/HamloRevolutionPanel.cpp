#include "UI/HamloRevolutionPanel.h"

#include <cmath>

namespace hpeq::ui
{
namespace
{
using namespace steel;
namespace rev = hpeq::revolution;

constexpr float kW = 1180.0f, kH = 700.0f;
constexpr float kPi = juce::MathConstants<float>::pi;

const juce::Rectangle<float> kPlate { 30.0f, 26.0f, 330.0f, 92.0f };
const juce::Rectangle<float> kWaveBezel { 374.0f, 22.0f, 242.0f, 92.0f }, kWaveScreen { 384.0f, 30.0f, 222.0f, 64.0f };
const juce::Rectangle<float> kStatusLcd { 915.0f, 176.0f, 240.0f, 68.0f };
const juce::Point<float> kGenerate { 652.0f, 69.0f }, kNewSeed { 730.0f, 69.0f }, kLamp { 1112.0f, 62.0f };
const juce::Point<float> kTriplet { 335.0f, 206.0f }, kFree { 410.0f, 206.0f };
constexpr std::array<float, 3> kRowY { 288.0f, 420.0f, 552.0f };
constexpr float kCellX = 400.0f, kCellPitch = 44.0f, kCellW = 41.0f, kCellTop = 16.0f, kCellH = 84.0f;
// per-lane GEN and LOCK, right of the cells
juce::Rectangle<float> laneGenRect(int engine);
juce::Rectangle<float> laneLockRect(int engine);
const juce::Rectangle<float> kUpgrade { 490.0f, 540.0f, 200.0f, 50.0f };

struct KnobInfo
{
    float x, y, s;
    double min, max;
    bool log;
    const char* label;
};
constexpr float kRowKnobDy = 66.0f;
const std::array<KnobInfo, HamloRevolutionPanel::NumParams> kKnobs { {
    { 1000.0f, 82.0f, 0.5f, 0.0, 1.0, false, "MIX" },
    { 495.0f, 212.0f, 0.44f, 20.0, 2000.0, true, "RATE" },
    { 585.0f, 212.0f, 0.44f, 1.0, 16.0, false, "STEPS" },
    { 675.0f, 212.0f, 0.44f, 0.0, 1.0, false, "SWING" },
    { 765.0f, 212.0f, 0.44f, 0.0, 1.0, false, "HUMAN" },
    { 855.0f, 212.0f, 0.44f, 0.5, 8.0, false, "FADE" },
    { 198.0f, kRowY[0] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "MIX" },
    { 274.0f, kRowY[0] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "DEPTH" },
    { 198.0f, kRowY[1] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "MIX" },
    { 274.0f, kRowY[1] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "DEPTH" },
    { 198.0f, kRowY[2] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "MIX" },
    { 274.0f, kRowY[2] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "DEPTH" },
    { 350.0f, kRowY[2] + kRowKnobDy, 0.4f, 0.0, 1.0, false, "SMOOTH" },
} };

// Selectors: centre, number of positions, the labels around them, the title under them.
struct SelectorInfo
{
    HamloRevolutionPanel::Switch sw;
    float x, y;
    int count;
    std::array<const char*, 6> labels;
    const char* title;
};
const std::array<SelectorInfo, 3> kSelectors { {
    { HamloRevolutionPanel::Character, 860.0f, 82.0f, 4, { "MIN", "MUS", "EXP", "DESTR", "", "" }, "CHARACTER" },
    { HamloRevolutionPanel::Order, 100.0f, 212.0f, 6, { "TGM", "TMG", "GTM", "GMT", "MTG", "MGT" }, "ORDER" },
    { HamloRevolutionPanel::Grid, 235.0f, 212.0f, 4, { "1/4", "1/8", "1/16", "1/32", "", "" }, "GRID" },
} };
constexpr float kSelectorScale = 0.46f, kSelectorLabelR = 52.0f;

float selectorAngle(int count, int position)
{
    const float total = count <= 4 ? 180.0f : 240.0f;
    return (-0.5f * total + total * static_cast<float>(position) / static_cast<float>(count - 1)) * kPi / 180.0f;
}

juce::Rectangle<float> cellRect(int engine, int step)
{
    return { kCellX + kCellPitch * static_cast<float>(step), kRowY[static_cast<size_t>(engine)] + kCellTop, kCellW, kCellH };
}
juce::Rectangle<float> plateRect(int engine) { return { 26.0f, kRowY[static_cast<size_t>(engine)] + 6.0f, 124.0f, 34.0f }; }
juce::Rectangle<float> laneGenRect(int engine) { return { 1110.0f, kRowY[static_cast<size_t>(engine)] + 18.0f, 56.0f, 34.0f }; }
juce::Rectangle<float> laneLockRect(int engine) { return { 1110.0f, kRowY[static_cast<size_t>(engine)] + 62.0f, 56.0f, 34.0f }; }
juce::Point<float> onLever(int engine) { return { 56.0f, kRowY[static_cast<size_t>(engine)] + 74.0f }; }
juce::Point<float> onLamp(int engine) { return { 108.0f, kRowY[static_cast<size_t>(engine)] + 60.0f }; }
const juce::Rectangle<float> kTargetWindow { 88.0f, kRowY[2] + 84.0f, 84.0f, 24.0f };
const juce::Rectangle<float> kStretchWindow { 88.0f, kRowY[0] + 84.0f, 84.0f, 24.0f };
const juce::Point<float> kListen { 410.0f, 124.0f };
constexpr std::array<const char*, 3> kStretchNames { "TAPE", "RHYTHM", "CLEAN" };

const juce::Colour kTimeInk { 0xff6dff9a }, kGlitchInk { 0xffffb347 }, kMotionInk { 0xff8fd8ff };
juce::Colour inkFor(int engine) { return engine == rev::Time ? kTimeInk : engine == rev::Glitch ? kGlitchInk : kMotionInk; }

void shadowText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::FontOptions font, juce::Colour colour,
                juce::Justification just = juce::Justification::centred)
{
    g.setFont(font);
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0.0f, 1.0f), just);
    g.setColour(colour);
    g.drawText(text, r, just);
}

void pushButton(juce::Graphics& g, juce::Point<float> c, float r, juce::Colour light, juce::Colour dark, bool lit)
{
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(c.x - r, c.y - r + 5.0f, 2.0f * r, 2.0f * r);
    g.setColour(juce::Colour(0xff111111));
    g.fillEllipse(c.x - r, c.y - r, 2.0f * r, 2.0f * r);
    juce::ColourGradient cap(lit ? light.brighter(0.5f) : light, c.x - 0.3f * r, c.y - 0.35f * r, lit ? dark.brighter(0.4f) : dark,
                             c.x + 0.6f * r, c.y + 0.7f * r, true);
    g.setGradientFill(cap);
    const float inset = 0.18f * r;
    g.fillEllipse(c.x - r + inset, c.y - r + inset, 2.0f * (r - inset), 2.0f * (r - inset));
    if (lit)
    {
        g.setColour(light.withAlpha(0.25f));
        g.fillEllipse(c.x - 1.5f * r, c.y - 1.5f * r, 3.0f * r, 3.0f * r);
    }
}
} // namespace

HamloRevolutionPanel::HamloRevolutionPanel()
{
    setOpaque(false);
    startTimerHz(30);
}

HamloRevolutionPanel::~HamloRevolutionPanel()
{
    stopTimer();
}

const char* HamloRevolutionPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "mix", "freeMs", "steps", "swing", "humanize", "fadeMs", "timeMix", "timeDepth",
                                          "glitchMix", "glitchDepth", "motionMix", "motionDepth", "motionSmooth" };
    return keys[p];
}

const char* HamloRevolutionPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "order", "character", "grid", "triplet", "free", "timeOn", "glitchOn", "motionOn", "motionTarget",
                                          "timeStretch", "listen" };
    return keys[s];
}

double HamloRevolutionPanel::defaultFor(Param p)
{
    return Values {}.knobs[static_cast<size_t>(p)];
}

juce::String HamloRevolutionPanel::format(Param p, double v)
{
    switch (p)
    {
        case Rate: return juce::String(juce::roundToInt(v)) + " ms";
        case Steps: return juce::String(juce::roundToInt(v));
        case Swing: return v < 0.005 ? juce::String("STRAIGHT") : juce::String(juce::roundToInt(50.0 + 25.0 * v)) + " %";
        case Fade: return juce::String(v, 1) + " ms";
        default: return juce::String(juce::roundToInt(v * 100.0)) + " %";
    }
}

double HamloRevolutionPanel::normFor(Param p) const
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    const double v = juce::jlimit(k.min, k.max, values.knobs[static_cast<size_t>(p)]);
    return k.log ? std::log(v / k.min) / std::log(k.max / k.min) : (v - k.min) / (k.max - k.min);
}

double HamloRevolutionPanel::valueFor(Param p, double n) const
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    const double v = k.log ? k.min * std::pow(k.max / k.min, n) : k.min + (k.max - k.min) * n;
    return p == Steps ? std::round(v) : v;
}

void HamloRevolutionPanel::setKnob(Param p, double v, bool notify)
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    v = juce::jlimit(k.min, k.max, p == Steps ? std::round(v) : v);
    values.knobs[static_cast<size_t>(p)] = v;
    if (notify && onKnobChange)
        onKnobChange(p, v);
    repaint();
}

void HamloRevolutionPanel::setSwitch(Switch s, int v)
{
    values.switches[static_cast<size_t>(s)] = v;
    if (onSwitchChange)
        onSwitchChange(s, v);
    repaint();
}

void HamloRevolutionPanel::setValues(const Values& v)
{
    if (dragKnob >= 0 || dragMotionStep >= 0)
        return;
    values = v;
    repaint();
}

void HamloRevolutionPanel::changePattern(const rev::Pattern& p, juce::int64 seed, const juce::String& what)
{
    values.pattern = p;
    values.seed = seed;
    if (onPatternChange)
        onPatternChange(p, seed, values.feel, what);
    repaint();
}

void HamloRevolutionPanel::pressGenerate()
{
    generateWith(values.seed, "Generate");
}

void HamloRevolutionPanel::pressNewSeed()
{
    generateWith(static_cast<juce::int64>(rev::nextSeed(static_cast<std::uint64_t>(values.seed))), "New seed");
}

void HamloRevolutionPanel::generateWith(juce::int64 seed, const juce::String& what)
{
    // LISTEN: freeze what the engine heard on each step now; the pattern is generated from that snapshot
    if (values.switches[Listen] != 0)
    {
        if (readStatus)
            live = readStatus();
        values.feel = rev::feelFrom(live.feel);
    }
    else
        values.feel = {};
    changePattern(rev::generateFollowing(values.pattern, static_cast<std::uint64_t>(seed), values.switches[Character], values.feel), seed, what);
    generateFlash = 10;
}

bool HamloRevolutionPanel::laneLocked(const rev::Pattern& p, int engine) noexcept
{
    return p.locks[static_cast<size_t>(engine)] == 0xffff;
}

void HamloRevolutionPanel::pressLaneLock(int engine)
{
    auto p = values.pattern;
    auto& lock = p.locks[static_cast<size_t>(engine)];
    lock = laneLocked(p, engine) ? 0 : 0xffff;
    changePattern(p, values.seed, juce::String(lock != 0 ? "Lock " : "Unlock ") + rev::engineName(engine));
}

void HamloRevolutionPanel::pressLaneGenerate(int engine)
{
    if (laneLocked(values.pattern, engine))
        return; // a locked lane is kept, as GENERATE keeps it
    // the other lanes are held as they are (all their steps locked for this one generate), a new seed
    auto p = values.pattern;
    const auto keep = p.locks;
    for (int o = 0; o < rev::kEngines; ++o)
        if (o != engine)
            p.locks[static_cast<size_t>(o)] = 0xffff;
    if (values.switches[Listen] != 0)
    {
        if (readStatus)
            live = readStatus();
        values.feel = rev::feelFrom(live.feel);
    }
    const auto seed = static_cast<juce::int64>(rev::nextSeed(static_cast<std::uint64_t>(values.seed)));
    auto generated = rev::generateFollowing(p, static_cast<std::uint64_t>(seed), values.switches[Character], values.feel);
    generated.locks = keep;
    changePattern(generated, seed, juce::String("Generate ") + rev::engineName(engine));
    generateFlash = 10;
}

void HamloRevolutionPanel::clickStep(int engine, int step, bool shift)
{
    auto p = values.pattern;
    const auto e = static_cast<size_t>(engine);
    if (shift)
    {
        p.locks[e] = static_cast<std::uint16_t>(p.locks[e] ^ (1u << step));
        changePattern(p, values.seed, "Lock step");
        return;
    }
    auto& v = p.steps[e][static_cast<size_t>(step)];
    v = static_cast<std::uint8_t>((v + 1) % rev::numValues(engine));
    changePattern(p, values.seed, "Step");
}

void HamloRevolutionPanel::setMotionLevel(int step, int level)
{
    auto p = values.pattern;
    auto& v = p.steps[rev::Motion][static_cast<size_t>(step)];
    if (v == static_cast<std::uint8_t>(level))
        return;
    v = static_cast<std::uint8_t>(juce::jlimit(0, rev::kMotionLevels - 1, level));
    changePattern(p, values.seed, "Motion level");
}

juce::AffineTransform HamloRevolutionPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloRevolutionPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloRevolutionPanel::resized()
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
}

void HamloRevolutionPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 1917);
    scratches(g, { { 3, { 1078.0f, 112.0f, 44.0f, 62.0f }, 8.0f }, { 14, { 22.0f, 122.0f, 32.0f, 46.0f }, -4.0f } });
    nameplate(g, kPlate, "HamloRevolution", juce::String::fromUTF8("MODEL \xd0\xa0\xd0\x92-1"), juce::String::fromUTF8("\xe2\x84\x96 1917"));
    crtBezel(g, kWaveBezel, kWaveScreen, "", kWaveBezel.getBottom());
    lcdWindow(g, kStatusLcd);
    engraved(g, "GENERATE", { kGenerate.x - 60.0f, 14.0f, 120.0f, 20.0f }, 14.0f);
    engraved(g, "NEW SEED", { kNewSeed.x - 50.0f, 14.0f, 100.0f, 20.0f }, 12.0f);
    engraved(g, "ON", { kLamp.x - 30.0f, kLamp.y + 30.0f, 60.0f, 20.0f }, 15.0f);
    engraved(g, "TRIPLET", { kTriplet.x - 40.0f, 160.0f, 80.0f, 18.0f }, 12.0f);
    engraved(g, "FREE", { kFree.x - 40.0f, 160.0f, 80.0f, 18.0f }, 12.0f);
    engraved(g, "SYNC", { kFree.x - 40.0f, 246.0f, 80.0f, 18.0f }, 12.0f, stencilDim);

    for (const auto& s : kSelectors)
    {
        knobScale(g, { s.x, s.y }, kSelectorScale);
        for (int i = 0; i < s.count; ++i)
        {
            const float a = selectorAngle(s.count, i);
            const juce::Point<float> at(s.x + kSelectorLabelR * std::sin(a), s.y - kSelectorLabelR * std::cos(a));
            engraved(g, s.labels[static_cast<size_t>(i)], { at.x - 24.0f, at.y - 8.0f, 48.0f, 16.0f }, 10.5f, stencil, juce::Justification::centred, 0.04f);
        }
        engraved(g, s.title, { s.x - 60.0f, s.y + 34.0f, 120.0f, 18.0f }, 12.5f);
    }
    for (int p = 0; p < NumParams; ++p)
    {
        const auto& k = kKnobs[static_cast<size_t>(p)];
        knobScale(g, { k.x, k.y }, k.s);
        engraved(g, k.label, { k.x - 50.0f, k.y - k.s * 60.0f - 22.0f, 100.0f, 16.0f }, p >= TimeMix ? 11.0f : 12.5f);
    }

    // engine rows: a groove between them, the plates, step numbers over TIME
    for (int e = 0; e < rev::kEngines; ++e)
    {
        const float y = kRowY[static_cast<size_t>(e)] - 6.0f;
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(juce::roundToInt(y), 20.0f, kW - 20.0f);
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(juce::roundToInt(y) - 1, 20.0f, kW - 20.0f);

        const auto plate = plateRect(e);
        juce::ColourGradient brass(juce::Colour(0xffcdb47a), plate.getX(), plate.getY(), juce::Colour(0xff6e5a2c), plate.getX(), plate.getBottom(), false);
        g.setGradientFill(brass);
        g.fillRoundedRectangle(plate, 3.0f);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.drawRoundedRectangle(plate, 3.0f, 1.2f);
        g.setFont(stencilFont(19.0f).withKerningFactor(0.12f));
        g.setColour(juce::Colour(0xff1d160a));
        g.drawText(rev::engineName(e), plate, juce::Justification::centred);
        for (const float rx : { plate.getX() + 7.0f, plate.getRight() - 7.0f })
        {
            g.setColour(juce::Colour(0xff3b3020));
            g.fillEllipse(rx - 2.5f, plate.getCentreY() - 2.5f, 5.0f, 5.0f);
        }
        for (int s = 0; s < rev::kSteps; ++s)
        {
            const auto r = cellRect(e, s);
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.fillRoundedRectangle(r.translated(0.0f, 1.5f).expanded(1.5f), 5.0f);
            g.setColour(juce::Colour(0xff030403));
            g.fillRoundedRectangle(r.expanded(1.5f), 5.0f);
        }
    }
    for (int s = 0; s < rev::kSteps; ++s)
    {
        const auto r = cellRect(0, s);
        engraved(g, juce::String(s + 1), { r.getX(), r.getY() - 18.0f, r.getWidth(), 14.0f }, s % 4 == 0 ? 11.5f : 9.5f,
                 s % 4 == 0 ? stencil : stencilDim, juce::Justification::centred, 0.02f);
    }
    lcdWindow(g, kTargetWindow);
    engraved(g, "TARGET", { kTargetWindow.getX(), kTargetWindow.getBottom() + 2.0f, kTargetWindow.getWidth(), 14.0f }, 10.0f);
    lcdWindow(g, kStretchWindow);
    engraved(g, "STRETCH", { kStretchWindow.getX(), kStretchWindow.getBottom() + 2.0f, kStretchWindow.getWidth(), 14.0f }, 10.0f);
    engraved(g, "LISTEN", { kListen.x + 16.0f, kListen.y - 9.0f, 70.0f, 18.0f }, 12.0f, stencil, juce::Justification::centredLeft);
}

void HamloRevolutionPanel::drawSelectors(juce::Graphics& g) const
{
    for (const auto& s : kSelectors)
    {
        const int pos = juce::jlimit(0, s.count - 1, values.switches[static_cast<size_t>(s.sw)]);
        brassKnob(g, { s.x, s.y }, selectorAngle(s.count, pos), kSelectorScale, 1.0f, KnobAsset::BakeliteRibbed, PointerAsset::RedTipBrassCollar);
    }
    lever(g, kTriplet, 0.0f, 0.9f, values.switches[Triplet] != 0 ? LeverPosition::Up : LeverPosition::Down);
    lever(g, kFree, 0.0f, 0.9f, values.switches[Free] != 0 ? LeverPosition::Up : LeverPosition::Down);
}

void HamloRevolutionPanel::drawKnobs(juce::Graphics& g) const
{
    for (int p = 0; p < NumParams; ++p)
    {
        const auto& k = kKnobs[static_cast<size_t>(p)];
        const auto param = static_cast<Param>(p);
        const bool dim = (param == Rate && values.switches[Free] == 0) || (param >= TimeMix && param <= TimeDepth && values.switches[TimeOn] == 0)
                         || (param >= GlitchMix && param <= GlitchDepth && values.switches[GlitchOn] == 0)
                         || (param >= MotionMix && values.switches[MotionOn] == 0);
        const float angle = (-135.0f + static_cast<float>(normFor(param)) * 270.0f) * kPi / 180.0f;
        brassKnob(g, { k.x, k.y }, angle, k.s, dim ? 0.55f : 1.0f, p >= TimeMix ? KnobAsset::LatheSteelRust : KnobAsset::AluminumBrassInsert,
                  PointerAsset::WhiteEnamel);
        const auto r = juce::Rectangle<float>(k.x - 50.0f, k.y + k.s * 60.0f + 4.0f, 100.0f, 18.0f);
        shadowText(g, format(param, values.knobs[static_cast<size_t>(p)]), r, stencilFont(p >= TimeMix ? 13.0f : 15.0f), dim ? stencilDim : stencil);
    }
}

void HamloRevolutionPanel::drawTop(juce::Graphics& g) const
{
    // the wave screen: input (dim) and output (bright) of the pattern cycle, step grid, play head
    {
        const auto s = kWaveScreen;
        crtBegin(g, s);
        const int steps = juce::jlimit(1, rev::kSteps, juce::roundToInt(values.knobs[Steps]));
        const int bins = steps * 8;
        float peak = 1.0e-4f;
        for (int b = 0; b < bins; ++b)
            peak = std::max({ peak, live.waveIn[static_cast<size_t>(b)], live.waveOut[static_cast<size_t>(b)] });
        const float mid = s.getCentreY(), half = s.getHeight() * 0.5f - 6.0f, binW = s.getWidth() / static_cast<float>(bins);
        for (int k = 1; k < steps; ++k)
        {
            g.setColour(phosphorDim.withAlpha(k % 4 == 0 ? 0.45f : 0.2f));
            g.drawVerticalLine(juce::roundToInt(s.getX() + binW * 8.0f * static_cast<float>(k)), s.getY() + 2.0f, s.getBottom() - 2.0f);
        }
        for (int b = 0; b < bins; ++b)
        {
            const float x = s.getX() + binW * static_cast<float>(b);
            const float hi = half * live.waveIn[static_cast<size_t>(b)] / peak, ho = half * live.waveOut[static_cast<size_t>(b)] / peak;
            g.setColour(phosphorDim.withAlpha(0.55f));
            g.fillRect(x, mid - hi, juce::jmax(1.0f, binW - 0.5f), 2.0f * hi);
            g.setColour(phosphor.withAlpha(0.85f));
            g.fillRect(x + binW * 0.25f, mid - ho, juce::jmax(1.0f, binW * 0.5f), 2.0f * ho);
        }
        if (live.step >= 0 && live.step < steps)
        {
            g.setColour(phosphor.withAlpha(0.18f));
            g.fillRect(s.getX() + binW * 8.0f * static_cast<float>(live.step), s.getY(), binW * 8.0f, s.getHeight());
        }
        crtText(g, "IN / OUT", { s.getX() + 4.0f, s.getY() + 2.0f, 80.0f, 12.0f }, 9.5f, juce::Justification::centredLeft, 0.7f);
        crtText(g, "SEED " + juce::String(values.seed).paddedLeft('0', 8), { s.getRight() - 110.0f, s.getY() + 2.0f, 106.0f, 12.0f }, 9.5f,
                juce::Justification::centredRight, 0.8f);
        crtEnd(g, s);
    }
    pushButton(g, kGenerate, 30.0f, juce::Colour(0xffff5a45), juce::Colour(0xff6e0d06), generateFlash > 0);
    pushButton(g, kNewSeed, 17.0f, juce::Colour(0xffc9c9c0), juce::Colour(0xff3b3b37), false);
    lamp(g, kLamp, values.on, 0.6f);

    const int steps = juce::roundToInt(values.knobs[Steps]);
    static constexpr const char* grids[] { "1/4", "1/8", "1/16", "1/32" };
    juce::String line1 = "STEP " + (live.step >= 0 ? juce::String(live.step + 1).paddedLeft('0', 2) : juce::String("--")) + "/"
                         + juce::String(steps) + "  " + rev::orderName(values.switches[Order]);
    juce::String line2;
    if (values.legacy)
        line2 = "LEGACY MONSTA";
    else if (live.warming)
        line2 = "WARMING UP";
    else if (values.switches[Free] != 0)
        line2 = "FREE " + format(Rate, values.knobs[Rate]);
    else
        line2 = juce::String("SYNC ") + grids[juce::jlimit(0, 3, values.switches[Grid])] + (values.switches[Triplet] != 0 ? "T" : "");
    lcdText(g, kStatusLcd.withHeight(kStatusLcd.getHeight() * 0.5f).translated(0.0f, 4.0f), line1, 16.0f);
    lcdText(g, kStatusLcd.withTrimmedTop(kStatusLcd.getHeight() * 0.5f).translated(0.0f, -4.0f), line2, 16.0f);
}

void HamloRevolutionPanel::drawRows(juce::Graphics& g) const
{
    const int steps = juce::roundToInt(values.knobs[Steps]);
    for (int e = 0; e < rev::kEngines; ++e) // GEN and LOCK of each lane
    {
        const bool locked = laneLocked(values.pattern, e);
        for (int b = 0; b < 2; ++b)
        {
            const auto r = b == 0 ? laneGenRect(e) : laneLockRect(e);
            const bool lit = b == 1 && locked;
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.fillRoundedRectangle(r.translated(0.0f, 3.0f), 5.0f);
            juce::ColourGradient face(lit ? juce::Colour(0xffffb347) : juce::Colour(0xff5a5a55), r.getX(), r.getY(),
                                      lit ? juce::Colour(0xff8a4d10) : juce::Colour(0xff26261f), r.getX(), r.getBottom(), false);
            g.setGradientFill(face);
            g.fillRoundedRectangle(r, 5.0f);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(r, 5.0f, 1.2f);
            shadowText(g, b == 0 ? "GEN" : (locked ? "LOCKED" : "LOCK"), r, stencilFont(b == 1 && locked ? 10.5f : 13.0f),
                       lit ? juce::Colour(0xff1a0f02) : (b == 0 ? inkFor(e) : stencil));
        }
    }
    for (int e = 0; e < rev::kEngines; ++e)
    {
        const bool engineOn = values.switches[static_cast<size_t>(TimeOn + e)] != 0;
        g.setColour(juce::Colours::white); // steel::lamp leaves a gradient fill, under which drawImage draws nothing
        lever(g, onLever(e), 0.0f, 0.75f, engineOn ? LeverPosition::Up : LeverPosition::Down);
        lamp(g, onLamp(e), engineOn && values.on, 0.28f, inkFor(e));
        const auto ink = inkFor(e);
        for (int s = 0; s < rev::kSteps; ++s)
        {
            const auto r = cellRect(e, s);
            const bool active = s < steps, here = active && s == live.step;
            const int v = values.pattern.steps[static_cast<size_t>(e)][static_cast<size_t>(s)];
            g.setColour(here ? juce::Colour(0xff17231b) : juce::Colour(0xff0a0d0b));
            g.fillRoundedRectangle(r, 4.0f);
            if (e == rev::Time)
            {
                // what LISTEN hears on this step (live), or the snapshot the pattern was made from
                const float f = values.switches[Listen] != 0 ? live.feel[static_cast<size_t>(s)] : values.feel[static_cast<size_t>(s)] / 15.0f;
                if (f > 0.02f)
                {
                    const float h = (r.getHeight() - 8.0f) * juce::jlimit(0.0f, 1.0f, f);
                    g.setColour(kGlitchInk.withAlpha(values.switches[Listen] != 0 ? 0.16f : 0.08f));
                    g.fillRect(r.getX() + 3.0f, r.getBottom() - 4.0f - h, r.getWidth() - 6.0f, h);
                }
            }
            const float alpha = (active ? 1.0f : 0.3f) * (engineOn ? 1.0f : 0.45f);
            if (e == rev::Motion)
            {
                if (v > 0)
                {
                    const float h = (r.getHeight() - 14.0f) * static_cast<float>(v) / 15.0f;
                    const auto bar = juce::Rectangle<float>(r.getX() + 7.0f, r.getBottom() - 6.0f - h, r.getWidth() - 14.0f, h);
                    g.setColour(ink.withAlpha(0.18f * alpha));
                    g.fillRect(bar.expanded(3.0f, 2.0f));
                    g.setColour(ink.withAlpha(0.85f * alpha));
                    g.fillRect(bar);
                }
                else
                {
                    g.setColour(ink.withAlpha(0.25f * alpha));
                    g.fillRect(r.getX() + 9.0f, r.getBottom() - 8.0f, r.getWidth() - 18.0f, 1.5f);
                }
            }
            else if (v > 0)
            {
                const auto text = rev::valueShortName(e, v);
                g.setFont(monoFont(e == rev::Time ? 22.0f : 17.0f).withStyle("Bold"));
                g.setColour(ink.withAlpha(0.22f * alpha));
                g.drawText(text, r.translated(0.0f, 1.5f).expanded(1.0f), juce::Justification::centred);
                g.setColour(ink.withAlpha(0.95f * alpha));
                g.drawText(text, r, juce::Justification::centred);
            }
            else
            {
                g.setColour(ink.withAlpha(0.25f * alpha));
                g.fillEllipse(r.getCentreX() - 1.8f, r.getCentreY() - 1.8f, 3.6f, 3.6f);
            }
            if (((values.pattern.locks[static_cast<size_t>(e)] >> s) & 1) != 0)
            {
                g.setColour(juce::Colour(0xffff3b2d));
                g.fillEllipse(r.getRight() - 10.0f, r.getY() + 4.0f, 6.0f, 6.0f);
            }
            if (here)
            {
                g.setColour(ink.withAlpha(0.8f));
                g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.6f);
            }
            // glass: a faint highlight on top
            g.setColour(juce::Colours::white.withAlpha(0.035f));
            g.fillRoundedRectangle(r.withHeight(r.getHeight() * 0.35f).reduced(2.0f, 2.0f), 3.0f);
        }
    }
    lcdText(g, kTargetWindow, rev::targetName(values.switches[MotionTarget]), 12.0f);
    lcdText(g, kStretchWindow, kStretchNames[static_cast<size_t>(juce::jlimit(0, 2, values.switches[Stretch]))], 12.0f);
    lamp(g, kListen, values.switches[Listen] != 0, 0.3f, kGlitchInk);
}

void HamloRevolutionPanel::drawLegacy(juce::Graphics& g) const
{
    const auto area = juce::Rectangle<float>(20.0f, kRowY[0] - 14.0f, kW - 40.0f, kH - kRowY[0] + 4.0f);
    g.setColour(juce::Colour(0xe0050605));
    g.fillRoundedRectangle(area, 8.0f);
    shadowText(g, "LEGACY MONSTA FX", area.withHeight(120.0f).translated(0.0f, 60.0f), stencilFont(34.0f), stencil);
    shadowText(g, "This slot was saved before HamloRevolution and still plays the old slicer, unchanged.",
               area.withHeight(40.0f).translated(0.0f, 160.0f), stencilFont(17.0f, false), stencilDim);
    shadowText(g, "UPGRADE starts HamloRevolution here (one undo step).", area.withHeight(30.0f).translated(0.0f, 190.0f),
               stencilFont(15.0f, false), stencilDim);
    crtButton(g, kUpgrade, "UPGRADE", true);
}

void HamloRevolutionPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    drawTop(g);
    drawSelectors(g);
    drawKnobs(g);
    drawRows(g);
    if (values.legacy)
        drawLegacy(g);
}

void HamloRevolutionPanel::timerCallback()
{
    if (!isShowing())
        return;
    Status s;
    if (readStatus)
        s = readStatus();
    if (!values.on || values.legacy)
        s = {};
    const bool changed = s.step != live.step || s.warming != live.warming || generateFlash > 0
                         || (values.switches[Listen] != 0 && s.feel != live.feel);
    live = s;
    if (generateFlash > 0)
        --generateFlash;
    if (changed)
        repaint();
}

void HamloRevolutionPanel::settleStatus(const Status& s)
{
    live = s;
    repaint();
}

int HamloRevolutionPanel::knobAt(juce::Point<float> d) const
{
    for (int p = 0; p < NumParams; ++p)
    {
        const auto& k = kKnobs[static_cast<size_t>(p)];
        if (d.getDistanceFrom({ k.x, k.y }) <= 60.0f * k.s)
            return p;
    }
    return -1;
}

int HamloRevolutionPanel::selectorAt(juce::Point<float> d) const
{
    for (size_t i = 0; i < kSelectors.size(); ++i)
        if (d.getDistanceFrom({ kSelectors[i].x, kSelectors[i].y }) <= 34.0f)
            return static_cast<int>(i);
    return -1;
}

bool HamloRevolutionPanel::stepAt(juce::Point<float> d, int& engine, int& step) const
{
    for (int e = 0; e < rev::kEngines; ++e)
        for (int s = 0; s < rev::kSteps; ++s)
            if (cellRect(e, s).expanded(1.5f, 0.0f).contains(d))
            {
                engine = e;
                step = s;
                return true;
            }
    return false;
}

void HamloRevolutionPanel::showStepMenu(int engine, int step)
{
    juce::PopupMenu menu;
    const int current = values.pattern.steps[static_cast<size_t>(engine)][static_cast<size_t>(step)];
    for (int v = 0; v < rev::numValues(engine); ++v)
        menu.addItem(1 + v, rev::valueName(engine, v), true, v == current);
    menu.addSeparator();
    menu.addItem(100, "Lock step", true, ((values.pattern.locks[static_cast<size_t>(engine)] >> step) & 1) != 0);
    juce::Component::SafePointer<HamloRevolutionPanel> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [self, engine, step](int result)
    {
        if (self == nullptr || result <= 0)
            return;
        if (result == 100)
        {
            self->clickStep(engine, step, true);
            return;
        }
        auto p = self->values.pattern;
        p.steps[static_cast<size_t>(engine)][static_cast<size_t>(step)] = static_cast<std::uint8_t>(result - 1);
        self->changePattern(p, self->values.seed, "Step");
    });
}

void HamloRevolutionPanel::showPlateMenu(int engine)
{
    juce::PopupMenu menu;
    const auto e = static_cast<size_t>(engine);
    if (engine != rev::Motion)
    {
        menu.addSectionHeader("Generate may use");
        for (int a = 1; a < rev::numValues(engine); ++a)
            menu.addItem(1 + a, rev::valueName(engine, a), true, ((values.pattern.allow[e] >> a) & 1) != 0);
        menu.addSeparator();
    }
    menu.addItem(200, "Clear unlocked steps");
    menu.addItem(201, "Lock all steps");
    menu.addItem(202, "Unlock all steps");
    juce::Component::SafePointer<HamloRevolutionPanel> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [self, e](int result)
    {
        if (self == nullptr || result <= 0)
            return;
        auto p = self->values.pattern;
        juce::String what = "Pattern";
        if (result == 200)
        {
            for (int s = 0; s < rev::kSteps; ++s)
                if (((p.locks[e] >> s) & 1) == 0)
                    p.steps[e][static_cast<size_t>(s)] = 0;
            what = "Clear steps";
        }
        else if (result == 201 || result == 202)
        {
            p.locks[e] = result == 201 ? 0xffff : 0;
            what = "Lock steps";
        }
        else
        {
            p.allow[e] = static_cast<std::uint16_t>(p.allow[e] ^ (1u << (result - 1)));
            what = "Generate actions";
        }
        self->changePattern(p, self->values.seed, what);
    });
}

void HamloRevolutionPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (d.getDistanceFrom(kLamp) <= 26.0f)
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    if (values.legacy)
    {
        if (kUpgrade.contains(d) && onUpgrade)
            onUpgrade();
        return;
    }
    if (d.getDistanceFrom(kGenerate) <= 32.0f)
        return pressGenerate();
    for (int eng = 0; eng < rev::kEngines; ++eng)
    {
        if (laneGenRect(eng).contains(d))
            return pressLaneGenerate(eng);
        if (laneLockRect(eng).contains(d))
            return pressLaneLock(eng);
    }
    if (d.getDistanceFrom(kNewSeed) <= 20.0f)
        return pressNewSeed();
    if (const int s = selectorAt(d); s >= 0)
    {
        const auto& info = kSelectors[static_cast<size_t>(s)];
        const int pos = values.switches[static_cast<size_t>(info.sw)];
        setSwitch(info.sw, (pos + (e.mods.isPopupMenu() ? info.count - 1 : 1)) % info.count);
        return;
    }
    if (juce::Rectangle<float>(kTriplet.x - 26.0f, kTriplet.y - 36.0f, 52.0f, 64.0f).contains(d))
        return setSwitch(Triplet, values.switches[Triplet] != 0 ? 0 : 1);
    if (juce::Rectangle<float>(kFree.x - 26.0f, kFree.y - 36.0f, 52.0f, 64.0f).contains(d))
        return setSwitch(Free, values.switches[Free] != 0 ? 0 : 1);
    for (int eng = 0; eng < rev::kEngines; ++eng)
    {
        const auto lv = onLever(eng);
        if (juce::Rectangle<float>(lv.x - 24.0f, lv.y - 32.0f, 48.0f, 56.0f).contains(d) || d.getDistanceFrom(onLamp(eng)) <= 12.0f)
        {
            const auto sw = static_cast<Switch>(TimeOn + eng);
            return setSwitch(sw, values.switches[static_cast<size_t>(sw)] != 0 ? 0 : 1);
        }
        if (plateRect(eng).contains(d))
            return showPlateMenu(eng);
    }
    if (kStretchWindow.contains(d))
        return setSwitch(Stretch, (values.switches[Stretch] + (e.mods.isPopupMenu() ? 2 : 1)) % 3);
    if (d.getDistanceFrom(kListen) <= 14.0f || juce::Rectangle<float>(kListen.x + 14.0f, kListen.y - 10.0f, 64.0f, 20.0f).contains(d))
        return setSwitch(Listen, values.switches[Listen] != 0 ? 0 : 1);
    if (kTargetWindow.contains(d))
        return setSwitch(MotionTarget, (values.switches[MotionTarget] + (e.mods.isPopupMenu() ? rev::kMotionTargets - 1 : 1)) % rev::kMotionTargets);
    if (const int k = knobAt(d); k >= 0)
    {
        dragKnob = k;
        dragStartNorm = normFor(static_cast<Param>(k));
        dragStartY = e.position.y;
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        return;
    }
    int engine = -1, step = -1;
    if (stepAt(d, engine, step))
    {
        if (e.mods.isPopupMenu())
        {
            if (engine != rev::Motion)
                showStepMenu(engine, step);
            else
                clickStep(engine, step, true);
            return;
        }
        if (e.mods.isShiftDown())
            return clickStep(engine, step, true);
        if (engine != rev::Motion)
            return clickStep(engine, step, false);
        dragMotionStep = step;
        const auto r = cellRect(engine, step);
        setMotionLevel(step, juce::roundToInt(15.0f * juce::jlimit(0.0f, 1.0f, (r.getBottom() - 6.0f - d.y) / (r.getHeight() - 14.0f))));
    }
}

void HamloRevolutionPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob >= 0)
    {
        const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
        setKnob(static_cast<Param>(dragKnob), valueFor(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange), true);
        return;
    }
    if (dragMotionStep >= 0)
    {
        const auto d = toDesign(e.position);
        const int step = juce::jlimit(0, rev::kSteps - 1, static_cast<int>(std::floor((d.x - kCellX) / kCellPitch)));
        const auto r = cellRect(rev::Motion, step);
        dragMotionStep = step;
        setMotionLevel(step, juce::roundToInt(15.0f * juce::jlimit(0.0f, 1.0f, (r.getBottom() - 6.0f - d.y) / (r.getHeight() - 14.0f))));
    }
}

void HamloRevolutionPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
    dragMotionStep = -1;
}

void HamloRevolutionPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int p = knobAt(toDesign(e.position));
    if (p < 0)
        return;
    if (onKnobGestureStart)
        onKnobGestureStart(static_cast<Param>(p));
    setKnob(static_cast<Param>(p), defaultFor(static_cast<Param>(p)), true);
    if (onKnobGestureEnd)
        onKnobGestureEnd();
}

void HamloRevolutionPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    const int dir = wheel.deltaY > 0 ? 1 : -1;
    if (const int s = selectorAt(d); s >= 0 && !values.legacy)
    {
        const auto& info = kSelectors[static_cast<size_t>(s)];
        setSwitch(info.sw, juce::jlimit(0, info.count - 1, values.switches[static_cast<size_t>(info.sw)] + dir));
        return;
    }
    const int p = knobAt(d);
    if (p < 0 || values.legacy)
        return;
    if (onKnobGestureStart)
        onKnobGestureStart(static_cast<Param>(p));
    if (p == Steps)
        setKnob(Steps, values.knobs[Steps] + dir, true);
    else
        setKnob(static_cast<Param>(p), valueFor(static_cast<Param>(p), normFor(static_cast<Param>(p)) + (e.mods.isShiftDown() ? 0.004 : 0.02) * dir), true);
    if (onKnobGestureEnd)
        onKnobGestureEnd();
}
juce::String HamloRevolutionPanel::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (d.getDistanceFrom(kGenerate) <= 32.0f)
        return "generate";
    for (int eng = 0; eng < rev::kEngines; ++eng)
    {
        if (laneGenRect(eng).contains(d))
            return "laneGenerate";
        if (laneLockRect(eng).contains(d))
            return "laneLock";
    }
    if (const int k = knobAt(d); k >= 0)
        return keyFor(static_cast<Param>(k));
    if (const int sel = selectorAt(d); sel >= 0)
        return keyFor(static_cast<Switch>(sel));
    return {};
}

juce::Rectangle<float> HamloRevolutionPanel::contentBounds() const
{
    return juce::Rectangle<float>(0.0f, 0.0f, kW, kH).transformedBy(toScreen());
}

} // namespace hpeq::ui
