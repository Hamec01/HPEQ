# HamloUdarnik v2: design and targets

Owner (2026-10-27): "to me it is the weakest one now"; improve it as the compressor was: references and targets
first, then the baseline, then the code. The targets below are fixed **before** the baseline run and the code.

## 1. What a good transient shaper does (references)

| Reference | Published behaviour used here |
|---|---|
| SPL Transient Designer | Differential Envelope Technology: two envelope followers, their difference drives the gain; no threshold, the same effect at any level; Attack +-15 dB, Sustain +-24 dB [1] |
| Native Instruments Transient Master | changes the attack / sustain contour, not by level: loud and soft passages alike; a built-in limiter so it can be pushed without clipping [2] |
| Waves Smack Attack | detection by the rate of change, works on low and high levels; Sensitivity and Duration per attack / sustain; a limiter / clipper; parallel mix [2] |
| Sonnox Oxford TransMod | acts on differential information, the long-term level is minimally affected; rise time, Overshoot (how long the change lasts); an Overdrive stage to prevent clipping [3] |

Four properties come out of all four: **(a)** the knob delivers what it says, **(b)** level independence, **(c)** the
output does not clip when the attack is pushed, **(d)** steady tones are left alone.

## 2. Hypothesis (to be checked by the baseline, not assumed)

Today the attack weight is `w_a = clamp(d / 6 dB)`, `d = 20 log10(fast / slow)`. On a real drum loop `d` often stays
below 6 dB, so SNAP +12 dB delivers only part of 12 dB ("you do not feel it"). PRAVDA has no output protection
(M0: SNAP +12 dB raised the test loop's peak from +0.8 to +12.8 dBFS).

## 3. Measurements (`HamloUdarnik_Lab <material dir>`)

Material: the owner's clips (material set 2): kick, snare, hihat, perc, drumbus1, each gained to an active RMS of
-18 dBFS. Hits: onsets of the input (1 ms RMS rises over 2.5 x its 50 ms mean and above -45 dBFS; 60 ms apart at
least). Per hit, with the output aligned by the plug-in's latency:

- attack window `A` = 0 .. 10 ms after the onset, body window `B` = 25 .. 100 ms (cut 5 ms before the next hit)
- delivered **attack contrast** = (out A - in A) - (out B - in B) in dB (RMS of the window); delivered **body
  contrast** = its negative; the mean over hits is reported.

## 4. Targets

| # | Check | Target |
|---|---|---|
| U1 | SNAP +12 dB, every mode, VOSTOK, on kick / snare / hats / perc / drum bus: attack contrast | >= +8 dB (two thirds of the knob) on every clip |
| U2 | SNAP +6 dB: attack contrast | >= +4 dB on every clip |
| U3 | SNAP -12 dB: attack contrast | <= -8 dB on every clip |
| U4 | BODY -12 dB: body contrast | <= -8 dB on every clip; BODY +12 dB >= +8 dB |
| U5 | Level independence: drum bus at -6 and at -30 dBFS RMS, SNAP +9: attack contrast | within 1 dB of each other |
| U6 | Steady tones (220 Hz + 1 kHz, -18 dBFS), SNAP +12 and BODY -12: level change; residual (out - in, re in) | <= 0.5 dB; <= -50 dB |
| U7 | Output protection on, SNAP +12 on the drum bus gained to a -1 dBFS peak: output sample peak | <= 0 dBFS |
| U8 | Onset not moved: SNAP +12, every mode: output onset = input onset + latency | within 0.1 ms |
| U9 | Old projects (CLASSIC) | reference vs HPEQ unchanged (<= -120 dB) |
| U10 | Block sizes 1 / 64 / 127 / 512 / 4096; finite | bit-identical |
| U11 | pluginval 10 + Steinberg validator | pass |

U1 .. U4 are run on the old laws too (the "before"), with the same code.

## Sources

1. SPL: [Barry Rudolph, SPL Transient Designer](https://barryrudolph.com/mix/transient.html), [Sound On Sound review](https://www.soundonsound.com/reviews/spl-transient-designer), [SPL TD4 product page (Sound Pure)](https://soundpure.com/p/spl-transient-designer-4-model-9842-dynamics/225)
2. [NI Transient Master manual](https://www.native-instruments.com/ni-tech-manuals/transient-master-manual/en/overview), [Waves Smack Attack user guide](https://www.fullcompass.com/common/files/29478-WavesSmackAttackUserGuide.pdf), [Plugin Boutique: Smack Attack](https://www.pluginboutique.com/product/2-Effects/36-Transient-Shaper/13598-Smack-Attack)
3. [Sonnox Oxford TransMod user manual](https://www.hhb.co.uk/wp-content/uploads/user_manual_20141103152919_26568.pdf), [Tape Op review](https://tapeop.com/reviews/gear/41/transmod-plug-in)
