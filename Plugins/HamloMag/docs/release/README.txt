HamloMag 0.2.0 - saturator, VST3, Windows x64 (HamloProd)

Controls
  MODEL        CLASSIC, TAPE, TUBE, IRON (transformer), GERMANIUM, VINYL: each its own curve and colour
  DRIVE        how hard the signal hits the model (0 .. 48 dB)
  TONE         tilts the colour darker or brighter
  LOW CUT      keeps lows out of the saturation; HIGH CUT keeps highs out
  MIX          blend of the saturated and the dry signal
  OUTPUT       level after the saturator (-24 .. +24 dB)
  HARMONICS    how many overtones the model adds; EVEN / ODD: warm and tube-like or edgy
  BIAS         tape bias: low = gritty and dull, high = clean and bright
  WOW, HISS, HUM 50, CRACKLE   tape / vinyl movement and noises; NOISE: ALWAYS or FOLLOW the signal
  SPEED        tape speed (or record rpm): slower = more low-end bump and a darker top
  OVERSAMPLE   1x / 2x / 4x / 8x against aliasing at high drive
  OVERLOAD, BUMP, 24 DB, AUTO   extra drive, head-bump, steeper filters, output kept at the input level
  SAMPLER      record through a classic sampler first: SP-1200 (26 kHz, 12-bit, aliasing), MPC60 (40 kHz,
               non-linear 12-bit), S950 (32 kHz, 12-bit)
  PHONE        adds the overtones of the bass / 808 so it is heard on phones and laptops (the sub untouched)
  SUB          protects a clean low band while the high band saturates (OFF = full-band saturation)
  DYNAMICS     +: the hits saturate and the tails stay clean; -: the tails saturate, the hits stay clean
  MATCH        tracks the input loudness for a level-matched comparison of colour
  DELTA        hear only what HamloMag adds
  ON           the lamp switch; off = the dry signal (20 ms crossfade)

Presets: the screen top left - GENRE and GROUP (click to choose), the arrows step through the presets of that
genre and group (or the mouse wheel; click the name for a menu), the line below says what the preset does.
* = a knob moved since it was loaded. 200 genre presets - Boom Bap, Trap, Techno, DnB, 50 each - in ten groups:
Kick, Snare / Clap, Hats / Cymbals, Percussion, Drum Bus, 808 / Bass, Vocals, Keys / Piano, Samples / Melody,
Master. Their DRIVE assumes tracks around -18 dBFS RMS (mixes -14). Factory: Tape Warmth, Tube Glow, Iron Weight,
Germanium Fuzz, Vinyl Dust, Cassette Lo-Fi, Parallel Grit.

Mono and stereo. Latency 7.3 ms at 48 kHz (351 samples, fixed, reported to the DAW).

V2 compatibility: new controls default to off; the original saved-state renders remain bit-identical after
latency alignment. OVERSAMPLE adds an 8x choice: saved integer 1x / 2x / 4x state values stay, but existing
normalized host automation can map differently with the expanded choice count.

Measured limits: the 9 kHz / 24 dB TUBE test reaches -77.8 dB alias suppression at 8x (target -90 dB);
MATCH's maximum measured mix loudness error is 0.58 LU (target 0.5 LU).

Uninstall: Settings -> Apps -> HamloMag, or Start menu -> HamloProd -> Uninstall HamloMag.
