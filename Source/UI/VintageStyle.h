#pragma once

#include <cmath>

#include "UI/Theme.h"

namespace hpeq::ui
{
// Owner request (module pass, compressor): knobs and needle meters in the style of old Soviet analogue
// studio gear — grey hammertone panel, black bakelite knobs with a white index line, cream meter faces
// with a black needle. Drawn as vectors (sharp at any scale); no bitmaps.
namespace vintage
{
const juce::Colour panel { 0xff8e9590 };     // grey-green hammertone
const juce::Colour panelDark { 0xff5f6661 };
const juce::Colour engraving { 0xff1d2120 };
const juce::Colour cream { 0xfff1e6c8 };
const juce::Colour needle { 0xff141414 };
const juce::Colour red { 0xffb8322a };
const juce::Colour bakelite { 0xff1b1b1b };

inline void drawPlate(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient grad(panel.brighter(0.08f), r.getX(), r.getY(), panel.darker(0.12f), r.getRight(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 5.0f);
    // hammertone speckle, deterministic
    juce::Random random(1977);
    g.setColour(panelDark.withAlpha(0.18f));
    for (int i = 0; i < static_cast<int>(r.getWidth() * r.getHeight() / 90.0f); ++i)
        g.fillEllipse(r.getX() + random.nextFloat() * r.getWidth(), r.getY() + random.nextFloat() * r.getHeight(), 1.6f, 1.6f);
    g.setColour(panelDark);
    g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.2f);
    // four screws
    for (auto p : { r.getTopLeft().translated(9, 9), r.getTopRight().translated(-9, 9), r.getBottomLeft().translated(9, -9), r.getBottomRight().translated(-9, -9) })
    {
        g.setColour(panel.brighter(0.25f));
        g.fillEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
        g.setColour(panelDark.darker(0.3f));
        g.drawEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f, 0.8f);
        g.drawLine(p.x - 2.6f, p.y - 1.0f, p.x + 2.6f, p.y + 1.0f, 1.0f);
    }
}

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(4.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + position * (endAngle - startAngle);

        // engraved scale: 11 ticks, the ends longer
        g.setColour(engraving);
        for (int t = 0; t <= 10; ++t)
        {
            const float a = startAngle + (endAngle - startAngle) * static_cast<float>(t) / 10.0f;
            const float r0 = radius * (t % 5 == 0 ? 0.80f : 0.86f), r1 = radius * 0.98f;
            g.drawLine(c.x + r0 * std::sin(a), c.y - r0 * std::cos(a), c.x + r1 * std::sin(a), c.y - r1 * std::cos(a), t % 5 == 0 ? 1.6f : 1.0f);
        }

        // fluted skirt
        const float skirt = radius * 0.74f;
        g.setColour(bakelite.brighter(0.08f));
        g.fillEllipse(c.x - skirt, c.y - skirt, 2 * skirt, 2 * skirt);
        g.setColour(juce::Colours::black);
        for (int f = 0; f < 24; ++f)
        {
            const float a = angle + juce::MathConstants<float>::twoPi * static_cast<float>(f) / 24.0f;
            g.drawLine(c.x + skirt * 0.82f * std::sin(a), c.y - skirt * 0.82f * std::cos(a), c.x + skirt * std::sin(a), c.y - skirt * std::cos(a), 1.2f);
        }
        // cap with a soft highlight
        const float cap = radius * 0.56f;
        juce::ColourGradient grad(juce::Colour(0xff4a4a4a), c.x - cap * 0.5f, c.y - cap * 0.7f, bakelite, c.x + cap * 0.4f, c.y + cap * 0.6f, true);
        g.setGradientFill(grad);
        g.fillEllipse(c.x - cap, c.y - cap, 2 * cap, 2 * cap);
        g.setColour(juce::Colours::black);
        g.drawEllipse(c.x - cap, c.y - cap, 2 * cap, 2 * cap, 1.0f);
        // white index line
        g.setColour(slider.isEnabled() ? juce::Colour(0xfff4f1e8) : juce::Colours::grey);
        g.drawLine(c.x + cap * 0.15f * std::sin(angle), c.y - cap * 0.15f * std::cos(angle), c.x + skirt * 0.95f * std::sin(angle),
                   c.y - skirt * 0.95f * std::cos(angle), 2.4f);
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox(slider);
        l->setColour(juce::Label::textColourId, engraving);
        l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        return l;
    }
};

