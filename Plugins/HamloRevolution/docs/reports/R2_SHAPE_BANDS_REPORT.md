# HamloRevolution R2: SHAPE lanes and bands

Owner (2026-11-02): "R2 SHAPE + bands". Design and targets first: docs/REVOLUTION_V2.md section 7.

## What was built

- **BAND per lane** (TIME, GLITCH, MOTION, SHAPE A, SHAPE B): FULL / LOW / MID / HIGH. A lane on a band splits its
  input with a 3-band Linkwitz-Riley tree (two Butterworth sections per filter; the low band through the upper
  crossover's all-pass), runs on its band only and adds the other two back. Zero latency. Crossovers: the automatable
  LOW / MID (40 .. 1000 Hz, 200) and MID / HIGH (1 .. 12 kHz, 2.5 kHz) knobs in the new BANDS footer. A band change
  restarts that lane's split filters from silence. FULL = no split.
- **SHAPE A / SHAPE B**: two level lanes after TIME / GLITCH / MOTION. ON, MIX, DEPTH, SMOOTH, RES, TARGET (VOLUME,
  LOW PASS, HIGH PASS, BAND PASS, PAN alternating per step, WIDTH, DRIVE level-matched at -10 dBFS, CRUSH), STEPS or
  CURVE (Catmull-Rom through the step levels), 16 levels per step, steps 17 .. 32 and the R1 lane settings. Their
  steps are scene state; their knobs and switches are parameters. An empty SHAPE lane is skipped (dry bit for bit).
- Slot 112 -> 160 floats (93 .. 151: shapes, their lanes, bands, crossovers). Lane text gains band and steps 1 .. 16
  (R1's five-part text still reads).
- **Page** 1180 x 700 -> 1180 x 1016: SHAPE A / B rows (plate, ON, MIX, DEPTH, SMOOTH, TARGET window, RES at the
  right), the BANDS footer; plate menus: BAND (every lane), CURVE (shapes), "Clear steps" (shapes); a tag beside a lane's
  lamp shows its band / CURVE. SHAPE rows have no GEN / LOCK until the generator (R4).

## Checks (Tests: 361 / 361)

| # | Check | Result |
|---|---|---|
| V1 | GoldenTests (R0's hashes, 24 programs + 9 variants); the slot 0 .. 47 = HPEQ's for every program | bit-identical |
| V3 | blocks 1 / 64 / 127 / 4096 after prepare(256): bands on TIME / GLITCH / SHAPE A, SHAPE A DRIVE length 5, SHAPE B BAND PASS RES 0.7 CURVE at 1/8 | bit-identical |
| V4 | SHAPE A VOLUME gate 15 / 0 at 1/16 on DC: largest second difference of the gain | STEPS -65.6 dB, CURVE -132.5 dB (target <= -40) |
| V5 | all five lanes on bands, empty patterns: impulse response magnitude 20 Hz .. 20 kHz | 0.000 .. 0.000 dB (target +-0.1) |
| band | GLITCH GAP on HIGH, 60 Hz + 6 kHz: in the gap | 60 Hz -0.00 dB, 6 kHz -32.2 dB (targets 0.5 / -30) |
| shape | LOW PASS level 15: 8 kHz -70.8 dB; PAN >= 10 dB alternating (4 steps); DRIVE level 15: THD 33.5 %, peak -0.00 dB; CURVE through the levels: largest miss 0.0000 | pass |
| shape | an empty SHAPE lane, each of the 8 targets, = dry | bit-identical |
| V6 / V13 | latency 0; bands, SHAPE steps and parameters round trip; R1 lane text reads | pass |
| V14 | pluginval 5 .. 10, Steinberg validator 537 / 537 (docs/reports/pluginval/r2, vst3validator/validator_r2.txt) | pass |

Method notes (disclosed):
- V4's wording was fixed before measuring: the first text ("the gain's change between two samples") is 0.016 = -36 dB
  for the 2 ms raised-cosine fade by design; the click measure is the second difference (doc section 7).
- Band isolation: the first run projected over 0.0625 s (3.75 periods of 60 Hz) and read 60 Hz at 0.50 dB - leakage,
  not the filter; re-measured over 0.05 s (whole periods of both tones). Thresholds unchanged.
- The 6 kHz figure is the LR4 low pass of the mid band at 2.5 kHz (theory -30.6 dB at 6 kHz); a higher MID / HIGH
  crossover isolates less, a lower one more.
- The page snapshot tool crashed (stack overflow): the processor grew with the slot; it is on the heap now.

Not done in R2: THROW lanes (R3), CPU measurement (V7, R3), a drawn curve editor (R6), listening in a DAW.

Panel: [r2_panel.png](r2_panel.png).
