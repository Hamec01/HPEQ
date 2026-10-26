#include "DSP/TubePreamp.h"

#include <algorithm>
#include <cmath>

#include <juce_dsp/juce_dsp.h>

namespace hpeq::dsp
{
namespace tube
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kGridConduction = 1500.0; // grid-cathode resistance once the grid is positive (ohm)

double softplus(double x) noexcept { return x > 40.0 ? x : std::log1p(std::exp(x)); }

double parallel(double a, double b) { return a * b / (a + b); }

// Bisection on a function rising in i over [lo, hi].
template <typename F>
double solveRising(F&& f, double lo, double hi)
{
    for (int i = 0; i < 80; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (f(mid) > 0.0 ? hi : lo) = mid;
    }
    return 0.5 * (lo + hi);
}
} // namespace

// Fitted to the datasheet points (fit script in the pass report): 6N3P mu 36, at 150 V / 240 ohm 8.50 mA
// and 5.8 mA/V; 6Zh32P mu 38 (EF86-like), at 250 / 140 V, -2 V 3.0 mA and 1.8 mA/V, screen 0.6 mA.
const Triode k6N3P { 36.0, 1.4704, 360.145, 588.695, 300.0 };
const Pentode k6Zh32P { 38.0, 1.4, 1261.63, 125.01, 10.0, 3642.9 };

double Triode::plateCurrent(double ug, double ua) const noexcept
{
    if (ua <= 0.0)
        return 0.0;
    const double e1 = ua / kp * softplus(kp * (1.0 / mu + ug / std::sqrt(kvb + ua * ua)));
    return e1 > 0.0 ? std::pow(e1, ex) / kg1 : 0.0;
}

double Pentode::plateCurrent(double ug1, double ug2, double ua) const noexcept
{
    if (ua <= 0.0 || ug2 <= 0.0)
        return 0.0;
    const double e1 = ug2 / kp * softplus(kp * (1.0 / mu + ug1 / ug2));
    return e1 > 0.0 ? std::pow(e1, ex) / kg1 * std::atan(ua / kvb) : 0.0;
}

double Pentode::screenCurrent(double ug1, double ug2) const noexcept
{
    const double v = ug1 + ug2 / mu;
    return v > 0.0 ? std::pow(v, 1.5) / kg2 : 0.0;
}

// ------------------------------------------------------------------------------------------ stage
void Stage::prepare(const StageSpec& s, double sampleRate)
{
    spec = s;
    dt = 1.0 / sampleRate;
    const auto plate = [&](double ug, double ua) { return spec.pentode ? k6Zh32P.plateCurrent(ug, spec.screenV, ua) : k6N3P.plateCurrent(ug, ua); };
    const auto cathodeCurrent = [&](double ug, double ia) { return spec.pentode ? ia + k6Zh32P.screenCurrent(ug, spec.screenV) : ia; };

    // resting point: self bias through the cathode resistor, the plate on its DC load
    const double maxI = spec.supply / spec.plateR;
    restingCurrent = solveRising([&](double ia)
    {
        double ik = ia;
        for (int k = 0; k < 4; ++k)
            ik = cathodeCurrent(-ik * spec.cathodeR, ia);
        return ia - plate(-ik * spec.cathodeR, spec.supply - ia * spec.plateR);
    }, 0.0, maxI);
    double ik0 = restingCurrent;
    for (int k = 0; k < 6; ++k)
        ik0 = cathodeCurrent(-ik0 * spec.cathodeR, restingCurrent);
    restingCathode = ik0 * spec.cathodeR;
    restingPlate = spec.supply - restingCurrent * spec.plateR;

    // the table: s = grid - cathode (bypassed) or grid - ground (unbypassed: the cathode resistor feeds
    // back inside the table); the plate on the AC load line; plate-to-grid feedback inside too
    const bool bypassed = spec.cathodeC > 0.0;
    tableMin = bypassed ? -restingCathode - 24.0 : -24.0;
    const double tableMax = bypassed ? 3.0 : restingCathode + 3.0;
    tableStep = (tableMax - tableMin) / (kTable - 1);
    plateSwing.assign(kTable, 0.0);
    current.assign(kTable, 0.0);
    const double maxAc = restingCurrent + restingPlate / spec.acLoad;
    for (int i = 0; i < kTable; ++i)
    {
        const double sv = tableMin + tableStep * i;
        const auto residual = [&](double ia)
        {
            const double ua = std::max(1.0, restingPlate - (ia - restingCurrent) * spec.acLoad);
            const double swing = ua - restingPlate;
            double ug = sv + spec.feedback * swing;
            if (!bypassed)
                ug -= cathodeCurrent(ug, ia) * spec.cathodeR;
            return ia - plate(ug, ua);
        };
        const double ia = solveRising(residual, 0.0, maxAc);
        const double ua = std::max(1.0, restingPlate - (ia - restingCurrent) * spec.acLoad);
        plateSwing[static_cast<size_t>(i)] = ua - restingPlate;
        double ik = ia;
        if (spec.pentode)
            ik = cathodeCurrent(sv, ia);
        current[static_cast<size_t>(i)] = ik;
    }
    // The resting point the table itself gives (a table step from the solved one): with a bypassed
    // cathode, solve the cathode voltage that the table's current holds up, so a stage at rest stays
    // exactly at rest (no drift, no thump when a button turns on).
    restingSwing = 0.0;
    double ikRest = 0.0;
    if (bypassed)
        restingCathode = solveRising([&](double vk)
        {
            double ik = 0.0;
            lookup(-vk, ik);
            return vk - ik * spec.cathodeR;
        }, 0.0, 2.0 * restingCathode + 1.0);
    restingSwing = lookup(bypassed ? -restingCathode : 0.0, ikRest);
    reset();
}

