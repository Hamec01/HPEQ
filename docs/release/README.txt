HamloUdarnik 0.1.0 - transient shaper, VST3, Windows x64 (HamloProd)

Controls
  SNAP         gain on the attack of each hit (-24 .. +24 dB)
  BODY         gain on the sustain / tail after the hit (-24 .. +24 dB)
  SPEED        how fast the attack and body envelopes follow
  RISE, DECAY  PERESTROIKA: stretch the rise time and the decay time of each hit (x0.5 .. x2)
  SENSITIVITY  which hits count: 1 = every one, lower = only clear hits over the background
  PRESERVE     keeps quiet and loud hits in proportion (ghost notes stay ghosts)
  MIX          parallel: processed vs dry
  OUTPUT       level after the shaper
  MODE         PRAVDA (direct gains), UDARNIK (loudness-neutral body, peak-safe snap),
               PERESTROIKA (reshapes the rise and the decay)
  LOOK-AHEAD   SPUTNIK 0 ms, VOSTOK 2 ms, SOYUZ 5 ms: sees the hit coming, catches its first sample
  LISTEN       FULL, ATTACK only, BODY only
  ON           the lamp switch; off = the dry signal (20 ms crossfade)

Presets (the DAW's program list): Default, Kick Punch, Snare Crack, Tighten Room, Soften Attack,
Drum Bus Snap, Sustain Bloom.

Mono and stereo tracks. Latency 5 ms (fixed, reported to the DAW).

Uninstall: Settings -> Apps -> HamloUdarnik, or Start menu -> HamloProd -> Uninstall HamloUdarnik.
