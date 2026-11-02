# HamloRevolution R4: SIMPLE page, generator, CONNECT TO HPDG

Owner (2026-11-02): "R4 SIMPLE + CONNECT". The design and its targets were written first (docs/REVOLUTION_V2.md
section 9). This stage takes the generator and SIMPLE page of stage 5, plus the CONNECT button, GENRE AUTO and GROOVE
from HPDG of stage 6.

## What was built

- **Generator** (`Core/RevolutionSimple`). It is a pure function of (settings, hits, seed).
  - **What it writes:** the shown scene's TIME / GLITCH / MOTION steps; all seven lanes (steps, length, rate,
    condition, probability, band); the groove table; and the lane parameters (targets, TIME / NOTE, depths, mixes,
    grid 1/16, STEPS 16, swing 0). Switching to ADVANCED shows exactly that.
  - **Rules:** the section 3 rules per genre (BOOM BAP, TRAP, TECHNO, DNB), each family weighted per INTENT zone
    (SUBTLE, GROOVE, FILLS, BREAKDOWN, CHAOS).
  - **AMOUNT** keeps the first round(AMOUNT x N) candidates by a seeded priority, and sets the depth.
  - **VARIATION** adds step probabilities to the weaker events. From 34 % GLITCH gets a second bar of its own; from
    25 % the TECHNO sweep runs at 1/4.
  - **FILL** puts the fill lanes (TIME, THROW B) on ALWAYS / 2:2 / 4:4 / 8:8, with events only in the last beat. The
    BREAKDOWN half-time fill is the exception: it takes the second half of the bar.
  - **TARGET** puts every lane on HIGH (HATS), MID (SNARE), the new band TOPS (MID + HIGH) or FULL. In TRAP, FULL
    becomes TOPS so the 808 stays clean. On-hit events go only where the target's role hits.
- **Hits.** When linked, they come from HPDG's events per 1/16: kick; snare + clap; hats (hi-hat, open hat, ride,
  hat FX); ghosts skipped. The groove is the mean offset per 1/16; a 1/16 without hits takes HPDG's swing on the odd
  ones and 0 on the even ones. Unlinked, the hits come from LISTEN's feel, or failing that from the genre's template
  with its swing. GENRE AUTO follows HPDG's genre; unlinked, it goes by tempo.
- **GROOVE in the engine** (slot 208 .. 224). A step boundary that falls on a 1/16 of the bar moves by GROOVE x
  table[that 1/16]. A zero table (every state before R4) leaves the clock as in R3.
- **The lane rule** (condition, then probability) now lives in Core (`laneEventValue`), shared by the engine and the
  checks. It is the same arithmetic as before, and GoldenTests hold.
- **SIMPLE page** (`UI/HamloRevolutionSimplePage`), drawn in the same 1180 x 1280 frame so the preset screen keeps
  its place.
  - Controls: CONNECT TO HPDG with its lamp and status line (right click: a list of running HPDGs), GENRE (AUTO shows
    what it resolved to), INTENT, AMOUNT, VARIATION, GROOVE and MIX, FILL, TARGET, GENERATE, LOCK, ADVANCED, and a
    read-only preview of the seven lanes with their bands and the play head.
  - ADVANCED has a SIMPLE button at the bottom left.
- **Processor.**
  - `simple*` parameters: GROOVE is read by the DSP; the others are page-only.
  - `generateSimple`, plus `simpleEdited` (regenerates unless LOCK).
  - `followHpdg` runs on a 4 Hz message-thread timer. In SIMPLE without LOCK, a new HPDG pattern regenerates the
    scene; when HPDG is gone, the last pattern stays.
  - The groove table is saved per scene. A new instance opens SIMPLE. A state saved before R4 has no mode and opens
    ADVANCED.

## Checks

Tests: 549 / 549.