void Stage::reset() noexcept
{
    cathodeV = restingCathode;
    couplingV = 0.0;
}

double Stage::lookup(double s, double& ik) const noexcept
{
    const double pos = std::clamp((s - tableMin) / tableStep, 0.0, static_cast<double>(kTable - 1) - 1.0e-9);
    const auto i = static_cast<size_t>(pos);
    const double f = pos - static_cast<double>(i);
    ik = current[i] + f * (current[i + 1] - current[i]);
    return plateSwing[i] + f * (plateSwing[i + 1] - plateSwing[i]) - restingSwing;
}

double Stage::staticOutput(double grid) const noexcept
{
    // as process() at rest: grid current limits a positive grid through what drives it
    if (const double ugk = grid - restingCathode; ugk > 0.0)
        grid = restingCathode + ugk * kGridConduction / (kGridConduction + spec.sourceR);
    double ik = 0.0;
    return lookup(spec.cathodeC > 0.0 ? grid - restingCathode : grid, ik);
}

double Stage::process(double input) noexcept
{
    const bool bypassed = spec.cathodeC > 0.0;
    double vg = spec.couplingC > 0.0 ? input - couplingV : input;
    const double cathode = bypassed ? cathodeV : restingCathode;
    double gridCurrent = 0.0;
    if (const double ugk = vg - cathode; ugk > 0.0)
    {
        // the grid conducts: it is pulled down through what drives it, and its current charges the
        // coupling capacitor (the bias moves negative after an overload: blocking)
        const double held = ugk * kGridConduction / (kGridConduction + spec.sourceR);
        gridCurrent = held / kGridConduction;
        vg = cathode + held;
    }
    if (spec.couplingC > 0.0)
        couplingV += dt * (vg / spec.gridLeak + gridCurrent) / spec.couplingC;
    double ik = 0.0;
    const double swing = lookup(bypassed ? vg - cathodeV : vg, ik);
    if (bypassed)
        cathodeV += dt * (ik - cathodeV / spec.cathodeR) / spec.cathodeC;
    return swing;
}

// ------------------------------------------------------------------------------------ transformer
void Transformer::prepare(double sampleRate, double saturationVolts)
{
    dt = 1.0 / sampleRate;
    resistance = 600.0;
    inductance = resistance / (2.0 * kPi * 10.0); // the magnetising inductance sets a 10 Hz corner
    fluxSat = saturationVolts / (2.0 * kPi * 90.0); // a 90 Hz sine of this amplitude reaches the knee
    // leakage inductance with the winding capacitance: a damped resonance near 45 kHz
    const double w = 2.0 * kPi * std::min(45000.0, 0.45 * sampleRate) / sampleRate, q = 0.85;
    const double alpha = std::sin(w) / (2.0 * q), cw = std::cos(w), a0 = 1.0 + alpha;
    b0 = (1.0 - cw) * 0.5 / a0;
    b1 = (1.0 - cw) / a0;
    b2 = b0;
    a1 = -2.0 * cw / a0;
    a2 = (1.0 - alpha) / a0;
    reset();
}

void Transformer::reset() noexcept
{
    flux = x1 = x2 = y1 = y2 = 0.0;
}

double Transformer::process(double input) noexcept
{
    // core: magnetising current rises steeply as the flux nears saturation; it flows through the
    // primary resistance, so the winding loses voltage (low frequencies first)
    const double r = flux / fluxSat;
    const double magnetising = flux / inductance * (1.0 + r * r * r * r);
    const double winding = input - resistance * magnetising;
    flux += dt * winding;
    const double y = b0 * winding + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = winding;
    y2 = y1;
    y1 = y;
    return y;
}

