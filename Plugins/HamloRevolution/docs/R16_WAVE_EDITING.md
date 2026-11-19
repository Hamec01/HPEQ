# R16 / 0.4.1 — wave editing on the TRANSITION page

Owner requests after R15 in FL (2026-11-19):

1. **Select with the right button, label with the left.** A right-click selects the hit (or mark) under the
   mouse; a right-drag selects a region (snapped to GRID). The selection stays on the wave (dashed box) and
   nothing is rendered yet. A left-click on the selection, or on an existing mark, opens *THIS IS ...*:
   KICK, SNARE, HAT, PERC, TOM, **SAMPLE**, REMOVE MARK, CANCEL SELECTION, UNDO LAST MARK, CLEAR ALL MARKS.
   A left-drag still drags the WAV into the DAW. A left-click elsewhere drops the selection.
2. **SAMPLE** (`transition::kSampleMark`, role 5): "not a drum" — a melody, vocal or sample part. Every
   attack inside the region is removed from the drum analysis, so it is never used as a kick / snare / hat
   slice and is not offered as a hit region. Kit counts, rows and switches stay five roles.
3. **CLEAR / START OVER**: the same source and seed without dropping it again. Marks, undo history,
   selection and zoom are cleared; style (PHRASE AUTO / Classic Rise), knobs, EDIT LAST 4 BEATS, FILTER +
   ROOM 35 % and all kit switches return to their defaults. GRID and the sample's BPM stay.
4. **GRID**: OFF (beat lines only, free selection — the R15 look), 1/4, 1/8, 1/16, 1/32, 1/8 T, 1/16 T.
   Lines on the wave; right-drag selections snap to them. View only: the render does not use it.
5. **BPM** of a dropped sample: HOST (as before, the default), FROM THE FILE NAME (`fill_88bpm.wav` -> 88),
   FIT THE SAMPLE TO N BARS (`bars * 240 / seconds`), 60 .. 199 in menus, or TYPE A VALUE (40 .. 250).
   A new BPM is a new source length: the source is rebuilt and marks start again (seed kept). A new sample
   starts on the host's tempo. The track's input always follows the host.
6. **Zoom**: the mouse wheel zooms around the mouse (one notch about x1.25, deepest 1/64 of the wave);
   shift + wheel or a sideways swipe moves along; double-click shows the whole wave. The kit rows stay whole.

Compatibility: without SAMPLE marks the analysis is unchanged (no new code runs); host-tempo samples render
as in R15. Transition settings are not part of the saved plug-in state, so nothing old is migrated.

Tests: `Drum phrase fill R15 / R16: SAMPLE mark, selection, GRID snap, zoom, sample BPM, CLEAR`.
Full run 2026-11-19: 6497 passed, 2 failed — both in `HPDG Link / The polling thread` (a 1-second link
timing check; link code untouched, FL Studio was running the plug-in during the run). Not re-checked on
the R15 binary. No pluginval / Steinberg validator run, no installer, no manual FL check by the agent.
