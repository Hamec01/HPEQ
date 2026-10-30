# HamloEcho 0.2.0

HamloEcho combines a stereo delay, an editable eight-note reflection score, frequency-dependent timing, a compact spatial field and captured-audio textures. Choose **NOTE SCORE** in the Engine selector or load a genre preset to use the new engine. **Legacy** retains the original sound and loads automatically for older projects.

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
