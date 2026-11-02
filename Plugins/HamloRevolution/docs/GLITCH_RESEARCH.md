# GLITCH: what makes it musical, why ours is not, and a new GLITCH (proposal, 2026-11-02)

Owner, after the first live test in FL: "RT sounds like shit - robotic", "I would redo the whole glitch effect - find
out more about glitch". This note covers four things: what the references do, what is wrong with today's GLITCH
(HPEQ pass 10 DSP, taken over unchanged in R0), the design of a new GLITCH, and how to measure it. The design is fixed
before any code (docs/RULES.md).

## 1. What the references do

| Reference | The mechanism that matters |
|---|---|
| Ableton Beat Repeat [1] | **Interval** (how often, e.g. once a bar), **Offset** (where in the cycle the grab starts), **Grid** (grab length = repeat rate), **Gate** (how long the effect stays on: grab + repeats; no effect if Gate <= Grid), **Chance**, **Variation** (the grid varies around its value). **Volume / Decay:** each repeat quieter than the last (the article's graphic: -2 dB for the first, then -33 % per repeat). **Pitch / Pitch Decay:** the repeats move in pitch. **Filter:** a band-pass on the repeats only. **Insert** mutes the source while repeating. The musical advice: "less is more", long intervals, low chance, variation; on pads a short gate lets the dry signal through between repeats. |
| iZotope Stutter Edit 2 [2] | A **gesture** is a stutter whose **rate travels a curve** (e.g. 1/8 -> 1/128) over its duration: an accelerating roll, quantized to note values or sliding. Step modes: Walk / Skip / Stagger / Random. **Gate width + tail** chops each repeat: width 1 = the whole repeat; lower = cut shorter; tail = the release. **Buffer position** (which part of the captured audio plays, forwards or backwards), **jitter**, **pan alternating repeats**. Every parameter has its own envelope over the gesture (TVM). Fast enough, the rate "turns into pitch" - on purpose, as a riser, not as the steady state. |
| Glitch² (illformed) [3] | Nine sequenced modules on a step grid: **Retrigger** (repeats a captured slice), **Shuffler** (rearranges slices of the bar), **Reverser**, **TapeStop**, **Modulator** (tempo-synced filter / pitch movement), etc. Each step picks a module. |
| Sugar Bytes Effectrix 2 [4] | Effects painted on a 32-step grid; **Looper A / B** with time, speed and pitch modulation, scratch, reverse; a modulation sequencer per effect; randomization with "Tempo" and "Smoothness". |
| N. Collins, BBCut [5, 6] | **Phrase -> block -> repeat.** A block = audio from one offset, cut into repeats. Syncopated cut sizes, e.g. **3 + 3R + 2** eighths (a 3-unit block, repeated, then 2 units from later in the bar). **Rolls / stutters at the phrase's tail** (a stutter chance applies to the last half bar). The stutter speed is an integer multiple of the cut unit. **Weights change over the phrase:** big cuts, many repeats and early offsets at the front, rolls at the end. **Statistical feedback:** rare choices cannot bunch together. Three kinds of block: **not subdivided, subdivided evenly, subdivided geometrically** (accelerating or decelerating rolls). Cut grains get an **exponential envelope**; without one, cuts click. Repeats faster than ~20 Hz become "wavetable playback" at audible pitch. The methods work on any source audio, not only breaks. |
| Click-free repeats [7] | Loop edges click unless the loops overlap and crossfade. Gross Beat ramps the volume to zero at each relocation and back after it. |

**The common rules, distilled:**
1. **A glitch is an event, not a texture.** It is a grab plus a few repeats, then the dry signal returns. It happens
   once a bar or less, mostly at the phrase tail. "Less is more."
2. **Repeats have a shape.** Each repeat is quieter (decay), often gated shorter (width / tail), sometimes pitched or
   filtered. A block of equal, ungated loops is what sounds mechanical.
3. **Rates are note values**: 1/8, 1/12, 1/16, 1/24, 1/32, 1/48. Faster rates (pitch) appear only as a moving
   gesture, an accelerating roll or a riser, never as a steady buzz.
4. **Cuts follow the bar.** Syncopated block sizes (3+3+2), offsets that jump within the bar, bigger cuts early and
   rolls late in the phrase.
5. **Envelope every grain.** Use a short exponential attack and release, or a crossfade at relocations.

## 2. What is wrong with today's GLITCH

Measured from the code (`DSP/HamloRevolution.cpp`, HPEQ pass 10):

- **STUTTER and RATCHET rate = DEPTH.** The loop is the step / 2^(1 + round(3 x DEPTH)): from 1/2 down to 1/16 of the
  step. SIMPLE writes DEPTH = 0.4 + 0.6 x AMOUNT. At the owner's 63 % that is 0.78, so the loop is 1/8 of a 1/16: 14 ms
  at 135 BPM, a **72 Hz buzz**. That is the "robot".
- **Every repeat is the same.** There is no decay, no gate, no pitch or filter per repeat. The only shaping is a
  click-free envelope at the loop edges, so the repeat block is a steady tone.
- **A run fills its whole step.** There is no gate, so the dry signal never comes back inside the event.
- **The generator puts RATCHET on every hat hit** (TRAP: 8 of 16 steps) and on FULL, so the whole mix machine-guns.
  No phrase shape (rolls late), no budget, no spacing.
- **The cut model is per step.** It has no block / offset notion: JUMP plays an earlier window, but there is no
  3+3+2 cutting and no bar-level shuffle.

## 3. A new GLITCH (proposal)

**The owner's principle (2026-11-02): the glitch changes the wave that is playing, it does not add a robot sound on
top.** So the engine works as **INSERT** by default. What you hear is always the source itself, cut, re-ordered,
repeated in musical pieces, slowed, sped up, reversed or pitched, the way a producer chops a sample by hand. Pieces
are mostly 1/16 .. 1/4 long. Short rolls are accents at phrase ends; a tone made of tiny loops appears only inside a
deliberate RISE. No layer of synthetic sound goes over the dry signal.

A **block engine**: an event = **grab** (where from) + **repeats** (how) + **shape** (each repeat's envelope, level,
pitch, filter), quantized to note values. The old actions keep their DSP for old projects (V1). The new engine is a
second GLITCH mode, chosen per scene: SIMPLE, the new presets and new instances use it.

**Grab (source):** THIS STEP (the run's own start), LAST 1/8 / LAST BEAT (what just played), SLICE n OF THE BAR (a cut
from another place in the bar: BBCut offsets, the shuffle).

**Repeats (rate):**
- **EVEN:** 1/8, 1/12, 1/16, 1/24, 1/32, 1/48.
- **ACCEL:** geometric, e.g. 1/16 -> 1/64 over the run (the Stutter Edit gesture / Warp roll).
- **DECEL:** the reverse.
- The minimum steady rate is 1/48. Faster only inside ACCEL, and never below ~5 ms.

**Shape per repeat:**
- **Decay:** each repeat x 0.5 .. 1, -3 dB by default (Beat Repeat's graphic).
- **Gate width:** 0.3 .. 1 of the repeat, with an exponential tail.
- **Pitch:** per repeat, -12 .. +12 semitones with pitch decay. A rising roll is the riser; -12 gives a "half-speed"
  repeat.
- **Filter:** per repeat (a band-pass narrowing or a low pass closing), plus pan alternating.
- **Insert / mix:** insert mutes the source while the event plays; mix plays both.

**Envelope:** every grain gets a 1 .. 2 ms raised-cosine attack and an exponential release. Relocations crossfade.

**Actions = presets of the block engine** (what a step can hold, with readable names):

| Action | What it is |
|---|---|
| REPEAT | the last 1/8 once more, slight decay |
| ROLL | even 1/32 (or 1/48) repeats of the hit, decay -3 dB each |
| RISE | accelerating roll 1/16 -> 1/64, pitch +7 st, a filter opening (the riser into a drop) |
| CHOP | 1/16 repeats gated to 40 %: a chopped stutter with the dry between |
| DROP | repeats pitched -12 with decay (the half-speed fall) |
| SHUFFLE | play slice n of the bar here (BBCut offset), 3+3+2 patterns over a bar |
| REVERSE | the last beat backwards (kept) |
| STOP | tape stop (kept) |
| SCRATCH | kept |
| GAP | kept |

**Generation (with the "formula" for SIMPLE):**
- **A budget of events per bar** by INTENT: SUBTLE 0 .. 1, GROOVE 1 .. 2, FILLS 0 .. 1 plus 2 .. 4 in the fill bar,
  BREAKDOWN, CHAOS 4 .. 8. Rolls and risers sit at the phrase's tail (the last half bar of the last bar).
- **Spacing:** at most 2 adjacent glitch steps. Rare actions are kept apart by statistical feedback, so two RISEs
  never land in a row.
- **Weights move over the phrase:** SHUFFLE / REPEAT early, ROLL / RISE / STOP late.
- **Material:**
  - Percussive input: repeats on the strongest 1 .. 2 hits of the bar.
  - Sustained input (vocal, pad, keys): CHOP / DROP / RISE on beat positions, more SHAPE and THROW, fewer glitches.
  - LISTEN tells which: onset density and attack strength.

## 4. How it is measured (before / after)

- **Buzz:** the output's amplitude-modulation spectrum, 20 .. 100 Hz, relative to the input's. Today's RATCHET at
  DEPTH 0.78 is the "before". Target: no steady modulation peak in 20 .. 100 Hz outside RISE gestures.
- **Shape:** the repeat levels follow the decay within 0.5 dB; gate width and the rates are exact to the sample.
- **Clicks:** V4, the fourth difference at grain edges, <= -40 dB.
- **Generation:** events per bar within the INTENT budget; adjacency <= 2; rolls in the phrase tail >= 90 %.
- **Listening set:** the drum loop, a vocal phrase, a pad, a bass line; renders before and after for the owner.
- **V1:** old projects unchanged (the old actions keep their DSP).

## Sources

1. [Sound On Sound: Ableton Beat Repeat](https://www.soundonsound.com/techniques/ableton-beat-repeat-plugin)
2. [iZotope Stutter Edit 2 manual: Stutter and Buffer](https://s3.amazonaws.com/izotopedownloads/docs/stutteredit2/en/stutter-and-buffer/index.html), [Learn Stutter Edit 2 in 10 minutes](https://www.izotope.com/en/learn/learn-stutter-edit-in-10-minutes), [MusicTech review](https://musictech.com/reviews/plug-ins/izotope-stutter-edit-2-review/)
3. [illformed: Glitch²](https://illformed.org/), [Synthonia: Glitch 2](https://www.synthonia.com/blog/item/214-glitch-vst-the-glitch-2)
4. [Sugar Bytes Effectrix 2](https://sugar-bytes.de/index.php/effectrix2), [MusicRadar review](https://www.musicradar.com/reviews/sugar-bytes-effectrix2-review)
5. [N. Collins, Further Automatic Breakbeat Cutting Methods (2001)](https://www.generativeart.com/on/cic/ga2001_PDF/collins.pdf)
6. [Csound manual: bbcutm](https://csound.com/manual/opcodes/bbcutm/)
7. [JUCE forum: beat repeater clicks](https://forum.juce.com/t/beat-repeater-stutter-effect/44969), [FL Studio manual: Gross Beat](https://www.image-line.com/fl-studio-learning-content/fl-studio-online-manual/html/plugins/GrossBeat_MainOptions.htm)
