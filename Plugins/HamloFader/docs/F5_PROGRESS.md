# Historical F5 progress notes / 2026-11-04

F5 is now complete. The process/session IDs and NEXT list below describe an
earlier in-flight checkpoint and are inactive. Use docs/F5_HANDOFF.md and the
final report/acceptance receipt for current state. Calibrationv4 passed200/200,
the compiled bank and independent8xcheck passed200/200;delivery validated.

# Earlier in-progress checkpoint

Owner requested completion of Fader UI +200presets +release tests. Continue
autonomously, no subagents. E:/HamloFader master; completed F4commit6455e9f.
Working tree now contains owned uncommitted F5 library/tools/labs. Do not revert.
F4receipt/copies remain historical once F5 modifies source. No other products,
HPEQ/push/installer/installed replacement. Owner previously requested VST only.

F4complete:163native,224host,678Legacyrenders,pluginval5..10/Steinberg537.
New own UI QUICK/ADVANCED/LEGACY,all modes,learn/stop/apply,atomic meters,
graph2..32points. Shared old UI unchanged. out/f4/accepted is frozen.

F5goals BEFORE changes:docs/F5_PRESETS.md.200recipes(4x50),pergenreMatch11,
Ride19,Motion20;old7+Default unchanged=208. Newmetadata appended ModulePreset.
Tools/fader_presets.py generatesSource/Core/FaderGenrePresets.inc +JSON bank.
Host setCurrentProgram loads appended controls for programs8..207 and old0..7
Legacy unchanged. Editor now uses genre/group/note metadata. DSP kernel unchanged.
Version still0.1.0 pending final0.2.0 bump. No final package yet.

F5paired Legacy method:Tests/F5LegacyLab.cpp derived F3LegacyLab with explicit
0..7contract enumeration. Own method hashes source/allocation files/F5doc.
Captured before library changes:out/f5/legacy_reference,legacy_before.json/log,
226cases x3observations=678. Do not modify this method or F5design doc. Need
rebuild HamloFader_F5Legacy and verify after all changes against that directory.

Preset lab is native unitTests/F5LibraryTests.cpp. It was compiled/runnable on
actual F4 baseline and printedF5_LIBRARY=n/a. docs/reports/f5/before.txt;
out/f5/Tests-before.exe,F5LibraryTests-before.cpp. SHA256
5c6efe4915d05ad932513409c3de6cb81ebf1c82d541badd26eaaa12d3070515.
It remains UNCHANGED before/after. It validates208programs/IDs/effective recipe
uniqueness/ranges/state. Heavy material run is opt-in via environment:
HAMLO_FADER_LIBRARY_MODE=calibrate or check
HAMLO_FADER_LIBRARY_DIR=E:/HamloFader/out/f5/<UNUSED or resumable folder>
HAMLO_FADER_MATERIALS=C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav
build/Release/HamloFader_Tests.exe 'F5 preset library'
Scope env vars to that process; full native suite without env does light library
checks but not calibrate/check. Outputidentity stores DSP/method/skeleton/material
hashes; reject mismatched resumes. PerID records resume;check exports24bitWAVs.
Prepared strongest6s,48k,RMS-18subjectpeak-9(headroom for6dBRideboost). Active20ms
windows after0.5s,inputRMS>-60. Workmeanabs actualwindow gain;bands1..3/3..6/6..12.

Calibrationv1 completed198/200. BBHeavyKeys40 andDnBHeavyKeys40 both hadbest
gain near6dBboost saturation, just BELOW fixedHeavy6dB bound. No tolerance
relaxed. v2 increased NaturalStrength/timing,BB40passed6.01213 butDnBstillfailed.
v3Fast detector/timing stillDnBfailed5.999999999. All raw dirs/logs retained.
v4 now remeasures DnB40 with +12dBInput /-12dBGain (netunity fixed gain),
activityFloor shifted+12 too, to extend detector target range for sparse keys2
(prepared RMS-30.191dBFS). Keep exactgoal9.325/band6..12 andmaxBoost6/maxCut24.
Recipe note explicitly says compensated staging. All other recipes unchanged.
Live last command session21073 builds nativeTests andruns calibration_v4.
Readout/f5/calibration_v4.txt andrecords/fader-dnb-40.json;do not assume pass.

v2migration copied198 EXACT unchanged Python recipe dicts +passedrecords;
v3/v4 copied199 exact unchanged passed records. out/f5/migrate_v*.py andeach
caldir/migration.json/uncalibrated.json pin the provenance. All changes perID;
no old failed recipe reused. DSP/method/material identities must be equal across
versions; only bank skeleton changes for recipe edits. Finalcheck must render
ALL200 again independently; no use of migrated calibration as final audio proof.
Tools/fader_presets.py --calibration <caldir/result.json> additionally checks
currentDSP/method,exactuncalibratedrecipebank,sourcefile hashes before applying.
Its fingerprint mirrors native lab. May require debugging if hash mismatch;
do not bypass it. Generator changes are allowed; immutable native lab is not.

NEXT ifv4passes200:
1.python -B -I Tools/fader_presets.py --calibration out/f5/calibration_v4/result.json
2.bumpCMakeversion0.2.0;buildTests/VST3/Snapshot/F3Host/F5Legacy, m2.
3.runcheck innewout/f5/check_final (sameenv instructions);must200/200,all states
exact,peaksfinite,inband,workflowvalid. Fullnative andunchangedF3Host224.
4.E:/HamloShirota/build/s3_venv/Scripts/python.exe -B -I Tools/analyze_f5.py out/f5/calibration_v4 out/f5/check_final docs/reports/f5/analysis_final.json
independent8xpeaks all200<=-1dBTP;calibration/checkDsp/method/material/skeleton
equal andworkdelta<.01. WillreportexactPCMcount. Don't normalize checks.
5.rebuild/runF5Legacyverifyagainstout/f5/legacy_reference;678exact.
6.finalversionvalidators5..10/Steinberg537, source/deliveryidentity,snapshots.
7.prepare12private dry/processedA/Bs acrossgenres/modes fromsamepreparedinput
(reconstructfromidentitystart/rate/gain,verify preparedPCMhash) +exportedcheckWAV;
onecommongain perpair,notindependentRMSnormalization;8xpeakverify.
8.docsGENRE_PRESETS/readme/report/disclosures/AGENTS/NEXT,receiptverifier/freeze,
VST-onlyZIP/SHA256 matchesvalidatedbinary;localstagecommit. No push.

Tools/analyze_f5.py already written. No finalizer/verify_f5.py/validate_f5.ps1
or supplemental audition utility yet. CurrentCMake buildsF5Legacy target and
nativeF5LibraryTests. Need carefully preserve all historical methods/evidence.
Do not claim all calibrationmidpoints exact:some recipes hit target range edge
orclosestallowedsetting but pass the definedband. Need reportactualvalues.
Reasoning/model unchanged; medium routine,xhigh for hard DSP/metric failures.
