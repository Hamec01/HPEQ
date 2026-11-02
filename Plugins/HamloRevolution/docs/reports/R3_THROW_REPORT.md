# HamloRevolution R3: THROW lanes

Owner (2026-11-02): "R3 THROW". Design and targets first: docs/REVOLUTION_V2.md section 8.

## What was built

- **THROW A / THROW B**: two lanes after SHAPE A / B (lanes 5, 6). Each has ON, MIX, DEPTH, TAIL, TONE, TARGET, TIME
  (1/32 .. 4 bars, triplets, dotted) and NOTE (C3 .. B3), plus 16 levels per step, steps 17 .. 32, BAND and the R1
  lane settings. The steps are scene state; the knobs and switches are parameters.
  - **Sends.** DELAY is a ping-pong delay synced to TIME, with feedback 0.9 x TAIL and damping. TONAL is a delay
    tuned to NOTE, i.e. a pitched comb, with feedback 0.97 x TAIL. REVERB is an 8-line FDN behind 4 diffusers, with
    RT60 0.3 .. 8 s from TAIL. A step's level is its send, and the tail keeps ringing after the step. Output =
    in + MIX x return.
  - **Inserts.** PHASER: 6 all-pass stages on a TIME-synced LFO. FLANGER: a 0.3 .. 5 ms swept delay. VINYL: wow,
    record EQ, crackle (TONE), and a brake that drops the speed to 1 - TAIL over the step. DOWNSAMPLE: hold factor
    1 .. 32. A step's level is the intensity; level 0 is exactly dry.
  - **LFOs and crackle** come from the musical position and the sample counter. The result does not depend on the
    block size, and it repeats on every playback.
- **Idle lanes.** An empty THROW lane is skipped once its return has stayed below -120 dBFS for its buffer length.
  It is then dry bit for bit on FULL. Its state is cleared when it wakes up or changes target. Buffers are allocated in
  `prepare` (a 2 x 2.5 s tape per lane, plus the FDN).
- **Slot** grows from 160 to 208 floats. Indices 152 .. 207 hold the throws, their lane settings and their bands.
  `slot::laneSettings()` and `slot::laneBand()` give each lane's location.
- **Page** grows from 1180 x 1016 to 1180 x 1280, with THROW A / B rows: plate, ON, MIX, DEPTH, TAIL, the TARGET
  window, and TONE on the right.
  - The TARGET window shows the target plus its TIME or NOTE. Click: next target. Right click: a TARGET / TIME / NOTE
    menu.
  - The editor opens at 75 % (885 x 960) so it fits a 1080p screen; it resizes from 50 to 200 %.

## Checks (Tests: 470 / 470)

