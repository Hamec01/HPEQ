#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace hpeq::test
{
// Minimal SVG line plot so every figure in a report is generated from measured data by code
// (03 Part D "plots генерируются инструментально"), with no Python dependency.
struct PlotSeries
{
    juce::String name;
    juce::String colour; // CSS colour
    std::vector<std::pair<double, double>> points;
    bool dashed = false;
};

struct PlotSpec
{
    juce::String title, xLabel, yLabel;
    bool logX = true;
    double xMin = 20.0, xMax = 20000.0, yMin = -12.0, yMax = 3.0;
    std::vector<double> xTicks { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    double yStep = 3.0;
};

bool writeSvgPlot(const juce::File&, const PlotSpec&, const std::vector<PlotSeries>&);
} // namespace hpeq::test