// ----------------------------------------------------------------------------------------- preamp
void Preamp::prepare(Model m, double oversampledRate)
{
    model = m;
    stages.clear();
    if (model == Model::Lenin)
    {
        // UP-25, table 25: L1 plate ~60 V, screen ~70 V, bias ~-1.5 V; L2 plates ~80 / ~100 V, bias -1.6 /
        // -1.25 V; L3 two triodes in series (plates ~210 / ~105 V), bias -1.3 / -1.4 V over 220 ohm.
        StageSpec s1;
        s1.pentode = true; s1.supply = 250.0; s1.plateR = 360e3; s1.cathodeR = 2.6e3; s1.cathodeC = 50e-6; s1.screenV = 70.0;
        s1.acLoad = parallel(360e3, 1e6); s1.sourceR = 600.0;
        StageSpec s2;
        s2.supply = 250.0; s2.plateR = 120e3; s2.cathodeR = 1.0e3; s2.cathodeC = 10e-6; s2.acLoad = parallel(120e3, 470e3);
        s2.feedback = 220e3 / 3.9e6; s2.couplingC = 0.033e-6; s2.gridLeak = 470e3; s2.sourceR = s1.acLoad;
        StageSpec s3;
        s3.supply = 250.0; s3.plateR = 28e3; s3.cathodeR = 230.0; s3.cathodeC = 0.0; s3.acLoad = parallel(28e3, 560e3);
        s3.couplingC = 0.033e-6; s3.gridLeak = 560e3; s3.sourceR = s2.acLoad;
        StageSpec s4; // the series pair: the lower triode with the upper one as its plate load
        s4.supply = 210.0; s4.plateR = 19e3; s4.cathodeR = 220.0; s4.cathodeC = 0.0;
        // the upper triode as a load: ra + (mu + 1) Rk = 6.2 k + 37 x 220 ohm, the output load 100 k beside it
        s4.acLoad = parallel(6.2e3 + 37.0 * 220.0, 100e3); s4.feedback = 56e3 / (560e3 + 56e3);
        s4.couplingC = 0.033e-6; s4.gridLeak = 1e6; s4.sourceR = s3.acLoad;
        for (const auto& spec : { s1, s2, s3, s4 })
        {
            stages.emplace_back();
            stages.back().prepare(spec, oversampledRate);
        }
    }
    else
    {
        // UP-27 (a typical layout; no factory schematic found): three 6Zh32P stages, a 6N3P output
        StageSpec p1;
        p1.pentode = true; p1.supply = 200.0; p1.plateR = 220e3; p1.cathodeR = 2.2e3; p1.cathodeC = 100e-6; p1.screenV = 85.0;
        p1.acLoad = parallel(220e3, 470e3); p1.sourceR = 600.0;
        StageSpec p2 = p1;
        p2.couplingC = 0.033e-6; p2.gridLeak = 470e3; p2.sourceR = p1.acLoad;
        StageSpec p3 = p2;
        p3.plateR = 100e3; p3.cathodeR = 1.5e3; p3.screenV = 100.0; p3.acLoad = parallel(100e3, 470e3); p3.sourceR = p2.acLoad;
        StageSpec t4;
        t4.supply = 200.0; t4.plateR = 22e3; t4.cathodeR = 470.0; t4.cathodeC = 100e-6; t4.acLoad = parallel(22e3, 47e3);
        t4.couplingC = 0.1e-6; t4.gridLeak = 470e3; t4.sourceR = p3.acLoad;
        for (const auto& spec : { p1, p2, p3, t4 })
        {
            stages.emplace_back();
            stages.back().prepare(spec, oversampledRate);
        }
    }

    // Calibration: small signals at unity; the drive is set where a -12 dBFS sine gives the chain's static
    // THD: LENIN 1 %, STALIN 4 % (the drive point: a choice, documented in the report). Owner 2026-10-26:
    // at -6 dBFS it was barely audible; 6 dB more drive, MIX takes it back down.
    const double small = 1.0e-7;
    const double gain = (staticChain(small) - staticChain(-small)) / (2.0 * small);
    const double target = model == Model::Lenin ? 0.01 : 0.04;
    const auto thd = [&](double amplitude)
    {
        constexpr int n = 128;
        std::array<double, 2 * 10> re {}; // harmonics 1..9: sin and cos sums
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * kPi * i / n, y = staticChain(amplitude * std::sin(ph));
            for (int k = 1; k <= 9; ++k)
            {
                re[static_cast<size_t>(2 * k)] += y * std::sin(k * ph);
                re[static_cast<size_t>(2 * k + 1)] += y * std::cos(k * ph);
            }
        }
        const auto mag = [&](int k) { return std::hypot(re[static_cast<size_t>(2 * k)], re[static_cast<size_t>(2 * k + 1)]); };
        double h = 0.0;
        for (int k = 2; k <= 9; ++k)
            h += mag(k) * mag(k);
        return std::sqrt(h) / std::max(1.0e-30, mag(1));
    };
    const double reference = std::pow(10.0, -12.0 / 20.0);
    double lo = std::log(1.0e-7), hi = std::log(20.0);
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (thd(std::exp(mid) * reference) > target ? hi : lo) = mid;
    }
    inScale = std::exp(0.5 * (lo + hi));
    outScale = 1.0 / (gain * inScale); // also undoes an inverting chain
    transformer.prepare(oversampledRate, inScale * 2.0 * reference); // the core's knee 6 dB above the drive point
    dcBlockR = std::exp(-2.0 * kPi * 4.0 / oversampledRate);
    reset();
}

