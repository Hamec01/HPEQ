HamloComp 0.1.0 - compressor, VST3, Windows x64 (HamloProd)

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

Presets (the DAW's program list): Default, Vocal Leveler, Drum Punch, Bus Glue,
New York Parallel, Bass Control, Gentle Master, Squash (parallel), Drum Bus VCA Glue,
Drum Smash FET (parallel), Vocal FET Upfront, Vocal Opto Leveler, Keys Opto Smooth,
Pads Tube Warmth, Bass FET Grip, Mix Bus VCA 2:1, Master Tube Gentle.

Mono and stereo tracks. Latency 0.

Uninstall: Settings -> Apps -> HamloComp, or Start menu -> HamloProd -> Uninstall HamloComp.
