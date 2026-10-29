#pragma once

#include <cmath>
#include <complex>
#include <vector>

#if defined(_MSC_VER)
 #define HPEQ_RESTRICT __restrict
#else
 #define HPEQ_RESTRICT __restrict__
#endif

namespace hpeq::dsp
{
// Complex FFT for HamloRevolution's CLEAN stretch (pass 10b, P6). Iterative radix-2 on split real /
// imaginary arrays: the first two stages need no multiplications, the later ones walk the twiddles of
// their stage in order (stored one stage after another), so the compiler can vectorise the butterflies.
// Out of place; the inverse is scaled by 1/N (as juce::dsp::FFT). prepare() allocates; perform() does not.
class RevolutionFft
{
public:
    using C = std::complex<float>;

    void prepare(int newSize)
    {
        size = newSize;
        reversed.assign(static_cast<size_t>(size), 0);
        int bits = 0;
        while ((1 << bits) < size)
            ++bits;
        for (int i = 0; i < size; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                r |= ((i >> b) & 1) << (bits - 1 - b);
            reversed[static_cast<size_t>(i)] = r;
        }
        twRe.clear();
        twIm.clear();
        for (int half = 1; half < size; half <<= 1)
            for (int j = 0; j < half; ++j)
            {
                const double a = -3.14159265358979323846 * j / half;
                twRe.push_back(static_cast<float>(std::cos(a)));
                twIm.push_back(static_cast<float>(std::sin(a)));
            }
        re.assign(static_cast<size_t>(size), 0.0f);
        im.assign(static_cast<size_t>(size), 0.0f);
    }

    void perform(const C* in, C* out, bool inverse) noexcept
    {
        float* HPEQ_RESTRICT r = re.data();
        float* HPEQ_RESTRICT m = im.data();
        const float sign = inverse ? -1.0f : 1.0f; // the inverse is conj(FFT(conj(x))) / N
        for (int i = 0; i < size; ++i)
        {
            const auto k = static_cast<size_t>(reversed[static_cast<size_t>(i)]);
            r[k] = in[i].real();
            m[k] = sign * in[i].imag();
        }
        // stage half = 1: w = 1
        for (int i = 0; i < size; i += 2)
        {
            const float ar = r[i], ai = m[i], br = r[i + 1], bi = m[i + 1];
            r[i] = ar + br;
            m[i] = ai + bi;
            r[i + 1] = ar - br;
            m[i + 1] = ai - bi;
        }
        // stage half = 2: w = 1, -i
        if (size >= 4)
            for (int i = 0; i < size; i += 4)
            {
                float ar = r[i], ai = m[i], br = r[i + 2], bi = m[i + 2];
                r[i] = ar + br;
                m[i] = ai + bi;
                r[i + 2] = ar - br;
                m[i + 2] = ai - bi;
                ar = r[i + 1];
                ai = m[i + 1];
                br = m[i + 3]; // (b) * (-i) = (bi, -br)
                bi = -r[i + 3];
                r[i + 1] = ar + br;
                m[i + 1] = ai + bi;
                r[i + 3] = ar - br;
                m[i + 3] = ai - bi;
            }
        const float* wr = twRe.data() + 3;
        const float* wi = twIm.data() + 3;
        for (int half = 4; half < size; half <<= 1)
        {
            for (int i = 0; i < size; i += 2 * half)
            {
                float* HPEQ_RESTRICT aR = r + i;
                float* HPEQ_RESTRICT aI = m + i;
                float* HPEQ_RESTRICT bR = r + i + half;
                float* HPEQ_RESTRICT bI = m + i + half;
                for (int j = 0; j < half; ++j)
                {
                    const float vr = bR[j] * wr[j] - bI[j] * wi[j];
                    const float vi = bR[j] * wi[j] + bI[j] * wr[j];
                    const float xr = aR[j], xi = aI[j];
                    aR[j] = xr + vr;
                    aI[j] = xi + vi;
                    bR[j] = xr - vr;
                    bI[j] = xi - vi;
                }
            }
            wr += half;
            wi += half;
        }
        const float scale = inverse ? 1.0f / static_cast<float>(size) : 1.0f;
        for (int i = 0; i < size; ++i)
            out[i] = C(r[i] * scale, sign * m[i] * scale);
    }

    int getSize() const noexcept { return size; }

private:
    int size = 0;
    std::vector<int> reversed;
    std::vector<float> twRe, twIm, re, im;
};
} // namespace hpeq::dsp