double Preamp::staticChain(double volts) const noexcept
{
    double v = volts;
    for (const auto& s : stages)
        v = s.staticOutput(v);
    return v;
}

void Preamp::reset() noexcept
{
    for (auto& s : stages)
        s.reset();
    transformer.reset();
    dcX = dcY = 0.0;
}

double Preamp::process(double x) noexcept
{
    double v = x * inScale;
    if (model == Model::Stalin)
        v = transformer.process(v);
    for (auto& s : stages)
        v = s.process(v);
    const double y = v * outScale;
    // the output coupling capacitor (4 Hz): no DC from the moving bias reaches the mix
    dcY = y - dcX + dcBlockR * dcY;
    dcX = y;
    return dcY;
}
} // namespace tube

// ------------------------------------------------------------------------------------ the colour
TubeColour::TubeColour() = default;
TubeColour::~TubeColour() = default;

void TubeColour::prepare(double sampleRate, int maxBlockSize)
{
    rate = sampleRate;
    // linear phase (FIR): on the whole mix the transients must not smear, and the reported latency is exact
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false, true);
    oversampling->initProcessing(static_cast<size_t>(std::max(1, maxBlockSize)));
    latency = static_cast<int>(std::lround(oversampling->getLatencyInSamples()));
    for (auto& p : lenin)
        p.prepare(tube::Model::Lenin, 4.0 * sampleRate);
    for (auto& p : stalin)
        p.prepare(tube::Model::Stalin, 4.0 * sampleRate);
    fadeStep = 1.0 / (0.02 * 4.0 * sampleRate);
    reset();
}

void TubeColour::reset() noexcept
{
    if (oversampling != nullptr)
        oversampling->reset();
    for (auto& p : lenin)
        p.reset();
    for (auto& p : stalin)
        p.reset();
    mixLenin = targetLenin;
    mixStalin = targetStalin;
}

void TubeColour::setTargets(bool l, bool s, double lm, double sm) noexcept
{
    if (!isRunning() && (l || s) && oversampling != nullptr)
        oversampling->reset(); // starting from silence of the filters
    wantLenin = l;
    wantStalin = s;
    targetLenin = l ? std::clamp(lm, 0.0, 1.0) : 0.0;
    targetStalin = s ? std::clamp(sm, 0.0, 1.0) : 0.0;
}

bool TubeColour::isRunning() const noexcept
{
    return wantLenin || wantStalin || mixLenin > 0.0 || mixStalin > 0.0;
}

void TubeColour::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    if (!isRunning() || oversampling == nullptr || numSamples <= 0)
        return;
    numChannels = std::min(numChannels, 2);
    juce::dsp::AudioBlock<float> block(channels, static_cast<size_t>(numChannels), static_cast<size_t>(numSamples));
    auto up = oversampling->processSamplesUp(block);
    const auto count = up.getNumSamples();
    for (size_t i = 0; i < count; ++i)
    {
        mixLenin = mixLenin < targetLenin ? std::min(targetLenin, mixLenin + fadeStep) : std::max(targetLenin, mixLenin - fadeStep);
        mixStalin = mixStalin < targetStalin ? std::min(targetStalin, mixStalin + fadeStep) : std::max(targetStalin, mixStalin - fadeStep);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* d = up.getChannelPointer(static_cast<size_t>(ch));
            double x = d[i];
            // a button that is on keeps its preamp running even at MIX 0 (no restart when it comes back)
            if (wantLenin || mixLenin > 0.0)
                x += mixLenin * (lenin[static_cast<size_t>(ch)].process(x) - x);
            if (wantStalin || mixStalin > 0.0)
                x += mixStalin * (stalin[static_cast<size_t>(ch)].process(x) - x);
            d[i] = static_cast<float>(x);
        }
    }
    oversampling->processSamplesDown(block);
    // a preamp that faded out starts clean next time
    if (!wantLenin && mixLenin <= 0.0)
        for (auto& p : lenin)
            p.reset();
    if (!wantStalin && mixStalin <= 0.0)
        for (auto& p : stalin)
            p.reset();
}
} // namespace hpeq::dsp
