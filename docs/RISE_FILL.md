# RISE / FILL: transitions made from the last bars (design, 2026-11-02, before the code)

Owner: "In SIMPLE, separate buttons [RISE] and [FILL]: pressing one makes that sound from the last 4 / 2 / 1 bars (a
choice), I listen to it, and I drag the WAV into the DAW with the mouse. A mini mode: RISE or FILL opens another page."

## What the references do (risers and fills from any sound)

**Risers:**
- **Reverse reverb.** Reverse a sound, put a long reverb on it, reverse the result: the tail swells into the hit. The
  reverb's length sets the riser's length [1, 2].
- **Pitch up** 12 .. 24 semitones over the build [3, 4].
- **Filter:** a low pass opening (e.g. 150 Hz -> 10 kHz over 8 bars), or a high pass rising [4].
- **An accelerating stutter:** a short slice looped shorter and shorter into the drop [1].
- **Reverb, delay and width** opening at the peak.
- **One main motion:** do not let pitch, filter and volume all rise at full strength at once [1].

**Fills from a loop:**
- **Roll the last bar,** halving the length each time (a beat, 1/2 beat, 1/4 beat), and release on the downbeat. A
  roll released mid-bar sounds unresolved [5].
- **A reversed long sound** leading into the fill [6].
- **Re-slice the loop's last bar** (BBCut's 3+3+2 blocks), and end on a tape stop or a reverse [6, 7].

## The design

**Capture.** The plug-in keeps the last 20 s of its input (before any effect) in a ring buffer, and records the
musical position of each block (ppq, tempo, playing).
- **Playing:** "the last N bars" is aligned to the bar. It ends at the most recent bar line and starts N bars before
  it (4/4).
- **Stopped:** it is the last N bars of audio, whatever it was.
- **Not enough heard yet:** the start is silent, and the page says so.

**RISE** (length = N bars, ending exactly on the next downbeat):
- **ROLL (main motion):** the source's last beat repeated at 1/4 over the first half of the riser, then 1/8, 1/16 and
  1/32 over the last quarter, eighth and eighth (the loop-roll halving). Each grain is enveloped; the pitch climbs to
  +PITCH (0 .. +12 st) by resampling.
- **SWELL:** the last bar reversed, through a long reverb (decay ~ the riser's length), wet only, reversed back. It
  swells into the end.
- **SWEEP:** a resonant high pass rising 30 Hz -> 30 .. 1500 Hz, and the level rising -18 dB -> 0 dB (exponential).
  The width opens at the end.
- **The end:** a 5 ms fade at the bar line, so it lands on the downbeat.
- **Knobs:** PITCH, ROLL, SWELL, SWEEP (0 .. 100 %). The seed (GENERATE) picks the grabbed beat (the last or the one
  before) and the acceleration's shape.

**FILL** (length = N bars):
- **The bars before the last** play the source with light edits: one repeat or shuffle per bar, by the seed.
- **The last bar** is cut BBCut-style: blocks of 3 + 3 + 2 eighths from seeded offsets in the bar, then a roll on the
  last beat (1/8 -> 1/16 -> 1/32).
- **The ending:** a REVERSE of the last beat, a TAPE STOP, or neither (seeded, weighted by the knobs).
- **Knobs:** CHOP, ROLL, REVERSE, STOP.

**The page** (a third page, the same 1180 x 1080 frame and the preset screen):
- RISE / FILL tabs and BARS 1 / 2 / 4.
- The four knobs; turning one re-renders the same capture with the same seed.
- GENERATE: capture now + a new seed.
- PLAY / STOP: the clip replaces the plug-in's output while previewing.
- The waveform. Dragging from it exports the WAV to the DAW.
- A SHOW FILE button, for hosts where dragging out of a plug-in fails.
- BACK (to SIMPLE).

The WAV is 24-bit at the session rate, written to `Documents\HamloProd\HamloRevolution\Renders\`. SIMPLE gets [RISE]
and [FILL] beside LOCK.

## Targets

- **Capture:** a click on every bar line at 120 BPM: the captured last 2 bars start on the click (within 1 sample) and
  are exactly 2 bars long.
- **RISE** (on the drum loop, a pad and a sine):
  - exactly N bars;
  - the last quarter >= 6 dB louder than the first;
  - the spectral centroid rises (last quarter > first);
  - at PITCH 100 % the sine's last grain is an octave up (within 3 %);
  - the last 5 ms fade to 0;
  - on the sine, the fourth difference at grain edges <= -40 dB.
- **FILL:**
  - exactly N bars;
  - the bars before the last >= 75 % identical to the source;
  - the last bar differs (> -20 dB difference);
  - the last beat's repetition period <= 1/8.
- **Determinism:** the same capture, settings and seed give the same samples; a new seed gives a different clip.
- **The WAV** is written and reads back sample-equal at 24-bit (within 2^-22).
- **Preview:** while previewing, processBlock outputs the clip.
- **V1:** GoldenTests unchanged (the capture only reads the input). V14 validators.

## Sources

1. [Unison: how to create risers](https://unison.audio/how-to-create-risers/)
2. [MusicRadar: reverb risers](https://www.musicradar.com/tutorials/music-production-tutorials/pluginweek-reverb-risers-pads)
3. [MusicRadar: classic pitched riser](https://www.musicradar.com/tuition/tech/how-to-create-a-classic-pitched-riser-effect-618058)
4. [MusicRadar: synth sweep FX](https://musicradar.com/how-to/synth-sweep-fx)
5. [Mixgraph: loop roll](https://www.mixgraph.io/glossary/loop-roll)
6. [Audiotent: drum fill processing](https://www.audiotent.com/blogs/production-tips/drum-fill-processing)
7. [N. Collins, Further Automatic Breakbeat Cutting Methods](https://www.generativeart.com/on/cic/ga2001_PDF/collins.pdf)
8. [JUCE forum: external drag and drop from a plug-in](https://forum.juce.com/t/implementing-drag-drop-from-juce-to-daw/55905)
