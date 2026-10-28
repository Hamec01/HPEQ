# HamloRedWall 0.2.0 (v2): report

Owner's request (2026-10-28): a limiter better than the others (Oxford: clean; L2: works very well), a DRIVE knob
(blend in harmonics) and a TUBE / BLANKET knob (fatter and warmer right in the limiter). Design and targets fixed
before any code: [docs/REDWALL_V2.md](../../REDWALL_V2.md). The same lab code (`HamloRedWall_Lab`, owner clips set 2)
made the "before" ([before.md](before.md)) and the "after" ([after.md](after.md)).

## What changed

- **ENGINE** (new `engine`, Classic / V2; a new instance is V2). V2 computes TRUE PEAK on an 8x grid instead of 4x
  (same 16-tap Kaiser sinc, same 8-sample delay). CLASSIC is HPEQ's limiter exactly. A project saved without
  `engine` (HPEQ's module, HamloRedWall 0.1.0) opens as CLASSIC; the six HPEQ presets set CLASSIC.
- **DRIVE** (new `drive`, 0 .. 100 %): `tanh(k u) / k`, k = 1 + 2.5 DRIVE, on u = x * push / LIMIT - odd harmonics
  whose amount follows LOUDNESS and LIMIT. Before the limiter, 4x oversampled (JUCE half-band FIR equiripple).
- **TUBE** (new `tube`, 0 .. 100 %): relative to the program's own level (RMS 50 ms, linked): the positive
  half-wave bent (`v - h v^2 / (1 + v)`, even harmonics, no expansion) and the peaks above 12 - 9 TUBE dB over that
  level softly rounded (`K + w tanh((|v| - K) / w)`): fatter, never a higher peak. Same oversampled stage, then a
  5 Hz DC blocker. Source: `Source/DSP/RedWallColour.h`.
- The colour stage has a **constant latency** (the oversampler's 61 samples), so the plug-in's fixed latency is now
  488 + 61 = **549 samples** at 48 kHz (11.4 ms) whatever the knobs. At DRIVE = TUBE = 0 the stage is the input
  delayed, bit-exact; turning it on or off crossfades over 10 ms.
- Page: the stage grew to 1180 x 820 for a bottom strip with the ENGINE lever and the DRIVE / TUBE knobs; help
  texts for the three. Screenshot: [panel_v2.png](panel_v2.png).
- 12 v2 presets (Clean V2 Master, Warm Master, Fat Loud Master, Streaming Warm -1 dBTP, Edge Push, Boom Bap Weight,
  Trap 808 Glue, Drum Bus Crunch, Drum Bus Thick, Vocal Presence, Keys Warmth, Parallel Smash); the six HPEQ presets
  unchanged.

## Results (before -> after)

| # | Check | Before | After | Target | Pass |
|---|---|---|---|---|---|
| R1 | Sample peak (drums +18 dB, every style, TP on / off, DRIVE / TUBE 0 and 100 %) | -1.000 dBFS | -1.000 dBFS | <= LIMIT | yes |
| R2 | True peak 8x, pushed 9 dB | -0.885 dBTP | **-0.916 dBTP** | <= LIMIT + 0.1 | **fixed** |
| R3 | 50 Hz THD at 6 dB GR | 0.231 % | 0.231 % | <= 0.3 % | yes |
| R4 | IMD 60 Hz + 3 kHz | -57.7 dB | -57.8 dB | <= -40 dB | yes |
| R5 | Pumping: dip; recovery | -7.6 dB; 65 ms | -7.6 dB; 65 ms | no worse | yes |
| R6 | Loudness gain for a -6 dB push | 3.95 LU | 3.95 LU | >= 4.5 LU | **no** |
| R7 | DRIVE 25 / 50 / 100 %: THD | n/a | 1.33 / 2.47 / 5.42 %, H3 -25.3 dB > H2 | rising, 2 .. 10 %, H3 > H2 | yes |
| R8 | DRIVE 100 %, 9 kHz: aliasing | n/a | -109.4 dB | <= -80 dB | yes |
| R9 | TUBE 100 %, 1 kHz -12 dBFS | n/a | THD 2.28 %, H2 -33.1 > H3 -46.5 dB | H2 > H3, 0.5 .. 5 % | yes |
| R10 | TUBE 100 %: crest lowered (Ardnold / boom bap / sad trap / drum bus); peak | n/a | 2.0 / -0.7 / 0.1 / 3.6 dB; every peak lower | >= 2 dB each, peak not raised | **no** (2 of 4) |
| R11 | DRIVE = TUBE = 0, CLASSIC vs HPEQ's module | 184 / 192 bit-identical, worst -157.6 dB | 182 / 192, worst -157.6 dB, 0 over | <= -120 dB | yes |
| R12 | Blocks 1 / 64 / 127 / 512 / 4096 after 256, every style with DRIVE / TUBE | - | bit-identical | bit-identical | yes |
| R13 | pluginval 5 .. 10 (+ Steinberg validator at 10); validator -e | m0 pass | pass; 537 / 537 | pass | yes |

Unit tests: 211 expectations, 0 failed (latency, MIX / ON, the colour stage back to a pure delay, DRIVE / TUBE block
identity and ceiling, migration, every program under its LIMIT, page).

## Not met, and why

- **R6 (loudness for the push).** The limiter core cannot reach +4.5 LU for a -6 dB push on these mixes: the best of
  every style and release was 4.47 LU (STAKHANOV, AUTO, 10 ms); GLASNOST default 3.95. Two auto-release changes
  were tried and **reverted**: a 50 ms charge (R6 3.51 LU, R5 recovery 165 ms - worse) and slow charge / fast
  release (R6 3.97 LU, R3 0.40 % - fails R3). The core stays as in HPEQ (only the TP grid changed); loudness and
  density are the colour stage's job.
- **R10 (TUBE crest factor).** Met on the drum bus (3.6 dB) and the Ardnold mix (2.0 dB), not on boom bap 3
  (-0.7 dB) and sad trap (0.1 dB). Those two are already limited masters (9541 and 1535 samples within 0.5 dB of
  their peak; median 50 ms crest 7.0 / 7.4 dB): their whole-file crest is macro-dynamics, which a pre-limiter
  stage that never raises a peak cannot lower. Every output peak was lower than the input's (no case raised it).
  Two earlier TUBE designs were measured and replaced: half-waves compressed relative to the ceiling plus a
  5 / 300 ms RMS density compressor (R9 H2 -36.3 vs H3 -35.9 dB - fails; R10 0.9 / -0.3 / -0.5 / 1.9), and a peak
  follower density stage with look-ahead and make-up (R10 0.8 / -1.0 / -2.4 / 2.2). The target was not changed.

## Disclosures (method changes after a run)

- **R8 lab bug, fixed after the first run.** The alias scan counted k = 15 (135 kHz folds to exactly 9 kHz, the
  fundamental) and read 0.0 dB. Aliases that fold onto the fundamental or H2 are now skipped (they cannot be told
  apart from them); comment in `Tests/LabMain.cpp`.
- **Reference tool.** It now aligns both plug-ins by their reported latency (HamloRedWall's colour stage adds a
  constant delay HPEQ does not have) and compares only HPEQ's six presets (the v2 presets use the colour stage).
  With the alignment, two more cases (drums, style 0, TP off, AUTO, blocks 512 and 97) differ by -240 dB (float
  rounding), so 182 instead of 184 bit-identical; the worst difference is unchanged at -157.6 dB.
- **Tests.** "DRIVE / TUBE back to 0" is checked on the colour stage itself (bit-exact delayed input after the fade),
  not through the whole plug-in: the limiter after it saw a different signal and settles only asymptotically
  (first version through the plug-in failed for that reason). The every-program ceiling test skips programs with
  MIX < 100 % (Parallel Smash passes its dry part by design: 8.8 dBFS with drums +12 dB).
- Earlier (M0, still valid): the reference matches parameters by name and occurrence (two named "Release"); the
  true-peak unit test uses fs / 4 at 45 degrees.

## Files

`dist/HamloRedWall_VST3_Setup_0.2.0.exe` (+ .sha256), `dist/HamloRedWall_VST3_0.2.0.zip` (the .vst3 bundle);
logs in `docs/reports/pluginval/v2/`, `docs/reports/vst3validator/validator_v2.txt`, reference table
`docs/reports/v2/reference_check_v2.md`.
