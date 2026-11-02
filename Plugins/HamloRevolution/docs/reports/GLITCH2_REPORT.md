# GLITCH 2: the block engine and the glitch formula

Owner (2026-11-02, after the first FL test): "RT sounds like shit", "the glitch must change the wave that is
playing, not add a robot sound on top", "redo the glitch". The research and the design are in
[docs/GLITCH_RESEARCH.md](../GLITCH_RESEARCH.md).

## What was built

- **The block engine** (`RevolutionEngine::blockHead`; a second GLITCH mode, the `glitchEngine` parameter). What plays
  is always the stage's own input, cut, repeated in musical pieces, re-ordered or pitched.

  | Action | What it does |
  |---|---|
  | REPEAT | the step before replays; each repeat -20 % x DEPTH |
  | ROLL | the run's own start retriggered at 1/32 (1/48 on a triplet grid, never below 30 ms), each repeat quieter, gated to 85 % |
  | RISE | an accelerating roll, 1/16 down to 1/64 (>= 5 ms), rising up to +7 semitones |
  | CHOP | 1/16 pieces gated to 45 %: the dry audio with silence between |
  | DROP | the 1/8 before plays at half speed (an octave down), repeats decaying |
  | SHUFFLE | another piece of the bar plays here: 3/16, 1/4, 3/8 or 1/2 earlier (BBCut offsets) |
  | REVERSE, STOP, GAP | backwards, tape stop, silence |

  - Every grain has a 1.5 ms raised-cosine attack and a release of up to 4 ms.
  - DEPTH sets how far the repeats decay; it no longer sets the repeat rate. That rate link was the robot.
- **Old projects keep the legacy GLITCH.** A state without `glitchEngine` loads as Legacy, even into an instance that
  had Block on. HPEQ's programs stay Legacy, and GoldenTests hold. SIMPLE, the genre presets and the GLITCH plate menu
  ("Engine: GLITCH 2") turn Block on. In Block mode the ADVANCED GENERATE writes the block counterparts.
- **The glitch formula in SIMPLE.**
  - **Budget:** events per bar by INTENT (SUBTLE 1, GROOVE 2, FILLS 2, BREAKDOWN 3, CHAOS 6) x AMOUNT; x 0.6 on
    sustained material.
  - **Spacing:** never two events side by side, and GLITCH takes <= 25 % of the bar (except CHAOS).
  - **Rolls and risers:** RISE only in the phrase tail, the bar's last beat. Rolls go on the target's hits (Trap,
    DnB) or in the tail.
  - **Placement:** repeats go on the strongest hits for drums, on the beats for sustained material.
  - **Ranking:** the events are ranked once, so a higher AMOUNT keeps every lower one. Genre weights:
    - BOOM BAP: SHUFFLE / REPEAT / DROP, then STOP / REVERSE;
    - TRAP: ROLL / REPEAT / DROP, then RISE / ROLL;
    - TECHNO: CHOP / REPEAT, then RISE;
    - DNB: SHUFFLE / ROLL / REVERSE.
- **SHAPE in every intent.** The owner saw no SHAPE events in TRAP SUBTLE. Now SHAPE A moves in SUBTLE too, and
  SHAPE B swells (a filter dip or lift) into the next bar in every genre.
- **The material.** The engine measures the input's dynamics: the loudest 10 ms against the median 10 ms over the
  last second. At >= 9 dB the input is percussive; below that it is sustained (pad, held vocal, keys). The result is
  kept in the scene.

- **GEN on SHAPE A / B and THROW A / B** (ADVANCED, owner: "the three have it, these do not"). It writes the
  generator's steps for that lane only: SIMPLE's genre, and per press a new seed with an INTENT zone and AMOUNT drawn
  from it. An empty result, or the same as now, is drawn again. The other lanes, the lane's band and all parameters
  stay. RES / TONE moved below the button.
- **The nameplate is gone** (owner: the preset screen covered it). The preset screen has the top left, and the wave
  screen starts right of it (x 404, it was 374).

## Checks

| Check | Before | After |
|---|---|---|
| the robot: the level's repetition period on a steady 1 kHz sine (a 4-step run) | legacy RATCHET at DEPTH 78 % (what SIMPLE wrote at AMOUNT 63 %): **16 ms = 62.5 Hz buzz** | ROLL 62 ms (1/32 at 120 BPM); REPEAT, CHOP, DROP, SHUFFLE, REVERSE: no repetition in 5 .. 120 ms (target: none under 30 ms) |
| the owner's case: TRAP, SUBTLE, AMOUNT 63 %, FULL, hats on the eighths | 8 RATCHETs per bar | 1 event (a REPEAT on 1/16 11) |
| ROLL's shape: the repeats against the first at DEPTH 50 % | - | -1.4 / -2.8 / -4.2 / -5.6 / -7.1 / -8.5 dB, largest miss 0.00 dB (target 0.5); gated |
| CHOP's gate: the silent share of the run | - | 56.9 % (gate 45 %) |
| the playing audio itself: SHUFFLE against the input 1.5 beats earlier | - | mean difference 3e-9 (the source, unchanged) |
| clicks: the fourth difference, a 100 Hz sine through each action | - | -65.3 .. -70.7 dB (target <= -40) |
| the formula: 4 genres x 5 intents x 3 seeds x AMOUNT 0.3 / 0.63 / 1 (180 cases) | - | within the budget 180, never side by side 180, <= 25 % 180, RISE in the tail 80 / 80, SHAPE written 180 |
| material | - | a pad 2.9 dB -> sustained, 1 glitch event, on a beat; the drum loop 23.2 dB -> percussive |
| old projects: a state without glitchEngine | - | Legacy; a GLITCH 2 state keeps Block |
| GEN on each of SHAPE A, SHAPE B, THROW A, THROW B | none | the lane written, the other lanes unchanged, a second press differs (4 / 4) |

The full suite: 1050 / 1050 (GoldenTests: R0 bit for bit); pluginval 5 .. 10, Steinberg 537 / 537. Renders for listening: `E:\HPDG_Revolution_Live\renders_glitch2\` (written by the test with
`HAMLO_RENDER`):
- the drums and the pad dry;
- the legacy RT ("BEFORE");
- each new action on the drums and on the pad;
- the owner's TRAP case as SIMPLE writes it now.

## Found and fixed on the way

- **The metric's first version measured itself.** At 440 Hz a 1 ms frame does not hold whole periods, so a steady
  level showed false repeats of 8 .. 25 ms. It also picked a multiple of the legacy period (78 ms = 5 x 15.6).
  - The test tone is now 1 kHz, with one period per frame.
  - A level that moves by less than 5 % does not count as repeating.
  - The fundamental is the shortest autocorrelation peak >= 60 % of the highest.
  - The 30 ms target was not changed.
- **Clicks at the first bar's events.** REPEAT, DROP, SHUFFLE and REVERSE reach back before the run. Just after
  play (or a jump) the tape had not recorded that far, so the head fell back to dry inside the run (-15 .. -30 dB).
  Now such a run plays dry whole.
- **Clicks at the run ends.** C-ATD's level correction (HPEQ's smart join, capped at +3 dB) put a kink in the fade
  whenever one head was near silence: a gate, or a grain's attack. It is off for GLITCH 2 segments; their grains have
  their own envelopes. The legacy GLITCH keeps it.
- **Material detection.** The energy novelty and R5's onset detector both read a chord's beating as attacks, so the
  pad came out percussive. This was replaced by the 10 ms dynamics measure.

Open:
- The owner listens in FL. Not measured: how it sounds on a real vocal or a real 808, and SIMPLE's other lanes
  (THROW, TIME) on sustained material.
