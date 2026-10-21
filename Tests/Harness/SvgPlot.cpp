#include "Harness/SvgPlot.h"

#include <cmath>

namespace hpeq::test
{
bool writeSvgPlot(const juce::File& file, const PlotSpec& spec, const std::vector<PlotSeries>& series)
{
    constexpr double width = 900, height = 460, left = 70, right = 190, top = 40, bottom = 50;
    const double plotW = width - left - right, plotH = height - top - bottom;
    const auto xPos = [&](double x)
    {
        const double u = spec.logX ? (std::log(x) - std::log(spec.xMin)) / (std::log(spec.xMax) - std::log(spec.xMin))
                                   : (x - spec.xMin) / (spec.xMax - spec.xMin);
        return left + juce::jlimit(0.0, 1.0, u) * plotW;
    };
    const auto yPos = [&](double y)
    {
        return top + (1.0 - juce::jlimit(0.0, 1.0, (y - spec.yMin) / (spec.yMax - spec.yMin))) * plotH;
    };
    const auto fmt = [](double v) { return juce::String(v, 2); };

    juce::String svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height
        << "\" font-family=\"Segoe UI, Arial, sans-serif\" font-size=\"12\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n"
        << "<text x=\"" << left << "\" y=\"24\" font-size=\"15\" font-weight=\"600\">" << spec.title << "</text>\n";

    for (double t : spec.xTicks)
    {
        if (t < spec.xMin || t > spec.xMax)
            continue;
        const auto x = xPos(t);
        svg << "<line x1=\"" << fmt(x) << "\" y1=\"" << top << "\" x2=\"" << fmt(x) << "\" y2=\"" << top + plotH
            << "\" stroke=\"#e3e6ea\"/>\n<text x=\"" << fmt(x) << "\" y=\"" << top + plotH + 16
            << "\" text-anchor=\"middle\" fill=\"#555\">" << (t >= 1000 ? juce::String(t / 1000.0) + "k" : juce::String(t)) << "</text>\n";
    }
    for (double y = std::ceil(spec.yMin / spec.yStep) * spec.yStep; y <= spec.yMax + 1e-9; y += spec.yStep)
    {
        const auto py = yPos(y);
        svg << "<line x1=\"" << left << "\" y1=\"" << fmt(py) << "\" x2=\"" << left + plotW << "\" y2=\"" << fmt(py)
            << "\" stroke=\"" << (std::abs(y) < 1e-9 ? "#9aa3ad" : "#e3e6ea") << "\"/>\n<text x=\"" << left - 8 << "\" y=\""
            << fmt(py + 4) << "\" text-anchor=\"end\" fill=\"#555\">" << juce::String(y, 1) << "</text>\n";
    }
    svg << "<rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << plotW << "\" height=\"" << plotH
        << "\" fill=\"none\" stroke=\"#9aa3ad\"/>\n"
        << "<text x=\"" << left + plotW / 2 << "\" y=\"" << height - 10 << "\" text-anchor=\"middle\">" << spec.xLabel << "</text>\n"
        << "<text transform=\"translate(18 " << top + plotH / 2 << ") rotate(-90)\" text-anchor=\"middle\">" << spec.yLabel << "</text>\n";

    int legendRow = 0;
    for (const auto& s : series)
    {
        juce::String path;
        bool penDown = false;
        for (const auto& [x, y] : s.points)
        {
            if (!std::isfinite(y) || x < spec.xMin || x > spec.xMax)
            {
                penDown = false;
                continue;
            }
            path << (penDown ? " L" : " M") << fmt(xPos(x)) << " " << fmt(yPos(y));
            penDown = true;
        }
        svg << "<path d=\"" << path << "\" fill=\"none\" stroke=\"" << s.colour << "\" stroke-width=\"1.8\""
            << (s.dashed ? " stroke-dasharray=\"5 4\"" : "") << "/>\n";
        const double ly = top + 10 + 20 * legendRow++;
        svg << "<line x1=\"" << left + plotW + 14 << "\" y1=\"" << ly << "\" x2=\"" << left + plotW + 38 << "\" y2=\"" << ly
            << "\" stroke=\"" << s.colour << "\" stroke-width=\"2\"" << (s.dashed ? " stroke-dasharray=\"5 4\"" : "") << "/>\n"
            << "<text x=\"" << left + plotW + 44 << "\" y=\"" << ly + 4 << "\">" << s.name << "</text>\n";
    }
    svg << "</svg>\n";
    file.getParentDirectory().createDirectory();
    return file.replaceWithText(svg);
}
} // namespace hpeq::test
