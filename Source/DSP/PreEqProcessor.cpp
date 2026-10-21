#include "DSP/PreEqProcessor.h"

#include <cmath>

#include "DSP/Biquad.h"

namespace hpeq::dsp
{
void PreEqProcessor::RuntimeSection::setSpec(const SectionSpec& s, bool interpolate) noexcept
{
    if (!interpolate || s.onePole != spec.onePole)
    {
        spec = s;
        remaining = 0;
        derive();
        return;
    }
    constexpr double n = kControlInterval;
    step.g = (s.g - spec.g) / n;
    step.k = (s.k - spec.k) / n;
    step.m0 = (s.m0 - spec.m0) / n;
    step.m1 = (s.m1 - spec.m1) / n;
    step.m2 = (s.m2 - spec.m2) / n;
    step.onePole = s.onePole;
    target = s;
    remaining = kControlInterval;
}

void PreEqProcessor::RuntimeSection::advance() noexcept
{
    if (remaining <= 0)
        return;
    if (--remaining == 0)
        spec = target; // land exactly
    else
    {
        spec.g += step.g;
        spec.k += step.k;
        spec.m0 += step.m0;
        spec.m1 += step.m1;
        spec.m2 += step.m2;
    }
    derive();
}

void PreEqProcessor::RuntimeSection::derive() noexcept
{
    if (spec.onePole)
    {
        onePoleG = spec.g / (1.0 + spec.g);
        return;
    }
    a1 = 1.0 / (1.0 + spec.g * (spec.g + spec.k));
    a2 = spec.g * a1;
    a3 = spec.g * a2;
}

double PreEqProcessor::RuntimeSection::process(int ch, double x) noexcept
{
    auto& st = state[static_cast<size_t>(ch)];
    if (spec.onePole)
    {
        const double v = (x - st[0]) * onePoleG;
        const double low = v + st[0];
        st[0] = low + v;
        return spec.m1 * (x - low) + spec.m2 * low;
    }
    const double v3 = x - st[1];
    const double v1 = a1 * st[0] + a2 * v3;
    const double v2 = st[1] + a2 * st[0] + a3 * v3;
    st[0] = 2.0 * v1 - st[0];
    st[1] = 2.0 * v2 - st[1];
    return spec.m0 * x + spec.m1 * v1 + spec.m2 * v2;
}

void PreEqProcessor::FilterSet::reset() noexcept
{
    for (auto& s : sections)
    {
        s.state = {};
        s.remaining = 0;
    }
    fresh = true;
}

void PreEqProcessor::FilterSet::advance() noexcept
{
    for (int i = 0; i < count; ++i)
        sections[static_cast<size_t>(i)].advance();
}

double PreEqProcessor::FilterSet::run(int ch, double x) noexcept
{
    for (int i = 0; i < count; ++i)
        x = sections[static_cast<size_t>(i)].process(ch, x);
    return x;
}

void PreEqProcessor::FilterSet::apply(double& l, double& r, int numChannels) noexcept
{
    if (numChannels == 1)
    {
        // Mono: there is no right channel and no side signal; those bands leave mono untouched.
        if (mode != ChannelMode::Right && mode != ChannelMode::Side)
            l = run(0, l);
        return;
    }
    switch (mode)
    {
        case ChannelMode::Stereo:
        case ChannelMode::LinkedStereo:
        case ChannelMode::DualMono: l = run(0, l); r = run(1, r); break;
        case ChannelMode::Left:     l = run(0, l); break;
        case ChannelMode::Right:    r = run(1, r); break;
        case ChannelMode::Mid:
        {
            const double m = run(0, 0.5 * (l + r)), s = 0.5 * (l - r);
            l = m + s;
            r = m - s;
            break;
        }
        case ChannelMode::Side:
        {
            const double m = 0.5 * (l + r), s = run(0, 0.5 * (l - r));
            l = m + s;
            r = m - s;
            break;
        }
    }
}

void PreEqProcessor::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    rampSteps = stepsFor(kParameterRampSeconds, sampleRate / kControlInterval);
    birthSteps = stepsFor(kBirthSeconds, sampleRate / kControlInterval);
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    crossfadeSteps = stepsFor(kStructureCrossfadeSeconds, sampleRate);
    reset();
}

