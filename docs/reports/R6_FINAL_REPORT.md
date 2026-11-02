# HamloRevolution 0.2.0 (R6): presets, editing, help, installer - and the v2 summary

Owner (2026-11-02): "R6". The design and targets came first (docs/REVOLUTION_V2.md section 11). This report also
sums up v2 (R0 .. R6) against section 4's measurements.

## R6: what was built

- **200 genre presets** (programs 25 .. 224): 4 genres x 50, in 6 groups.
  - Group sizes: Drum Bus 10, Hats / Percussion 9, Loops / Samples 9, Vocals 7, Transitions / Fills 9, Master FX 6.
  - Each preset is a SIMPLE recipe (genre, INTENT, AMOUNT, VARIATION, GROOVE, FILL, TARGET, MIX, seed) written by
    `Tools/revolution_presets.py` into `Source/Core/RevolutionPresets.inc`. It is not a stored pattern.
  - Loading a preset resets the parameters, opens SIMPLE and runs the generator: on the genre's template, or on HPDG's
    beat when linked.
  - The preset screen shows genre / group / name and what the preset is for. HPEQ's 24 programs stay at 1 .. 24
    ("Factory").
- **ADVANCED editing.**
  - Dragging across TIME / GLITCH cells paints the first cell's new value; locked steps are skipped.
  - Level lanes already drew by dragging.
  - A SHAPE lane in CURVE now shows the curve it plays (Catmull-Rom, as the engine).
- **Help.** Hovering any control on either page shows what it does: a tooltip with texts from
  `UI/RevolutionHelp.cpp`. The `moduleHelp` inspector of HPEQ is not in this plug-in.
- **Version 0.2.0**, the installer and the zip in `dist`.

## R6 checks (Tests: 1011 expectations, all passing)

| Check | Result |
|---|---|
| programs: 225, names unique, HPEQ's 24 at 1 .. 24 (GoldenTests on 0 .. 24) | pass, bit-identical |
| every genre preset: the same result loaded twice, opens SIMPLE, >= 1 event, finite, peak <= +6 dBFS on the -1 dBFS drum loop | 200 / 200; loudest +2.85 dBFS ("Break Bus") |
| preset screen: Factory 24; each genre 50 = 10 / 9 / 9 / 7 / 9 / 6 by group; choosing one loads it and opens SIMPLE | pass |
| painting: mouse down on GLITCH step 3, drag to 7 -> steps 3 .. 7 = REPEAT, step 8 untouched | pass |
| help: every knob / switch of both pages and the other controls | none missing |
| V7 CPU (R3's test, all lane types, 28 throw pairs) | worst pair 3.7 % |
| V14 pluginval 5 .. 10, Steinberg validator | pass, Steinberg 537 / 537 (`docs/reports/pluginval/r6`, `vst3validator/validator_r6.txt`) |
| installer | `dist/HamloRevolution_VST3_Setup_0.2.0.exe` (sha256 6e06f1a8...dddfa) and `dist/HamloRevolution-0.2.0-win64-vst3.zip` (dist is not in git) |

How the suite was run:
- The first full R6 run crashed in RevolutionTests' V1 loop. It compared every program with HPEQ, including the 200
  new ones that HPEQ does not have.
- GoldenTests would have switched to "recording" because the program count changed. Both now cover HPEQ's 24
  programs only; the genre presets are PresetTests' job.
- The ADVANCED page test loaded "the last program", which is now a genre preset that opens SIMPLE. It now loads
  HPEQ's last program.
- After these fixes the full suite gave 1010 passed and 1 failed: that ADVANCED page test. Its group was then rerun
  with 42 / 42 passing. The total is 1011.

## v2 against section 4 (all stages)

| # | Target | Where | Result |
|---|---|---|---|
| V1 | Legacy = HPEQ's Revolution, bit for bit | R0 .. R6: GoldenTests (24 programs + 9 clock / stretch variants), slot 0 .. 47 = HPEQ's | bit-identical at every stage |
| V2 | Step boundaries within 1 sample | R1: lane at 1/8 on a 1/16 grid with swing | +1 sample |
| V3 | Blocks 1 / 64 / 127 / 512 / 4096 bit-identical | R1 lanes, R2 bands + shapes, R3 all 7 throw targets, R4 SIMPLE + groove | bit-identical |
| V4 | Splices <= -40 dB | R2 SHAPE gate -65.6 / -132.5 dB; R3 throws: DELAY -72.0, TONAL -121.6, REVERB -106.2, PHASER -66.1, FLANGER -75.8, VINYL -48.3 dB, DOWNSAMPLE = its own stairs | pass |
| V5 | Bands sum flat within 0.1 dB | R2: every lane on a band | 0.000 dB |
| V6 | Latency 0 | R1 .. R3 | 0 |
| V7 | CPU <= 15 % (48 kHz / 256, i5-10300H) | R3 .. R6, worst throw pair | 3.4 .. 7.2 % across runs |
| V8 | Events on the hits within 1 ms | R4 (HPDG), R5 (LISTEN): only on hit 1/16s, boundaries at the hit times | 0 off, 0.000 ms |
| V9 | GROOVE 100 % on HPDG's / the input's timing within 1 ms | R4: 0.000 ms (1/16s 5, 6, 13); R5: 0.000 ms, the input's timing 0.021 ms | pass |
| V10 | AMOUNT monotonic, fills at phrase ends >= 90 %, HATS never LOW | R4: 120 / 120, 3530 / 3530, 576 / 576 | pass |
| V11 | VARIATION 0 repeats, 100 % varies with rules | R4: 40 / 40 both | pass |
| V12 | HPDG Link round trip, LOST keeps, newer version refused | R0 link tests; R4 regeneration / LOCK / LOST; R5 key | pass |
| V13 | State recalled | every stage: scenes, lanes, shapes, throws, groove, heard timing, page, SIMPLE settings; R3 / R0 states load unchanged | pass |
| V14 | pluginval 10, Steinberg validator | every stage | pass, 537 / 537 |

## Disclosed across v2

- Every method or threshold change is written in the design doc (sections 6 .. 11) and in the stage reports
  (R1_LANES, R2_SHAPE_BANDS, R3_THROW, R4_SIMPLE_CONNECT, R5_LISTEN_KEY).
- **Changed by measurement:** VINYL's wow glide and blend (R3); HPDG's swing on 1/16s without hits (R4); rounding of
  choices into the slot (R4).
- **Two latent R2 page bugs fixed in R3** (out-of-bounds lane arrays).

## Not done / open

- **Not listened to in a DAW.** HPDG and Revolution have not been linked live across processes in FL Studio.
  - The link is tested through the same protocol file, with a test writer.
  - HPDG's publisher is in HPDG commit 9790ce0 and is not pushed.
- **SHAPE's filters and DRIVE** still use the straight blend ramp. Their splices are unmeasured; only VOLUME is.
- **An onset under a louder sustained sound** is not heard by LISTEN; that 1/16 takes the genre's swing.
- **The ADVANCED page** is grown, not redrawn: rows were added and painting / curves are in. The new layout from the
  original R6 plan (a full redraw) was not done. The page opens at 75 % to fit 1080p.
- **Nothing is pushed.** The repo has no remote.

Pages: [r6_simple.png](r6_simple.png), [r6_advanced.png](r6_advanced.png).
