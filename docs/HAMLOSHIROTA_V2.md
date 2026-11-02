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

## Stage S2: offline width and early space

Completed2026-11-02; targets and all method disclosures: S2_METHODS.md.
Final paired result:181/181checks over58render cases,4provenance fault tests,
40owner-material A/B examples. Original115native/160M0checks and production
VST3hash remain unchanged. See reports/s2/REPORT.md for measurements, the initial
startup failure, the mixed incremental build and12routing regressions caught by
extended coverage. New effects are not linked into the production plug-in yet.

## Later V2 stages

S3 completed2026-11-02:211assertions/67cases,5provenance rejection tests,
68auditions, unchanged Legacy/S2. Targets/disclosures:S3_METHODS.md;
results/remaining limits:reports/s3/REPORT.md. Prepared horizontal SADIE II
D1/H3/H4profiles carry their upstream license/attribution in Assets/Hrtf.
Owner K712 front/back audition remains pending. Mean CPU passes; individual
wall-time outliers require host/performance investigation before release.

After S3, integrate the accepted prototypes and resolve PDC/transitions/state
compatibility. Implement precise STEREO/SPACE/MOTION,
selectable binaural profiles, editable
timed paths, QUICK, neutral Default,200genre presets, source-pinned calibration,
validators, screenshots, installer/ZIP/SHA256 and owner FL listening examples.
Objective HRTF/filter tests do not replace owner front/back audition on K712.
The licensed prototype profiles are selected; no headphone EQ is claimed.
