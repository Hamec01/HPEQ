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

double PreEqProcessor::svf(std::array<double, 2>& st, double g, double k, int type, double x) noexcept
{
    // TPT state-variable filter: type 0 band pass (unity at the centre), 1 low pass, 2 high pass, 3 none
    if (type == 3)
        return x;
    const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const double v3 = x - st[1];
    const double v1 = a1 * st[0] + a2 * v3;
    const double v2 = st[1] + a2 * st[0] + a3 * v3;
    st[0] = 2.0 * v1 - st[0];
    st[1] = 2.0 * v2 - st[1];
    return type == 0 ? k * v1 : type == 1 ? v2 : x - k * v1 - v2;
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
    voiceG = std::tan(3.14159265358979323846 * 1000.0 / sampleRate);
    birthSteps = stepsFor(kBirthSeconds, sampleRate / kControlInterval);
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    crossfadeSteps = stepsFor(kStructureCrossfadeSeconds, sampleRate);
    reset();
}

void PreEqProcessor::reset() noexcept
{
    for (auto& b : bands)
        b = Band {};
    for (auto& o : shownOffset)
        o.store(0.0f, std::memory_order_relaxed);
    soloBand = -1;
    soloMix = 0.0;
    soloState = {};
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
        const auto d = designBand(set.shape, set.slope, sampleRate, f, b.gainDb.value() + b.dynOffset, b.q.value());
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
        const bool dynamicGain = t.dynamic && isGainShape(t.shape) && t.rangeDb != 0.0f;
        b.wanted = t.active && !(isGainShape(t.shape) && t.gainDb == 0.0f && !dynamicGain);

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

    // SOLO: the first soloed point; its filter follows the point (kept for the fade-out when it stops)
    soloBand = -1;
    for (size_t i = 0; i < targets.size() && soloBand < 0; ++i)
        if (targets[i].active && targets[i].solo)
            soloBand = static_cast<int>(i);
    if (soloBand >= 0)
    {
        const auto& t = targets[static_cast<size_t>(soloBand)];
        const double f = design::clampFrequency(t.frequencyHz, sampleRate);
        switch (t.shape)
        {
            case EqShape::LowShelf: case EqShape::LowCut: soloType = 1; soloK = 1.4142135623730951; break;
            case EqShape::HighShelf: case EqShape::HighCut: soloType = 2; soloK = 1.4142135623730951; break;
            default: soloType = 0; soloK = 1.0 / std::max(0.3, static_cast<double>(t.q)); break;
        }
        soloG = std::tan(3.14159265358979323846 * std::min(f, 0.45 * sampleRate) / sampleRate);
    }
}

void PreEqProcessor::detect(Band& b, double l, double r, int numChannels) noexcept
{
    double level = 0.0;
    const auto run = [&](int ch, double x) { return std::abs(svf(b.detState[static_cast<size_t>(ch)], b.detG, b.detK, b.detType, x)); };
    if (numChannels == 1)
        level = run(0, l);
    else
        switch (b.target.channel)
        {
            case ChannelMode::Left:  level = run(0, l); break;
            case ChannelMode::Right: level = run(1, r); break;
            case ChannelMode::Mid:   level = run(0, 0.5 * (l + r)); break;
            case ChannelMode::Side:  level = run(0, 0.5 * (l - r)); break;
            default:                 level = std::max(run(0, l), run(1, r)); break;
        }
    const double c = level > b.env ? b.attackCoef : b.releaseCoef;
    b.env = level + c * (b.env - level);
    if (b.target.detect == 1)
    {
        // the voice body, linked the same way; same attack / release, so the ratio follows syllables
        const auto voice = [&](int ch, double x) { return std::abs(svf(b.voiceState[static_cast<size_t>(ch)], voiceG, 1.0 / 0.6, 0, x)); };
        double v = 0.0;
        if (numChannels == 1)
            v = voice(0, l);
        else
            switch (b.target.channel)
            {
                case ChannelMode::Left:  v = voice(0, l); break;
                case ChannelMode::Right: v = voice(1, r); break;
                case ChannelMode::Mid:   v = voice(0, 0.5 * (l + r)); break;
                case ChannelMode::Side:  v = voice(0, 0.5 * (l - r)); break;
                default:                 v = std::max(voice(0, l), voice(1, r)); break;
            }
        const double cv = v > b.voiceEnv ? b.attackCoef : b.releaseCoef;
        b.voiceEnv = v + cv * (b.voiceEnv - v);
    }
}

