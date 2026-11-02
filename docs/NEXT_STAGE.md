# HamloShirota S2 handoff

Completed 2026-11-02. Owner scope: SHIROTA_V2_SCOPE.md; listening reference AKG
K712. Work only in E:/HamloShirota. No Verb/Echo/Mag/HPEQ changes, subtree, push
or subagents. Apply efficient-staged-work. No model/effort switch was performed.

S2 is an OFFLINE DSP checkpoint, not a new VST3 release. Production is still
0.1.0/S1, unchanged from commit5ba304c. The S2 completion is the latest local
commit after this handoff is saved; inspect git log/status before continuing.
No active jobs. Stage-owned changes should all be committed at that checkpoint.

Implemented: SpatialPrototype with width0..4, three spectral bands, mono-bass,
optional visible guard, Phase/Micro/Diffuse mono generation and Tight/Open/Scatter
reflection fields. Generated Side enters before bands/global width/mono-bass.
Space follows the stereo section and can create stereo even at Width0.
All histories allocate in prepare; continuous macros/mode weights are sample
clocked. Initial loaded settings snap before the first sample, live changes ramp.
Prototype fixed delay58/64/128samples at44.1/48/96kHz is NOT a host PDC decision.

Final evidence: docs/reports/s2/REPORT.md and acceptance.json. Paired v4 baseline
129passes/9n-a, candidate181/181 over58cases. The original86dry/input files match
first baseline; all58paired inputs match. Four provenance fault tests pass.
Native Legacy115/115 and immutable M0 renders160/160; production10-unit source
closure and validated DLL hash unchanged. S1 validators apply to that unchanged
binary; they were not rerun for offline S2. No installer/version bump.

Final DSP/method source ID:
a00303ddd9211a0a704fddca745e0ee3c6034ab25c0a74f23033c5049ef8d667
Production DLL SHA256:
ff712038bf196a3f75077ca2ed5545e43c334ba0c3ca20a77e77669a10ffb3a1

WIDTH2/4 gains+6.0206/+12.0412dB Side/Mid guardoff. Mono-bass150Hz reduces60Hz
original/generated Side31.839dB. Wet centroids10.655/27.054/65.043ms. Final CPU
means2.5423/2.5543/2.6994percent of48k/256block duration on i5-10300H, not total
system CPU or a host deadline guarantee. Raw maximum-setting stress is finite.

40owner-material A/B float WAVs: out/s2/listen/<material>_<scene>.wav. Four
materials vocal_lead1,keys,drumbus1,synth; each has dry,width2,phase,micro,diffuse,
tight,open,scatter,phase_open,diffuse_scatter. Compare within one material group.
Prepared worst8xpeak estimate-1.5477dBTP. Demo source/input/tool caches are pinned
and verified resume works. Owner listening is pending; these are not presets.

Preserve all out/s1 archives, and out/s2/before*,candidate*,routing_failed_v4,
final*,routing_before.cpp,actual_v4.cpp,v3_artifacts and source snapshots.
Failed startup ramp(138/1), timestamp mixed build(invalid99/9n-a), and omitted
routing combinations(169/12) are retained/disclosed; no thresholds were relaxed.
Final same-method pair fixes all12routing regressions. Original43jobs remain.

Tools/commands (PowerShell from E:/HamloShirota):

- cmake --build build --config Release --target HamloShirota_SpatialLab HamloShirota_SpatialDemo HamloShirota_StaleBuildProbe -- /m:2 /v:minimal
- Python311 -I -X utf8 Tools/analyze_s2.py out/s2/final_v4 docs/reports/s2/final_v4.json
- Python311 -I -X utf8 Tools/test_s2_guards.py
- Python311 -I -X utf8 Tools/prepare_s2_demos.py
- build/s2_venv/Scripts/python.exe -I -X utf8 Tools/report_s2.py

Python311 is C:/Users/Ham_h/AppData/Local/Programs/Python/Python311/python.exe.
S2 plots use isolated build/s2_venv with Matplotlib3.10.7; package versions in
reports/s2/python_environment.json. No global scientific-package upgrade.
Logs: build/s2_*v4*.log, s2_legacy_tests.log. Final source guard lives in both C++
translation units. A changed DSP/method invalidates cached results, not just
calibration. Never relabel an old render with a new source ID.

Tools/run_s2_pair.py <NEW-label> is the tracked reusable runner, with the exact
aligned-dry fixture Tests/SpatialBaseline.cpp. It refuses existing artifacts,
archives method/source snapshots, restores source with a fresh timestamp and
rebuilds the candidate in finally. Do not run concurrently with any other build.
The completed v4 pair used out/s2/paired_v4.py (same dry fixture and restore law).
Audit/demo/guard scripts intentionally target the accepted v4 checkpoint; update
that checkpoint explicitly after any future paired revision. Do not overwrite v4.

Known prototype limits before production integration:
- Size/predelay automation moves delay reads and can pitch; define transitions.
- Mono-bass coefficient changes are immediate; design/test a click policy.
- Guard budgets stationary Mid/Side energy, not every short-window correlation
  or the separate Space field. Mono conservation is Space-off only.
- Original Legacy bypass/mix remains block-dependent per M0; preserve old renders
  per schedule and keep new modes sample-clocked.
- No binaural, motion, host params, mono->stereo bus integration, UI or200presets.

Next coherent stage S3: numeric goals/methods BEFORE experiments for editable
position paths (free/tempo hold and travel, sample-clocked events), stereo source
spread/panning versus balance, and headphone binaural prototypes. Inspect exact
HRTF archive licensing/attribution/hashes before bundling; no dataset is selected
or downloaded. K712 does not imply a personalized HRTF or headphone EQ. Owner
front/back audition remains required to assess perception. Preserve Legacy and
S2 evidence; do not silently change stereo mono constraints for binaural effects.

Then integrate precise STEREO/SPACE/MOTION and QUICK, append-only host/state
compatibility, neutral Default,200genre presets/calibration, validators, visual
QA, owner FL test, report, installer/ZIP/SHA256. Original7host programs plus200
means207programs (not208). Shared PresetScreen stays unchanged/top-left.

Recommended effort: owner's Astra xhigh for S3 design/measurement diagnosis;
medium for established builds/tests/packaging. Owner changes the selector unless
a supported tool actually confirms a switch. No switch has been claimed.
