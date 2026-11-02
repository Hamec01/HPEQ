# HamloZastava: S1 complete, approved V2 next

2026-11-02; parent69901fb audit, production sourcef7330fd. S1completion commit
follows this handoff; latest git log identifies it. No active jobs. All source/
doc changes belong to S1. No other products/HPEQ, subagents, push/subtree,
installed plug-in change or model switch. Apply efficient-staged-work.

## Settled owner choices

All four roles with separate modes:natural cleanup, drum-tail control,
external-key ducking, rhythmic gate. Separate soft/hard profiles. Do not ask
these scope questions again. Approved scope:ZASTAVA_V2_SCOPE.md. QUICK/ADVANCED,
Black Steel/shared top-left PresetScreen,200genre recipes and VST-first delivery.
Installer is not immediate. Do not infer extra ML instrument classification.

## Accepted isolation

Version0.1.0; original15specs+bypass,7programs,state/class identity,5msPDC preserved.
Ten own translation units,25file local include closure. LayerEngine/FxNodeProcessor,
graph/project state/unrelated effect and panel units excluded. LegacyGateProcessor
copies both M6 and extended ZastavaCore behavior, including branch changes/reset.
LegacyZastavaCore preserves original slot mix/node delta/layer sum/alignment.
Source enum numeric values/parameter slot indices preserved in ZastavaSettings.h.

229primary PCM/input-state/latency cases exact to M0;458fresh/edited restored
state/audio/parameter/metadata observations exact to actual M0 restoration.
127native(71old+56isolation), zero C++ audio allocations first call and automated
branch/filter/bypass/lookahead; static7programs bit exact for1/64/127/512/4096after
prepare256. Six measured automation schedules also each identical across blocks.
pluginval5..10 and Steinberg537pass. Existing panel snapshot inspected.
Processor sizeof844152->2176bytes, not total heap/private working set.
Validated DLL SHA256:ef62bb5f012c8faacf50de92a9fe916828888dd92e6da18d95b3bc3b4522e738.
Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.
No V2sound/features/presets implemented or installer built in this stage.

Authoritative:docs/reports/s1/REPORT.md,acceptance.json,before_v3.json,
capture_v3.json,after_v3.json,native.txt,validators_final/,panel.png.
Original archive:out/s1/original(Source,configuration,VST,oldTests/CompatLab exes).
Final fixtures:out/s1/reference_v3. Preserve reference(v1),reference_v2,
isolated_observed_v2,debug outputs and all candidate/self-check reports.

## Disclosed method work

V1all229audio/latency pass but191combined state/readback assertions failed;
archived M0 against itself reproduces191. Serialized bytes unchanged in all229;
normalized parameter readback differs. Preserve this Legacy behavior, do not call
it a new V2state-quality pass. V2method records actual M0fresh/edited restored
bytes/PCM/readings. Its verification initially appended into a populated
MemoryBlock(File::loadFileAsData appends), falsely failing size comparisons.
Clearing the buffer fixes harness; no audio code/tolerances changed. Final v3
captures and verifies with the same driver. All229primary PCM/state/latency
fixtures still equal original v1. Diagnostics/old methods are retained in out/s1.

## Build and audit

PowerShell7 at E:/HamloZastava. Python available at
E:/HamloShirota/build/s3_venv/Scripts/python.exe; -I -X utf8, standard library only
for S1 scripts. No environment/global upgrades. Commands:

```powershell
cmake --build build --config Release --target HamloZastava_VST3 HamloZastava_CompatLab HamloZastava_Tests HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_CompatLab.exe verify E:/HamloZastava/out/s1/reference_v3 E:/HamloZastava/out/s1/NEW_verify.json
build/Release/HamloZastava_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_zastava_s1.py audit
build/Release/HamloZastava_Snapshot.exe out/s1/NEW_panel.png 1
pwsh -NoProfile -File Tools/validate_s1.ps1 -Label NEW_LABEL
```

Do not overwrite original native directories. Tools/verify_zastava_s1.py audit
is tied to this accepted checkpoint and current binary; new source requires new
matching evidence, not relabeling. Fixture hashes, compiled source/method stamps,
current include closure, reference/class IDs and validator DLL are checked.
Text identity normalizesCRLF; raw states/audio/binaries exact.

Frozen full M0 build:configure-DHAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON, explicitly
build HamloZastava_M0CompatLab. It uses out/s1/original headers/sources and current
same lab, with separate generated header. EXCLUDE_FROM_ALL; OFF by default for
fresh production builds. Archive required only for that optional baseline tool.
Tools/isolate_s1.py is the one-time reviewed extraction procedure from the local
frozen snapshot; do not rerun it after new V2changes. It uses the already-reviewed
local Shirota layer-sum skeleton only as an extraction aid, not a build dependency.
Do not build while validators fingerprint the DLL.

## Next coherent stage

S2write numerical goals/methods BEFORE new sound, capture n/a for unavailable
features, then implement own four-role processing with two profiles. Shared
legacy data fields/controllers cannot be repurposed or old normalized choices
expanded. Preserve old-state loading on an already edited new instance. Define
neutral modern startup and stable new-mode state/audio roundtrip explicitly;
Legacy's readback quirks are historical. Keep fixed5msPDC unless a separately
resolved design can prove compatibility/host notifications.

Key tests:noise/wanted signal distinction; quiet notes/word endings; attack and
body/tail windows; false/missed/chattering openings; detector/sidechain alignment;
lookahead/open/hold/release timing; stereo link/mono safety; rhythmic grid and
transport events; parameter transitions; sample/block determinism and zero audio
allocations. Use per-role metrics, not arbitrary compressor GR or reverb wet/dry.
Then numerical UI/QUICK,200genre recipes(207programs total), calibrated unique
recipes and cache guards, final VST validators/report/owner FL audition.
Recommend owner's Astra xhigh for new DSP/diagnosis, medium for known fixture/
build/package/git runs. Owner controls selector; no switch occurred.
