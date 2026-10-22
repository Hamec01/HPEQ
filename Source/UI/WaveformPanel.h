#pragma once

#include "UI/AnalysisModel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Top strip: real input (grey) and processed output (cyan) PCM of the last `kSeconds`, sample-
// aligned (no plugin latency), min/max per pixel column. Not live: a snapshot is taken every
// `kRefreshSeconds` (owner request: a calm, zoomed-out view to compare changes), or at once when the
// mode or size changes. Delta shows output - input of the same samples. Vertical scale is linear full
// scale (+-1 = 0 dBFS).
class WaveformPanel final : public juce::Component
{
public:
    enum class Mode { Overlay, Delta };
    static constexpr double kSeconds = 5.0, kRefreshSeconds = 5.0;

    WaveformPanel(const AnalysisModel& model, std::function<double()> sampleRate) : analysis(model), rate(std::move(sampleRate)) {}

    Mode mode = Mode::Overlay;

    // Called by the editor's timer: takes a new snapshot when it is due (or forced) and repaints only then.
    void tick(bool force = false)
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (!force && !needsSnapshot && now - lastSnapshotMs < kRefreshSeconds * 1000.0)
            return;
        if (analysis.frozen && !force && !needsSnapshot)
            return;
        takeSnapshot();
        lastSnapshotMs = now;
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
        auto area = getLocalBounds().toFloat();
        g.setColour(colours::panel);
        g.fillRoundedRectangle(area, 6.0f);
        auto plot = plotArea();

        g.setColour(colours::grid);
        g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
        for (int s = 1; s < static_cast<int>(kSeconds); ++s)
            g.drawVerticalLine(juce::roundToInt(plot.getX() + plot.getWidth() * static_cast<float>(s / kSeconds)), plot.getY(), plot.getBottom());
        g.setColour(colours::dim);
        g.setFont(10.0f);
        g.drawText("1", juce::Rectangle<float>(area.getX() + 4, plot.getY() - 6, 20, 12), juce::Justification::centredLeft);
        g.drawText("-1", juce::Rectangle<float>(area.getX() + 4, plot.getBottom() - 6, 20, 12), juce::Justification::centredLeft);

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
            drawColumns(outputColumns, colours::accent.withAlpha(0.75f));
        }
        else
            drawColumns(deltaColumns, colours::warning.withAlpha(0.85f));

        g.setFont(11.0f);
        auto legend = plot.removeFromTop(14.0f).removeFromLeft(420.0f).translated(6.0f, 1.0f);
        if (mode == Mode::Overlay)
        {
            g.setColour(colours::input);
            g.drawText("Input", legend.removeFromLeft(44.0f), juce::Justification::centredLeft);
            g.setColour(colours::accent);
            g.drawText("Processed", legend.removeFromLeft(70.0f), juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(colours::warning);
            g.drawText("Delta = processed - input", legend.removeFromLeft(170.0f), juce::Justification::centredLeft);
        }
        g.setColour(colours::dim);
        g.drawText("last " + juce::String(static_cast<int>(kSeconds)) + " s, new picture every " + juce::String(static_cast<int>(kRefreshSeconds)) + " s",
                   legend, juce::Justification::centredLeft);
        if (analysis.frozen)
        {
            g.setColour(colours::warning);
            g.drawText("FROZEN", getLocalBounds().toFloat().reduced(8.0f, 4.0f), juce::Justification::topRight);
        }
    }

private:
    juce::Rectangle<float> plotArea() const { return getLocalBounds().toFloat().reduced(24.0f, 5.0f); }

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
    double lastSnapshotMs = -1.0e12;
    bool needsSnapshot = true;
};
} // namespace hpeq::ui
