# HamloShirota V2 — acceptance and implementation stages

Owner approval to start:2026-10-31. Scope: SHIROTA_V2_SCOPE.md. Listening reference:
AKG K712. Work only in this repository; no subagents, HPEQ/Verb/Echo edits, subtree
or push. Product text is English. No model/effort change was performed.

## Stage S1: immutable M0 and isolated Legacy

Record these goals before the first new laboratory run. Use one unchanged
HamloShirota_Lab executable source for capture and verification. Never overwrite
an existing reference directory. Retain raw float output and binary states in
the ignored local archive, plus hashes and results in the tracked report.

| ID | Acceptance | Method |
|---|---|---|
| S01 | Every original parameter ID/order/range/default, program number/name and XML root identical | Exact JSON contract comparison; preserve original class IDs |
| S02 | Every captured audio byte identical after isolation | All factory programs at44.1/48/96kHz, mono/stereo, blocks127/512; extra boundaries, M6 path and automation schedules |
| S03 | Old parameter state identical on fresh and edited instances | Restore each archived binary state before prepare; exact parameter snapshot and state reserialization |
| S04 | Latency and reported tail identical for each schedule | Capture/compare latency and tail at prepare and scheduled events; preserve SAFE's original alignment |
| S05 | No foreign effect/panel translation units in production build | Inspect CMake source list; Shirota-owned core replaces LayerEngine/FxNodeProcessor |
| S06 | No C++ allocations in prepared audio | Audit existing programs plus rapid SAFE/style/mix/bypass changes |
| S07 | Existing native suite, pluginval5..10 and Steinberg validator pass | Preserve original assertions; run against the isolated final VST3 |
| S08 | Existing panel layout retained | Inspect snapshot; shared PresetScreen files unchanged |

The Legacy comparison uses identical block/event schedules. Differences between
different M0 block schedules are measured rather than hidden. New STEREO/SPACE/
MOTION/Binaural controls are n/a at this stage. S1 is an architectural checkpoint,
not the release of the requested new spatial features; do not ship it as V2.

S1 result:160/160bit-exact,115native expectations, pluginval5..10 and Steinberg
537/537. M0's bypass/mix automation family retains a measured block dependency;
new modes must not inherit it. See reports/s1/REPORT.md. A post-capture exploratory
Side/Mid diagnostic was added using existing renders (no pass threshold and no
change to the immutable lab): width100->200% with SAFE increased Side/Mid0.6734dB
on the synthetic input. Its method is disclosed in that report and
Tools/analyze_s1_width.py; it guides S2 and is not a newly imposed S1 criterion.

## Later V2 stages

After S1, measure width/SAFE behaviour and prototype the new stereo/spatial
algorithms. Define numeric sound/CPU/latency targets before those experiments.
Then implement precise STEREO/SPACE/MOTION, selectable binaural profiles, editable
timed paths, QUICK, neutral Default,200genre presets, source-pinned calibration,
validators, screenshots, installer/ZIP/SHA256 and owner FL listening examples.
Objective HRTF/filter tests do not replace owner front/back audition on K712.
Licensing/profile selection is not yet complete; no headphone EQ is claimed.
