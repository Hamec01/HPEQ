# HamloShirota S3 handoff

Completed 2026-11-02. Parent S2 commit:2b7ff77. S3 completion is the latest local
commit after this handoff; inspect git log/status. No active jobs. Work only in
E:/HamloShirota; no Verb/Echo/Mag/HPEQ changes, subtree, push or subagents. Apply
efficient-staged-work. No model/effort switch was performed.

This is an OFFLINE checkpoint. Production VST3 remains0.1.0/S1. New S2/S3 effects
are not in the plug-in/editor yet. S1 archives and S2 sources/reports are intact.
S3 report:docs/reports/s3/REPORT.md; acceptance.json binds all final artifacts.

Implemented: MotionClock (16 nodes, hold then travel, ms/beats, smooth/linear,
shortest/CW/CCW arcs, loop/one-shot/reset/tempo changes, zero-time protection),
Stereo matrix/balance, separate Stereo Spread and Point source models, horizontal
binaural renderer with three measured profiles. Coordinate:0front,+90left,
180back,270right. Path telemetry is sample-exact; audio targets interpolate at
16sample intervals. Continuous settings/profile/enable transitions use20ms.

Profiles: UI label Profile1=D1 (files profile0), Profile2=H3 (profile1),
Profile3=H4 (profile2). Licensed SADIE II v2-1, Apache2; full notice/license in
Assets/Hrtf. Bank1,769,516bytes, horizontal5degree grid; full native256/256/512taps
at44.1/48/96kHz. One per-profile gain, no trimming/phase conversion/resampling.
Raw archives are in out/s3/downloads, not in shipping resources. Preserve them.
Bank SHA256 is verified in C++ prepare via BCrypt; no runtime file IO in process.

Results: final211/211 assertions over67cases, dry baseline155passes/4n-a.
Initial pair141/4n-a and190/190 also retained. Seven coverage jobs added (six
isolated emitter impulses and exact enable ramp); original60jobs/thresholds and
DSP recipe unchanged. Final same-method pair and all auditions use source ID:
70140105adecb392b8a168e95dce1d29dc025d8c0f7eaef9ed5f836bb9e83705
All original baseline PCM/traces match; all67paired input files match.
Five rejection tests: bank corruption, mixed objects, stale method/data identity,
unimplemented candidate and corrupt PCM cache. All pass.

Twelve independent path traces agree within1.011e-9degrees; node boundaries
within1sample. All process/RT-setter allocation counts zero, all event audio
bit-identical for blocks1/64/127/512/4096. Five-minute stress finite. Native
Legacy115/115 and immutable M0 renders160/160 freshly rechecked in this stage.
Production DLL still SHA256:
ff712038bf196a3f75077ca2ed5545e43c334ba0c3ca20a77e77669a10ffb3a1
S1 validators apply to that unchanged DLL; no new plug-in validator run claimed.

Performance: mean12.7373/13.1601/11.5869percent of48k/256block duration passes
25percent target. Largest wall-time callbacks2.2734/8.5823/0.9816block periods.
Cause of outliers unresolved; investigate thread scheduling versus actual work
and combined live-host performance before shipping. Do not claim deadline safety.

68listening WAVs in out/s3/listen: four materials (vocal_lead1,keys,drumbus1,synth),
17scenes each (dry, three profiles front/left/back/right/orbit, spread_orbit).
Orbit is counterclockwise over4seconds. One common gain per material group;
worst prepared8xpeak-1.00999954dBTP. All cache groups resume with verified hashes.
Owner K712 front/back listening is PENDING, not a personalized HRTF calibration.

Commands from E:/HamloShirota (use PowerShell7, UTF-8):
- cmake --build build --config Release --target HamloShirota_MotionLab HamloShirota_StaleMotionProbe -- /m:2 /v:minimal
- build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_s3.py out/s3/final docs/reports/s3/final.json
- build/s3_venv/Scripts/python.exe -I -X utf8 Tools/test_s3_guards.py
- build/s3_venv/Scripts/python.exe -I -X utf8 Tools/prepare_s3_demos.py
- build/s2_venv/Scripts/python.exe -I -X utf8 Tools/report_s3.py
- build/Release/HamloShirota_Tests.exe
- build/Release/HamloShirota_Lab.exe verify E:/HamloShirota/out/s1/reference E:/HamloShirota/docs/reports/s3/legacy_160.json

Do not overwrite accepted runs. Tools/run_s3_pair.py <NEW-label> is the reusable
paired runner with tracked Tests/MotionBaseline.cpp and source restoration in
finally; do not build concurrently while it temporarily installs the dry stub.
Completed pair used out/s3/run_final_pair.py. Preserve out/s3/before_sources,
candidate_01_sources,final_sources,before*,candidate_01,final and downloads/demos.
Audit/demo/guard scripts explicitly target the accepted final.json checkpoint.
A changed DSP/method/bank invalidates cached measurements and auditions.

Environment: h5py3.15.1 installed ONLY in build/s3_venv with read-only access to
base numpy/scipy. Plotting reuses build/s2_venv Matplotlib3.10.7. Versions recorded
in reports/s3/python_environment.json. No global upgrades. Logs:build/s3_*.log.
Fingerprint-bound files/evidence preserve exact line endings via .gitattributes.

Next coherent stage S4: integrate S2/S3 into the actual VST with numerical
STEREO/SPACE/MOTION controls and QUICK; define acceptance BEFORE changing sound.
First resolve architecture/routing/PDC and profile data embedding. Preserve
original12parameter IDs/order/ranges/defaults, seven program numbers, XML root,
class ID and Legacy sample/state/latency behavior at the same block schedules.
Old state loaded into an edited new instance must clear every new setting.
Add new controls as separate IDs, not new entries in old normalized choices.
Shared PresetScreen remains unchanged and top-left. Neutral Default requested.

Specific integration risks:
- S2 reference delay58/64/128 and S3 delay128/128/256 are prototype decisions,
  not shipping PDC. Establish the combined path and host reporting explicitly.
- S3 supports only44.1/48/96k in this checkpoint. Shipping needs the supported
  host rate range, prepared resampling if required, and meaningful rate tests.
- Windows BCrypt and file-based bank prepare are experimental plumbing; embed
  licensed bank/notice, keep all loading/resampling/crypto out of audio callbacks.
- S2 size/predelay moving reads can pitch; mono-bass coefficient updates are
  immediate. Define and test automation/transition policies.
- Integration must avoid redundant per-block setters restarting manual-center
  ramps; pass changed events or make setters idempotent and test both schedules.
- Path command time is exact; filter interpolation/HRIR acoustics add delay.
- Stereo/mono conservation applies to the stereo-only section, not to Space or
  binaural rendering. Guard is stationary energy budgeting, not a universal
  short-window correlation guarantee.
- Legacy bypass/mix retains its measured M0 block dependency; preserve per-schedule
  identity and keep new modes sample-clocked.

Later:200genre presets with stable IDs/recipes/calibration, total207host programs
(7old+200new), screenshots and validators, owner FL/K712 listening, report,
installer/ZIP/SHA256 and stage commits. No push/subtree or other product edits.
Recommended effort:owner's Astra xhigh for S4 architecture/DSP/diagnosis, medium
for established build/test/packaging. Owner changes selector unless a supported
tool confirms a switch. No switch has occurred.