void PreEqProcessor::reset() noexcept
{
    for (auto& b : bands)
        b = Band {};
}

PreEqProcessor::Entry PreEqProcessor::entryOf(EqShape shape) noexcept
{
    if (isGainShape(shape))
        return Entry::Gain;
    if (shape == EqShape::LowCut || shape == EqShape::AllPass)
        return Entry::Park;
    return Entry::Wet;
}

bool PreEqProcessor::sameStructure(const FilterSet& set, const EqBandParams& t) noexcept
{
    const bool cut = t.shape == EqShape::LowCut || t.shape == EqShape::HighCut;
    return set.shape == t.shape && set.mode == t.channel && (!cut || set.slope == butterworthOrder(t.slopeDbPerOct) * 6);
}

void PreEqProcessor::configure(FilterSet& set, const EqBandParams& t) noexcept
{
    set.shape = t.shape;
    set.slope = butterworthOrder(t.slopeDbPerOct) * 6;
    set.mode = t.channel;
    set.reset();
}

void PreEqProcessor::updateCoefficients(Band& b) noexcept
{
    const double f = std::exp2(b.logFrequency.value());
    for (int s = 0; s < 2; ++s)
    {
        if (s != b.active && !b.crossfading)
            continue;
        auto& set = b.sets[static_cast<size_t>(s)];
        const auto d = designBand(set.shape, set.slope, sampleRate, f, b.gainDb.value(), b.q.value());
        const bool interpolate = !set.fresh && set.count == d.count;
        set.count = d.count;
        for (int i = 0; i < d.count; ++i)
            set.sections[static_cast<size_t>(i)].setSpec(d.sections[static_cast<size_t>(i)], interpolate);
        set.fresh = false;
    }
}

void PreEqProcessor::startStructureChange(Band& b) noexcept
{
    b.active ^= 1;
    configure(b.sets[static_cast<size_t>(b.active)], b.target);
    b.crossfading = true;
    b.crossfade.reset(0.0);
    b.crossfade.setTarget(1.0, crossfadeSteps);
    updateCoefficients(b);
}

void PreEqProcessor::setTargets(const std::array<EqBandParams, kMaxEqNodesPerLayer>& targets) noexcept
{
    const double parkLog = std::log2(kParkingHz);
    for (size_t i = 0; i < bands.size(); ++i)
    {
        auto& b = bands[i];
        const auto& t = targets[i];
        const double logF = std::log2(design::clampFrequency(t.frequencyHz, sampleRate));
        b.target = t;
        b.wanted = t.active && !(isGainShape(t.shape) && t.gainDb == 0.0f);

        if (b.wanted && !b.running)
        {
            b.running = true;
            b.active = 0;
            b.crossfading = false;
            configure(b.sets[0], t);
            b.q.reset(t.q);
            b.wet.reset(1.0);
            b.crossfade.reset(1.0);
            switch (entryOf(t.shape))
            {
                case Entry::Gain: b.logFrequency.reset(logF); b.gainDb.reset(0.0); b.gainDb.setTarget(t.gainDb, rampSteps); break;
                case Entry::Park: b.logFrequency.reset(parkLog); b.logFrequency.setTarget(logF, birthSteps); b.gainDb.reset(t.gainDb); break;
                case Entry::Wet:  b.logFrequency.reset(logF); b.gainDb.reset(t.gainDb); b.wet.reset(0.0); b.wet.setTarget(1.0, mixSteps); break;
            }
            updateCoefficients(b);
            continue;
        }
        if (!b.running)
            continue;

        const auto exitKind = entryOf(b.sets[static_cast<size_t>(b.active)].shape);
        if (b.wanted)
        {
            if (!b.crossfading && !sameStructure(b.sets[static_cast<size_t>(b.active)], t))
                startStructureChange(b);
            const bool fromParking = b.logFrequency.targetValue() == parkLog;
            b.logFrequency.setTarget(logF, fromParking ? birthSteps : rampSteps);
            b.gainDb.setTarget(t.gainDb, rampSteps);
            b.q.setTarget(t.q, rampSteps);
            b.wet.setTarget(1.0, mixSteps);
        }
        else
        {
            switch (exitKind)
            {
                case Entry::Gain: b.gainDb.setTarget(0.0, rampSteps); break;
                case Entry::Park: b.logFrequency.setTarget(parkLog, birthSteps); break;
                case Entry::Wet:  b.wet.setTarget(0.0, mixSteps); break;
            }
        }
    }
}

