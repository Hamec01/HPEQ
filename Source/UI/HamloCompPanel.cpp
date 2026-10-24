#include "UI/HamloCompPanel.h"
#include "UI/BlackSteel.h"

#include <cmath>

namespace hpeq::ui
{
namespace
{
constexpr float kW = 1180.0f, kH = 700.0f; // design stage
const juce::Colour stencil { 0xffe6d8a6 }, stencilDim { 0xffa89c76 };
constexpr float kPi = juce::MathConstants<float>::pi;

juce::FontOptions stencilFont(float size, bool bold = true)
{
    // The design uses Oswald (Google Fonts); Bahnschrift is the closest condensed face shipped with
    // Windows, so nothing is downloaded or embedded.
    return juce::FontOptions("Bahnschrift", size, bold ? juce::Font::bold : juce::Font::plain).withHorizontalScale(0.86f);
}

juce::Colour rgba(int r, int g, int b, float a)
{
    return juce::Colour(static_cast<juce::uint8>(r), static_cast<juce::uint8>(g), static_cast<juce::uint8>(b), a);
}

juce::FontOptions monoFont(float size)
{
    return juce::FontOptions("Consolas", size, juce::Font::plain);
}

void engraved(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, juce::Colour colour = stencil,
              juce::Justification just = juce::Justification::centred, float spacing = 0.12f)
{
    g.setFont(stencilFont(size).withKerningFactor(spacing));
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0.0f, 1.0f), just, false);
    g.setColour(colour.withAlpha(0.93f));
    g.drawText(text, r, just, false);
}

void hexagon(juce::Graphics& g, juce::Point<float> c, float w, float h, juce::Colour light, juce::Colour dark)
{
    juce::Path p;
    p.startNewSubPath(c.x - w * 0.25f, c.y - h * 0.47f);
    p.lineTo(c.x + w * 0.25f, c.y - h * 0.47f);
    p.lineTo(c.x + w * 0.48f, c.y);
    p.lineTo(c.x + w * 0.25f, c.y + h * 0.47f);
    p.lineTo(c.x - w * 0.25f, c.y + h * 0.47f);
    p.lineTo(c.x - w * 0.48f, c.y);
    p.closeSubPath();
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillPath(p, juce::AffineTransform::translation(0.0f, 3.0f));
    juce::ColourGradient grad(light, c.x - w * 0.1f, c.y - h * 0.2f, dark, c.x + w * 0.4f, c.y + h * 0.5f, true);
    g.setGradientFill(grad);
    g.fillPath(p);
}

struct MapInfo
{
    double min, max;
    bool logarithmic;
};
constexpr std::array<MapInfo, HamloCompPanel::NumKnobs> kMaps { { { -60.0, 0.0, false }, { 1.0, 20.0, true }, { 0.0, 40.0, false },
                                                                   { 0.1, 100.0, true }, { 10.0, 1000.0, true }, { 0.0, 24.0, false } } };
constexpr std::array<const char*, HamloCompPanel::NumKnobs> kKnobLabels { "Threshold", "Ratio", "Knee", "Attack", "Release", "Makeup" };
const std::array<juce::StringArray, HamloCompPanel::NumSwitches> kSwitchOptions { juce::StringArray { "Peak", "RMS" },
                                                                                  juce::StringArray { "Clean", "Punch", "Glue" },
                                                                                  juce::StringArray { "Off", "On" } };
constexpr std::array<const char*, HamloCompPanel::NumSwitches> kSwitchLabels { "Detector", "Character", "Auto Makeup" };

// Layout (design coordinates, from style.css): content x 56..1124.
constexpr float kKnobY = 253.0f, kLabelY = 160.0f, kReadoutY = 337.0f;
float switchTop(int i) { return 372.0f + static_cast<float>(i) * 88.0f; }
constexpr float kSwitchDialX = 66.0f, kSwitchDialW = 230.0f;
const juce::Point<float> kLamp { 1074.0f, 453.0f }, kToggle { 1074.0f, 559.0f };
} // namespace

