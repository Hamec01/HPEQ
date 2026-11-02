HamloVerb 0.2.0 - algorithmic reverb, VST3, Windows x64 (HamloProd)

NEW CHARACTERS AND WORKFLOW
  Clean: spatial FDN and natural reflections. Warm: cross-coupled diffusion and colour.
  Velvet: nested diffusion and soft density. Crisp: short dense plate for percussion.
  Deep: sparse early space and a developed distant tail. Bloom: cascaded build-up.
  New characters support 0-500 ms predelay and 0.1-30 s base decay.
  Insert blends dry/wet; Send outputs wet only and remembers Insert MIX.
  Mix Lock protects MIX on preset loading. Routing also survives preset changes.
  Output trims the complete Insert mix or Send return. Early/Tail have separate levels.
  Material buttons only filter presets; LOAD START changes the sound.
  Default + 7 original Factory + 200 new genre recipes = 208 host programs.
  Boom Bap, Trap, Techno and DnB: 50 each; Vocal, Drums, Samples/Keys, Synth, Atmosphere/FX.

ADVANCED
  Six Decay EQ bands and six Post EQ bands. Decay Value is an octave multiplier:
  +1 doubles the decay, -1 halves it. The curve displays a target, not a measured IR.
  Overlapping boosts are bounded by loop stability and may reduce attainable decay.
  Texture contains diffusion, modulation, bass/high decay, cuts and duck timing.
  Effects: input-triggered Gated, Reverse Swell, real Reverse Audio and independent
  Shimmer (-12 to +24 semitones). Reverse Audio reads overlapping captured wet
  fragments backwards (50-2000 ms); accumulation is part of wet timing, not host PDC.
  Freeze closes input/Shimmer injection, bypasses Time Shape and holds delay geometry
  and modulation. Output EQ, Width and Output remain available. Frozen tail is infinite.
  Character changes preserve one outgoing tail; repeated changes retire the oldest
  tail over 50 ms. At most two tanks run; outgoing Freeze releases.

COMPATIBILITY
  Old states restore Legacy and neutral new controls. Original parameter IDs, ranges,
  host order, Factory programs and plug-in identity are retained. Host latency: 0 samples.
  Legacy MIX adds wet over full dry. Insert/Send routing applies to new characters.
  Legacy Space/Color apply only to Legacy. Classic retains 250 ms / 20 s limits and
  its supported controls; it does not implement the later Freeze/Color features.
  Legacy Space switching retains its original block-dependent fade boundary.

Legacy controls (retained for old projects)
  PRE-DELAY    Time before the reverb starts. Keeps the dry hit clear in front of the room.
  SIZE         How big the space sounds.
  DECAY        How long the tail lasts (RT60 at 1 kHz).
  ATTACK       How fast the reverb builds up: 0 = instant, higher = swelling.
  DISTANCE     Close = more early reflections; far = more diffuse tail.
  WIDTH        Stereo width of the reverb.
  MIX          How much reverb is added. 0 = none (a new reverb starts here).
  EARLY DIFF   Smears the first reflections: low = distinct echoes, high = smooth.
  LATE DIFF    Density of the tail.
  MOD RATE     Speed of the movement inside the tail (avoids metallic ringing).
  MOD DEPTH    Amount of that movement; more = chorus-like.
  BASS X       Low frequencies decay this many times longer (or shorter) than the mids.
  BASS XOVER   Where BASS X begins.
  HIGH X       High frequencies decay this fraction as long: lower = darker tail.
  HIGH XOVER   Where HIGH X begins.
  LOW CUT      Removes low end from the reverb (keeps the mix clean).
  HIGH CUT     Removes highs from the reverb (less hiss and sibilance).
  DUCK         Lowers the reverb while the dry signal plays, brings it up in the gaps.
  SPACE        The kind of room: Room, Chamber, Hall, Plate, Cathedral, Ambience (Classic = the original HPEQ reverb).
  COLOR        Era of the unit: 1970 dark and grainy, 1980 bright digital, NOW clean.
  SYNC         Pre-delay locked to the song tempo.
  FREEZE       Holds the current tail forever (infinite reverb pad).
  ON           the lamp switch; off = the dry signal

Presets: the screen top left - the arrows step through the presets (or the mouse wheel; click the name for a menu).
The same presets are in the DAW's program list.

Mono and stereo. The latency is fixed and reported to the DAW.

Uninstall: Settings -> Apps -> HamloVerb, or Start menu -> HamloProd -> Uninstall HamloVerb.