| # | Check | Result |
|---|---|---|
| V1 | GoldenTests (R0's hashes), the slot 0 .. 47 = HPEQ's | bit-identical |
| empty | an empty THROW lane, each of the 7 targets, on A and B | dry bit for bit |
| DELAY | impulse: left echo peak at 21000 (expected 21000); right channel 0; echo 2 (right) / echo 1 -6.94 dB (expected -6.94); a level-0 step's burst echoes at -240 dB | pass |
| TONAL | NOTE A: spectrum peak 218.99 Hz (-0.46 %, target 1 %) | pass |
| REVERB | TAIL 50 %: T30 1.867 s vs 2.225 s nominal (-16.1 %, target 25 %) | pass |
| PHASER / FLANGER | 1 kHz over a 1-bar LFO: -16.6 .. 2.6 dB / -16.2 .. 2.8 dB (target >= 10 dB range) | pass |
| VINYL | wow 2.50 % (target >= 0.3 %); brake: 181.9 Hz in the step's last quarter (-81.8 %, target >= 40 %) | pass |
| DOWNSAMPLE | the 500 Hz alias at +6.0 dB re 1 kHz (target >= -30 dB) | pass |
| tail / idle | DELAY emptied at 0.5 s: its echo still there (0.4 dB re input at 0.7 .. 0.9 s); 5 .. 8 s = input | pass, bit-identical |
| V4 | 100 Hz at -6 dBFS, 1/16 gate 15 / 0: the largest fourth difference within 3 ms of a step boundary (see the table below) | pass |
| V3 | blocks 1 / 64 / 127 / 4096 after prepare(256), program 3 + both throws (lengths, rates, B on MID); 4 target pairs covering all 7 | bit-identical |
| V7 | 48 kHz / 256, Release, i5-10300H: TIME (CLEAN stretch), GLITCH, MOTION, SHAPE A / B on bands, both throws, all 28 target pairs; worst pair's mean time / block time | 3.4 .. 7.2 % over five runs (target <= 15 %) |
| V6 / V13 | latency 0; throw parameters, steps, bands round trip; the slots equal | pass |
| page | THROW levels, step clicks and BAND reach the processor | pass |
| V14 | pluginval 5 .. 10, Steinberg validator 537 / 537 (`docs/reports/pluginval/r3`, `vst3validator/validator_r3.txt`) | pass |

V4 per target (boundary / the same target at a constant level 15; the target is <= -40 dB, or not above the target's
own value):

| Target | DELAY | TONAL | REVERB | PHASER | FLANGER | VINYL | DOWNSMP |
|---|---|---|---|---|---|---|---|
| boundary | -72.0 | -121.6 | -106.2 | -66.1 | -75.8 | -48.3 | +1.2 |
| constant 15 | -31.6 | -122.2 | -106.0 | -69.7 | -111.2 | -62.8 | +1.9 |

DOWNSAMPLE passes by the second rule: its stairs are the effect. DELAY's -31.6 dB at constant level is the first echo
starting abruptly 0.375 s after the render begins. It is not a splice.

## Changes found by measurement (disclosed)

- **VINYL V4 failed three times.** In each case the effect itself caused the failure; the test was not changed.
  - -22.8 dB: the wow depth followed the 2 ms step fade, so the head moved up to 2 ms within 2 ms. Fix: the depth now
    glides over about 50 ms.
  - -27.1 dB: the blend `min(1, level / 0.1)` has a kink. Fix: a smooth step that is full at a quarter of the level;
    DOWNSAMPLE uses it too.
  - -34.5 dB, at sample 49 = the 16-sample coefficient refresh: the record EQ changed in a staircase. Fix: it is
    redesigned every sample while the level moves.
  - After the three fixes: -48.3 dB. The design doc records all three before the final run.
- **TONAL test bug.** The first run's peak search began its maximum at bin 0 (DC) and reported 0 Hz. The search now
  starts at 150 Hz. This changed the measurement, not the threshold.
- **Plug-in page test.** The preset screen sits at the same design place, scaled with the page (75 % at the opening
  size). The test now expects the scaled rectangle.
- **Two latent R2 page bugs fixed.** Both were found because the SHAPE page test failed only in the full run, when the
  memory past the arrays was no longer zero.
  - `secondHalf` was sized for 3 engines but indexed by lanes 3 .. 4. Out of bounds: a SHAPE click could land in
    steps 17 .. 32.
  - Right-clicking a SHAPE step opened the TIME / GLITCH step menu. That menu read and wrote `pattern.steps[lane]`
    for lanes 3 .. 4, out of bounds. Level lanes now set their level there, and LOCK is only offered on TIME / GLITCH.
- **The snapshot tool** takes a scale. `r3_panel.png` is the design size (opening size x 1.3333).

Open:
- SHAPE's filters and DRIVE (R2) still use the straight blend ramp. R2's V4 measured VOLUME only, so the other SHAPE
  targets are unmeasured and were not changed here.
- Not done in R3: TONAL following HPDG's key (R5), the generator and SIMPLE page (R4), listening in a DAW, and the
  installer (R6).

Panel: [r3_panel.png](r3_panel.png).