HamloCompPanel::HamloCompPanel()
{
    for (int i = 0; i < NumKnobs; ++i)
        knobs[static_cast<size_t>(i)] = { static_cast<Param>(i), { 56.0f + static_cast<float>(i) * (174.67f + 4.0f) + 87.3f, kKnobY } };
    const auto ticks = [](std::initializer_list<float> all, std::initializer_list<float> labelled)
    {
        std::vector<std::pair<float, bool>> t;
        for (float v : all)
            t.push_back({ v, std::find(labelled.begin(), labelled.end(), v) != labelled.end() });
        return t;
    };
    meters[0] = { { 331.0f, 385.0f, 326.0f, 238.0f }, -40.0f, 6.0f, 0.0f, -42.0f, "INPUT", 21.0f,
                  ticks({ -40, -35, -30, -25, -20, -15, -10, -8, -6, -4, -2, 0, 2, 4, 6 }, { -40, -30, -20, -10, 0, 6 }), 11, -42.0f };
    meters[1] = { { 673.0f, 385.0f, 326.0f, 238.0f }, -20.0f, 0.0f, -6.0f, 0.0f, "GAIN REDUCTION", 17.0f,
                  ticks({ -20, -18, -16, -14, -12, -10, -8, -6, -5, -4, -3, -2, -1, 0 }, { -20, -10, -5, -2, 0 }), 23, 0.0f };
    setOpaque(false);
    startTimerHz(30);
}

HamloCompPanel::~HamloCompPanel() = default;

const char* HamloCompPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "thresholdDb", "ratio", "kneeDb", "attackMs", "releaseMs", "makeupDb" };
    return keys[p];
}

const char* HamloCompPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "detector", "character", "autoMakeup" };
    return keys[s];
}

double HamloCompPanel::defaultFor(Param p)
{
    static constexpr double d[] { -30.0, 4.0, 6.0, 1.0, 80.0, 0.0 }; // = defaultModule(Compressor)
    return d[p];
}

juce::String HamloCompPanel::format(Param p, double v)
{
    switch (p)
    {
        case Threshold: case Knee: case Makeup: return juce::String(v, 1) + " dB";
        case Ratio: return juce::String(v, 1) + ":1";
        case Attack: return juce::String(v, 2) + " ms";
        case Release: return juce::String(juce::roundToInt(v)) + " ms";
        case NumKnobs: break;
    }
    return {};
}

double HamloCompPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.logarithmic ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloCompPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    return m.logarithmic ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
}

void HamloCompPanel::setNorm(Param p, double n, bool notify)
{
    values.knobs[static_cast<size_t>(p)] = valueFor(p, n);
    if (notify && onKnobChange)
        onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
    repaint();
}

void HamloCompPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return; // the drag owns the value until the gesture ends
    values = v;
    repaint();
}

juce::AffineTransform HamloCompPanel::toScreen() const
{
    return juce::AffineTransform::scale(scale).translated(offset);
}

juce::Point<float> HamloCompPanel::toDesign(juce::Point<float> p) const
{
    return (p - offset) / scale;
}

void HamloCompPanel::resized()
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

