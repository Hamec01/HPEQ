#include "DSP/CrossoverTree.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
void CrossoverTree::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    for (auto& e : edges)
        e.retunes.assign(static_cast<size_t>(maxBlockSize / kControlInterval + 2), {});
    glideSteps = stepsFor(kParameterRampSeconds, sampleRate / kControlInterval);
    birthSteps = stepsFor(kBirthSeconds, sampleRate / kControlInterval);
    presenceSteps = stepsFor(kMixRampSeconds, sampleRate);
    reset();
}

void CrossoverTree::reset() noexcept
{
    // Field by field: no temporary NodeEdges, whose std::vector would allocate in MSVC Debug builds
    // (iterator-debugging proxy) on the audio thread (found by the M7 Debug run).
    for (auto& e : edges)
    {
        e.allocated = e.parking = e.birth = false;
        e.logLow = e.logHigh = e.presence = LinearRamp {};
        for (auto* group : { &e.prefixAllPass, &e.highPass, &e.lowPass, &e.hornerAllPass })
            for (auto& f : *group)
                f = TptSvf {};
        e.retuneCount = 0;
    }
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
    // sections into the running chains does not produce a transient.
    for (auto* group : { &e.prefixAllPass, &e.highPass, &e.lowPass, &e.hornerAllPass })
        for (auto& f : *group)
            f.reset();
    updateCoefficients(node, false);
}

void CrossoverTree::updateCoefficients(int node, bool interpolate) noexcept
{
    auto& e = edges[static_cast<size_t>(node)];
    const double lo = std::exp2(e.logLow.value()), hi = std::exp2(e.logHigh.value());
    constexpr double q = design::kButterworthQ;
    for (int side = 0; side < 2; ++side)
    {
        const auto s = static_cast<size_t>(side);
        const double f = side == 0 ? lo : hi;
        e.prefixAllPass[s].setMode(TptSvf::Mode::AllPass);
        e.prefixAllPass[s].setFrequency(sampleRate, f, q, interpolate);
        if (!interpolate)
        {
            e.hornerAllPass[s].setMode(TptSvf::Mode::AllPass);
            e.hornerAllPass[s].setFrequency(sampleRate, f, q, false);
        }
        e.highPass[s].setMode(TptSvf::Mode::HighPass);
        e.highPass[s].setFrequency(sampleRate, lo, q, interpolate);
        e.lowPass[s].setMode(TptSvf::Mode::LowPass);
        e.lowPass[s].setFrequency(sampleRate, hi, q, interpolate);
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
                e.presence.reset(0.0); // the node's own contribution has already ended when it stops
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
    for (auto& e : edges)
        e.retuneCount = 0;
    int phase = controlPhase;
    for (int i = 0; i < numSamples; ++i)
    {
        if (phase == 0)
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (!e.allocated)
                    continue;
                if (e.logLow.isRamping() || e.logHigh.isRamping())
                {
                    e.logLow.next();
                    e.logHigh.next();
                    updateCoefficients(j, true);
                    if (e.retuneCount < static_cast<int>(e.retunes.size()))
                        e.retunes[static_cast<size_t>(e.retuneCount++)] = { i, std::exp2(e.logLow.value()), std::exp2(e.logHigh.value()) };
                }
                if (e.birth && !e.logLow.isRamping() && !e.logHigh.isRamping())
                {
                    e.birth = false;
                    e.presence.setTarget(1.0, presenceSteps);
                }
            }
        phase = (phase + 1) % kControlInterval;

        for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
        {
            auto& e = edges[static_cast<size_t>(j)];
            if (!e.allocated)
                continue;
            presence[j][i] = static_cast<float>(e.presence.next());
            for (auto* group : { &e.prefixAllPass, &e.highPass, &e.lowPass })
                for (auto& f : *group)
                    f.advance();
        }

        if (numChannels == 2)
        {
            // Stereo: both channels through each section at once (bit-identical to the scalar path).
            __m128d prefix = _mm_set_pd(in[1][i], in[0][i]);
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (!e.allocated)
                    continue;
                __m128d b = e.highPass[1].processStereo(e.highPass[0].processStereo(prefix));
                b = e.lowPass[1].processStereo(e.lowPass[0].processStereo(b));
                bands[j][0][i] = static_cast<float>(_mm_cvtsd_f64(b));
                bands[j][1][i] = static_cast<float>(_mm_cvtsd_f64(_mm_unpackhi_pd(b, b)));
                prefix = e.prefixAllPass[1].processStereo(e.prefixAllPass[0].processStereo(prefix));
            }
            dry[0][i] = static_cast<float>(_mm_cvtsd_f64(prefix));
            dry[1][i] = static_cast<float>(_mm_cvtsd_f64(_mm_unpackhi_pd(prefix, prefix)));
            continue;
        }
        for (int ch = 0; ch < numChannels; ++ch)
        {
            double prefix = in[ch][i];
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (!e.allocated)
                    continue;
                double b = e.highPass[1].processSample(ch, e.highPass[0].processSample(ch, prefix));
                b = e.lowPass[1].processSample(ch, e.lowPass[0].processSample(ch, b));
                bands[j][ch][i] = static_cast<float>(b);
                prefix = e.prefixAllPass[1].processSample(ch, e.prefixAllPass[0].processSample(ch, prefix));
            }
            dry[ch][i] = static_cast<float>(prefix);
        }
    }
}

void CrossoverTree::combine(const float* const* const* deltas, float* const* out, int numChannels, int numSamples) noexcept
{
    std::array<int, kMaxFxNodesPerLayer> nextRetune {};
    for (int i = 0; i < numSamples; ++i)
    {
        for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
        {
            auto& e = edges[static_cast<size_t>(j)];
            if (!e.allocated)
                continue;
            auto& next = nextRetune[static_cast<size_t>(j)];
            if (next < e.retuneCount && e.retunes[static_cast<size_t>(next)].sample == i)
            {
                const auto& r = e.retunes[static_cast<size_t>(next++)];
                e.hornerAllPass[0].setFrequency(sampleRate, r.lowHz, design::kButterworthQ, true);
                e.hornerAllPass[1].setFrequency(sampleRate, r.highHz, design::kButterworthQ, true);
            }
            for (auto& f : e.hornerAllPass)
                f.advance();
        }

        if (numChannels == 2)
        {
            __m128d acc = _mm_setzero_pd();
            bool started = false;
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (!e.allocated)
                    continue;
                if (started)
                    acc = e.hornerAllPass[1].processStereo(e.hornerAllPass[0].processStereo(acc));
                acc = _mm_add_pd(acc, _mm_set_pd(deltas[j][1][i], deltas[j][0][i]));
                started = true;
            }
            out[0][i] += static_cast<float>(_mm_cvtsd_f64(acc));
            out[1][i] += static_cast<float>(_mm_cvtsd_f64(_mm_unpackhi_pd(acc, acc)));
            continue;
        }
        for (int ch = 0; ch < numChannels; ++ch)
        {
            double acc = 0.0;
            bool started = false;
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
            {
                auto& e = edges[static_cast<size_t>(j)];
                if (!e.allocated)
                    continue;
                // The first node's pair would only ever see zeros: skip it (its state stays zero).
                if (started)
                    acc = e.hornerAllPass[1].processSample(ch, e.hornerAllPass[0].processSample(ch, acc));
                acc += deltas[j][ch][i];
                started = true;
            }
            out[ch][i] += static_cast<float>(acc);
        }
    }
}
} // namespace hpeq::dsp
