# R8: ShaperBox-length movement, a compact page

Owner (2026-11-02, after FL): "study ShaperBox 2 and 3 better, think what is wrong with ours", "make the windows
smaller so it looks nice", "in SIMPLE the effects are too short". The owner also linked
github.com/PhillipAmend/shaperbox-importer.

## What ShaperBox does (2 and 3)

- **One drawn curve per shaper**, made of points, curves, steps and ramps. It loops over a synced length from 1/128
  to 32 bars (straight, triplet, dotted) or runs free. It has a smooth function against jumps, and the grid follows
  the length [1, 2].
- **Shapers:** Volume, Filter, Time, Pan, Width, Drive, Crush, Noise, Liquid. Each has a mix, up to 3 bands with
  their own curves, an envelope follower that can be laid over the LFO, and an audio trigger (curves started by
  transients) [1, 3].
- **TimeShaper draws time itself:** flat is a stutter, a slope is speed, downwards is reverse [3].
- **Typical moves:** a pump over a beat, gates at 1/8 or 1/16, filter sweeps over 1 .. 4 bars, throws on phrase ends
  [2, 3].
- **The importer** only bulk-imports `.vstpreset` / `.fst` packs into ShaperBox 3's SQLite store; the presets are
  zlib-compressed JUCE ValueTrees, schema 75 [4]. It describes no shape format we could learn from.

## What was wrong with ours

SIMPLE wrote blips. A SHAPE lane got a few single 1/16 steps (a drive level on one snare), so it moved only 22.8 % of
its cycle. A THROW sent one 1/16, a mean of 0.25 beats. GLITCH events were 1 .. 2 sixteenths, 0.46 beats. ShaperBox
thinks in movements over a beat, a bar or bars.

## What changed

- **SHAPE lanes are whole movements.** Each genre gets one per lane, with CURVE on. INTENT scales the depth (SUBTLE
  half) and AMOUNT how deep it goes; a higher AMOUNT never writes fewer levels.
  - TECHNO: the pump on every beat, 15 / 10 / 5 / 1 (CHAOS: a 1/16 gate).
  - TRAP: the pan a beat at a time, left / right.
  - DNB: drive breathing over the eighth after each snare.
  - BOOM BAP: a dusty low pass at 1/8 that opens into the bar end.
  - SHAPE B: a sweep. Over 4 bars (rate 1/4) from VARIATION 25 %, over one bar below that (so VARIATION 0 still
    repeats bar for bar).
- **THROW sends** last an eighth (A) or the rest of the beat (B, the fill throw), not a 1/16.
- **GLITCH events** are longer:
  - REPEAT, ROLL and SHUFFLE an eighth; DROP and CHOP a beat; the tail actions the last beat; RISE half a bar in
    FILLS and CHAOS.
  - GLITCH's share of the bar per INTENT is 4 / 6 / 8 / 8 steps, CHAOS unlimited. It was 25 % for all.
- **TIME's tape stop** in BOOM BAP takes the last beat (it was the last eighth).
- **The page is compact.** Rows 132 -> 104 apart, cells 84 -> 66 tall, smaller row knobs and lane switches. The frame
  goes 1180 x 1280 -> 1180 x 1080 and the editor opens at 885 x 810 (it was 885 x 960). SIMPLE is laid out in the
  same frame, so the preset screen stays where it is.

## Checks

| Check | Before | After (target) |
|---|---|---|
| SHAPE A / B: the share of the lane's cycle that moves (4 genres x 5 intents x 3 seeds, AMOUNT 50 %) | 22.8 % | 84.7 % (>= 50 %) |
| GLITCH: the mean event length | 0.46 beats | 0.69 beats (>= 1/8 = 0.5) |
| THROW: the mean event length | 0.25 beats | 0.64 beats (>= 0.5) |
| the glitch formula, 180 cases | - | within the budget, never side by side, within the INTENT's share, RISE in the second half, SHAPE written: 180 / 180 |
| V10, V11 (SIMPLE) | pass | pass (VARIATION 0 still repeats bar for bar) |
| the full suite | 1050 | 1053 / 1053 (GoldenTests: R0 bit for bit) |
| pluginval 5 .. 10, Steinberg | pass | pass, 537 / 537 |

Pages: [r8_advanced.png](r8_advanced.png), [r8_simple.png](r8_simple.png).

**Rules changed on purpose** (the owner asked for longer effects):
- GLITCH's share of the bar: 25 % -> 4 / 6 / 8 / 8 steps by INTENT.
- RISE "in the last beat" -> "in the second half".

The formula test now checks the new rules.

## Sources

1. [Sound On Sound: ShaperBox 3](https://www.soundonsound.com/reviews/cableguys-shaperbox-3)
2. [Cableguys support: timing, Length](https://www.cableguys.com/support/audio-automation-midi-and-timing), [ShaperBox 3 manual](https://www.cableguys.com/downloads/Cableguys-ShaperBox-3-Manual.pdf?v=3.3.1)
3. [MusicRadar: ShaperBox 2](https://www.musicradar.com/reviews/cableguys-shaperbox-2), [MusicTech: ShaperBox 2](https://musictech.com/reviews/plug-ins/cableguys-shaperbox-2/), [MusicTech: ShaperBox 3](https://musictech.com/reviews/plug-ins/cableguys-shaperbox-3-review/)
4. [PhillipAmend/shaperbox-importer](https://github.com/PhillipAmend/shaperbox-importer)
