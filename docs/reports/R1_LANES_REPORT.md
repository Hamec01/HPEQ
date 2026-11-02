# HamloRevolution R1: lanes and scenes

Owner (2026-11-02): "R1 lanes first" (before the generator / SIMPLE page). Design and targets were written first:
docs/REVOLUTION_V2.md section 6. R0's output was hashed before any lane code (Tests/GoldenTests.cpp, commit f87683e).

## What was built

- **Lanes inside HPEQ's engine** (`DSP/HamloRevolution.*`): TIME, GLITCH and MOTION each get LENGTH (follow STEPS or
  1 .. 32; steps 17 .. 32 have their own values), RATE (follow GRID or 1/4 .. 1/32 and triplets; swing and humanize on
  the lane's own grid; FREE mode: lanes follow), CONDITION per lane cycle from bar 1 (ALWAYS, 1:2, 2:2, 1:4, 4:4, 8:8,
  NOT 1:4) and a PROBABILITY per step (100 .. 13 % in eighths; a hash of seed, lane and absolute step: different per
  cycle, the same on every playback, independent of the block size). All at 0 = the engine of R0.
- The slot: `FxSlotParams::values` 48 -> 112 floats in this repository; lanes at 48 .. 92 (`slot::kLane*`).
- **Scenes**: 8 per instance (pattern, feel, seed, lanes), the automatable **SCENE** parameter picks the one that plays
  and that the page shows; a scene never edited follows scene 1; COPY TO / CLEAR. Saved: scene 1 and the lanes on the
  REVOLUTION element as R0 wrote it (plus `lane0 .. lane2`), edited scenes as `SCENE` children; an R0 state loads with
  lanes at their defaults. A program resets the scenes.
- **Page** (until R6 redraws it): right click on a lane plate - LENGTH, RATE, CONDITION, "show steps 17 - 32"; right
  click on a step - PROBABILITY (shown in the cell); cells beyond a lane's length dimmed; the play-head frame only on
  lanes that follow the global clock; the status LCD shows the scene (S1 ..) and opens the scene menu.
- GENERATE still writes steps 1 .. 16 and keeps the lanes; steps 17 .. 32 are edited by hand (no locks there).

## Checks (Tests: 277 / 277)

| # | Check | Result |
|---|---|---|
| V1 | the slot of Default and the 24 presets = HPEQ's (RevolutionTests); R0's output hashes for the 24 programs and 9 clock / stretch variants with a tempo ramp and a jump (GoldenTests) | bit-identical |
| V2 | RATE 1/8 on a 1/16 grid, swing 65 %: first changed sample vs the lane's swung step (0.325 / 0.825 / 1.325 s) | +1 sample each (the boundary sample's crossfade weight is ~0) - within 1 |
| V3 | blocks 1 / 64 / 127 / 4096 after prepare(256), lengths 5 / 3 / 32, 1/8T, NOT 1:4, probabilities, steps 17 .. 32 | bit-identical |
| V6 | latency with lanes | 0 samples |
| lane | LENGTH 3 every 3 steps (30 steps); LENGTH 32 with step 22 (64 steps); 1:2 and 4:4 (36 steps) | exact |
| lane | PROBABILITY 50 %: 60 / 128 events over 64 cycles (target 40 .. 60 %), same on a second run; 100 %: 128 / 128 | pass |
| scene | scene 2 plays its own gaps (4 / 4); scene 1 dry; an unedited and a cleared scene = scene 1 bit for bit | pass |
| V13 | scenes and lanes round trip; an R0 state loads with default lanes and no scenes | exact |
| V14 | pluginval 5 .. 10, Steinberg validator 537 / 537 (docs/reports/pluginval/r1, vst3validator/validator_r1.txt) | pass |

Method note: the probability test first measured 8 s while 64 cycles of 1/32 x 4 last 16 s (and counted the very
first step, which starts during the 10 ms mix ramp); corrected to 16.6 s from the second cycle on. Thresholds unchanged.

Not done in R1: the 3-band split (R2, with SHAPE), a drawn lane editor (R6), listening in a DAW.

Panel: [r1_panel.png](r1_panel.png).