void PreEqProcessor::advanceControl(Band& b) noexcept
{
    if (!b.crossfading && b.wanted && !sameStructure(b.sets[static_cast<size_t>(b.active)], b.target))
        startStructureChange(b);

    if (b.logFrequency.isRamping() || b.gainDb.isRamping() || b.q.isRamping())
    {
        b.logFrequency.next();
        b.gainDb.next();
        b.q.next();
        updateCoefficients(b);
    }

    if (b.wanted || b.crossfading)
        return;
    const double parkLog = std::log2(kParkingHz);
    switch (entryOf(b.sets[static_cast<size_t>(b.active)].shape))
    {
        case Entry::Gain: b.running = b.gainDb.isRamping() || b.gainDb.value() != 0.0; break;
        case Entry::Park: b.running = b.logFrequency.isRamping() || b.logFrequency.value() != parkLog; break;
        case Entry::Wet:  b.running = b.wet.isRamping() || b.wet.value() != 0.0; break;
    }
}

void PreEqProcessor::process(float* const* channels, int numChannels, int numSamples, int controlPhase) noexcept
{
    const bool stereo = numChannels > 1;
    for (auto& b : bands)
    {
        if (!b.running)
            continue;
        int phase = controlPhase;
        for (int i = 0; i < numSamples; ++i)
        {
            if (phase == 0)
            {
                advanceControl(b);
                if (!b.running)
                    break;
            }
            phase = (phase + 1) % kControlInterval;

            b.sets[static_cast<size_t>(b.active)].advance();
            if (b.crossfading)
                b.sets[static_cast<size_t>(b.active ^ 1)].advance();

            const double xl = channels[0][i];
            const double xr = stereo ? static_cast<double>(channels[1][i]) : 0.0;
            double l = xl, r = xr;
            b.sets[static_cast<size_t>(b.active)].apply(l, r, numChannels);

            if (b.crossfading)
            {
                double ol = xl, orr = xr;
                b.sets[static_cast<size_t>(b.active ^ 1)].apply(ol, orr, numChannels);
                const double c = b.crossfade.next();
                if (c < 1.0)
                {
                    l = ol + c * (l - ol);
                    r = orr + c * (r - orr);
                }
                if (!b.crossfade.isRamping())
                    b.crossfading = false;
            }
            if (b.wet.isRamping() || b.wet.value() != 1.0)
            {
                const double w = b.wet.next();
                l = xl + w * (l - xl);
                r = xr + w * (r - xr);
            }
            channels[0][i] = static_cast<float>(l);
            if (stereo)
                channels[1][i] = static_cast<float>(r);
        }
    }
}

bool PreEqProcessor::isNeutral() const noexcept
{
    for (const auto& b : bands)
        if (b.running)
            return false;
    return true;
}

BandDesign PreEqProcessor::currentDesign(int index) const noexcept
{
    const auto& b = bands[static_cast<size_t>(index)];
    const auto& set = b.sets[static_cast<size_t>(b.active)];
    return designBand(set.shape, set.slope, sampleRate, std::exp2(b.logFrequency.value()), b.gainDb.value(), b.q.value());
}
} // namespace hpeq::dsp
