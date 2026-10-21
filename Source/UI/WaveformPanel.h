#pragma once

#include "UI/AnalysisModel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Top strip: real input (grey) and processed output (cyan) PCM of the last `kSeconds`, sample-
// aligned (no plugin latency), min/max per pixel column. Delta shows output - input of the same
// samples. Vertical scale is linear full scale (+-1 = 0 dBFS).
class WaveformPanel final : public juce::Component
{
public:
    enum class Mode { Overlay, Delta };
    static constexpr double kSeconds = 1.5;

    WaveformPanel(const AnalysisModel& model, std::function<double()> sampleRate) : analysis(model), rate(std::move(sampleRate)) {}

    Mode mode = Mode::Overlay;

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour(colours::panel);
        g.fillRoundedRectangle(area, 6.0f);
        auto plot = area.reduced(28.0f, 8.0f);

        g.setColour(colours::grid);
        g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
        g.setColour(colours::dim);
        g.setFont(10.0f);
        g.drawText("1", juce::Rectangle<float>(area.getX() + 4, plot.getY() - 6, 20, 12), juce::Justification::centredLeft);
        g.drawText("0", juce::Rectangle<float>(area.getX() + 4, plot.getCentreY() - 6, 20, 12), juce::Justification::centredLeft);
        g.drawText("-1", juce::Rectangle<float>(area.getX() + 4, plot.getBottom() - 6, 20, 12), juce::Justification::centredLeft);

        const int samples = juce::jmin(AnalysisModel::kHistory, static_cast<int>(kSeconds * rate()));
        const int columns = juce::jmax(1, static_cast<int>(plot.getWidth()));
        const int first = AnalysisModel::kHistory - samples;
        const auto y = [&](float v) { return plot.getCentreY() - juce::jlimit(-1.0f, 1.0f, v) * plot.getHeight() * 0.5f; };

        const auto drawColumns = [&](auto sampleAt, juce::Colour colour)
        {
            g.setColour(colour);
            for (int c = 0; c < columns; ++c)
            {
                const int s0 = first + static_cast<int>(static_cast<long long>(samples) * c / columns);
                const int s1 = first + static_cast<int>(static_cast<long long>(samples) * (c + 1) / columns);
                float lo = 1.0f, hi = -1.0f;
                for (int s = s0; s < juce::jmax(s0 + 1, s1); ++s)
                {
                    const float v = sampleAt(s);
                    lo = juce::jmin(lo, v);
                    hi = juce::jmax(hi, v);
                }
                g.drawVerticalLine(juce::roundToInt(plot.getX()) + c, y(hi), juce::jmax(y(hi) + 1.0f, y(lo)));
            }
        };

        if (mode == Mode::Overlay)
        {
            drawColumns([&](int s) { return analysis.input(s); }, colours::input.withAlpha(0.85f));
            drawColumns([&](int s) { return analysis.output(s); }, colours::accent.withAlpha(0.75f));
        }
        else
            drawColumns([&](int s) { return analysis.output(s) - analysis.input(s); }, colours::warning.withAlpha(0.85f));

        g.setFont(11.0f);
        auto legend = plot.removeFromTop(16.0f).removeFromLeft(260.0f).translated(6.0f, 2.0f);
        if (mode == Mode::Overlay)
        {
            g.setColour(colours::input);
            g.drawText("Input (real PCM)", legend.removeFromLeft(110.0f), juce::Justification::centredLeft);
            g.setColour(colours::accent);
            g.drawText("Processed", legend, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(colours::warning);
            g.drawText("Delta = processed - input", legend, juce::Justification::centredLeft);
        }
        if (analysis.frozen)
        {
            g.setColour(colours::warning);
            g.drawText("FROZEN", getLocalBounds().toFloat().reduced(8.0f), juce::Justification::topRight);
        }
    }

private:
    const AnalysisModel& analysis;
    std::function<double()> rate;
};
} // namespace hpeq::ui
