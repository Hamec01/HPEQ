# HamloMag v2: design and targets

Owner (2026-10-28): HamloMag next, "what can we give it to compete with its analogues". Agreed set: a sampler
model (SP-1200 / MPC60 / S950), bass that translates to phones, sub protection, drive that follows the hit,
loudness-matched comparison, 8x oversampling and DELTA, harmonics on the screen, own switch names, 200 genre presets
and the preset screen (Mid / Side later). Targets fixed **before** the baseline and the code.

## 1. What the references do

| Reference | Published behaviour used here |
|---|---|
| Soundtoys Decapitator | five voiced hardware models, DRIVE, TONE, LOW / HIGH CUT, MIX, a gain-boost switch for heavy distortion [1] |
| FabFilter Saturn 2 | 28 styles; up to 6 bands with 6 .. 48 dB/oct crossovers, each its own drive; modulation (envelope followers, LFOs) of any control [2] |
| SP-1200 | 26.04 kHz, 12-bit; its sound credited to the low rate, its aliasing / imaging and the SSM2044 analog low-pass [3] |
| Akai MPC60 | fixed 40 kHz, 12-bit (non-linear), about 18 kHz bandwidth; sampling engine from the S900 [4] |
| Akai S950 | 12-bit, variable rate 7.5 .. 40 / 48 kHz (sources disagree), a low-pass filter credited for its sound [4] |
| Psychoacoustic bass (MaxxBass) | split off the lows, generate their 2nd .. 4th harmonics, add them: the ear hears the fundamental a small speaker cannot play [5] |

So HamloMag v2 adds what the two big references do not have for hip-hop and electronic producers - the samplers and
bass translation - takes Saturn's two most useful ideas in a one-knob form (a protected sub band, drive following the
envelope), and keeps its own models. Every new control defaults to off: a project saved before sounds the same.

## 2. New controls (all new parameters; the HPEQ IDs stay)

As built (DSP/MagV2.h, the plug-in processor); the changes against the first design are marked and reported in
docs/reports/v2/V2_REPORT.md.

- **SAMPLER** (`sampler`: Off / SP-1200 / MPC60 / S950): before the model, at 4x: MPC60 / S950 an anti-alias
  low-pass (a 257-tap Kaiser FIR at 18 / 14 kHz, computed only at the device's ticks; *first built as an 8th-order
  Butterworth*), SP-1200 none (it aliases); sampled at 26.04 / 40 / 32 kHz, 12-bit (MPC60 companded, mu = 7), held
  like the DAC, output low-pass 12 / 18 / 14 kHz (4th order); latency the 4x oversampler's + 32 samples.
- **PHONE** (`phoneBass` 0 .. 100 %): from everything below 120 Hz, H2 .. H6 by Chebyshev polynomials of the
  envelope-normalised signal (*first H2 .. H4*), band-limited to 70 .. 700 Hz, added; the sub untouched.
- **SUB** (`subProtectHz`: 0 = off, up to 300 Hz): 4th-order Linkwitz-Riley split; the high band saturates, the low
  band is delayed to match (CLASSIC: through the same 10 Hz DC blocker) and added back.
- **DYNAMICS** (`dynamics` -100 .. +100 %): the saturated share follows the hit: w = 1 - |DYNAMICS| (1 - s),
  s = the hit measure (fast / slow envelopes) at +, 1 - it at -; y = dry + w (saturated - dry). *First built as a
  +-12 dB drive change with the level given back - HamloLaw's bounded harmonics made it work backwards.*
- **MATCH** (`match`): K-weighted output loudness follows the input's: 100 ms blocks gated like BS.1770 (-10 dB re
  the 10 s level), 1.5 s average, 0.5 s smoothing (*first 3 s per-sample mean squares, ungated*).
- **OVERSAMPLE 8x** (the `oversampling` choice gets a fourth entry).
- **DELTA** (`delta`): the output minus the aligned input.
- Own switch names on the page and in the host: PUNISH -> OVERLOAD, THUMP -> BUMP, STEEP -> 24 DB (IDs unchanged).

## 3. Measurements (`HamloMag_Lab`) and targets

