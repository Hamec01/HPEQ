# HamloRedWall v2: design and targets

Owner (2026-10-28): "it must be better than the other limiters - especially Oxford, it is good and clean; L2 works
very well. Maybe add a DRIVE knob (blend in harmonics) and a BLANKET or TUBE knob to make the mix fatter and warmer
right in the limiter. Let us make a diamond among limiters." Targets fixed **before** the baseline and the code.

## 1. What the references do

| Reference | Published behaviour used here |
|---|---|
| Sonnox Oxford Limiter | four stages: pre-process (gain, soft knee 0 .. 10 dB), **Enhance** 0 .. 125 % (dynamic enhancement: loudness and density, sample-value limiting from 100 %), reconstruction metering with **Auto Comp** for overs after the D/A, dither [1] |
| Waves L2 | look-ahead brickwall "without overshoot"; threshold = loudness control, the output ceiling set separately; **ARC** auto release adapting to the material; IDR dither [2] |
| FabFilter Pro-L 2 | eight styles (Transparent, Punchy, Dynamic, Allround, Aggressive, Modern, Bus, Safe); oversampling up to 32x; **4x with >= 0.1 ms look-ahead keeps inter-sample peaks within 0.1 dB**; a true-peak stage after the limiter [3] |
| Limiter design | a short release follows each cycle of a bass note and distorts it, a long one pumps; look-ahead smooths the gain; split-band / frequency-dependent release keeps the bass clean [4] |

So a limiter is judged on: **the ceiling** (sample and true peak), **bass distortion** at a given gain reduction,
**pumping** (how much a steady part is ducked by a hit, and for how long), **loudness for the push**, and its own
**character** stage (Enhance, styles). RedWall adds DRIVE and TUBE as that stage.

## 2. New controls

- **DRIVE** (0 .. 100 %, new `drive`): harmonics blended in before the limiter, mostly odd, level-independent amount
  (relative to the ceiling), oversampled so it does not alias. 0 = exactly today's RedWall.
- **TUBE** (0 .. 100 %, new `tube`): warmth and weight - even harmonics and a slow, program-dependent density stage
  (fatter: lower crest factor without a higher peak). 0 = exactly today's RedWall.
- Both sit before the limiter, so LIMIT and TRUE PEAK still hold.

## 3. Measurements (`HamloRedWall_Lab`) and targets

| # | Check | Target |
|---|---|---|
| R1 | Sample peak <= LIMIT, every style, TRUE PEAK on / off, DRIVE / TUBE 0 and 100 % (drums +18 dB) | <= LIMIT |
| R2 | True peak (8x reconstruction) with TRUE PEAK, pushed 9 dB on the mixes and drums, DRIVE / TUBE 0 and 100 % | <= LIMIT + 0.1 dB |
| R3 | Bass distortion: 50 Hz sine at 6 dB gain reduction, AUTO release, GLASNOST: THD (H2 .. H5) | <= 0.3 % |
| R4 | Intermodulation: 60 Hz (-6 dB) + 3 kHz (-18 dB) at 6 dB GR: sidebands 3 kHz +- 60 / 120 Hz re 3 kHz | <= -40 dB |
| R5 | Pumping: a 1 kHz tone (-20 dBFS) under a kick every 500 ms (peaks 0 dBFS), pushed 6 dB: deepest dip of the tone; time back to within 1 dB | before -> after: dip and time no worse |
| R6 | Loudness for the push: the three mixes, LOUDNESS -6 dB, LIMIT -1 dBTP: integrated loudness gain | >= +4.5 LU |
| R7 | DRIVE: 1 kHz sine at -12 dBFS, DRIVE 25 / 50 / 100 %: THD rising monotonically, at 100 % 2 .. 10 %; H3 > H2 | yes |
| R8 | DRIVE 100 %, 9 kHz sine at -6 dBFS: the largest non-harmonic (aliased) component re the fundamental | <= -80 dB |
| R9 | TUBE: 1 kHz sine at -12 dBFS, TUBE 100 %: H2 > H3, THD 0.5 .. 5 % | yes |
| R10 | TUBE 100 % on the drum bus and the mixes (LOUDNESS 0, LIMIT 0): crest factor (peak - RMS) lowered, the sample peak not raised | crest >= 2 dB lower, peak <= input + 0.1 dB |
| R11 | DRIVE = TUBE = 0: today's RedWall | reference vs HPEQ unchanged (<= -120 dB) |
| R12 | Block sizes 1 / 64 / 512 / 4096 after 256, every style with DRIVE / TUBE | bit-identical |
| R13 | pluginval 10 + Steinberg validator | pass |

R3 .. R6 are run on today's RedWall too (the "before").

## Sources

1. [Sonnox Oxford Limiter manual (manualsdir): overview](https://www.manualsdir.com/manuals/741876/sonnox-oxford-limiter.html?page=5), [enhancement](https://www.manualsdir.com/manuals/741876/sonnox-oxford-limiter.html?page=7), [reconstruction](https://www.manualsdir.com/manuals/741876/sonnox-oxford-limiter.html?page=28)
2. [Waves L2 user guide (manualmachine)](https://manualmachine.com/wavesaudio/l2ultramaximizer/6490343-user-guide), [Joseph Nilo: L2 explained](https://josephnilo.com/blog/waves-l2-explained/), [Barry Rudolph: L2](https://barryrudolph.com/mix/waves.html)
3. [FabFilter Pro-L 2 help: true peak limiting](https://www.fabfilter.com/help/pro-l/using/truepeaklimiting), [Sound On Sound review](https://www.soundonsound.com/reviews/fabfilter-pro-l-2), [MusicRadar review](https://www.musicradar.com/reviews/fabfilter-pro-l-2)
4. [Sound On Sound: limiter release settings](https://www.soundonsound.com/sound-advice/q-what-release-settings-should-use-limiter), [Audacity: legacy limiter](https://manual.audacityteam.org/man/legacy_limiter.html), [Har-Bal limiter help](https://www.har-bal.com/har-bal-limiter-help/), [Limit Pro](https://www.pluginboutique.com/products/17985-Limit-Pro)
