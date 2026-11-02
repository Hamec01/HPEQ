# HamloShirota continuation: S4 complete, S5 next

2026-11-02. Parent d1ef423; S4 completion commit is the latest local stage commit
in git log. No active jobs; the accepted build/evidence are complete. No other
product, installed VST, push or subtree was changed. Follow AGENTS.md and
C:/Users/Ham_h/.codex/skills/efficient-staged-work/SKILL.md; no subagents.
Owner's latest request authorizes work on Shirota. No model switch occurred.

## Completed and accepted

Actual VST Legacy/Stereo-Space/Binaural with full numerical STEREO/SPACE/MOTION,
QUICK, old LEGACY,16editable nodes and three embedded horizontal profiles.
Original12parameters/programs/class IDs/state root preserved;147appended for159.
Seven original host programs; do not confuse the final count:7+200=207.
PDC round2.5ms in every mode, neutral modern dry/bypass exact, all allocation-free
prepared audio. Defaults preserve original Legacy while new fields are neutral.
Old states clear every new field; modern state stores all16nodes/program/view.
Shared PresetScreen unchanged at56,20,336x116. All six final snapshots inspected.

Evidence:docs/reports/s4/REPORT.md, acceptance.json, stage_final.json,
stage_final_legacy.json, guard_tests.json, timing_stage_final.json,
validators_stage_final/,stage_final_page_0..4.png,stage_final_binaural.png.
451native expectations;160M0bit-exact audio/state/latency cases;106integration
checks/31cases;5identity/corruption rejections;pluginval5..10;Steinberg537/537.
Block1/64/127/512/4096 same-position tempo/path/engine events are byte identical.
Zero allocations; prepare256 accepts4096. Rates8/32/44.1/48/88.2/96/176.4/192k.
I07out-of-carrier-43.3906dB versus-30target. Legacy old block dependence retained.

Bundle:build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3
DLL SHA256:10565fadcadcb279831ba8016483743638627ff2a3699d777fddbdcc2c854498.
Version still0.1.0 development, not final release. Keep entire bundle/resources.
New production.bin has own minimum-phase48k+ITD layout; original S3 bank untouched.
Bank SHA256:ec0a289b2e7e2d609286cc1bd3278762186a88109f1272910541f5319958b965.
Source/method pins compiled into processor/kernels/lab; actual embedded bytes
hashed in prepare. New source identities normalize text endings; bank exact.
Native48k magnitude error1.949e-5dB/ITD0; full other-rate spectra still needed.

## Reproduce or extend

PowerShell7 from E:/HamloShirota, isolated Python build/s3_venv/Scripts/python.exe
with -I -X utf8. No dependency/global upgrades needed.

```powershell
cmake --build build --config Release --target HamloShirota_VST3 HamloShirota_Tests HamloShirota_LegacyCompat HamloShirota_IntegrationLab HamloShirota_IntegrationTiming HamloShirota_EmbeddedBankFault HamloShirota_Snapshot -- /m:2 /v:minimal
build/Release/HamloShirota_Tests.exe
build/Release/HamloShirota_LegacyCompat.exe verify E:/HamloShirota/out/s1/reference E:/HamloShirota/out/s4/NEW_legacy.json
build/Release/HamloShirota_IntegrationLab.exe E:/HamloShirota/out/s4/NEW_RUN
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_s4.py out/s4/NEW_RUN out/s4/NEW_report.json
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/test_s4_guards.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/report_s4.py
pwsh -NoProfile -File Tools/validate_s4.ps1 -Label NEW_LABEL
build/Release/HamloShirota_Snapshot.exe out/s4/NEW_page.png 0 3 2
```

Snapshot args:output/program/page0..4/engine0..2. Timing tool takes NEW JSON path;
run alone, record all three means/outliers. Never overwrite accepted raw runs
or run builds while validators fingerprint a binary. report_s4 is tied to this
accepted checkpoint; new DSP requires new source-bound evidence, never relabeling.
LegacyCompat projects original12prefix; original Lab total12contract is historic.

Same final method baseline:out/s4/before_final_v2,94pass/3n-a. Materialize frozen
facade via Tools/prepare_s4_baseline.py; configure
-DHAMLO_SHIROTA_BUILD_S4_BASELINE=ON and build HamloShirota_S4Baseline explicitly.
It uses six old facade files plus unchanged common Legacy objects. Run with new
output directory then analyze_s4.py --baseline. Frozen source.json binds d1ef423.
Do not run old S2/S3 report scripts against this current DLL: old S1-hash guards
are intentional. S4 audit verifies their accepted code/data/reports separately.

Preserve out/s1/reference,binaries,original_sources; all S2/S3 archives; S4frozen,
before*,candidate*,final,accepted,accepted_final and sources snapshots. WIP history
in docs/S4_WORKLOG.md. Current final native log:docs/reports/s4/native_stage_final.txt;
build:s4_stage_final_build.log;raw lab:s4_stage_final_lab.log;validator logs tracked.
Reports include earlier UI/validator iterations; accepted_* is authoritative.
All S4changes belong to the single integration-stage commit. After completion
there is no uncommitted stage work; verify git status before starting S5.

## S5 objective and open checks

First read genre/category specification and existing preset/calibration tools in
this project before editing. Implement200meaningfully distinct recipes with
permanent IDs/metadata, old seven programs unchanged,207host programs total.
Calibrate on owner's local material after DSP acceptance; wet/dry depth and
true peaks, unique recipes beyond Mix/Output, finite/state checks, resumable
per-preset caches bound to DSP/material/tool identity. Prepare actual production
A/B listening examples; S3 WAVs use older phase/delay and are not S4 auditions.

Complete full resampled spectral tests before final release. Final CPU means
12.42..12.63percent pass25percent, maximum0.3342periods. An earlier run reached
1.0062periods; this remains
an OPEN live-host concern. Thread-cycle counts do not establish scheduling as
the cause. Old S3 larger outliers unresolved; no deadline-safety claim.
Owner FL Studio/K712 front/back listening is pending; horizontal HRTF is not
personalized or headphone EQ. No elevation/head tracking. Path timing includes
smoothing/acoustic delay; guard stationary Side budget is not arbitrary-window
mono guarantee. Keep Legacy compact program-metadata omission for compatibility.

After acceptance:version0.2.0, installer/ZIP/SHA256, validate exact packaged VST,
report every unpassed goal, local staged commits with Codex coauthor. No push,
subtree or other product changes. Recommend Astra xhigh for DSP/unexpected
measurements; medium for established tests/calibration/package/git. Owner changes
selector unless a supported tool actually confirms a switch.
