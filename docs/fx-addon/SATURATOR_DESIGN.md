# HamloMag — design note (module pass 4: Saturator)

Owner, 2026-10-23: "Soundtoys Decapitator, но придумай что-то наше: подмешивать советские шумы
аналогов + разработай научную формулу, которой нет у других; имитация лент, пластинки и насыщение
гармоник; дизайн такой же — можно сделать бобины".

## 1. Function parity (Decapitator) and what is ours

| Function | Decapitator | HamloMag |
|---|---|---|
| Analog models | 5 styles (A, E, N, T, P) | **MODEL**: TAPE, TUBE, IRON (transformer), GERMANIUM, VINYL (+ CLASSIC = the M6 saturator) |
| Drive, Punish (+20 dB) | yes | **Drive**, **PUNISH** |
| Tone (tilt) | yes | **Tone** (±6 dB tilt around 1 kHz) |
| Low cut + Thump, High cut, Steep | yes | **Low cut** + **THUMP**, **High cut**, **STEEP** (12 → 24 dB/oct) |
| Mix, Output, Auto | yes | module **Mix**, **Output**, **AUTO** |
| Harmonic shaping | — | **Harmonics** amount and **Even / Odd** balance (the HamloLaw, §2) |
| Tape | — | **SPEED** 19 / 9.5 / 4.76 cm/s (head bump, high loss), **Bias** (hysteresis), **Wow / flutter** |
| Vinyl | — | **SPEED** 33 / 45 / 78 rpm (eccentric wow, bandwidth), pre-emphasis distortion of the highs |
| Soviet analog noises | — | **Hiss** (model-shaped), **Hum** (50 Hz mains + rectifier buzz), **Crackle** (pops and clicks), **NOISE: ALWAYS / FOLLOW** |
| Oversampling | — | 1× / 2× / 4× (half-band FIR, as M6) |

## 2. The HamloLaw — envelope-normalised Chebyshev harmonics with the power law of real hardware

A weakly nonlinear device (tube stage, tape, transformer) behaves like a power series: for a tone of
amplitude A, its n-th harmonic grows as Aⁿ, i.e. (n−1) dB faster than the tone per dB, until the device
saturates. Ordinary waveshapers inherit this only as a by-product of one curve, so the harmonic
balance cannot be chosen without changing the curve.

HamloMag adds a harmonic layer where every harmonic is chosen on its own and still obeys that law:

```
E   = peak envelope of x (instant attack, 25 ms hold, 50 ms release)          u = clamp(x / E, -1, 1)
ℓ   = tanh(E / E_m)                                                E_m: the model's knee level
k_n = the model's signature (even / odd balance applied)            n = 2 … 7
h   = E_m · Σ H k_n ℓ^n · T_n(u)                                   T_n: Chebyshev polynomials
y   = core(x) + h
```

For a steady tone u = cos θ, and T_n(cos θ) = cos(nθ): the layer produces exactly harmonic n with
amplitude E_m · H · k_n · tanh(A/E_m)^n — at low level ≈ H k_n Aⁿ / E_mⁿ⁻¹, so the n-th harmonic rises
(n−1) dB per dB above the tone, and when driven it settles at E_m · H · k_n (bounded at any drive). The model sets k_n (TUBE: strong 2nd; TAPE
and IRON: 3rd; GERMANIUM: 3rd and 5th; VINYL: 2nd and 3rd); the Even / Odd knob tilts it. The core is the
model's own nonlinearity (tape hysteresis, asymmetric triode, frequency-dependent transformer core,
germanium clip), which supplies compression and the waveform. Combining an envelope-normalised Chebyshev
bank with a per-harmonic power law is our construction; we have not seen it in the plugins we know, but
we cannot prove no one has done it.

## 3. Engine (written for HPEQ, D3)

```
x -> drive (+ punish) -> [model pre-emphasis] -> oversampled { core + HamloLaw harmonics } -> [de-emphasis,
     head bump, high loss] -> wow / flutter (tape, vinyl) -> + noises (hiss, hum, crackle; ALWAYS or FOLLOW)
  -> tone tilt -> low cut (+ thump) / high cut (12 or 24 dB/oct) -> DC block -> auto gain / output
```

- TAPE: pre / de-emphasis around the core (highs saturate first), hysteresis whose width follows Bias,
  head bump (60 / 45 / 35 Hz) and high loss (16 / 10 / 6 kHz) by speed, wow 0.6 Hz + flutter 7 Hz.
- TUBE: asymmetric triode curve (grid conduction on positive swings), pink hiss.
- IRON: lows saturate harder than highs (core saturation follows flux, i.e. the integral of the signal).
- GERMANIUM: asymmetric hard-knee clip, odd-rich, white hiss.
- VINYL: strong pre-emphasis (inner-groove distortion of highs), eccentric wow at 33⅓ / 45 / 78 rpm, 78 rpm
  band-limited (100 Hz – 5 kHz) like shellac, surface noise and crackle.
- Wow is a modulated delay around 4 ms; TAPE and VINYL report that delay as latency (PDC), so the node
  stays aligned. Noises come from a seeded generator: renders do not depend on the block size.
- CLASSIC keeps the M6 law bit-exact (projects saved before this pass have no `model`).

## 4. Page (HamloMag, Black Steel)

Two aluminium tape reels at the top (they turn with the signal, at the speed set), the tape running
over guide rollers and a head block under the nameplate **HamloMag · MODEL МАГ-4 · № 0422**, amber LCD.
SPEED and OVERSAMPLING levers between the reels and the nameplate. MODEL selector (chicken-head, six
positions) on the left; green phosphor CRT in the centre — **CURVE** (the transfer curve at the current
drive, the tape hysteresis loop drawn) and **HARMONICS** (bars of H1 … H9 for the live input level,
computed from the engine itself); OUTPUT needle meter, ON lamp, PUNISH / THUMP / STEEP / AUTO toggles on
the right. Large knobs: Drive, Tone, Low cut, High cut, Mix, Output. Small knobs: Harmonics, Even / Odd,
Bias, Wow, Hiss, Hum, Crackle, and the NOISE ALWAYS / FOLLOW toggle.

## 5. Measurements (thresholds fixed before the runs, see the pass report)

Classic bit-exact; HamloLaw: a sine's harmonics 2…7 equal the formula ±0.5 dB, and below the knee the
2nd / 3rd harmonics rise 2 / 3 dB per dB of input (±10 %); models differ (TUBE even > odd, GERMANIUM odd
> even); TAPE head bump and high loss by speed; IRON distorts 50 Hz more than 1 kHz at the same level;
VINYL 78 rpm band-limited; wow present only in TAPE / VINYL and the reported latency equals the measured;
hum is 50 Hz with harmonics; crackle density follows the knob; FOLLOW silences the noise without input;
auto gain keeps small signals at unity; no DC; alias products with 4× below those at 1×; block sizes
identical; fuzz finite; 0 allocations; CPU reported.
