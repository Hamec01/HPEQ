#pragma once

#include "UI/AnalysisModel.h"
#include "UI/BlackSteel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Top strip: real input (grey) and processed output (green) PCM of the last `kSeconds`, sample-
// aligned (no plugin latency), min/max per pixel column, live (owner, EQ pass: real time again; a new
// picture at every editor tick, about 30 per second; FREEZE holds it). Delta shows output - input of the
// same samples. Vertical scale is linear full scale (+-1 = 0 dBFS).
class WaveformPanel final : public juce::Component
{
public:
    enum class Mode { Overlay, Delta };
    static constexpr double kSeconds = 5.0;

    WaveformPanel(const AnalysisModel& model, std::function<double()> sampleRate) : analysis(model), rate(std::move(sampleRate)) {}

    Mode mode = Mode::Overlay;

    // Called by the editor's timer: takes a new snapshot when it is due (or forced) and repaints only then.
    void tick(bool force = false)
    {
        if (analysis.frozen && !force && !needsSnapshot)
            return;
        takeSnapshot();
        needsSnapshot = false;
        repaint();
    }
    void setMode(Mode m)
    {
        mode = m;
        tick(true);
    }
    void resized() override { needsSnapshot = true; }

    void paint(juce::Graphics& g) override
    {
        // a green CRT in a gunmetal bezel (owner: as the module pages' scopes)
        auto area = getLocalBounds().toFloat();
        const auto screen = screenArea();
        steel::crtBezel(g, area, screen, juce::String::fromUTF8("SIGNAL \xc2\xb7 LAST 5 S"), area.getBottom() - 19.0f);
        steel::crtBegin(g, screen);
        auto plot = plotArea();

        g.setColour(colours::grid);
        g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
        for (int s = 1; s < static_cast<int>(kSeconds); ++s)
            g.drawVerticalLine(juce::roundToInt(plot.getX() + plot.getWidth() * static_cast<float>(s / kSeconds)), plot.getY(), plot.getBottom());
        g.setColour(colours::phosphor.withAlpha(0.45f));
        g.setFont(juce::FontOptions("Consolas", 10.0f, juce::Font::plain));
        g.drawText("1", juce::Rectangle<float>(screen.getX() + 3, plot.getY() - 2, 20, 12), juce::Justification::centredLeft);
        g.drawText("-1", juce::Rectangle<float>(screen.getX() + 3, plot.getBottom() - 10, 20, 12), juce::Justification::centredLeft);

        const auto y = [&](float v) { return plot.getCentreY() - juce::jlimit(-1.0f, 1.0f, v) * plot.getHeight() * 0.5f; };
        const auto drawColumns = [&](const std::vector<std::pair<float, float>>& columns, juce::Colour colour)
        {
            g.setColour(colour);
            for (size_t c = 0; c < columns.size(); ++c)
                g.drawVerticalLine(juce::roundToInt(plot.getX()) + static_cast<int>(c), y(columns[c].second),
                                   juce::jmax(y(columns[c].second) + 1.0f, y(columns[c].first)));
        };
        if (mode == Mode::Overlay)
        {
            drawColumns(inputColumns, colours::input.withAlpha(0.85f));
            drawColumns(outputColumns, colours::phosphor.withAlpha(0.75f));
        }
        else
            drawColumns(deltaColumns, colours::warning.withAlpha(0.85f));

        g.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::bold));
        auto legend = plot.removeFromTop(14.0f).removeFromLeft(440.0f).translated(6.0f, 1.0f);
        if (mode == Mode::Overlay)
        {
            g.setColour(colours::input);
            g.drawText("Input", legend.removeFromLeft(44.0f), juce::Justification::centredLeft);
            g.setColour(colours::phosphor);
            g.drawText("Processed", legend.removeFromLeft(70.0f), juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(colours::warning);
            g.drawText("Delta = processed - input", legend.removeFromLeft(170.0f), juce::Justification::centredLeft);
        }
        g.setColour(colours::phosphor.withAlpha(0.45f));
        g.drawText("REAL TIME", legend, juce::Justification::centredLeft);
        if (analysis.frozen)
        {
            g.setColour(colours::warning);
            g.drawText("FROZEN", screen.reduced(10.0f, 4.0f), juce::Justification::topRight);
        }
        steel::crtEnd(g, screen);
    }

private:
    juce::Rectangle<float> screenArea() const
    {
        return getLocalBounds().toFloat().withTrimmedLeft(22.0f).withTrimmedRight(22.0f).withTrimmedTop(9.0f).withTrimmedBottom(22.0f);
    }
    juce::Rectangle<float> plotArea() const { return screenArea().reduced(20.0f, 4.0f); }

    void takeSnapshot()
    {
        const auto plot = plotArea();
        const int samples = juce::jmin(AnalysisModel::kHistory, static_cast<int>(kSeconds * rate()));
        const int columns = juce::jmax(1, static_cast<int>(plot.getWidth()));
        const int first = AnalysisModel::kHistory - samples;
        for (auto* v : { &inputColumns, &outputColumns, &deltaColumns })
            v->assign(static_cast<size_t>(columns), { 1.0f, -1.0f });
        for (int c = 0; c < columns; ++c)
        {
            const int s0 = first + static_cast<int>(static_cast<long long>(samples) * c / columns);
            const int s1 = juce::jmax(s0 + 1, first + static_cast<int>(static_cast<long long>(samples) * (c + 1) / columns));
            auto& in = inputColumns[static_cast<size_t>(c)];
            auto& out = outputColumns[static_cast<size_t>(c)];
            auto& delta = deltaColumns[static_cast<size_t>(c)];
            for (int s = s0; s < s1; ++s)
            {
                const float x = analysis.input(s), yv = analysis.output(s), d = yv - x;
                in = { juce::jmin(in.first, x), juce::jmax(in.second, x) };
                out = { juce::jmin(out.first, yv), juce::jmax(out.second, yv) };
                delta = { juce::jmin(delta.first, d), juce::jmax(delta.second, d) };
            }
        }
    }

    const AnalysisModel& analysis;
    std::function<double()> rate;
    std::vector<std::pair<float, float>> inputColumns, outputColumns, deltaColumns;
    bool needsSnapshot = true;
};
} // namespace hpeq::ui
