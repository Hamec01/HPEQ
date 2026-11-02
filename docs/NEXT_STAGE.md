# HamloZastava: S2 complete, S3 host integration next

2026-11-02. S1 parent 894abf8; the S2 completion commit follows this handoff.
Find it with git log -1. No active jobs or unrelated changes. No subagents,
other products/HPEQ, installed plugin change, installer, push/subtree or model
switch. Apply efficient-staged-work; keep one coherent stage per chat.

## Settled scope

All four roles: natural cleanup, drum-tail shaping, external-key ducking and
rhythm, with separate Soft/Hard profiles. QUICK/ADVANCED, Black Steel and the
unchanged shared top-left PresetScreen. Later 200 genre recipes, 207 host
programs total. Owner's answers are settled; do not repeat scope questions.
VST-first delivery, no immediate installer.

## S2 delivered

Own pure-C++ kernel: Source/DSP/V2Gate.h/.cpp. Modes Clean/Drum/Duck/Rhythm.
Gate/expansion, knee/range/hysteresis/hold, Peak/RMS, HP/LP key filters, main or
external key, stereo link, 0..5 ms opening lookahead, Mix/Output/bypass. Soft has
a two-pole envelope; Hard one pole, both T90. Drum uses finite Attack/Body/Tail
and fast/slow onset contrast; Soft quintic, Hard linear. Rhythm supports 16..32
levels, free time or beat time, tempo and seek. Missing key/stopped synced
Rhythm fail open. All audio/key history allocated in prepare; fixed 5 ms PDC.

No host/UI integration in S2: production remains the accepted S1 Legacy 0.1.0.
Do not tell the owner these new modes are already available in the VST.

Final same-method pair: 66 cases, control 218 passes/0 failures/9 unavailable;
kernel 275 passes/0 failures. Exact blocks1/64/127/512/4096, zero C++ audio/set
allocations, five-minute192k stress, recovery and phase checks. Five negative
cache/source/method/role guards rejected. Eight real owner-material A/B WAVs
in out/s2/auditions_01, finite/allocation-free/peak <= -6 dBTP. No listening
verdict or FL test recorded. Full observations/limits:docs/reports/s2/REPORT.md.

Source ID:c08e4c9de2b82a7db9eeffaff95dd74d2b8584a596a1e03d442294086081a084.
Method ID:0567998ff0e306a19cf7a981d2902b05d304b5a3d954f926bc8406964dcb1ff9.
Worst48k256mean1.20825% of audio time; broader mean1.319%; stress192k1.87305%.
Recorded one-sample callback wall outlier204.48%; no host-deadline claim.
Nonfatal C4244 is std::fill of float storage with exactly representable literal0;
archived stub has same warning. No warning suppression or acceptance relaxation.

## Evidence and disclosed revisions

Final:docs/reports/s2/{acceptance,before_v3,after_v3,regression_v3,auditions}.json.
Primary methods:docs/S2_METHODS.md,Tests/V2LabMain.cpp,Tools/analyze_zastava_s2.py.
Audit:Tools/verify_zastava_s2.py; audition:Tests/V2RenderMain.cpp and
Tools/render_zastava_s2.py. Source/method/binary/material/raw hashes bind results.

Initial49cases:167control passes/9n-a and200candidate01 passes. Independent
Hold/knee/RMS/HP, tempo/seek, recovery, CPU max/cycles and full-window expiry
checks were missing. Expanded66case method found a real queue-capacity defect:
insert-before-expire filled the circular queue. Candidate02 reserves one more
slot. Archived candidate01 reproduces four failures with the final driver.
Final v3 added the required disclosure comments, so both sides were recaptured;
all132PCM outputs equal revision2. No numerical targets changed. Keep all runs.

out/s2/before_sources is the API-compatible unavailable neutral stub; its source
snapshot JSON and before.exe are retained. out/s2/candidate_01_sources is the
faulty initial kernel, candidate_01.exe retained. v1_method holds original test
sources. Final dirs/exes:before_v3,after_v3,regression_v3. Earlier before,
before_final,candidate_01,candidate_02,regression_01_final_method and acceptance_01
remain diagnostic history, not the final same-method pair. Do not overwrite runs.

