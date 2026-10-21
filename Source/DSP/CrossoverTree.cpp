#include "DSP/CrossoverTree.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
void CrossoverTree::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    glideSteps = stepsFor(kParameterRampSeconds, sampleRate / kControlInterval);
    birthSteps = stepsFor(kBirthSeconds, sampleRate / kControlInterval);
    presenceSteps = stepsFor(kMixRampSeconds, sampleRate);
    reset();
}

void CrossoverTree::reset() noexcept
{
    for (auto& e : edges)
        e = NodeEdges {};
    anyAllocated = false;
}

void CrossoverTree::allocate(int node) noexcept
{
    auto& e = edges[static_cast<size_t>(node)];
    const double park = std::log2(kParkingHz);
    e.allocated = true;
    e.parking = false;
    e.birth = true;
    e.presence.reset(0.0);
    e.logLow.reset(park);
    e.logHigh.reset(park);
    // Zero state is (numerically) the steady state of an all-pass parked at 1 Hz, so inserting these
    // sections into running chains does not produce a transient.
    for (auto& b : e.dryAllPass) b.reset();
    for (auto& b : e.highPass) b.reset();
    for (auto& b : e.lowPass) b.reset();
    for (auto& pair : e.foreignAllPass)
        for (auto& b : pair) b.reset();
    for (int k = 0; k < kMaxFxNodesPerLayer; ++k)
        for (auto& b : edges[static_cast<size_t>(k)].foreignAllPass[static_cast<size_t>(node)])
            b.reset();
    updateCoefficients(node);
}

void CrossoverTree::updateCoefficients(int node) noexcept
{
    auto& e = edges[static_cast<size_t>(node)];
    const double lo = std::exp2(e.logLow.value()), hi = std::exp2(e.logHigh.value());
    constexpr double q = design::kButterworthQ;
    for (int side = 0; side < 2; ++side)
    {
        e.dryAllPass[static_cast<size_t>(side)].setMode(TptSvf::Mode::AllPass);
        e.dryAllPass[static_cast<size_t>(side)].setFrequency(sampleRate, side == 0 ? lo : hi, q);
        e.highPass[static_cast<size_t>(side)].setMode(TptSvf::Mode::HighPass);
        e.highPass[static_cast<size_t>(side)].setFrequency(sampleRate, lo, q);
        e.lowPass[static_cast<size_t>(side)].setMode(TptSvf::Mode::LowPass);
        e.lowPass[static_cast<size_t>(side)].setFrequency(sampleRate, hi, q);
    }
    for (int k = 0; k < kMaxFxNodesPerLayer; ++k)
    {
        auto& pair = edges[static_cast<size_t>(k)].foreignAllPass[static_cast<size_t>(node)];
        for (int side = 0; side < 2; ++side)
        {
            pair[static_cast<size_t>(side)].setMode(TptSvf::Mode::AllPass);
            pair[static_cast<size_t>(side)].setFrequency(sampleRate, side == 0 ? lo : hi, q);
        }
    }
}

void CrossoverTree::update(const std::array<bool, kMaxFxNodesPerLayer>& running,
                           const std::array<double, kMaxFxNodesPerLayer>& lowHz,
                           const std::array<double, kMaxFxNodesPerLayer>& highHz) noexcept
{
    const double park = std::log2(kParkingHz);
    anyAllocated = false;
    for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
    {
        auto& e = edges[static_cast<size_t>(j)];
        const auto lo = std::log2(design::clampFrequency(lowHz[static_cast<size_t>(j)], sampleRate));
        const auto hi = std::log2(design::clampFrequency(highHz[static_cast<size_t>(j)], sampleRate));

        if (running[static_cast<size_t>(j)])
        {
            if (!e.allocated)
                allocate(j);
            if (e.parking)
            {
                e.parking = false;
                e.birth = true; // coming back from the parking glide: same path as a birth
            }
            const int steps = e.birth ? birthSteps : glideSteps;
            e.logLow.setTarget(lo, steps);
            e.logHigh.setTarget(hi, steps);
        }
        else if (e.allocated)
        {
            if (!e.parking)
            {
                e.parking = true;
                e.birth = false;
                e.presence.reset(0.0); // the node's own mix is already 0 when it stops running
                e.logLow.setTarget(park, birthSteps);
                e.logHigh.setTarget(park, birthSteps);
            }
            else if (!e.logLow.isRamping() && !e.logHigh.isRamping())
            {
                e.allocated = false; // parked at 1 Hz: leave the chains
                e.parking = false;
            }
        }
        anyAllocated = anyAllocated || e.allocated;
    }
}

void CrossoverTree::process(const float* const* in, int numChannels, int numSamples, int controlPhase,
                            float* const* dry, float* const* const* bands, float* const* presence) noexcept
{
    int phase = controlPhase;
    for (int i = 0; i < numSamples; ++i)
    {
        if (phase == 0)
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (e.allocated && (e.logLow.isRamping() || e.logHigh.isRamping()))
                {
                    e.logLow.next();
                    e.logHigh.next();
                    updateCoefficients(j);
                }
                if (e.allocated && e.birth && !e.logLow.isRamping() && !e.logHigh.isRamping())
                {
                    e.birth = false;
                    e.presence.setTarget(1.0, presenceSteps);
                }
            }

        for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            if (edges[static_cast<size_t>(j)].allocated)
                presence[j][i] = static_cast<float>(edges[static_cast<size_t>(j)].presence.next());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const double x = in[ch][i];
            double d = x;
            for (auto& e : edges)
                if (e.allocated)
                    d = e.dryAllPass[1].processSample(ch, e.dryAllPass[0].processSample(ch, d));
            dry[ch][i] = static_cast<float>(d);

            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& own = edges[static_cast<size_t>(j)];
                if (!own.allocated)
                    continue;
                double b = x;
                for (auto& h : own.highPass) b = h.processSample(ch, b);
                for (auto& l : own.lowPass) b = l.processSample(ch, b);
                for (int k = 0; k < kMaxFxNodesPerLayer; ++k)
                {
                    if (k == j || !edges[static_cast<size_t>(k)].allocated)
                        continue;
                    auto& pair = own.foreignAllPass[static_cast<size_t>(k)];
                    b = pair[1].processSample(ch, pair[0].processSample(ch, b));
                }
                bands[j][ch][i] = static_cast<float>(b);
            }
        }
        phase = (phase + 1) % kControlInterval;
    }
}
} // namespace hpeq::dsp
