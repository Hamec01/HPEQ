#include "DSP/HamloUdarnik.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1.0e-9;

double coefficientFor(double ms, double sampleRate) noexcept
{
    return std::exp(-1.0 / (std::max(0.01, ms) * 1.0e-3 * sampleRate));
}

double follow(double e, double x, double attack, double release) noexcept
{
    return x + (x > e ? attack : release) * (e - x);
}

double db(double ratio) noexcept { return 20.0 * std::log10(ratio); }
} // namespace

int UdarnikCore::lookaheadSamples(int index, double sampleRate) noexcept
{
    static constexpr double ms[] { 0.0, 2.0, 5.0 };
    return static_cast<int>(std::lround(ms[std::clamp(index, 0, 2)] * 1.0e-3 * sampleRate));
}

void UdarnikCore::prepare(double sampleRate)
{
    fs = sampleRate;
    capacity = lookaheadSamples(2, fs) + 2;
    decimation = std::max(1, static_cast<int>(std::lround(fs / 12000.0)));
    historySize = static_cast<int>(std::ceil(0.08 * fs / decimation)) + 4;
    windowBuckets = static_cast<int>(std::ceil(0.010 * fs / decimation));
    riseWindow = std::max(1, static_cast<int>(std::lround(0.0005 * fs)));
    {
        // K-weighting, BS.1770-4 analog prototypes (as in Analysis/LoudnessMeter.cpp)
        const double f0 = 1681.974450955533, g = 3.999843853973347, q = 0.7071752369554196;
        const double k = std::tan(kPi * f0 / fs);
        const double vh = std::pow(10.0, g / 20.0), vb = std::pow(vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;
        kWeight.s0 = (vh + vb * k / q + k * k) / a0;
        kWeight.s1 = 2.0 * (k * k - vh) / a0;
        kWeight.s2 = (vh - vb * k / q + k * k) / a0;
        kWeight.sa1 = 2.0 * (k * k - 1.0) / a0;
        kWeight.sa2 = (1.0 - k / q + k * k) / a0;
        const double fh = 38.13547087602444, qh = 0.5003270373238773;
        const double kh = std::tan(kPi * fh / fs);
        const double ah = 1.0 + kh / qh + kh * kh;
        kWeight.ha1 = 2.0 * (kh * kh - 1.0) / ah;
        kWeight.ha2 = (1.0 - kh / qh + kh * kh) / ah;
    }
    for (auto& c : audio)
        c.assign(static_cast<size_t>(capacity), 0.0f);
    for (auto* r : { &level, &wa, &ws })
        r->assign(static_cast<size_t>(capacity), 0.0f);
    history.assign(static_cast<size_t>(historySize), 0.0f);
    dequeValue.assign(static_cast<size_t>(windowBuckets + 2), 0.0f);
    dequeIndex.assign(static_cast<size_t>(windowBuckets + 2), 0);
    lookahead = -1;
    reset();
}

void UdarnikCore::reset() noexcept
{
    clearState();
    meterEvents.store(0, std::memory_order_relaxed);
}

void UdarnikCore::clearState() noexcept
{
    counter = 0;
    fast = slow = e1 = e10 = accent = riseSum = hold = 0.0;
    holdLeft = 0;
    armed = true;
    lastOnset = -1000000;
    events = {};
    newest = -1;
    eventCount = 0;
    bucketMax = 0.0f;
    dequeHead = dequeTail = 0;
    std::fill(history.begin(), history.end(), 0.0f);
    historyLast = -1;
    for (auto& c : audio)
        std::fill(c.begin(), c.end(), 0.0f);
    for (auto* r : { &level, &wa, &ws })
        std::fill(r->begin(), r->end(), 0.0f);
    govT0 = -1;
    peakDecay = risePeak = riseFadeFrom = 0.0;
    pastPeak = riseActive = false;
    riseFadeLeft = 0;
    gainSmooth = beta = betaRaw = 0.0;
    aIn = aOut = bIn = 0.0;
    kWeight.z = {};
    mAin = mAout = mBin = mBout = 0.0;
    capT0 = -1;
    capBins = 0;
}

float UdarnikCore::envelopeAt(double t) const noexcept
{
    if (historyLast < 0)
        return 0.0f;
    t = std::clamp(t, static_cast<double>(std::max(0LL, historyLast - static_cast<long long>(historySize - 2) * decimation)),
                   static_cast<double>(historyLast));
    const double j = t / decimation;
    const auto j0 = static_cast<long long>(std::floor(j));
    const double frac = j - static_cast<double>(j0);
    const float v0 = history[static_cast<size_t>(j0 % historySize)];
    if (frac <= 0.0 || (j0 + 1) * decimation > historyLast)
        return v0;
    const float v1 = history[static_cast<size_t>((j0 + 1) % historySize)];
    return static_cast<float>(v0 + (v1 - v0) * frac);
}

void UdarnikCore::publishHit() noexcept
{
    if (capBins < 8)
        return;
    hitVersion.fetch_add(1, std::memory_order_acq_rel); // odd: writing
    for (int i = 0; i < kGraphPoints; ++i)
    {
        hitIn[static_cast<size_t>(i)].store(i < capBins ? capIn[static_cast<size_t>(i)] : 0.0f, std::memory_order_relaxed);
        hitOut[static_cast<size_t>(i)].store(i < capBins ? capOut[static_cast<size_t>(i)] : 0.0f, std::memory_order_relaxed);
    }
    hitVersion.fetch_add(1, std::memory_order_release);
    hitSequence.fetch_add(1, std::memory_order_relaxed);
}

int UdarnikCore::lastHit(std::array<float, kGraphPoints>& in, std::array<float, kGraphPoints>& out) const noexcept
{
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        const int v0 = hitVersion.load(std::memory_order_acquire);
        if ((v0 & 1) != 0)
            continue;
        for (int i = 0; i < kGraphPoints; ++i)
        {
            in[static_cast<size_t>(i)] = hitIn[static_cast<size_t>(i)].load(std::memory_order_relaxed);
            out[static_cast<size_t>(i)] = hitOut[static_cast<size_t>(i)].load(std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        if (hitVersion.load(std::memory_order_relaxed) == v0)
            return hitSequence.load(std::memory_order_relaxed);
    }
    return 0;
}

UdarnikCore::Meters UdarnikCore::meters() const noexcept
{
    Meters m;
    m.impactDb = meterImpact.load(std::memory_order_relaxed);
    m.gainMinDb = meterGainMin.load(std::memory_order_relaxed);
    m.gainMaxDb = meterGainMax.load(std::memory_order_relaxed);
    m.inputDb = meterInput.load(std::memory_order_relaxed);
    m.events = meterEvents.load(std::memory_order_relaxed);
    m.confidence = meterConfidence.load(std::memory_order_relaxed);
    m.strengthDb = meterStrength.load(std::memory_order_relaxed);
    return m;
}

void UdarnikCore::process(float* const* channels, int numChannels, int numSamples, const TransientSettings& p) noexcept
{
    const int style = std::clamp(static_cast<int>(std::lround(p.values[slot::kTrStyle])), 0, 2);
    const int L = std::min(capacity - 2, lookaheadSamples(static_cast<int>(std::lround(p.values[slot::kTrLookahead])), fs));
    if (L != lookahead)
    {
        lookahead = L;
        clearState();
    }
    const double snap = std::clamp(p.values[slot::kAttackGainDb], -24.0f, 24.0f);
    const double body = std::clamp(p.values[slot::kSustainGainDb], -24.0f, 24.0f);
    const double speed = 0.5 + std::clamp(p.values[slot::kTransientSpeed], 0.0f, 1.0f);
    const double rise = p.values[slot::kTrRiseLog2] == 0.0f ? 1.0 : std::exp2(std::clamp(p.values[slot::kTrRiseLog2], -1.0f, 1.0f));
    const double decayK = p.values[slot::kTrDecayLog2] == 0.0f ? 1.0 : std::exp2(std::clamp(p.values[slot::kTrDecayLog2], -1.0f, 1.0f));
    const double sigma = 1.0 - std::clamp(p.values[slot::kTrStrictness], 0.0f, 1.0f);
    const double preserve = std::clamp(p.values[slot::kTrPreserve], 0.0f, 1.0f);
    const int listen = std::clamp(static_cast<int>(std::lround(p.values[slot::kTrListen])), 0, 2);
    const float outputGain = static_cast<float>(std::pow(10.0, std::clamp(p.values[slot::kTrOutputDb], -24.0f, 12.0f) / 20.0));
    const bool fullSensitivity = sigma >= 1.0;
    const bool m6Detection = style == Pravda && fullSensitivity;

    const double fastA = coefficientFor(0.5, fs), fastR = coefficientFor(20.0 * speed, fs);
    const double slowA = coefficientFor(20.0 * speed, fs), slowR = coefficientFor(250.0 * speed, fs);
    const double e1A = coefficientFor(0.05, fs), e1R = coefficientFor(1.0, fs);
    const double e10A = coefficientFor(1.0, fs), e10R = coefficientFor(10.0, fs);
    const double accentR = coefficientFor(2000.0, fs), holdR = coefficientFor(200.0, fs);
    const double smoothA = coefficientFor(0.1, fs), betaA = coefficientFor(20.0, fs), sumA = coefficientFor(300.0, fs);
    const int holdSamples = static_cast<int>(0.060 * fs);
    const long long eventSpan = static_cast<long long>(0.010 * fs);
    const long long confidenceSpan = static_cast<long long>(0.005 * fs);
    const double thetaOn = 6.0 + 18.0 * (1.0 - sigma);
    const double gateDb = 12.0 - 72.0 * sigma;
    const double attackWindow = 0.020 * speed * fs;
    const double rampIn = 0.00025 * fs;
    const long long riseSpan = static_cast<long long>(0.020 * fs);
    const double balanceHold = 0.300 * fs, balanceFade = 0.100 * fs;
    const int riseFade = std::max(1, static_cast<int>(0.002 * fs));
    const double halfL = 0.5 * L, quarterL = 0.25 * L;
    const double dbToLog = std::log(10.0) / 20.0;

    const auto gateOk = [&](const Event& e) { return fullSensitivity || e.strengthDb >= gateDb; };
    const auto attackCurve = [&](double delta)
    {
        if (L > 0)
        {
            if (delta < -halfL)
                return 0.0;
            if (delta < -quarterL)
                return (delta + halfL) / (halfL - quarterL);
        }
        else
        {
            if (delta < 0.0)
                return 0.0;
            if (delta < rampIn)
                return (delta + 1.0) / (rampIn + 1.0);
        }
        if (delta < 0.7 * attackWindow)
            return 1.0;
        if (delta < attackWindow)
            return 0.5 + 0.5 * std::cos(kPi * (delta - 0.7 * attackWindow) / (0.3 * attackWindow));
        return 0.0;
    };
    const auto preRamp = [&](double delta, double wMax)
    {
        if (L <= 0 || delta < -halfL)
            return 0.0;
        if (delta < -quarterL)
            return wMax * (delta + halfL) / (halfL - quarterL);
        if (delta <= 0.0)
            return wMax;
        if (delta < halfL)
            return wMax * (1.0 - delta / halfL);
        return 0.0;
    };

    float blockInput = 0.0f;
    double gMin = 1.0, gMax = 1.0;
    for (int i = 0; i < numSamples; ++i)
    {
        // ------------------------------------------------------------ detector (time n = counter)
        const long long n = counter;
        float lvl = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            lvl = std::max(lvl, std::abs(channels[c][i]));
        blockInput = std::max(blockInput, lvl);
        const double x = lvl;
        const double e10Before = e10;
        fast = follow(fast, x, fastA, fastR);
        slow = follow(slow, x, slowA, slowR);
        e1 = follow(e1, x, e1A, e1R);
        e10 = follow(e10, x, e10A, e10R);
        accent = std::max(x, accent * accentR);
        if (x >= hold)
        {
            hold = x;
            holdLeft = holdSamples;
        }
        else if (holdLeft > 0)
            --holdLeft;
        else
            hold = x + holdR * (hold - x);

        const double ns = db((e1 + kEps) / (e10 + kEps));
        if (armed && ns >= thetaOn && x > 1.0e-5)
        {
            newest = (newest + 1) % static_cast<int>(events.size());
            auto& e = events[static_cast<size_t>(newest)];
            e = Event {};
            e.used = true;
            e.t0 = n;
            e.before = static_cast<float>(e10Before);
            e.peak = lvl;
            armed = false;
            lastOnset = n;
            ++eventCount;
            if (style == Pravda && preserve > 0.0 && e10Before < slow) // PRESERVE: d against the level before the hit
                slow = std::exp((1.0 - preserve) * std::log(slow + kEps) + preserve * std::log(e10Before + kEps));
        }
        else if (!armed && ns < thetaOn - 3.0 && n - lastOnset >= eventSpan)
            armed = true;

        const double d = db((fast + kEps) / (slow + kEps));
        const float wAttack = static_cast<float>(std::clamp(d / 6.0, 0.0, 1.0));
        const float wSustain = static_cast<float>(std::clamp(-d / 6.0, 0.0, 1.0));
        if (newest >= 0)
        {
            auto& e = events[static_cast<size_t>(newest)];
            if (n - e.t0 < confidenceSpan) // confidence: how far the 10 ms envelope rises over its level before the hit
            {
                e.riseMax = std::max(e.riseMax, static_cast<float>(e10));
                e.confidence = static_cast<float>(std::clamp((db((e.riseMax + kEps) / (e.before + kEps)) - 6.0) / 6.0, 0.0, 1.0));
            }
            if (n - e.t0 < eventSpan)
            {
                e.peak = std::max(e.peak, lvl);
                e.strengthDb = static_cast<float>(db((e.peak + kEps) / (accent + kEps)));
            }
            if (n - e.t0 <= std::max(L, 1))
                e.wMax = std::max(e.wMax, wAttack);
        }

        // sliding max of the level over 10 ms, in buckets of `decimation` samples
        bucketMax = std::max(bucketMax, lvl);
        const long long bucket = n / decimation;
        if (n % decimation == decimation - 1)
        {
            const int cap = static_cast<int>(dequeValue.size());
            while (dequeTail != dequeHead && dequeValue[static_cast<size_t>((dequeTail - 1 + cap) % cap)] <= bucketMax)
                dequeTail = (dequeTail - 1 + cap) % cap;
            dequeValue[static_cast<size_t>(dequeTail)] = bucketMax;
            dequeIndex[static_cast<size_t>(dequeTail)] = bucket;
            dequeTail = (dequeTail + 1) % cap;
            while (dequeIndex[static_cast<size_t>(dequeHead)] <= bucket - windowBuckets)
                dequeHead = (dequeHead + 1) % cap;
            bucketMax = 0.0f;
        }
        const double windowMax = std::max(static_cast<double>(bucketMax),
                                          dequeTail != dequeHead ? static_cast<double>(dequeValue[static_cast<size_t>(dequeHead)]) : 0.0);
        const int w = static_cast<int>(n % capacity);
        for (int c = 0; c < numChannels; ++c)
            audio[static_cast<size_t>(c)][static_cast<size_t>(w)] = channels[c][i];
        level[static_cast<size_t>(w)] = lvl;
        wa[static_cast<size_t>(w)] = wAttack;
        ws[static_cast<size_t>(w)] = wSustain;
        // rise envelope: mean level over (n - 1 ms, n], centred at n - 1 ms / 2 (stored with that time)
        riseSum += lvl;
        if (n >= riseWindow)
            riseSum -= level[static_cast<size_t>((n - riseWindow) % capacity)];
        const long long centre = n - riseWindow / 2;
        if (centre >= 0 && centre % decimation == 0)
        {
            history[static_cast<size_t>((centre / decimation) % historySize)] = static_cast<float>(std::max(0.0, riseSum) / riseWindow);
            historyLast = centre;
        }

        // ------------------------------------------------------------ audio (time m = n - L)
        const long long m = n - L;
        const int r = static_cast<int>(((m % capacity) + capacity) % capacity);
        const double lvlM = level[static_cast<size_t>(r)];

        // the governing event (newest accepted one whose window has begun) and the one before it
        const Event* gov = nullptr;
        const Event* prev = nullptr;
        for (int k = 0; k < static_cast<int>(events.size()); ++k)
        {
            const auto& e = events[static_cast<size_t>((newest - k + 2 * static_cast<int>(events.size())) % static_cast<int>(events.size()))];
            if (newest < 0 || !e.used || static_cast<double>(e.t0) - (L > 0 ? halfL : 0.0) > static_cast<double>(m) || !gateOk(e))
                continue;
            if (gov == nullptr)
                gov = &e;
            else
            {
                prev = &e;
                break;
            }
        }
        if (gov != nullptr && gov->t0 != govT0)
        {
            if (capT0 >= 0)
                publishHit();
            govT0 = gov->t0;
            peakDecay = 0.0;
            pastPeak = false;
            riseActive = true;
            risePeak = 0.0;
            riseFadeLeft = 0;
            capT0 = gov->t0;
            capBins = 0;
            capIn.fill(0.0f);
            capOut.fill(0.0f);
            meterConfidence.store(gov->confidence, std::memory_order_relaxed);
            meterStrength.store(gov->strengthDb, std::memory_order_relaxed);
        }
        const double k = gov != nullptr ? (m6Detection ? 1.0 : gov->confidence * (gateOk(*gov) ? 1.0 : 0.0)) : (m6Detection ? 1.0 : 0.0);
        double a = 0.0;
        if (gov != nullptr)
            a = std::max(attackCurve(static_cast<double>(m - gov->t0)), prev != nullptr ? attackCurve(static_cast<double>(m - prev->t0)) : 0.0);
        // body: after the attack window, while the level decays (the M6 sustain weight), so a steady signal is not "body"
        const double b = gov != nullptr ? (1.0 - a) * ws[static_cast<size_t>(r)] : 0.0;
        // UDARNIK energy balance: the hit's whole body (after the attack window, up to 300 ms, faded out by 400 ms)
        double bBalance = 0.0;
        if (gov != nullptr && style == Udarnik)
        {
            const double since = static_cast<double>(m - gov->t0);
            bBalance = (1.0 - a) * k * std::clamp(1.0 - (since - balanceHold) / balanceFade, 0.0, 1.0);
        }

        double G = 0.0, split = a;
        if (style == Pravda)
        {
            double wA = wa[static_cast<size_t>(r)];
            double wS = ws[static_cast<size_t>(r)];
            if (!fullSensitivity)
            {
                wA *= a;
                wS *= 1.0 - a;
            }
            if (gov != nullptr)
            {
                wA = std::max(wA, preRamp(static_cast<double>(m - gov->t0), gov->wMax));
                if (prev != nullptr)
                    wA = std::max(wA, preRamp(static_cast<double>(m - prev->t0), prev->wMax));
            }
            G = k * (snap * wA + body * wS);
            split = wA;
        }
        else if (style == Udarnik)
        {
            if (snap != 0.0 && bIn > 1.0e-20)
            {
                if ((n & 15) == 0)
                    betaRaw = std::clamp(10.0 * std::log10(std::max(1.0e-30, aIn + bIn - aOut) / bIn), -12.0, 6.0);
                beta = betaRaw + betaA * (beta - betaRaw);
            }
            G = k * (snap * a + body * b) + beta * bBalance;
        }
        else
        {
            double gRise = 0.0, gDecay = 0.0;
            if (gov != nullptr && m >= gov->t0)
            {
                peakDecay = std::max(peakDecay, windowMax);
                if (!pastPeak && windowMax < peakDecay * 0.9440608762859234) // -0.5 dB
                    pastPeak = true;
                if (pastPeak && decayK != 1.0 && peakDecay > 0.0)
                    gDecay = std::clamp((1.0 / decayK - 1.0) * std::max(db((windowMax + kEps) / peakDecay), -60.0), -36.0, 24.0);
                if (rise != 1.0)
                {
                    if (riseActive)
                    {
                        double u = static_cast<double>(gov->t0) + static_cast<double>(m - gov->t0) / rise;
                        u = std::min(u, static_cast<double>(n));
                        const double eu = envelopeAt(u), em = envelopeAt(static_cast<double>(m));
                        risePeak = std::max(risePeak, eu);
                        // faster (r < 1): the target holds the peak it reached until the source itself gets there
                        const double target = rise < 1.0 ? risePeak : eu;
                        gRise = em > 1.0e-6 ? std::clamp(db((target + kEps) / (em + kEps)), -24.0, 24.0) : 0.0;
                        const bool done = rise < 1.0 ? (eu < risePeak && em >= risePeak * 0.9885530946569389) // source within 0.1 dB of the peak
                                                     : eu < risePeak * 0.8912509381337456;                     // source time past the peak (-1 dB)
                        if (done || u - static_cast<double>(gov->t0) > static_cast<double>(riseSpan))
                        {
                            riseActive = false;
                            riseFadeFrom = gRise;
                            riseFadeLeft = riseFade;
                        }
                    }
                    else if (riseFadeLeft > 0)
                        gRise = riseFadeFrom * static_cast<double>(riseFadeLeft--) / static_cast<double>(riseFade);
                }
            }
            G = k * (snap * a + body * b + gRise + gDecay);
        }

        gainSmooth = G + smoothA * (gainSmooth - G);
        double g = gainSmooth == 0.0 ? 1.0 : std::exp(gainSmooth * dbToLog);
        // UDARNIK, gain above 1: y = s tanh(g artanh(x / s)) — g x for small x, never beyond s (the peak reference)
        const bool peakNeutral = style == Udarnik && g > 1.0;
        double ceiling = 1.0, guard = 1.0;
        if (peakNeutral)
        {
            const double eventPeak = gov != nullptr ? gov->peak : windowMax;
            double reference = hold;
            if (preserve >= 1.0)
                reference = eventPeak;
            else if (preserve > 0.0)
                reference = std::exp((1.0 - preserve) * std::log(hold + kEps) + preserve * std::log(eventPeak + kEps));
            ceiling = std::max({ reference, windowMax, lvlM, 1.0e-9 });
            // true-peak guard: the shaped peak lands 0.1 * boost (dB) under the reference; slope g at 0 is kept
            guard = std::exp(-0.1 * std::log(g)); // = 10^(-0.1 * 20 log10 g / 20)
        }
        gMin = std::min(gMin, g);
        gMax = std::max(gMax, g);

        double x2 = 0.0, y2 = 0.0, yPeak = 0.0, x2k = 0.0, y2k = 0.0;
        const auto gf = static_cast<float>(g);
        const auto wf = static_cast<float>(std::clamp(split, 0.0, 1.0));
        for (int c = 0; c < numChannels; ++c)
        {
            const float xd = audio[static_cast<size_t>(c)][static_cast<size_t>(r)];
            float y = xd * gf;
            if (peakNeutral)
            {
                const double u = std::clamp(static_cast<double>(xd) / ceiling, -0.9999999, 0.9999999);
                y = static_cast<float>(ceiling * guard * std::tanh(g / guard * std::atanh(u)));
            }
            x2 += static_cast<double>(xd) * xd;
            y2 += static_cast<double>(y) * y;
            yPeak = std::max(yPeak, static_cast<double>(std::abs(y)));
            if (style == Udarnik)
            {
                const double xk = kWeight.process(2 * c, xd), yk = kWeight.process(2 * c + 1, y);
                x2k += xk * xk;
                y2k += yk * yk;
            }
            if (listen == 1)
                y = y * wf;
            else if (listen == 2)
                y = y - y * wf;
            if (outputGain != 1.0f)
                y *= outputGain;
            channels[c][i] = y;
        }
        if (style == Udarnik)
        {
            aIn = a * k * x2k + sumA * (aIn - a * k * x2k);
            aOut = a * k * y2k + sumA * (aOut - a * k * y2k);
            bIn = bBalance * x2k + sumA * (bIn - bBalance * x2k);
        }
        const double ws1 = std::clamp(split, 0.0, 1.0);
        mAin = ws1 * x2 + sumA * (mAin - ws1 * x2);
        mAout = ws1 * y2 + sumA * (mAout - ws1 * y2);
        mBin = (1.0 - ws1) * x2 + sumA * (mBin - (1.0 - ws1) * x2);
        mBout = (1.0 - ws1) * y2 + sumA * (mBout - (1.0 - ws1) * y2);

        if (capT0 >= 0 && m >= capT0)
        {
            const int bin = static_cast<int>((m - capT0) / std::max(1LL, static_cast<long long>(0.0025 * fs)));
            if (bin < kGraphPoints)
            {
                capIn[static_cast<size_t>(bin)] = std::max(capIn[static_cast<size_t>(bin)], static_cast<float>(lvlM));
                capOut[static_cast<size_t>(bin)] = std::max(capOut[static_cast<size_t>(bin)], static_cast<float>(yPeak));
                capBins = std::max(capBins, bin + 1);
            }
            else
            {
                publishHit();
                capT0 = -1;
            }
        }
        ++counter;
    }

    meterInput.store(static_cast<float>(db(blockInput + 1.0e-6)), std::memory_order_relaxed);
    meterGainMin.store(static_cast<float>(db(gMin)), std::memory_order_relaxed);
    meterGainMax.store(static_cast<float>(db(gMax)), std::memory_order_relaxed);
    const double impact = (mAin > 1.0e-20 && mBin > 1.0e-20 && mAout > 1.0e-30 && mBout > 1.0e-30)
                              ? 10.0 * std::log10(mAout / mAin) - 10.0 * std::log10(mBout / mBin) : 0.0;
    meterImpact.store(static_cast<float>(impact), std::memory_order_relaxed);
    meterEvents.store(eventCount, std::memory_order_relaxed);
}
} // namespace hamlo::dsp