void PreEqProcessor::advanceControl(Band& b, size_t index) noexcept
{
    if (!b.crossfading && b.wanted && !sameStructure(b.sets[static_cast<size_t>(b.active)], b.target))
        startStructureChange(b);

    // DYNAMIC: the offset from the detector (or back to 0 when the point is static again)
    const auto& t = b.target;
    const bool dynamic = b.wanted && t.dynamic && isGainShape(b.sets[static_cast<size_t>(b.active)].shape);
    double offset = 0.0;
    if (dynamic)
    {
        const double f = std::exp2(b.logFrequency.value()), q = b.q.value();
        const auto shape = b.sets[static_cast<size_t>(b.active)].shape;
        const int type = shape == EqShape::Bell ? 0 : shape == EqShape::LowShelf ? 1 : shape == EqShape::HighShelf ? 2 : 3;
        if (f != b.detF || q != b.detQ || type != b.detType)
        {
            b.detF = f;
            b.detQ = q;
            b.detType = type;
            b.detG = std::tan(3.14159265358979323846 * std::min(f, 0.45 * sampleRate) / sampleRate);
            b.detK = type == 0 ? 1.0 / std::max(0.3, q) : 1.4142135623730951;
        }
        b.attackCoef = std::exp(-1.0 / (std::max(0.1, static_cast<double>(t.attackMs)) * 1.0e-3 * sampleRate));
        b.releaseCoef = std::exp(-1.0 / (std::max(5.0, static_cast<double>(t.releaseMs)) * 1.0e-3 * sampleRate));
        const double bandDb = 20.0 * std::log10(b.env + 1.0e-9);
        const double detectDb = t.detect == 1 ? (bandDb < -60.0 ? -200.0 : bandDb - 20.0 * std::log10(b.voiceEnv + 1.0e-9) - 20.0) : bandDb;
        const double over = detectDb - t.thresholdDb;
        const double knee = over <= -3.0 ? 0.0 : over < 3.0 ? (over + 3.0) * (over + 3.0) / 12.0 : over;
        const double amount = std::min(std::abs(static_cast<double>(t.rangeDb)), knee * (1.0 - 1.0 / std::max(1.0, static_cast<double>(t.ratio))));
        offset = t.rangeDb < 0.0f ? -amount : amount;
    }
    else if (std::abs(b.dynOffset) > 1.0e-3)
        offset = 0.85 * b.dynOffset; // glide home (about 6 ms per 4 dB at 48 kHz)
    const bool offsetMoved = offset != b.dynOffset;
    b.dynOffset = offset;
    shownOffset[index].store(static_cast<float>(offset), std::memory_order_relaxed);

    if (b.logFrequency.isRamping() || b.gainDb.isRamping() || b.q.isRamping() || offsetMoved)
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
        case Entry::Gain: b.running = b.gainDb.isRamping() || b.gainDb.value() != 0.0 || b.dynOffset != 0.0; break;
        case Entry::Park: b.running = b.logFrequency.isRamping() || b.logFrequency.value() != parkLog; break;
        case Entry::Wet:  b.running = b.wet.isRamping() || b.wet.value() != 0.0; break;
    }
}

void PreEqProcessor::process(float* const* channels, int numChannels, int numSamples, int controlPhase) noexcept
{
    const bool stereo = numChannels > 1;
    for (size_t index = 0; index < bands.size(); ++index)
    {
        auto& b = bands[index];
        if (!b.running)
            continue;
        const bool dynamic = b.target.dynamic && b.wanted;
        int phase = controlPhase;
        for (int i = 0; i < numSamples; ++i)
        {
            if (phase == 0)
            {
                advanceControl(b, index);
                if (!b.running)
                    break;
            }
            phase = (phase + 1) % kControlInterval;

            b.sets[static_cast<size_t>(b.active)].advance();
            if (b.crossfading)
                b.sets[static_cast<size_t>(b.active ^ 1)].advance();

            const double xl = channels[0][i];
            const double xr = stereo ? static_cast<double>(channels[1][i]) : 0.0;
            if (dynamic)
                detect(b, xl, xr, numChannels);
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

    // SOLO: listen to the soloed point's region (10 ms in and out)
    if (soloBand < 0 && soloMix <= 0.0)
        return;
    const double target = soloBand >= 0 ? 1.0 : 0.0, step = 1.0 / (0.01 * sampleRate);
    for (int i = 0; i < numSamples; ++i)
    {
        soloMix = target > soloMix ? std::min(target, soloMix + step) : std::max(target, soloMix - step);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const double x = channels[ch][i];
            const double y = svf(soloState[static_cast<size_t>(ch)], soloG, soloK, soloType, x);
            channels[ch][i] = static_cast<float>(x + soloMix * (y - x));
        }
    }
    if (soloMix <= 0.0)
        soloState = {};
}

bool PreEqProcessor::isNeutral() const noexcept
{
    for (const auto& b : bands)
        if (b.running)
            return false;
    return !isSoloing();
}

BandDesign PreEqProcessor::currentDesign(int index) const noexcept
{
    const auto& b = bands[static_cast<size_t>(index)];
    const auto& set = b.sets[static_cast<size_t>(b.active)];
    return designBand(set.shape, set.slope, sampleRate, std::exp2(b.logFrequency.value()), b.gainDb.value(), b.q.value());
}
} // namespace hpeq::dsp