void HamloCompPanel::renderStatic(juce::Graphics& g) const
{
    const juce::Rectangle<float> stage(0.0f, 0.0f, kW, kH);
    // frame (16 px border, 135 degree steel gradient) and the inner panel
    {
        juce::ColourGradient border(juce::Colour(0xff444444), 0, 0, juce::Colour(0xff3a3a38), kW, kH, false);
        border.addColour(0.30, juce::Colour(0xff0a0a0a));
        border.addColour(0.55, juce::Colour(0xff2b2b2a));
        border.addColour(0.80, juce::Colour(0xff050505));
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(stage.translated(0, 14).reduced(6), 12.0f);
        g.setGradientFill(border);
        g.fillRoundedRectangle(stage, 10.0f);
        const auto inner = stage.reduced(16.0f);
        juce::ColourGradient fill(juce::Colour(0xff30302e), kW * 0.5f, inner.getY(), juce::Colour(0xff111211), kW * 0.5f, inner.getY() + kW * 0.62f, true);
        fill.addColour(0.45, juce::Colour(0xff1d1e1d));
        g.setGradientFill(fill);
        g.fillRoundedRectangle(inner, 4.0f);

        g.saveState();
        g.reduceClipRegion(inner.toNearestInt());
        juce::Random r(2024);
        // mottling and fine grain (the design's two turbulence layers)
        for (int i = 0; i < 26; ++i)
        {
            const float x = inner.getX() + r.nextFloat() * inner.getWidth(), y = inner.getY() + r.nextFloat() * inner.getHeight();
            const float rad = 60.0f + r.nextFloat() * 180.0f;
            juce::ColourGradient blob(juce::Colours::black.withAlpha(0.22f), x, y, juce::Colours::transparentBlack, x + rad, y, true);
            g.setGradientFill(blob);
            g.fillEllipse(x - rad, y - rad, 2 * rad, 2 * rad);
        }
        for (int i = 0; i < 9000; ++i)
        {
            g.setColour(juce::Colour::fromFloatRGBA(0.75f, 0.72f, 0.65f, 0.04f + r.nextFloat() * 0.07f));
            g.fillRect(inner.getX() + r.nextFloat() * inner.getWidth(), inner.getY() + r.nextFloat() * inner.getHeight(), 1.0f, 1.0f);
        }
        // wear, after drawWear() in app.js: oily smudges, rust near the edges, chipped paint, scratches, pitting
        const float w = inner.getWidth(), h = inner.getHeight(), ox = inner.getX(), oy = inner.getY();
        const auto edgeBias = [&r]
        {
            const float e = r.nextFloat();
            return e < 0.5f ? r.nextFloat() : (r.nextFloat() < 0.5f ? r.nextFloat() * 0.12f : 1.0f - r.nextFloat() * 0.12f);
        };
        for (int i = 0; i < 14; ++i)
        {
            const float x = ox + r.nextFloat() * w, y = oy + r.nextFloat() * h, rad = 40.0f + r.nextFloat() * 140.0f;
            const bool lite = r.nextFloat() < 0.5f;
            juce::ColourGradient s(lite ? juce::Colour::fromFloatRGBA(0.78f, 0.75f, 0.63f, 0.045f) : juce::Colours::black.withAlpha(0.22f), x, y,
                                   juce::Colours::transparentBlack, x + rad, y, true);
            g.setGradientFill(s);
            g.fillEllipse(x - rad, y - rad, 2 * rad, 2 * rad);
        }
        for (int i = 0; i < 46; ++i)
        {
            const bool nearX = r.nextFloat() < 0.5f;
            const float x = ox + (nearX ? edgeBias() : r.nextFloat()) * w, y = oy + (nearX ? r.nextFloat() : edgeBias()) * h;
            const float rad = 4.0f + r.nextFloat() * 34.0f;
            juce::ColourGradient s(rgba(105 + r.nextInt(30), 48 + r.nextInt(16), 18, 0.12f + r.nextFloat() * 0.2f),
                                   x, y, rgba(90, 40, 14, 0.0f), x + rad, y, true);
            s.addColour(0.6, rgba(90, 40, 14, 0.12f));
            g.setGradientFill(s);
            g.fillEllipse(x - rad, y - rad, 2 * rad, 2 * rad);
        }
        for (int i = 0; i < 30; ++i)
        {
            const int side = r.nextInt(4);
            const float t = r.nextFloat();
            const float x = ox + (side == 0 ? t * w : side == 1 ? w - r.nextFloat() * 8 : side == 2 ? t * w : r.nextFloat() * 8);
            const float y = oy + (side == 0 ? r.nextFloat() * 8 : side == 1 ? t * h : side == 2 ? h - r.nextFloat() * 8 : t * h);
            const float s = 1.5f + r.nextFloat() * 5.0f;
            juce::Path chip;
            for (int k = 0; k < 7; ++k)
            {
                const float a = static_cast<float>(k) / 7.0f * 2.0f * kPi, rr = s * (0.5f + r.nextFloat() * 0.7f);
                if (k == 0)
                    chip.startNewSubPath(x + std::cos(a) * rr, y + std::sin(a) * rr * 0.7f);
                else
                    chip.lineTo(x + std::cos(a) * rr, y + std::sin(a) * rr * 0.7f);
            }
            chip.closeSubPath();
            g.setColour(r.nextFloat() < 0.55f ? rgba(120, 118, 110, 0.5f) : rgba(95, 46, 18, 0.6f));
            g.fillPath(chip);
        }
        for (int i = 0; i < 380; ++i)
        {
            const float x = ox + r.nextFloat() * w, y = oy + r.nextFloat() * h, a = r.nextFloat() * 2.0f * kPi;
            const float l = i < 24 ? 120.0f + r.nextFloat() * 380.0f : 6.0f + r.nextFloat() * 70.0f;
            g.setColour(rgba(200 + r.nextInt(40), 195 + r.nextInt(40), 170 + r.nextInt(30), 0.05f + r.nextFloat() * (i < 24 ? 0.16f : 0.26f)));
            juce::Path scratch;
            scratch.startNewSubPath(x, y);
            scratch.quadraticTo(x + std::cos(a) * l * 0.5f + (r.nextFloat() - 0.5f) * 6.0f, y + std::sin(a) * l * 0.5f + (r.nextFloat() - 0.5f) * 6.0f,
                                x + std::cos(a) * l, y + std::sin(a) * l);
            g.strokePath(scratch, juce::PathStrokeType(0.4f + r.nextFloat() * 0.7f));
        }
        for (int i = 0; i < 900; ++i)
        {
            g.setColour(r.nextFloat() < 0.5f ? juce::Colours::black.withAlpha(0.2f + r.nextFloat() * 0.4f)
                                             : rgba(210, 200, 170, 0.05f + r.nextFloat() * 0.2f));
            g.fillRect(ox + r.nextFloat() * w, oy + r.nextFloat() * h, 0.7f + r.nextFloat() * 1.6f, 0.7f + r.nextFloat() * 1.6f);
        }
        juce::ColourGradient vignette(juce::Colours::transparentBlack, ox + w * 0.5f, oy + h * 0.5f, juce::Colours::black.withAlpha(0.38f),
                                      ox + w * 0.5f + w * 0.62f, oy + h * 0.5f, true);
        g.setGradientFill(vignette);
        g.fillRect(inner);
        g.restoreState();
    }

    // bolts
    for (auto c : { juce::Point<float>(39, 39), juce::Point<float>(kW - 39, 39), juce::Point<float>(39, kH - 39), juce::Point<float>(kW - 39, kH - 39) })
    {
        hexagon(g, c, 30.0f, 30.0f, juce::Colour(0xff77776f), juce::Colour(0xff1c1c1a));
        g.setColour(juce::Colour(0xff1a1a19));
        g.fillEllipse(c.x - 5.5f, c.y - 5.5f, 11.0f, 11.0f);
    }

    // owner's scratched pictures in free corners of the steel
    steel::scratches(g, { { 1, { 255.0f, 16.0f, 82.0f, 120.0f }, -5.0f }, { 21, { 832.0f, 16.0f, 82.0f, 120.0f }, 4.0f },
                          { 10, { 236.0f, 598.0f, 64.0f, 92.0f }, -3.0f } });

    // nameplate
    {
        const juce::Rectangle<float> plate(590.0f - 185.0f, 41.0f, 370.0f, 92.0f);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(plate.translated(0, 6).expanded(2), 6.0f);
        juce::ColourGradient grad(juce::Colour(0xffd9d7ce), plate.getX(), plate.getY(), juce::Colour(0xffbdbcb2), plate.getRight(), plate.getBottom(), false);
        grad.addColour(0.45, juce::Colour(0xffa6a69d));
        grad.addColour(0.55, juce::Colour(0xff8d8d85));
        g.setColour(juce::Colour(0xff1a1a19));
        g.fillRoundedRectangle(plate.expanded(2), 6.0f);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(plate, 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        for (float x = plate.getX(); x < plate.getRight(); x += 3.0f)
            g.drawVerticalLine(juce::roundToInt(x), plate.getY(), plate.getBottom());
        for (float rx : { plate.getX() + 21.0f, plate.getRight() - 21.0f })
        {
            juce::ColourGradient rivet(juce::Colour(0xfffafafa), rx - 3, plate.getCentreY() - 4, juce::Colour(0xff3c3c38), rx + 9, plate.getCentreY() + 9, true);
            g.setGradientFill(rivet);
            g.fillEllipse(rx - 9, plate.getCentreY() - 9, 18, 18);
        }
        g.setColour(juce::Colour(0xff2a2926));
        g.setFont(stencilFont(44.0f).withHorizontalScale(0.9f));
        g.drawText("HamloComp", plate.withHeight(50.0f).translated(0, 4), juce::Justification::centred);
        g.setFont(stencilFont(13.0f).withKerningFactor(0.12f));
        auto row = plate.withTrimmedTop(54.0f).withHeight(17.0f);
        g.drawText("MODEL SK-1", row.withTrimmedRight(row.getWidth() * 0.5f + 16.0f), juce::Justification::centredRight);
        juce::Path star;
        star.addStar({ row.getCentreX(), row.getCentreY() }, 5, 3.0f, 7.5f, 0.0f);
        g.setColour(juce::Colour(0xffb3261e));
        g.fillPath(star);
        const auto no = juce::Rectangle<float>(row.getCentreX() + 16.0f, row.getY(), 70.0f, 17.0f);
        g.setColour(juce::Colour(0xff8d8a7c));
        g.fillRoundedRectangle(no, 2.0f);
        g.setColour(juce::Colour(0xff2a2926));
        g.setFont(monoFont(13.0f));
        g.drawText(juce::String::fromUTF8("\xe2\x84\x96 0419"), no, juce::Justification::centred);
        g.setFont(stencilFont(11.5f).withKerningFactor(0.2f));
        g.drawText("MADE IN USSR", plate.withTrimmedTop(72.0f).withHeight(15.0f), juce::Justification::centred);
    }

    // knob labels and engraved scales
    for (const auto& k : knobs)
    {
        engraved(g, juce::String(kKnobLabels[static_cast<size_t>(k.param)]).toUpperCase(),
                 juce::Rectangle<float>(k.centre.x - 87.0f, kLabelY - 17.0f, 174.0f, 34.0f), 24.0f, stencil, juce::Justification::centred, 0.1f);
        for (int i = 0; i < 21; ++i)
        {
            const float a = (-135.0f + 270.0f / 20.0f * static_cast<float>(i)) * kPi / 180.0f;
            const bool major = i % 2 == 0;
            const float r1 = 62.0f, r2 = major ? 71.0f : 67.0f;
            g.setColour(juce::Colour(0xffefe3bd));
            g.drawLine(k.centre.x + std::sin(a) * r1, k.centre.y - std::cos(a) * r1, k.centre.x + std::sin(a) * r2, k.centre.y - std::cos(a) * r2,
                       major ? 2.4f : 1.4f);
        }
        // ring with its drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(k.centre.x - 50.0f, k.centre.y - 40.0f, 100.0f, 100.0f);
        g.setColour(juce::Colours::black);
        g.fillEllipse(k.centre.x - 49.0f, k.centre.y - 49.0f, 98.0f, 98.0f);
    }

    // switch labels and separators
    for (int i = 0; i < NumSwitches; ++i)
    {
        const float top = switchTop(i);
        if (i > 0)
        {
            g.setColour(juce::Colours::black);
            g.drawHorizontalLine(juce::roundToInt(top - 3.0f), 56.0f, 306.0f);
            g.setColour(juce::Colours::white.withAlpha(0.07f));
            g.drawHorizontalLine(juce::roundToInt(top - 4.0f), 56.0f, 306.0f);
        }
        engraved(g, juce::String(kSwitchLabels[static_cast<size_t>(i)]).toUpperCase(), juce::Rectangle<float>(56.0f, top, 250.0f, 22.0f), 16.0f);
    }

    // power label
    engraved(g, "ON", juce::Rectangle<float>(kLamp.x - 50.0f, 491.0f, 100.0f, 30.0f), 20.0f);

    // LCD window
    {
        const juce::Rectangle<float> lcd(1124.0f - 172.0f, 66.0f, 172.0f, 42.0f);
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(lcd.expanded(2), 4.0f);
        g.setColour(juce::Colour(0xff120b04));
        g.fillRoundedRectangle(lcd, 3.0f);
    }

    // meters: brass bezel and the aged dial paper
    for (const auto& m : meters)
    {
        const auto b = m.bezel;
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(b.translated(0, 10).expanded(2), 10.0f);
        juce::ColourGradient brass(juce::Colour(0xfff0d588), b.getX(), b.getY(), juce::Colour(0xffb8923f), b.getRight(), b.getBottom(), false);
        brass.addColour(0.45, juce::Colour(0xffa07a2e));
        brass.addColour(0.55, juce::Colour(0xff66491a));
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(b.expanded(2), 10.0f);
        g.setGradientFill(brass);
        g.fillRoundedRectangle(b, 9.0f);
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        for (float x = b.getX(); x < b.getRight(); x += 3.0f)
            g.drawVerticalLine(juce::roundToInt(x), b.getY() + 2, b.getBottom() - 2);
        for (float sx : { b.getX() + 11.5f, b.getRight() - 11.5f })
        {
            juce::ColourGradient screw(juce::Colour(0xfffff1c4), sx - 2, b.getBottom() - 11, juce::Colour(0xff3a2a0a), sx + 5, b.getBottom() - 3, true);
            g.setGradientFill(screw);
            g.fillEllipse(sx - 5.5f, b.getBottom() - 14.0f, 11.0f, 11.0f);
        }

        const auto face = b.reduced(15.0f);
        g.setColour(juce::Colour(0xff1c1408));
        g.fillRoundedRectangle(face.expanded(3), 4.0f);
        g.saveState();
        g.reduceClipRegion(face.toNearestInt());
        g.addTransform(juce::AffineTransform::scale(face.getWidth() / 300.0f, face.getHeight() / 200.0f).translated(face.getX(), face.getY()));
        constexpr float W = 300.0f, H = 200.0f, PX = W / 2, PY = H + 60.0f, A = 0.9f, R = 190.0f;
        const auto ang = [&](float v) { return -A + (v - m.min) / (m.max - m.min) * 2.0f * A; };
        const auto pt = [&](float v, float r) { return juce::Point<float>(PX + std::sin(ang(v)) * r, PY - std::cos(ang(v)) * r); };
        juce::ColourGradient paper(juce::Colour(0xfff1dfa0), W / 2, H * 0.5f, juce::Colour(0xffa98b4a), W / 2 + 230.0f, H * 0.55f, true);
        paper.addColour(0.55, juce::Colour(0xffdcc57c));
        g.setGradientFill(paper);
        g.fillRect(0.0f, 0.0f, W, H);
        juce::Random r(m.seed);
        for (int i = 0; i < 16; ++i)
        {
            const float x = r.nextFloat() * W, y = r.nextFloat() * H, rad = 14.0f + r.nextFloat() * 50.0f;
            juce::ColourGradient stain(rgba(120, 80, 20, 0.05f + r.nextFloat() * 0.09f), x, y, rgba(120, 80, 20, 0.0f), x + rad, y, true);
            g.setGradientFill(stain);
            g.fillEllipse(x - rad, y - rad, 2 * rad, 2 * rad);
        }
        for (int i = 0; i < 160; ++i)
        {
            g.setColour(rgba(70, 45, 15, 0.12f + r.nextFloat() * 0.3f));
            g.fillRect(r.nextFloat() * W, r.nextFloat() * H, 0.8f + r.nextFloat() * 1.2f, 0.8f + r.nextFloat() * 1.2f);
        }
        const juce::Colour ink(0xff1b1710), redInk(0xffa82820);
        juce::Path arc;
        arc.addCentredArc(PX, PY, R, R, 0.0f, ang(m.min), ang(m.redFrom), true);
        g.setColour(ink);
        g.strokePath(arc, juce::PathStrokeType(3.0f));
        juce::Path redArc;
        redArc.addCentredArc(PX, PY, R - 1, R - 1, 0.0f, ang(m.redFrom), ang(m.max), true);
        g.setColour(redInk);
        g.strokePath(redArc, juce::PathStrokeType(6.0f));
        juce::Path inner;
        inner.addCentredArc(PX, PY, R - 12, R - 12, 0.0f, ang(m.min), ang(m.max), true);
        g.setColour(ink);
        g.strokePath(inner, juce::PathStrokeType(1.6f));
        g.setFont(stencilFont(15.0f, true));
        for (const auto& [v, labelled] : m.ticks)
        {
            const bool red = v >= m.redFrom;
            g.setColour(red ? redInk : ink);
            g.drawLine(juce::Line<float>(pt(v, R - 12), pt(v, R + (labelled ? 10.0f : 5.0f))), labelled ? 2.4f : 1.4f);
            if (labelled)
            {
                const auto p = pt(v, R - 28);
                g.drawText(v > 0 ? "+" + juce::String(static_cast<int>(v)) : juce::String(static_cast<int>(v)),
                           juce::Rectangle<float>(p.x - 18, p.y - 9, 36, 18), juce::Justification::centred);
            }
        }
        g.setColour(ink);
        g.setFont(stencilFont(m.titleSize, true));
        g.drawText(m.title, juce::Rectangle<float>(0, 124 - 14, W, 28), juce::Justification::centred);
        g.setFont(stencilFont(14.0f, false));
        g.drawText("dB", juce::Rectangle<float>(0, 144 - 9, W, 18), juce::Justification::centred);
        g.restoreState();
    }
}

void HamloCompPanel::drawKnob(juce::Graphics& g, const Knob& k) const
{
    const float angle = (-135.0f + static_cast<float>(normFor(k.param)) * 270.0f) * kPi / 180.0f;
    const auto c = k.centre;
    steel::brassKnob(g, c, angle, 1.0f, 1.0f, steel::KnobAsset::AluminumBrassInsert, steel::PointerAsset::WhiteEnamel);

    // readout
    g.setFont(stencilFont(25.0f));
    const auto r = juce::Rectangle<float>(c.x - 80.0f, kReadoutY - 16.0f, 160.0f, 32.0f);
    g.setColour(juce::Colours::black);
    g.drawText(format(k.param, values.knobs[static_cast<size_t>(k.param)]), r.translated(0, 1), juce::Justification::centred);
    g.setColour(stencil);
    g.drawText(format(k.param, values.knobs[static_cast<size_t>(k.param)]), r, juce::Justification::centred);
}

void HamloCompPanel::drawSwitch(juce::Graphics& g, int which) const
{
    const auto& options = kSwitchOptions[static_cast<size_t>(which)];
    const int n = options.size(), selected = juce::jlimit(0, n - 1, values.switches[static_cast<size_t>(which)]);
    const float dialTop = switchTop(which) + 22.0f;
    for (int i = 0; i < n; ++i)
    {
        const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
        const auto r = juce::Rectangle<float>(x - 40.0f, dialTop, 80.0f, 18.0f);
        g.setFont(stencilFont(13.5f).withKerningFactor(0.1f));
        if (i == selected)
        {
            g.setColour(juce::Colour(0x55ffe9a0));
            g.drawText(options[i].toUpperCase(), r.expanded(1.0f), juce::Justification::centred);
            g.setColour(juce::Colour(0xfffff6d0));
        }
        else
            g.setColour(stencilDim);
        g.drawText(options[i].toUpperCase(), r, juce::Justification::centred);
    }
    const juce::Point<float> base(kSwitchDialX + kSwitchDialW * 0.5f, dialTop + 46.0f);
    const float a = (n == 1 ? 0.0f : (-38.0f + 76.0f / static_cast<float>(n - 1) * static_cast<float>(selected))) * kPi / 180.0f;
    steel::lever(g, base, a, 1.0f);
}

void HamloCompPanel::drawPower(juce::Graphics& g) const
{
    steel::lamp(g, kLamp, values.on, 1.0f);
    steel::lever(g, kToggle, values.on ? 0.0f : kPi, 1.15f,
                 values.on ? steel::LeverPosition::Up : steel::LeverPosition::Down);
}

void HamloCompPanel::drawMeterDynamic(juce::Graphics& g, const Meter& m) const
{
    const auto face = m.bezel.reduced(15.0f);
    g.saveState();
    g.reduceClipRegion(face.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(face.getWidth() / 300.0f, face.getHeight() / 200.0f).translated(face.getX(), face.getY()));
    constexpr float W = 300.0f, H = 200.0f, PX = W / 2, PY = H + 60.0f, A = 0.9f, R = 190.0f;
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float tremor = static_cast<float>((std::sin(now / 83.0) + std::sin(now / 31.0) * 0.6) * 0.03 * (m.max - m.min) / 46.0);
    const float v = juce::jlimit(m.min - 2.0f, m.max + 1.5f, m.shown + tremor);
    const float a = -A + (v - m.min) / (m.max - m.min) * 2.0f * A;
    const juce::Point<float> tip(PX + std::sin(a) * (R + 6), PY - std::cos(a) * (R + 6));
    g.setColour(juce::Colour(0x552a1a0a));
    g.drawLine(PX + 4, PY + 4, tip.x + 4, tip.y + 5, 3.0f);
    g.setColour(juce::Colour(0xff0d0b08));
    g.drawLine(PX, PY, tip.x, tip.y, 2.2f);
    // bottom plate hiding the pivot
    juce::Path plate;
    plate.startNewSubPath(W / 2 - 62, H);
    plate.quadraticTo(W / 2, H - 46, W / 2 + 62, H);
    plate.closeSubPath();
    juce::ColourGradient pl(juce::Colour(0xff2d2a22), 0, H - 30, juce::Colour(0xff0c0b09), 0, H, false);
    g.setGradientFill(pl);
    g.fillPath(plate);
    g.setColour(juce::Colour(0xffb79b4a));
    g.fillEllipse(W / 2 - 6, H - 15, 12, 12);
    g.setColour(juce::Colour(0xff6a5520));
    g.fillRect(W / 2 - 4, H - 10, 8.0f, 2.0f);
    // scratched glass, glare, edge shadow
    juce::Random r(m.seed + 1000);
    for (int i = 0; i < 26; ++i)
    {
        const float x = r.nextFloat() * W, y = r.nextFloat() * H, ang = r.nextFloat() * kPi, l = 8.0f + r.nextFloat() * (i < 4 ? 130.0f : 38.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f + r.nextFloat() * 0.28f));
        g.drawLine(x, y, x + std::cos(ang) * l, y + std::sin(ang) * l, 0.5f + r.nextFloat() * 0.5f);
    }
    juce::ColourGradient glare(juce::Colour(0x40ffffff), 0, 0, juce::Colour(0x00ffffff), W * 0.41f, H * 0.41f, false);
    glare.addColour(0.97, juce::Colour(0x12ffffff));
    g.setGradientFill(glare);
    g.fillRect(0.0f, 0.0f, W, H);
    juce::ColourGradient edge(juce::Colour(0x00000000), W / 2, H / 2, juce::Colour(0x662a1a0a), W / 2 + 215.0f, H / 2, true);
    edge.addColour(80.0 / 215.0, juce::Colour(0x00000000));
    g.setGradientFill(edge);
    g.fillRect(0.0f, 0.0f, W, H);
    g.restoreState();
}

void HamloCompPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    for (const auto& k : knobs)
        drawKnob(g, k);
    for (int s = 0; s < NumSwitches; ++s)
        drawSwitch(g, s);
    drawPower(g);
    for (const auto& m : meters)
        drawMeterDynamic(g, m);
    // LCD text
    const juce::Rectangle<float> lcd(1124.0f - 172.0f, 66.0f, 172.0f, 42.0f);
    const auto text = "GR " + juce::String(std::abs(grText), 1) + " dB";
    g.setFont(monoFont(21.0f));
    g.setColour(juce::Colour(0x66ff9a1f));
    g.drawText(text, lcd.translated(0, 0.5f).expanded(1.0f), juce::Justification::centred);
    g.setColour(juce::Colour(0xffffb347));
    g.drawText(text, lcd, juce::Justification::centred);
}

void HamloCompPanel::timerCallback()
{
    if (!isShowing())
        return;
    float inDb = -60.0f, grDb = 0.0f;
    if (readMeters)
        std::tie(inDb, grDb) = readMeters();
    if (!values.on)
        grDb = 0.0f;
    // the design's per-frame ballistics (60 fps) applied at 30 Hz
    auto& in = meters[0];
    const float inTarget = juce::jlimit(-60.0f, 12.0f, inDb);
    in.shown += (inTarget - in.shown) * (inTarget > in.shown ? 0.80f : 0.17f);
    auto& gr = meters[1];
    const float grTarget = -std::max(0.0f, grDb);
    gr.shown += (grTarget - gr.shown) * (grTarget < gr.shown ? 0.70f : 0.19f);
    grText += (grTarget - grText) * 0.35f;
    repaint();
}

