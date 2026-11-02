# HamloRevolution R5: LISTEN timing and TONAL in HPDG's key

Owner (2026-11-02): "R5". The design and targets were written first (docs/REVOLUTION_V2.md section 10). R5 covers
what R4 left: GROOVE from the input, and TONAL in HPDG's key.

## What was built

- **LISTEN timing (engine).** Besides the attack strength per step, the engine finds the input's onsets at sample
  resolution.
  - An onset is a sample where the louder channel is above 3 x its 20 ms mean and above -50 dBFS. A 30 ms dead time
    follows.
  - Each 1/16 of the bar keeps the offset of its first onset from the straight grid. The window is half a 1/16 either
    side, on the musical clock, so SWING and GROOVE settings do not move it.
  - Offset and presence are averaged bar by bar, half old and half new.
  - The analysis only listens; the audio is untouched. GoldenTests hold.
- **SIMPLE, unlinked.** GENERATE captures what the engine heard into the scene: the feel, the heard offsets, and a mask
  of the 1/16s whose presence is >= 0.5.
  - The groove table is the heard offset where there was a hit. Elsewhere it is the genre's swing on the odd 1/16s
    and 0 on the even ones.
  - Knob moves regenerate from the captured data, so the result is the same each time.
  - The capture is saved with the scene.
- **TONAL in HPDG's key.** When linked, each new HPDG snapshot sets both throws' NOTE to HPDG's key root. This works
  on either page, and LOCK keeps everything.

## Checks (Tests: 567 / 567)

The test loop: kicks on 1/16s 1 and 9; snares on 5 and 13, 10 ms late; hats on 3, 7, 11, 15 on the grid and on the
odd 1/16s 15 ms late; no hit on 10 and 12.

| # | Check | Result |
|---|---|---|
| timing | after 4 bars: the largest offset error on the hit 1/16s; presence right (>= 0.5 on hits, < 0.5 without) | 0.021 ms (target 1 ms); 16 / 16 |
| V8 (LISTEN) | GENERATE unlinked: the heard mask = the loop's hits (0xf5ff); on-hit events off the input's hits | exact; 0 of 4 |
| V8 | knob moves after GENERATE: the groove table unchanged | pass |
| V9 (LISTEN) | GROOVE 100 %: a GLITCH gap on 1/16 5 / 6 moves by the loop's 480 / 720 samples | off by 0.000 ms each (target 1 ms) |
| V13 | the feel, the heard offsets and mask round trip | pass |
| key | linked, ADVANCED: HPDG's key A then D -> NOTE 9 then 2 on both throws; LOCK keeps NOTE | pass |
| V1 / V3 | GoldenTests; the R1 - R4 block-size tests | bit-identical |
| V7 | the CPU check, rerun with the analysis | worst pair 4.0 % (target 15 %) |
| V14 | pluginval 5 .. 10, Steinberg validator (`docs/reports/pluginval/r5`, `vst3validator/validator_r5.txt`) | pass; Steinberg 537 / 537 |

Disclosed: the design text first said "hats on every odd 1/16". The loop leaves 1/16s 10 and 12 without hits, so the
"no hit" side of presence gets checked. The wording was changed before the run.

Open:
- An onset hidden under a louder sustained sound (a hat under a long 808 tail) is not detected: the 3 x mean rule
  fails. That 1/16 then takes the genre's swing.
- Nothing was listened to in a DAW, and HPDG has not been linked live across processes in FL.
- R6 is next: the ADVANCED redraw, 200 presets, help, installer.