## S1 compatibility checkpoint retained

S1report:docs/reports/s1/REPORT.md. Original archive:out/s1/original; accepted
reference:out/s1/reference_v3. Ten production units,25local include files, no
LayerEngine/FxNodeProcessor/project graph/foreign effects/panels. 15old specs
plus bypass,Default+6programs,class/state IDs,ranges/order and5msPDC unchanged.
229primary M0 matches +458fresh/edited actual restored matches,127native,
pluginval5..10 and Steinberg537/537. S1production DLL remains byte-identical:
ef62bb5f012c8faacf50de92a9fe916828888dd92e6da18d95b3bc3b4522e738.
Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.

Original normalized readback changes in191roundtrips despite stable serialized
bytes; preserve historical Legacy behavior. Final S1harness compares actual M0
restores. The earlier MemoryBlock append mistake and its fix are disclosed.
Do not rerun Tools/isolate_s1.py. S1global source stamp includes all Source files;
S2audit instead verifies retained accepted files, shipping units and exact DLL.
S3rebuilds need fresh compatibility evidence, not relabeled S1validator outputs.

## Reproduce S2

PowerShell at E:/HamloZastava; Python/numpy/scipy available at
E:/HamloShirota/build/s3_venv/Scripts/python.exe. No environment upgrades.

cmake --build build --config Release --target HamloZastava_V2Lab -- /m:2 /v:minimal
build/Release/HamloZastava_V2Lab.exe out/s2/NEW_run
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_zastava_s2.py out/s2/NEW_run out/s2/NEW_report.json
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_zastava_s2.py NEW_audit_label

The paired archived control is optional, EXCLUDE_FROM_ALL, defaultOFF. Current
local cache has HAMLO_ZASTAVA_BUILD_S2_REFERENCE=ON and ZASTAVA_S2_REFERENCE set
to E:/HamloZastava/out/s2/before_sources. Build target HamloZastava_V2Reference
before running it. Normal production builds do not need archived controls.
ReferenceIdentity uses the current method but the selected archived source.
Do not accidentally select faulty candidate01 as the neutral control; analyzer
rejects a role/source mismatch. Archived accepted exes remain authoritative.

Do not build the shipping targets just to recheck S2: this changes their global
source stamp even while the kernel is unconnected. Do not silently replace the
audition renderer exe either; its SHA binds auditions.json. A changed kernel,
method or renderer needs corresponding fresh measured/rendered evidence.

## S3 next coherent stage

Connect the kernel through an appended engine selector Legacy/Clean/Drum/Duck/
Rhythm and new stable parameter IDs, retaining all old IDs/ranges/order and seven
program numbers. Keep the Legacy path mathematically exact; use the fixed PDC
only once on either path. Add optional external mono/stereo key bus without
changing main mono/stereo layouts. Add host BPM/PPQ/transport adapter and explicit
key-missing status. Inspect existing processor before deciding buffer routing.
Kernel common attackMs/releaseMs mean rising/falling gain T90. In Duck map user
onset/recovery to falling/rising gain respectively (the demo uses releaseMs2,
attackMs100). Do not expose reversed Duck timing labels by accident. Drum's
Attack/Body/Tail are finite full-duration segments, a separate documented model.

Specify and test the neutral modern startup, full new state restoration, and
old state load into both fresh and already-edited modern instances. Missing
new fields must select Legacy and clear every new control; don't infer Legacy
from new parameter defaults. Preserve old program recipes. Include bypass and
main/key alias/routing tests, mono key -> stereo main, automation/clock at exact
sample positions, state/audio roundtrips and no audio allocations. Prove Legacy
against archived M0 using fresh matching S1driver evidence for rebuilt binary.

Later stages:QUICK/ADVANCED numerical editor and contextual help/key audition,
200calibrated unique recipes with guarded continuation, final VST validators,
visual snapshots and owner FL listening. No plugin release claim before these.
Recommend owner's Astra xhigh for adapter/state/clock ambiguities, medium for
known builds/checks. No model or effort switch has been applied by the agent.

Continuation prompt: Continue HamloZastava with S3 from this handoff; keep all
four approved modes and both profiles; no subagents or repeat scope questions.
