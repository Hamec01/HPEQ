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
  ENGINE       V2 (the knobs deliver their value on every hit, level independent, steady tones
               untouched) or CLASSIC (the original laws; older projects open on it)
  PROTECT      V2: what the shaper adds is kept under 0 dBFS (soft clip from -3 dBFS)
  ON           the lamp switch; off = the dry signal (20 ms crossfade)

Presets (the DAW's program list): Default; CLASSIC: Kick Punch, Snare Crack, Tighten Room, Soften Attack,
Drum Bus Snap, Sustain Bloom; V2: Kick Punch V2, Kick Long Boom, Snare Crack V2, Snare Fat Body, Tight Drums,
Room Killer, Drum Bus Punch V2, Soft Hits, Hats Shorter, Long Decay Bloom, Snappy Perc, Lo-Fi Dull Attack.

Mono and stereo tracks. Latency 5 ms (fixed, reported to the DAW).

Uninstall: Settings -> Apps -> HamloUdarnik, or Start menu -> HamloProd -> Uninstall HamloUdarnik.