// Needle meter with a cream face. Scale minDb..maxDb; the needle rests at `restDb` and moves with
// VU-like ballistics (about 300 ms to 99 % at the 15 Hz UI refresh). Red zone above `redFromDb`
// (input) or the whole scale tinted for gain reduction.
class NeedleMeter final : public juce::Component
{
public:
    NeedleMeter(juce::String caption, float minDb, float maxDb, float restDb, std::vector<float> marks, float redFromDb)
        : title(std::move(caption)), lo(minDb), hi(maxDb), rest(restDb), labels(std::move(marks)), red(redFromDb), shown(restDb)
    {
        setInterceptsMouseClicks(false, false);
    }

    void setValueDb(float db)
    {
        target = juce::jlimit(lo, hi, db);
        shown += (target - shown) * 0.35f; // per UI frame
        repaint();
    }
    float shownDb() const noexcept { return shown; }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xff2b2b2b));
        g.fillRoundedRectangle(r, 4.0f);
        auto face = r.reduced(5.0f);
        juce::ColourGradient grad(cream, face.getCentreX(), face.getY(), cream.darker(0.12f), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(face, 3.0f);

        // Geometry: the arc and its labels stay inside the face.
        const float span = 0.70f; // radians each side
        const juce::Point<float> pivot(face.getCentreX(), face.getBottom() + face.getHeight() * 0.18f);
        const float radius = juce::jmin((face.getWidth() * 0.5f - 8.0f) / std::sin(span), pivot.y - face.getY() - 10.0f);
        const auto angleFor = [&](float db) { return -span + 2.0f * span * (db - lo) / (hi - lo); };
        // red zone
        if (red < hi)
        {
            juce::Path arc;
            arc.addCentredArc(pivot.x, pivot.y, radius * 0.9f, radius * 0.9f, 0.0f, angleFor(red), angleFor(hi), true);
            g.setColour(vintage::red);
            g.strokePath(arc, juce::PathStrokeType(4.0f));
        }
        // scale
        juce::Path scale;
        scale.addCentredArc(pivot.x, pivot.y, radius * 0.86f, radius * 0.86f, 0.0f, -span, span, true);
        g.setColour(engraving);
        g.strokePath(scale, juce::PathStrokeType(1.2f));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        for (float m : labels)
        {
            const float a = angleFor(m);
            const float r0 = radius * 0.86f, r1 = radius * 0.95f;
            g.drawLine(pivot.x + r0 * std::sin(a), pivot.y - r0 * std::cos(a), pivot.x + r1 * std::sin(a), pivot.y - r1 * std::cos(a), 1.2f);
            const float rt = radius * 0.78f;
            g.drawText(juce::String(static_cast<int>(std::lround(m))), juce::Rectangle<float>(pivot.x + rt * std::sin(a) - 14.0f, pivot.y - rt * std::cos(a) - 7.0f, 28.0f, 12.0f),
                       juce::Justification::centred);
        }
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(title, face.withTrimmedTop(face.getHeight() * 0.68f).removeFromTop(14.0f), juce::Justification::centred);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("dB", face.withTrimmedTop(face.getHeight() * 0.80f).removeFromTop(12.0f), juce::Justification::centred);

        // needle
        g.saveState();
        g.reduceClipRegion(face.toNearestInt());
        const float a = angleFor(shown);
        g.setColour(needle);
        g.drawLine(pivot.x, pivot.y, pivot.x + radius * 0.97f * std::sin(a), pivot.y - radius * 0.97f * std::cos(a), 1.6f);
        g.restoreState();
        g.setColour(juce::Colour(0xff2b2b2b));
        g.fillRect(face.getX(), face.getBottom() - 6.0f, face.getWidth(), 6.0f);
    }

private:
    juce::String title;
    float lo, hi, rest;
    std::vector<float> labels;
    float red, shown, target = 0.0f;
};
} // namespace vintage
} // namespace hpeq::ui
