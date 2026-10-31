# HamloShirota S1 handoff

Owner authorized implementation2026-10-31. Scope: SHIROTA_V2_SCOPE.md. Listening
reference:AKG K712. Work only in E:/HamloShirota; no Verb/Echo/Mag/HPEQ edits,
subtree, push or subagents. No model/effort switch was performed.

Completed stage: immutable M0 plus isolated Legacy. Baseline93d965f; S1 completion
is the latest local commit (git log -1). No uncommitted work should remain after
that commit; check status before continuing. No active jobs.

Production compiles10Shirota-owned units, without LayerEngine/FxNodeProcessor or
foreign effect/panel dependencies. Historical unused sources remain uncompiled.
Original ShirotaCore.cpp and panel/screen files are unchanged. Lightweight slot
indices and the copied M6 law preserve the old insert ramp/node delta/layer sum.
Conditional alignment histories, SAFE, initial ramps and mono behaviour stay
Legacy-compatible. Original12parameters/7programs, state root, metadata and class
IDs remain. Version is still0.1.0; no new installer or spatial V2 release claimed.

Evidence:160/160raw M0 renders match byte-for-byte; state restoration/reserialization
and latency events match. Original59expectations pass; extended115/115 includes
zero C++ audio allocations and all static programs across blocks1/64/127/512/4096.
Pluginval5..10 and Steinberg537/537 pass. Screenshot layout inspected;447pixels
of time-dependent meter tremor differ, while all remaining pixels match.
Processor sizeof844152->3032bytes (not total heap); DLL12561408->10658816bytes.
Validated DLL SHA256:ff712038bf196a3f75077ca2ed5545e43c334ba0c3ca20a77e77669a10ffb3a1.

Preserve out/s1/reference (160states/raw renders, contract, capture metadata),
out/s1/binaries (original VST3, tests, snapshot and lab), and original_sources.
Never rerun capture into an existing archive. Capture/verify lab source is
Tests/ShirotaLabMain.cpp; same method before/after. Final tracked results are in
reports/s1, especially REPORT.md and acceptance.json. Tools/verify_s1.py is a
checkpoint-specific audit; it intentionally requires the S1 contract/build.

Commands/results:

- `cmake --build build --config Release --target HamloShirota_Lab HamloShirota_Tests HamloShirota_VST3 HamloShirota_Snapshot -- /m:2 /v:minimal`: success.
- `build/Release/HamloShirota_Tests.exe`:115expectations,0failures.
- `build/Release/HamloShirota_Lab.exe verify E:/HamloShirota/out/s1/reference E:/HamloShirota/docs/reports/s1/after.json`:160cases,0failures.
- `Python311 -I -X utf8 Tools/verify_s1.py`:PASS; uses final tests/validator results and validated binary hash.
- `Python311 -I -X utf8 Tools/analyze_s1_width.py`:exploratory diagnostic from archived renders only.

Logs:build/s1_capture.log,s1_selfcheck.log,s1_isolated_build.log,s1_audit_build.log,
s1_final_tests.log,s1_after.log,s1_acceptance.log. Validator raw output is in
reports/s1/validators. Existing CMAKE_GENERATOR_PLATFORM environment warning is
harmless; compilation succeeds. No failed audio cases or relaxed thresholds.

Important inherited limitation: the M0 bypass/mix automation family produces4
distinct renders across5block sizes. Other tested event families are invariant.
Legacy matches every original schedule exactly; new modes must not inherit this
dependency. Do not alter Legacy to satisfy new-mode block goals.

Exploratory width evidence (not an S1 acceptance test):48kHz mixed-sine/noise,
discard100ms, Side/Mid energy -2.4132dB atWidth100%/SAFEon versus-1.7399dB at200%/
SAFEon; only0.6734dB gain. This supports investigating SAFE's width suppression,
not a universal perception claim. Method was added after capture and is disclosed
in HAMLOSHIROTA_V2.md/REPORT.md. No owner headphone listening has happened.

Next stage S2: define numeric targets/method before new experiments, then prototype
strong stereo width, mono-to-stereo and spatial characters with explicit visible
guard intervention. Preserve old state defaults and source IDs. Then timed paths
and binaural profiles. SADIE II is a candidate only: exact archive licensing,
attribution, profile processing and hashes must be checked before bundling.
Do not infer a personalized HRTF or headphone EQ from the K712 model name.

Later: precise STEREO/SPACE/MOTION controls, QUICK, neutral Default,200genre
presets/calibration with resume/source guards, validators, screenshots, FL owner
audition, report, installer/ZIP/SHA256 and stage commits. Original M0 PCM and
states must survive later append-only compatibility work unchanged.

Recommended reasoning for S2 algorithm/measurement work:owner's Astra xhigh.
Use medium for established build/test/packaging steps. Owner selects settings
unless a supported tool actually confirms a change.
