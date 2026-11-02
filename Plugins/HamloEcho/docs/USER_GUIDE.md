# HamloEcho 0.2.2

HamloEcho combines a stereo delay, an editable eight-note reflection score, frequency-dependent timing, a compact spatial field and captured-audio textures. **Default** now opens NOTE SCORE with Mix, feedback, space, glitch, modulation and pitch shifts at zero. It passes the dry signal unchanged. Raise Mix and then Time to explore the prepared first unpitched reflection. Default is available under Factory / General in the preset screen and deliberately resets Send, Mix Lock, bypass, Freeze and existing tails. Output 0 dB, Width 100%, open filters and unity sends are neutral rather than muted. **Legacy** retains the original sound and loads automatically for older saved projects. Loading an old saved Default restores its stored settings; select Default again for the new clean reset.

## QUICK: start with three controls

Select QUICK above the detailed pages. It uses the full Note Score engine and keeps your stored detailed score. Pick ECHO, TAPE, BOUNCE, NOTE FALL, NOTE RISE, REVERSE, TEXTURE or CLOUD, then raise MIX to hear the effect. Default remains dry until MIX rises (Insert).

- TIME: the primary echo interval. Host Sync follows tempo; Milliseconds covers 1–2000 ms. Double-click restores the middle setting. Typed divisions must match the displayed choices, such as 1/8 or 1/8 D.
- MOTION: adds up to four reflections and up to 68% loop feedback. The selected move determines character, stereo placement, note intervals, accumulated loop pitch or captured chopping. Note Fall/Rise reach a -12/+12 st step per circulation at maximum Motion.
- SPACE: blends in space and increases decay. Cloud reaches 12 s; other moves reach 6 s. Zero adds no spatial field.

MIX is separate so you can shape a move while remaining dry. Routing, Mix Lock, Freeze and Wet Output remain accessible. Send is wet-only regardless of stored Insert Mix; choose Insert for a zero-effect starting point. QUICK shares those controls with the detailed editor. Project state saves the macros and the underlying score.

Full-page tabs restore the detailed score you had before QUICK. **EDIT IN NOTE SCORE** instead transfers the exact effective move into the full editor, retaining the running sound and shared controls. You can then change individual notes, taps, bands or spatial routing. Loading a factory preset or Default disables QUICK. These eight moves are controllers, not extra host programs: the bank remains 208 programs.

Pitch remains relative to a manual C5/MIDI60 reference; an entire melody shifts by the selected intervals. TIME shows the requested primary interval: pitched capture has an 80 ms minimum, Reverse/Stutter add a 50–400 ms capture window, and effective delays cap at 4 s. All existing capture/headroom limits still apply. QUICK is a separate beginner view; Legacy and the complete Note Score pages remain available.

## Start with a musical reference

Set SOURCE NOTE to a known reference in your source audio. FL Studio labels are the default: C5 = MIDI 60 = 261.63 Hz. Standard labels use C4 for that same stored MIDI number. Changing the label convention changes text only.

Each reflection's NOTE transposes the complete input by its interval from SOURCE. For source C5, targets C4 / G4 / C5 / C3 produce -12 / -5 / 0 / -24 semitone reflections. Type notes, sharps/flats or MIDI numbers directly, use the +/- buttons, or select an Echo card and click the piano. Supported fixed intervals are +/-24 semitones; a red RANGE warning means that reflection is muted. This is a declared reference, not automatic pitch tracking or chord correction. An evolving melody keeps its motion.

FILL NOTES fills all eight note cells from the selected card's note: Same note repeats it, Step pattern adds the selected semitone interval to each subsequent cell. Existing Enabled switches and timing remain as set. Check any red range warnings before enabling a generated note.

## Place the reflections

Each card has Enabled, Note, Time, Sync, Level, Pan and Action. Free Time is independent of tempo; Sync selects a host-tempo division, including dotted and triplet values. The Time cell remains the stored free-time value when Sync is active. Swing, on Loop / Color, delays synced reflections 2/4/6/8 by a fraction of a sixteenth note. Reorder notes/times freely; cells do not have to be chronological.

Forward unity echoes use the requested time, to one sample. Transposition needs 80 ms of captured audio: effective pitched Forward delay is max(requested,80 ms). Dry audio and host PDC stay at zero samples. Reverse and fragment actions need their additional capture window, shown as CAP +...ms on the card. Changing pitch preserves the captured fragment duration and its effective timing grid.

