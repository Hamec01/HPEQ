# RISE / FILL: transitions from the last bars

Owner (2026-11-02): "In SIMPLE, separate buttons [RISE] and [FILL]: make that sound from the last 4 / 2 / 1 bars (a
choice), preview it, drag the WAV into the DAW; a mini mode on its own page." The design and the targets are in
[docs/RISE_FILL.md](../RISE_FILL.md); the references are summarised there.

## What was built

- **Capture** (`ModulePluginProcessor`):
  - A 20 s ring of the plug-in's input, before any effect, and the position of each block (ppq, tempo, playing)
    behind a seqlock.
  - "The last N bars": while playing, aligned to the most recent bar line (a bar that just ended counts); while
    stopped, the latest audio.
  - When less than N bars were heard, the start is silent and the page says so.
- **The renderer** (`DSP/TransitionRender`, offline, deterministic per seed):
  - **RISE:**
    - **ROLL:** the last beats repeated 1/4 -> 1/8 -> 1/16 -> 1/32, halving (or on a smooth curve, by the seed).
      The pitch climbs to +12 x PITCH semitones. The read never wraps inside a grain.
    - **SWELL:** the last bar reversed, through a long reverb (wet), reversed back.
    - **SWEEP:** a resonant high pass rising to 30 .. 1500 Hz.
    - **Level:** the build is levelled first (short-term RMS, +-18 dB), then the level ramps -24 -> 0 dB and the
      width opens.
    - **End:** a 5 ms fade on the bar line.
  - **FILL:**
    - **The bars before the last:** the source, at most one beat edited per bar.
    - **The last bar:** blocks of 3 + 3 + 2 eighths (or a rotation) from seeded places in the bar.
    - **The roll:** the last beat's first 1/16 at 1/16 x 2, 1/32 x 2, 1/64 x 2, released on the downbeat.
    - **The ending:** a REVERSE or a TAPE STOP of the last eighth (by the knobs and the seed).
    - **Edges:** 1.5 ms fades on every block.
  - **Peak:** never above the source's peak or -0.1 dBFS.
- **The page** (`UI/HamloRevolutionTransitionPage`, the same frame and preset screen):
  - RISE / FILL tabs and BARS 1 / 2 / 4.
  - Four knobs (RISE: PITCH, ROLL, SWELL, SWEEP; FILL: CHOP, ROLL, REVERSE, STOP). They re-render when released.
  - GENERATE: capture now and a new seed.
  - PLAY / STOP: the clip replaces the plug-in's output.
  - The waveform with beat lines and the play head. Drag it into the DAW: `performExternalDragDropOfFiles`.
  - SHOW FILE (Explorer) and BACK.
  - The WAV is 24-bit at the session rate, in `Documents\HamloProd\HamloRevolution\Renders\`, named
    `Rise_120bpm_2bars_<seed>.wav`.
  - SIMPLE has [RISE] and [FILL] beside LOCK; each opens the page with a fresh clip.
  - Help texts for every new control.

- **A sample as the source** (owner: "and dropping a sample from the DAW into the same window"). Drop any WAV, AIFF,
  FLAC, MP3, OGG, WMA or M4A from the DAW or Explorer onto the page. RISE / FILL are then made from it: from its
  start, repeated to fill the bars at the host's tempo, resampled to the session rate (Lagrange), at most 60 s read.
  The status line names the sample. USE INPUT goes back to the track's input, and GENERATE keeps the sample, with a
  new seed.

## Checks

| Check | Result (target) |
|---|---|
| capture: a click on every bar line at 120 BPM, the last 2 bars | 192000 samples = 2 bars, the first click at sample 0, 2 clicks, aligned (exact) |
| RISE on the drum loop (2 bars) | the last quarter +14.0 dB above the first (>= 6); centroid 511 -> 10927 Hz (rises); ends at 0 |
| RISE on a pad | +12.6 dB; centroid 553 -> 1264 Hz |
| RISE on a 440 Hz sine, PITCH 100 % | the end at 884.7 Hz (880, within 3 %: 0.5 %); the fourth difference -52.2 dB re the peak (<= -40) |
| FILL on the drum loop (2 bars) | bar 1 = the source (up to one gain) 100 % (>= 75 %); bar 2 differs -3.7 dB (> -20); the roll's 1/16 repeats correlate 1.000 |
| determinism | the same seed = bit-identical; a new seed = another clip (RISE and FILL) |
| WAV | 192000 samples, read back within 1.2e-7 (2^-22); the clip <= -0.1 dBFS |
| preview | processBlock outputs the clip |
| page | SIMPLE's [RISE] opens the page with a clip; FILL and BARS 4 re-render; BACK returns |
| a dropped sample (0.8 s, 1 kHz, 44.1 kHz mono WAV) | the source; 1000.0 Hz at 48 kHz; repeated every 38400 samples (correlation 1.0000); USE INPUT returns to the input; a missing file is refused |
| the full suite / validators | 1096 / 1096 (GoldenTests: R0 bit for bit) / pluginval 5 .. 10, Steinberg 537 / 537 |

Renders for listening (`E:\HPDG_Revolution_Live\renders_rise_fill\`): rises from drums (1, 2, 4 bars) and from a pad,
fills from drums (2 and 4 bars), with their sources. Page: [transition_page.png](transition_page.png).

## Found and fixed on the way

- **The capture lost a bar that had just ended.** The "last bar line" was taken from the newest sample, so audio
  stopping exactly on a bar line dropped that bar. It is now taken from the end of what was written.
- **RISE clicked on a sine (-17.7 dB).** A grain sped up past its slice wrapped back to the slice's start. The grab is
  now two beats long and the read stops at its end: -52.2 dB.
- **RISE on drums fell instead of rising (+2.5, then -2.3 dB).** The early 1/4 grains carry the kick, the late
  1/32 grains through the high pass do not. The build is now levelled before the ramp: +14.0 dB.
- **FILL's first roll had no repeats** (1/8, 1/16, 1/32 once each). It is now 1/16 x 2, 1/32 x 2, 1/64 x 2.
- **The WAV clipped** at 1.07 (the test loop peaks over 0 dBFS). The clip is now capped at -0.1 dBFS. The FILL test
  compares bar 1 up to that one gain.

Open:
- **Drag into FL Studio is not tested.** JUCE's external drag works in many hosts but not in all. SHOW FILE is the
  fallback.
- **Not listened to by the owner.**