| # | Check | Result |
|---|---|---|
| V1 | GoldenTests (R0's hashes); the slot 0 .. 47 = HPEQ's | bit-identical |
| hits | an HPDG snapshot -> roles per 1/16, groove (+20 / +40 ticks), GENRE AUTO = TRAP | exact |
| V10 | 4 genres x 5 intents x 3 seeds x (HPDG hits, template): events vs AMOUNT 0 .. 100 % | 120 / 120 never fall |
| V10 | fill events at the phrase end, on a lane with FILL's condition | 3530 / 3530 |
| V10 | TARGET HATS: lanes with events on HIGH | 576 / 576 |
| V8 | TARGET SNARE, every genre x intent at AMOUNT 100 %: on-hit events off the snare's 1/16s | 0 of 54 |
| V11 | VARIATION 0, FILL OFF: bar n = bar n + 1 over 8 bars, every lane (Core's lane rule) | 40 / 40 |
| V11 | VARIATION 100 %: bars differ; TARGET HATS still all HIGH | 40 / 40, 40 / 40 |
| V9 | GROOVE 100 %, linked: GLITCH gap start minus the straight one vs HPDG's offset (1/16s 5, 6, 13: +500, +1000, +500 samples) | 0.000 ms each (target 1 ms); GROOVE 0: 2 samples after the grid (the gap fade) |
| V12 | linked SIMPLE: AUTO = HPDG's genre, NOTE = HPDG's key; a new HPDG pattern regenerates (TRAP -> TECHNO), nothing new = no regeneration; LOCK keeps; HPDG gone (LOST) keeps | pass |
| V3 | blocks 1 / 64 / 127 / 4096 after prepare(256), a SIMPLE BOOM BAP pattern, GROOVE 100 % | bit-identical |
| V13 | page mode, SIMPLE settings, groove round trip, slots equal; an R3 state (no mode) opens ADVANCED with the same slot | pass |
| TOPS | GLITCH GAP on TOPS over 60 Hz + 6 kHz | 60 Hz -0.07 dB, 6 kHz -120 dB |
| page | a new instance shows SIMPLE; GENERATE (new seed), GENRE, AMOUNT, ADVANCED <-> SIMPLE reach the processor | pass |
| V7 | the R3 CPU check, rerun | worst pair 4.9 % (target 15 %) |
| V14 | pluginval 5 .. 10, Steinberg validator 537 / 537 (`docs/reports/pluginval/r4`, `vst3validator/validator_r4.txt`) | pass |

## Changes found by measurement (disclosed)

- **V9, first run.** Step 6 stayed on the straight grid: the HPDG pattern had no hit on that 1/16, so there was no
  offset. 1/16s without hits now take HPDG's swing (odd) or 0 (even). The design doc was amended.
- **V8, first run.** 20 TECHNO off-beat delay throws on hats were marked "on the hit" under TARGET SNARE. The mark is
  now the target's hit only; where the events go did not change.
- **V9 test bug.** The render was 1.5 s, which ends where step 13 starts. It is now 2.2 s.
- **V13 found an R3 slot imprecision.** THROW TIME (a 14-way choice) came back from a saved state as 6.9999995, not
  7. The engine rounds it, so the sound was the same, but the slot was not equal. HamloRevolution's choices are now
  rounded into the slot.
- **The ADVANCED page test** opened the default page, which is now SIMPLE, and the hidden ADVANCED page was not
  refreshed. The test now opens ADVANCED. The editor also refreshes a page as it comes up.
- **V10 wording, fixed before measuring.** BREAKDOWN's half-time fill takes the second half of the bar; a one-beat
  half-time switch would not be one.

Open (R5):
- GROOVE from LISTEN's timing: unlinked, the groove is the genre's swing.
- TONAL follows HPDG's key only through NOTE, which is set on generation.
- Not listened to in a DAW. HPDG and Revolution have not been linked live across processes in a real host; the tests
  use the same protocol file through a test writer.

Page: [r4_simple.png](r4_simple.png). ADVANCED is unchanged except for the SIMPLE button.
