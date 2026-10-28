HamloRedWall 0.2.0 - limiter, VST3, Windows x64 (HamloProd)

Controls
  LOUDNESS   pushes the signal into the limiter (0 .. -30 dB threshold): louder, never over LIMIT
  LIMIT      the highest the output can reach (-20 .. 0 dB)
  RELEASE    how fast the limiting recovers (1 .. 1000 ms); AUTO adapts it to the material
  LOOKAHEAD  how early the limiter sees a peak (0.1 .. 10 ms)
  LINK       0 = each channel on its own, 1 = both channels together
  OUTPUT     level after the limiter (-24 .. 0 dB)
  MIX        parallel: limited vs dry
  STYLE      GLASNOST (clean), STAKHANOV (fast, loud), OCTOBER (soft clip before the limiter)
  TRUE PEAK  also catches the peaks between samples (+8 samples latency)
  MONITOR    NORMAL, UNITY (hear it at the input level), DELTA (hear what is taken away)
  DRIVE      harmonics before the limiter, mostly odd, oversampled: edge and presence (0 = off)
  TUBE       warmth and weight: even harmonics and softly rounded peaks - fatter, never a higher peak (0 = off)
  ENGINE     V2 (true peak on a finer grid) or CLASSIC (exactly the limiter of HPEQ); older projects open in CLASSIC
  ON         the lamp switch; off = the dry signal (20 ms crossfade)

Presets (the DAW's program list): Default, Safe Ceiling, Streaming -1 dBTP, Loud Master, Soft Clip Push,
Drum Peak Catcher, Broadcast -2 dB (these six run CLASSIC), and the v2 presets Clean V2 Master, Warm Master,
Fat Loud Master, Streaming Warm -1 dBTP, Edge Push, Boom Bap Weight, Trap 808 Glue, Drum Bus Crunch, Drum Bus Thick,
Vocal Presence, Keys Warmth, Parallel Smash.

Mono and stereo. Latency 11.4 ms at 48 kHz (549 samples) (fixed, reported to the DAW).

Uninstall: Settings -> Apps -> HamloRedWall, or Start menu -> HamloProd -> Uninstall HamloRedWall.
