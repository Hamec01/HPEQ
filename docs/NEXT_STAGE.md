# HamloShirota: S5 VST-only completion and owner audition

2026-11-02. Parent5765c5e; S5 completion commit is the latest local stage commit
following this handoff. All stage jobs completed, no pending native/build/validator
sessions. No subagents, other products, HPEQ, installed VST, push or subtree edits.
Follow AGENTS.md and efficient-staged-work. No model/effort switch occurred.
Owner explicitly excludes the installer; deliver VST only unless requested later.

## Deliverable and verification

VST0.2.0:dist/HamloShirota-0.2.0-VST3/HamloShirota.vst3; entire folder/resources.
DLL SHA256:49864b3ddcfb8f34608d3201f9ab3e6beba14dd44df59f10156f148417e9b73e.
Adjacent SHA256SUMS.txt covers all bundled files. Copy/rescan in FL Studio manually;
agent did not overwrite any installed plug-in. No installer or ZIP was built.

207programs(0Default,1..6old,7..206new),200authored genre recipes with stable IDs,
genre/group/note/material/tempo/depth, calibrated on owner's WAVs. All200unique
excluding Mix/Output and UI-only fields; actual state/program/audio roundtrip exact.
Gentle/Medium/Heavy use residual energy vs aligned dry, Output gain excluded;
bands-24..-16/-16..-8/-8..0dB, centers-20/-12/-4. Worst prepared preset peak
-1.04999923dBTP,8xmeasurement. Four genres x50; category counts in GENRE_PRESETS.md.

2457native,160M0audio/state/latency,106integration/31cases,96resampled actual FIRs,
5provenance rejections,pluginval5..10 and Steinberg537pass. All31existing modern
PCM match S4 exactly. Current meanCPU11.16..11.45percent of48k/256period, max0.27314.
Five library_*.png inspected; final circle labels inside avoid BACK overlap.
47production listening WAVs indexed docs/reports/s5/demos.json, stored out/s5/listen.
Owner FL/K712 listening remains pending; profiles are not personalized/headphone EQ.

Authoritative report/evidence:docs/reports/s5/REPORT.md,acceptance.json,
verified.json,filters.json,calibration.json,catalog_before.json,catalog_final.json,
integration_release.json,legacy_final.json,native_final.txt,guard_tests.json,
validators_release/,delivery.json,timing_final.json,library_*.png.
Earlier validators_final/ and before_* reports are retained historical iterations.
All accepted native/calibration/verification attempts are in out/s5; preserve them.

## Changes and boundaries

Original12parameter prefix/ranges/class IDs/programs/state root unchanged;
147append for159. PDC round2.5ms every mode. Modern neutral aligned dry; Default
restores original Legacy and clears new controls. Legacy compact state still
omits selected-program metadata by design. Modern state stores permanent ID and
resolves it before the numeric program index, plus full settings/view/modified.

State repair:reset defaults now notifies listeners before APVTS replaceState.
Previous silent reset caused UI value1 with saved/DSP value3 if pre-load value
already equaled saved. Added regression; temporary diagnostic driver restored
byte-exact, final before/after method identical. Repeated200calibrations/96filters/
200state+audio checks;200full-strength PCM unchanged. First BB Bass Answer missed
center(-12.097 vs-12); room/predelay strengthened without changing goals; first
199passed results resumed until state repair invalidated sound fingerprint.
Only node-number placement changed after state verification; UI excluded from
sound fingerprint, full actual VST validators repeated on final UI binary.

DSP kernels/SADIEbanks/old Core presets/shared PresetScreen untouched. Source/
method/bank changes invalidate caches. Table excluded from sound identity so
calibrated Mix generation does not invalidate unchanged processing; compiled
recipe settings/IDs are independently verified against author/calibration JSON.
Original M0 and S2/S3/S4 source/raw evidence remain immutable.

## Reproduction

PowerShell7, E:/HamloShirota; isolated Python build/s3_venv/Scripts/python.exe with
-I -X utf8. No package/global upgrades required. Commands:

```powershell
cmake --build build --config Release --target HamloShirota_VST3 HamloShirota_Tests HamloShirota_PresetLab HamloShirota_V2Compat HamloShirota_IntegrationLab HamloShirota_IntegrationTiming HamloShirota_Snapshot -- /m:2 /v:minimal
build/Release/HamloShirota_Tests.exe
build/Release/HamloShirota_V2Compat.exe verify E:/HamloShirota/out/s1/reference E:/HamloShirota/out/s5/NEW_legacy.json
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py calibrate
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/genre_presets.py --calibration docs/reports/s5/calibration.json
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py verify
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/prepare_s5_demos.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/test_s5_guards.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/report_s5.py
```

Use NEWdirs/labels for raw native or validator runs; do not overwrite accepted
runs. Tools/validate_s5.ps1 -Label NEW_LABEL performs validators without rebuild;
never build while fingerprinting a DLL. Tools/deliver_s5.py copies exact validated
bundle and refuses a different existing delivery. Current script label is
validators_release; change audit/delivery references deliberately for a new build.
Snapshot args:output/program/page0..4/engine0..2. Bench timing takes a NEW JSON
path and must run alone. Old Lab/LegacyCompat assume old total program count;
V2Compat projects original first7programs/12parameters for unchanged M0fixtures.
S1..S4 audit scripts deliberately reject current source/DLL; use current S5 audit.

## Next action

Owner auditions delivered VST in FL Studio on AKG K712, especially left/right/
rear timing, three profiles, mono fold and saved projects. Human listening is not
performed by the agent. Live-host deadline assessment remains open: earlier
1.0062period S4callback/larger S3outliers unexplained despite current passing mean.
Do not claim deadline safety or personalized front/back calibration.

Installer deferred by owner, not a failed required stage goal. No automatic
subtree/push/new chat. If owner reports a DSP problem, recommend Astra xhigh for
diagnosis; medium for established builds/package/git. Model remains owner's
selector unless a supported tool confirms a change. Check git status before new
work; the completed S5stage should leave a clean checkout.