Effective reflection delays are limited to 1–4000 ms; loop requests to 20–4000 ms. Sync and band multipliers resolve inside that range, so very slow-tempo or large multiplier requests can reach the 4 s ceiling. Pitch capture also limits very short pitched feedback intervals. The time cells show requested free times; Sync and capture can change the effective time.

## Feedback and character

Loop Time/Sync determine the interval between feedback rounds. Loop Pitch Step applies once per round, independently of the initial reflection notes. A -12 step gives a falling octave sequence; +12 rises by octaves. The accumulated result can leave the fixed-reflection +/-24 st range. Very low/high accumulated frequencies eventually reach the bandwidth limits of the audio and pitch renderer.

Clean, Warm, Tape, BBD and Dust affect the feedback loop. Drive adds soft saturation; Tape/BBD darken returns, while Dust adds coarse quantisation. This is character processing, not a promise of a named hardware emulation. Low Cut and High Cut shape the wet output. Per-reflection cuts live in Glitch / Details.

Frequency Bands splits the wet repeats into complementary Low/Mid/High regions. Each band has its own delay multiplier, feedback, level, pan and enable switch. Equal neutral bands reconstruct the delayed source. Moving a split changes a broad crossover region rather than a brick-wall division. A shared feedback safety trim limits high combined return gain; at moderate settings no trim is needed. Band controls can therefore interact at the highest combined feedback settings.

## Space and captured textures

Space offers Room (short discrete field), Plate (closely spaced loops) and Cloud (expanded field), with Amount, Decay, Size and Damping. After Echo adds the field after the delay; In Feedback circulates it through the loop; Before Echo transforms the source before its note reflections. The Decay control sets the spatial network's loss scale; the complete echo-plus-space decay also depends on feedback, damping, routing and source.

Each reflection's Space Send is on Glitch / Details. In After Echo/In Feedback routing it can leave an early repeat close while sending later repeats into the field. Before Echo uses the global Amount because space precedes every reflection.

Forward plays normally. Reverse plays overlapping backward fragments. Stutter repeats subdivisions of the previous captured fragment; Repeats sets the subdivision count. Tape Stop slows playback through each fragment. Skip omits that reflection.

The independent Glitch section targets Echoes, Space Tail or both. Repeat repeats captured windows in pairs; Reverse reverses them; Stutter repeats four subdivisions; Tape Stop slows them; Omit removes the selected branch. Fragment boundaries use short edge fades. Amount blends the transformed capture with its current branch. Capture needs the displayed fragment time before useful transformed audio is available. Glitch is an intentional rhythmic discontinuity effect, not a time-stretch algorithm.

## Insert, Send and Freeze

Insert uses ordinary dry/wet mixing. Send is wet-only and keeps the stored Insert Mix for a return to Insert. Wet Output changes only the wet level. Width=0 folds wet to mono; dry stereo stays intact. Duck reduces wet level while the input is active.

Mix Lock preserves Mix when loading factory presets. Loading a project restores its full saved state, including Mix, routing and the selected preset. A star marks settings changed since the recipe was loaded. Category buttons only filter; LOAD START loads the first recipe in the current genre/category. The shared preset screen also selects genre, category and individual programs.

Freeze closes new input and captures the rendered feedback-length texture in a stable held loop. Output EQ, Width, Mix and Wet Output remain available. Pitch motion and fresh effects stop entering the held loop. Freeze reports an infinite tail to the host. Releasing it fades back and lets the retained loop decay. Changing a structural setting starts a prepared new path while the old tail finishes; repeated requests fade the previous outgoing path over 50 ms and apply the latest request. Outgoing tails are limited to 120 s.

Projects save the controls rather than live captured audio. A newly loaded frozen instance starts with empty capture memory; release Freeze and feed new material before holding it again.

The plugin is an echo instrument, not a limiter: high combined levels can exceed 0 dBFS. The supplied demonstration files use a documented common gain so dry/Insert/Send stay below -1 dBTP. Preset measurements and any remaining limitations are in the release report.

## Factory library

The original Default and seven programs retain host indices 0..7. New programs 8..207 contain 50 recipes each for Boom Bap, Trap, Techno and DnB: Vocal 10, Drums 12, Samples / Keys 10, Synth 8, Atmosphere / FX 10. Their descriptions declare material and reference assumptions. Factory rhythms follow host tempo; metadata records the calibration tempo. Start with the appropriate category, then adjust SOURCE to your material and Mix to your arrangement.

Compatibility scope: saved effect states and the original effect controls retain their behaviour. The wrapper-generated host Program selector now has 208 choices rather than eight, so an existing normalised automation lane for Program needs review/rebinding. Direct program-number recall retains the original indices. Details are in the release report.