void HamloCompPanel::settleMeters(float inputDb, float reductionDb)
{
    meters[0].shown = juce::jlimit(-42.0f, 7.0f, inputDb);
    meters[1].shown = juce::jlimit(-21.0f, 0.0f, -reductionDb);
    grText = -reductionDb;
    repaint();
}

int HamloCompPanel::knobAt(juce::Point<float> d) const
{
    for (const auto& k : knobs)
        if (d.getDistanceFrom(k.centre) <= 60.0f)
            return k.param;
    return -1;
}

bool HamloCompPanel::hitSwitch(juce::Point<float> d, int& which, int& option) const
{
    for (int s = 0; s < NumSwitches; ++s)
    {
        const float dialTop = switchTop(s) + 22.0f;
        if (d.y < dialTop || d.y > dialTop + 66.0f || d.x < kSwitchDialX || d.x > kSwitchDialX + kSwitchDialW)
            continue;
        which = s;
        const int n = kSwitchOptions[static_cast<size_t>(s)].size();
        option = -1;
        if (d.y <= dialTop + 20.0f)
            for (int i = 0; i < n; ++i)
            {
                const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
                if (std::abs(d.x - x) <= 40.0f)
                    option = i;
            }
        if (option < 0)
            option = (values.switches[static_cast<size_t>(s)] + 1) % n; // the lever: next position
        return true;
    }
    return false;
}

