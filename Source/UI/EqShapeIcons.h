#pragma once

#include <cmath>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/FxProjectState.h"

namespace hpeq::ui
{
// EQ pass: a small picture of each EQ shape's response (low frequencies left, boost up), for the shape
// chooser on the point page and the shape menus. `secondary` gets AllPass's phase twist (else empty).
inline juce::Path eqShapePath(EqShape shape, juce::Rectangle<float> r, juce::Path* secondary = nullptr)
{
    const auto y = [shape](double u)
    {
        const auto gauss = [](double v, double w) { return std::exp(-(v * v) / (w * w)); };
        const auto step = [](double v, double k) { return 1.0 / (1.0 + std::exp(-k * v)); };
        switch (shape)
        {
            case EqShape::Bell: return 0.5 - 0.36 * gauss(u - 0.5, 0.13);
            case EqShape::LowShelf: return 0.5 - 0.3 * (1.0 - step(u - 0.45, 18.0));
            case EqShape::HighShelf: return 0.5 - 0.3 * step(u - 0.55, 18.0);
            case EqShape::LowCut: return 0.42 + 0.5 * (1.0 - step(u - 0.32, 16.0));
            case EqShape::HighCut: return 0.42 + 0.5 * step(u - 0.68, 16.0);
            case EqShape::Notch: return 0.42 + 0.48 * gauss(u - 0.5, 0.045);
            case EqShape::BandPass: return 0.25 + 0.65 * (1.0 - gauss(u - 0.5, 0.2));
            case EqShape::Tilt: return 0.72 - 0.44 * u;
            case EqShape::AllPass: return 0.5;
        }
        return 0.5;
    };
    juce::Path p;
    constexpr int points = 48;
    for (int i = 0; i <= points; ++i)
    {
        const double u = static_cast<double>(i) / points;
        const juce::Point<float> at(r.getX() + static_cast<float>(u) * r.getWidth(), r.getY() + static_cast<float>(y(u)) * r.getHeight());
        if (i == 0)
            p.startNewSubPath(at);
        else
            p.lineTo(at);
    }
    if (secondary != nullptr)
    {
        secondary->clear();
        if (shape == EqShape::AllPass)
            for (int i = 0; i <= points; ++i)
            {
                const double u = static_cast<double>(i) / points;
                const juce::Point<float> at(r.getX() + static_cast<float>(u) * r.getWidth(),
                                            r.getY() + static_cast<float>(0.5 + 0.22 * std::sin(2.0 * juce::MathConstants<double>::pi * u)) * r.getHeight());
                if (i == 0)
                    secondary->startNewSubPath(at);
                else
                    secondary->lineTo(at);
            }
    }
    return p;
}

// The picture as a menu icon (PopupMenu::addItem's image).
inline std::unique_ptr<juce::Drawable> eqShapeIcon(EqShape shape, juce::Colour colour)
{
    auto group = std::make_unique<juce::DrawableComposite>();
    const juce::Rectangle<float> box(0.0f, 0.0f, 30.0f, 16.0f);
    auto frame = std::make_unique<juce::DrawablePath>();
    juce::Path baseline;
    baseline.startNewSubPath(0.0f, 8.0f);
    baseline.lineTo(30.0f, 8.0f);
    frame->setPath(baseline);
    frame->setFill(juce::FillType());
    frame->setStrokeFill(colour.withAlpha(0.25f));
    frame->setStrokeThickness(0.8f);
    group->addAndMakeVisible(frame.release());
    juce::Path twist;
    auto curve = std::make_unique<juce::DrawablePath>();
    curve->setPath(eqShapePath(shape, box.reduced(1.0f), &twist));
    curve->setFill(juce::FillType());
    curve->setStrokeFill(colour);
    curve->setStrokeThickness(1.6f);
    group->addAndMakeVisible(curve.release());
    if (!twist.isEmpty())
    {
        auto phase = std::make_unique<juce::DrawablePath>();
        phase->setPath(twist);
        phase->setFill(juce::FillType());
        phase->setStrokeFill(colour.withAlpha(0.6f));
        phase->setStrokeThickness(1.0f);
        group->addAndMakeVisible(phase.release());
    }
    group->setBoundingBox(box);
    return group;
}
} // namespace hpeq::ui
