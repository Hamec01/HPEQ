HamloEcho 0.2.1 - musical echo machine, VST3, Windows x64 (HamloProd)

Default starts NOTE SCORE with Mix, feedback, space, glitch and pitch shifts at0.
Choose Default under Factory / General to clear all effects/tails, even with
Send, Mix Lock or Freeze active. Raise Mix and Time to explore the first echo.
Older saved projects retain their stored engine/settings. Genre recipes are unchanged.

Score engine
  Eight editable note reflections, piano/text input, host sync and Swing.
  SOURCE is a manual reference, not automatic pitch tracking or chord correction.
  FL Studio labels: C5 = MIDI60; Standard labels: C4 = MIDI60.
  Fixed reflection range +/-24 semitones; unsupported notes are visibly muted.
  Loop Pitch Step accumulates once per circulation, independently of reflection notes.
  Three frequency bands have independent timing, feedback, level and pan.
  Room / Plate / Cloud space: Before Echo, After Echo or In Feedback.
  Per-reflection Space Send, Reverse / Stutter / Tape Stop / Skip.
  Independent captured Glitch for echoes, spatial tail or both.
  Insert dry/wet, wet-only Send, Mix Lock, Duck, Width and stable Freeze.
  Dry and host latency: zero samples. Pitched capture minimum80ms; Reverse/Glitch
  need their displayed additional fragment capture. High combined band feedback
  receives a shared safety trim. Wet Output controls wet level only.

200 new stable-ID recipes plus Default and seven original programs:208 total.
Category buttons filter the library; LOAD START loads the first match.
Projects restore full saved state and selected program. Mix Lock applies to presets.
See USER-GUIDE.txt for timing, source-note semantics, routing and headroom.

Compatibility: old effect controls and saved states retain their behaviour.
The host's extra Program selector expands from eight to208choices; review any
existing normalised Program-selection automation. Original program numbers stay.

Legacy controls
  TIME L       Delay time of the left echo (or both when linked).
  TIME R       Delay time of the right echo.
  FEEDBACK     How many repeats: each echo comes back this much quieter.
  CROSS        Feeds each side's echo into the other side.
  DRIVE        Saturation inside the echo loop (tape / BBD grit).
  WIDTH        Stereo width of the echoes.
  MIX          How much echo is added. 0 = none (a new delay starts here).
  MOD RATE     Speed of the wobble (tape wow, BBD clock drift).
  MOD DEPTH    Amount of that wobble.
  DIFFUSION    Smears the repeats toward a reverb-like wash.
  PITCH        Shifts each repeat in semitones.
  LO-FI        Bit and sample-rate reduction on the repeats.
  LOW CUT      Thins out the repeats.
  HIGH CUT     Darkens the repeats; each one gets darker.
  BAND FREQ    Centre of the bell filter in the loop.
  BAND GAIN    Boost or cut of that bell, on every repeat.
  BAND Q       Width of that bell.
  DUCK         Lowers the echoes while the dry signal plays.
  LOW RES      Resonance at the low cut of the loop: a hollow, boxy echo.
  HIGH RES     Resonance at the high cut of the loop: a whistling, filtered echo.
  CHARACTER    CLEAN digital, TAPE warm and wobbly, BBD dark analog, LO-FI crushed (Classic = original).
  TIME MODE    What happens when you turn TIME: TAPE bends the pitch like a tape machine, FADE crossfades.
  SYNC         Left time as a note value of the tempo (FREE = milliseconds).
  SYNC R       Right time as a note value.
  LINK         Right time follows the left.
  PING-PONG    Echoes bounce between left and right.
  FREEZE       Loops the current repeats forever.
  ON           the lamp switch; off = the dry signal

Presets: the screen top left - the arrows step through the presets (or the mouse wheel; click the name for a menu).
The same presets are in the DAW's program list.

Mono and stereo. The latency is fixed and reported to the DAW.

Uninstall: Settings -> Apps -> HamloEcho, or Start menu -> HamloProd -> Uninstall HamloEcho.
