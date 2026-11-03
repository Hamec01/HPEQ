# HamloZastava controls

Choose **INIT** for a neutral modern start, select a role, then turn **PROCESS
ON** when ready to hear it. Processing Off passes unity dry with the same fixed
5 ms delay. The host's original Default program retains its Legacy recipe;
INIT is the modern neutral start.

## QUICK

- **Clean:** shape pauses with Threshold, Range, Attack, Hold, Release and Mix.
  Choose Gate or Expand in Clean Shape. Soft rounds the gain response; Hard
  responds more directly. Range sets how far quiet audio is reduced.
- **Drum:** control Threshold, Range, Attack, Body, Tail and Mix. Body holds full
  level after attack; Tail ends exactly at the Range floor. Advanced exposes
  Onset Contrast and Retrigger Gap for deciding which hits start a new envelope.
- **Duck:** the key lowers the main audio while active. Choose External and route
  a sidechain in your host for an independent key. Attack engages ducking;
  Release recovers. Hold and hysteresis help keep its action steady.
- **Rhythm:** set pattern length, clock speed, Attack, Release, Range and Mix.
  Edit 16–32 steps individually. Tempo uses quarter-note beats per step:
  1 = quarter, 0.5 = eighth, 0.25 = sixteenth. Free uses milliseconds.
  Stopped transport passes dry with Tempo clock; the status line shows this.

Type exact values in the boxes, drag knobs/bars, or use the mouse wheel.
Double-click a knob to restore its default. Step boxes use percent: 0% is the
Range floor and 100% is full level. ALL/ALTERNATE buttons provide starting shapes.

## ADVANCED

**Envelope** exposes numerical gain and Drum envelope settings. **Key / Mix**
contains detector time, lookahead, HP/LP filters, Peak/RMS, key source, stereo
link, Mix and Output. **Rhythm** provides the complete pattern and clock controls.
Controls that do not affect the selected role are dimmed and disabled; their
stored values remain available when you select the appropriate role.

Clean/Duck/Rhythm Attack and Release measure a 90% gain response time. Drum
Attack/Body/Tail are finite full-duration segments. The QUICK envelope drawing is
a guide to these stages, not a measured waveform. TARGET GR reports the internal
gain envelope before Mix; it can preview action while Processing is off.

**LISTEN KEY** auditions the signed filtered key, independently of Processing,
Mix and Output. It uses the same fixed delay, with a 20 ms transition when toggled.
Its red button and status line identify audition. Host bypass and missing
requested external key return dry. Set either key filter to 0/OFF to disable it.

Hover controls for contextual help. A missing-key message means the host's
external key bus has no available signal path; route the bus or select Main.
Stereo main maps left/right key channels, duplicating a mono key. With a mono
main layout the first key channel is used.

## Legacy and projects

LEGACY selects the original engine and panel; QUICK/ADVANCED return to modern
controls. Original seven host program numbers and recipes stay available.
Changing pages within ADVANCED leaves audio settings unchanged.

Modern project states restore settings, selected program, view and tab.
Original Legacy states retain their historical restoration behavior. MODIFIED
and the preset name's asterisk indicate changes from its recipe, including
retained modern settings. Fresh modern INIT is neutral and differs from the
original Legacy Default recipe. The 200 genre presets are available alongside the original seven host programs.


## Genre library

GENRE filters Boom Bap, Trap, Techno or DnB; GROUP filters Clean, Drum, Duck or
Rhythm. Arrows and the preset-name menu navigate within the filter. Selecting a
recipe loads its processing settings and turns LISTEN KEY off. Global host
bypass remains independent. QUICK opens the chosen role; Rhythm retains its
pattern tab. The preset display stays at the top left on every page.

Reference tempos are 88/144/132/174 BPM respectively. Synced Rhythm recipes use
the actual host tempo; free recipes retain milliseconds. Duck recipes expect a
kick routed to the optional External Key input. Adjust Threshold to the main or
key level, use LISTEN KEY to inspect filtering, then turn audition off.

Gentle/Medium/Strong describe measured role-specific depth, not a universal
loudness target. Clean measures pause rejection with wanted/quiet detail checks;
Drum measures post-envelope rejection and attack preservation; Duck measures
key-active attenuation and recovery; Rhythm measures full/floor step contrast.
See GENRE_PRESETS.md for the declared acceptance bands and purposes.

Project states store permanent recipe IDs. A recognized ID takes precedence
over a stale numeric index; an unknown ID retains the supplied settings with a
valid numeric fallback. Loading an older state clears new fields as before.
Changing a processing control adds the modified marker; restoring its recipe
value clears it. Opening ADVANCED or another tab does not change the recipe.