| # | Check | Target |
|---|---|---|
| G1 | New controls at their defaults: the output equals HamloMag 0.1.0 (every model, 1x / 2x / 4x, drums) | bit-identical |
| G2 | Aliasing: TUBE, DRIVE 24 dB, 9 kHz at -6 dBFS: the largest non-harmonic component re the fundamental, 4x / 8x | 4x: before -> after no worse; 8x <= -90 dB |
| G3 | SAMPLER SP-1200, 15 kHz at -12 dBFS: the alias at 26.04 - 15 = 11.04 kHz re the tone | >= -30 dB (it aliases, like the device) |
| G4 | SAMPLER MPC60 21 kHz -> 19 kHz, S950 18 kHz -> 14 kHz (a tone above the device's Nyquist): the input alias *(first: rate - 15 kHz - that is the DAC image for a tone under their Nyquist; reported beside)* | <= -50 dB (anti-alias filtered) |
| G5 | SAMPLER: bandwidth - response at 1 kHz vs 15 kHz (pink noise) | SP-1200 15 kHz <= -10 dB; MPC60 at 15 kHz within -6 dB; level at 1 kHz within +-1 dB |
| G6 | SAMPLER: 12-bit word - a 1 kHz tone at -60 dBFS: THD + N re the tone | linear 12-bit -6 .. -24 dB, MPC60 (companded) -20 .. -40 dB *(first -6 .. -24 for all)*; off < -60 dB |
| G7 | PHONE 100 %, 50 Hz at -12 dBFS: H2 + H3 + H4 re the fundamental; the fundamental's change; anything above 1 kHz | >= -12 dB; within +-0.5 dB; <= -60 dB |
| G8 | PHONE 100 % on the bass clips: loudness through a phone speaker (high-pass 4th order 200 Hz) | >= +4 LU |
| G9 | SUB 120 Hz, TUBE, DRIVE 24 dB, 40 Hz (-12) + 1 kHz (-18): THD of the 40 Hz part (H2 .. H5 of 40 Hz) | <= 1 %, and >= 10 x lower than SUB off |
| G10 | SUB 120 Hz, DRIVE 0 dB CLASSIC, pink noise: magnitude of the sum | flat within 0.2 dB (20 Hz .. 20 kHz) |
| G11 | DYNAMICS +100 % / -100 %, TUBE DRIVE 12 dB: the saturation's own energy N = y(x) - 2 y(x/2) in the attacks (0 .. 10 ms) minus in the body (25 .. 100 ms), against DYNAMICS 0, on clean hits (100 Hz, 150 ms decay, every 500 ms); the drum bus reported *(first: y - x on the drum bus)* | +100: >= +3 dB; -100: <= -3 dB |
| G12 | MATCH on, the mixes, TAPE / TUBE / GERMANIUM at DRIVE 0 / 12 / 24 dB: integrated loudness out - in | within +-0.5 LU |
| G13 | MATCH on, steady pink noise at DRIVE 24 dB after 3 s: gain movement (output with MATCH / without, 400 ms windows) *(first: output / input)* | within +-0.3 dB |
| G14 | DELTA on: DELTA + the aligned input = the output without DELTA | <= -120 dB |
| G15 | Block sizes 1 / 64 / 512 / 4096 after 256, every new control on | bit-identical |
| G16 | pluginval 10 + Steinberg validator | pass |

G2 (4x) and G1 run on HamloMag 0.1.0 too (the "before"); G3 .. G14 report "n/a" before.

Release calibration disclosure (2026-10-29): *first: a monotonically increasing DRIVE-to-saturation assumption
sent several bounded-HamloLaw recipes to 48 dB on the wrong branch.* Calibration now scans DRIVE in 4 dB steps
for the first target crossing, then bisects in either direction. It keeps the nearest scanned value when no
crossing exists, so an unreachable recipe is exposed by the independent final check. During recipe repair it
also prefers candidates within the existing +/-6 dB level and +3 dBFS peak constraints, measured on the quiet
output, over a closer midpoint on a heavily attenuated branch. Material, saturation-share
formula and depth / peak / level bands are unchanged. Only unused peak / noise rendering is skipped during
calibration; `check` still measures the complete preset.

## Sources

### Follow-up refinement (2026-10-29)

The owner requested that G2 and G12 meet their original targets: <= -90 dB at 8x and <= 0.5 LU MATCH error.
Baseline remains the 0.2.0 report (-77.8 dB / 0.58 LU). The laboratory and thresholds stay unchanged.
The first candidate strengthens only the 8x FIR stages to -120 dB rejection and speeds the MATCH power average
to 0.5 s with 0.1 s gain smoothing. Legacy 1x / 2x / 4x and MATCH-off paths must retain bit identity.
Latency is remeasured from the actual FIR chain and reported to the host; G13 steady-noise movement must still
remain within +/-0.3 dB. These candidate settings are subject to the same complete lab, block and preset checks.

Final follow-up design: 8x TUBE uses first-order ADAA on the original piecewise tube curve, alongside the stronger
FIR chain. MATCH retains the original averaging / smoothing and adds compensated-output residual feedback.
G2 -105.6 dB, G12 0.49 LU, G13 +/-0.08 dB; all 15 lab targets pass. Full experiment history and regression
results: `docs/reports/v2/TARGETS_FIX_REPORT.md`. Fixed host latency at 48 kHz is now 395 samples.

1. [Soundtoys Decapitator manual](https://www.fullcompass.com/common/files/33650-SoundToysDecapitatorManual.pdf), [Dubspot: Decapitator vs Saturn 2](https://blog.dubspot.com/plugins/compare/decapitator-vs-saturn-2)
2. [MusicTech: FabFilter Saturn 2](https://musictech.com/news/fabfilter-saturn-2/), [Production Expert: Saturn 2](https://www.pro-tools-expert.com/production-expert-1/2020/5/19/saturn-2-from-fabfilter-new-version-offers-everything-for-saturation-and-distortion-from-subtle-to-wild)
3. [Rossum Electro: SP-1200 education](https://www.rossum-electro.com/pages/education-sp-1200), [Rossum SP-1200](https://www.rossum-electro.com/products/sp-1200/), [Sound On Sound: Isla S2400](https://www.soundonsound.com/reviews/isla-instruments-s2400)
4. [Vintage Digital: Akai MPC60](https://www.vintagedigital.com.au/akai-mpc60), [Akai MPC3000 manual (on the MPC60)](https://www.manualslib.com/manual/207365/Akai-Mpc-3000.html?page=20), [Attack Magazine: replicating vintage samplers](https://attackmagazine.com/technique/character-crunch-replicating-the-sound-of-vintage-samplers/2), [Vintage Digital: Akai S950](https://www.vintagedigital.com.au/?p=171834), [Zoe Blade: S950 notes](https://notebook.zoeblade.com/S950.html)
5. [Waves MaxxBass user guide](https://manualmachine.com/wavesaudio/maxxbass/6496600-user-guide), [Production Expert: bass on small speakers](https://www.production-expert.com/production-expert-1/how-to-make-sure-bass-is-heard-on-small-speaker-devices), [diyAudio: harmonic bass enhancement](https://www.diyaudio.com/community/goto/post?id=3684217)