bool HamloCompPanel::hitPower(juce::Point<float> d) const
{
    return d.getDistanceFrom(kLamp) <= 34.0f || juce::Rectangle<float>(kToggle.x - 22, kToggle.y - 30, 44, 60).contains(d);
}

void HamloCompPanel::mouseDown(const juce::MouseEvent& e)
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
    int which = -1, option = -1;
    if (hitSwitch(d, which, option))
    {
        values.switches[static_cast<size_t>(which)] = option;
        if (onSwitchChange)
            onSwitchChange(static_cast<Switch>(which), option);
        repaint();
        return;
    }
    if (hitPower(d))
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    for (const auto& k : knobs) // a click on a readout types a value
        if (juce::Rectangle<float>(k.centre.x - 80.0f, kReadoutY - 16.0f, 160.0f, 32.0f).contains(d))
        {
            editReadout(k.param);
            return;
        }
}

void HamloCompPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
    setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
}

void HamloCompPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloCompPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        values.knobs[static_cast<size_t>(k)] = defaultFor(static_cast<Param>(k));
        if (onKnobChange)
            onKnobChange(static_cast<Param>(k), values.knobs[static_cast<size_t>(k)]);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        repaint();
    }
}

void HamloCompPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        const double step = (e.mods.isShiftDown() ? 0.004 : 0.02) * (wheel.deltaY > 0 ? 1.0 : -1.0);
        setNorm(static_cast<Param>(k), normFor(static_cast<Param>(k)) + step, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloCompPanel::editReadout(Param p)
{
    const auto& k = knobs[static_cast<size_t>(p)];
    const auto area = juce::Rectangle<float>(k.centre.x - 70.0f, kReadoutY - 16.0f, 140.0f, 32.0f).transformedBy(toScreen()).toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(stencilFont(22.0f * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    typing->setText(juce::String(values.knobs[static_cast<size_t>(p)], p == Attack ? 2 : 1), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloCompPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, closeEditor]
    {
        if (typing == nullptr)
            return;
        const double v = typing->getText().retainCharacters("-0123456789.").getDoubleValue();
        const auto& m = kMaps[static_cast<size_t>(p)];
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
        if (onKnobChange)
            onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        closeEditor();
        repaint();
    };
    typing->onReturnKey = commit;
    typing->onFocusLost = commit;
    typing->onEscapeKey = closeEditor;
    addAndMakeVisible(*typing);
    typing->grabKeyboardFocus();
}
} // namespace hpeq::ui
