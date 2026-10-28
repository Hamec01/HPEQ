HamloComp 0.2.0 - compressor, VST3, Windows x64 (HamloProd)

Controls
  THRESHOLD  level where compression starts (-60 .. 0 dB)
  RATIO      how hard the level above the threshold is reduced (1:1 .. 20:1)
  KNEE       how softly compression begins around the threshold (0 .. 24 dB in effect)
  ATTACK     how fast it clamps down (0.1 .. 100 ms)
  RELEASE    how fast it lets go (10 .. 1000 ms)
  MAKEUP     gain after compression (0 .. 24 dB)
  MIX        parallel compression: compressed vs dry
  DETECTOR   PEAK reacts to every spike; RMS follows the average loudness
  CHARACTER  CLEAN transparent; PUNCH adds bite on the hits; GLUE program-dependent release
  AUTO MAKEUP  brings the output back up by what the compressor takes off on average
  ON         the lamp switch; off = the dry signal (20 ms crossfade)
  MODEL      CLASSIC (the original, CHARACTER applies), VCA (clean glue, auto release),
             FET (fast, aggressive, gritty), OPTO (smooth two-stage release), TUBE (ratio grows
             with the level, warm)
  SOURCE     what the detector listens to: ALL, DRUMS (the kick does not pump the kit),
             SAMPLES (melodies, keys, pads), VOC (plosives ignored, harsh syllables first)

Presets: the screen top left. GENRE and GROUP (click to choose), the arrows step through the presets of
that genre and group (or the mouse wheel; click the name for a menu of all of them), the line below says what
the preset does. * = a knob moved since it was loaded. The same presets are in the DAW's program list.
200 genre presets - Boom Bap, Trap, Techno, DnB, 50 each - in ten groups: Kick, Snare / Clap, Hats / Cymbals,
Percussion, Drum Bus, 808 / Bass, Vocals, Keys / Piano, Samples / Melody, Master. Release times follow each
genre's tempo. Their thresholds assume tracks around -18 dBFS RMS (mixes -14).

Factory presets (the DAW's program list): Default and 51 presets for drums (kick, snare, hats, drum bus),
vocals, bass, samples (keys, guitars, synths, pads) and the mix bus / master. Their thresholds
assume tracks around -18 dBFS RMS (mixes -14): trim THRESHOLD if your level differs.

Mono and stereo tracks. Latency 0.

Uninstall: Settings -> Apps -> HamloComp, or Start menu -> HamloProd -> Uninstall HamloComp.
