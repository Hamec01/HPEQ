# S1: independent Zastava Legacy processor

2026-11-02; audit69901fb, original productionf7330fd. Isolation complete; version
remains0.1.0. This is the compatibility foundation, not the four-role V2 release.
Owner approved cleanup, drum tails, external-key ducking and rhythmic gating,
with separate soft/hard profiles. Those new algorithms/controls/presets are next.
No installer, installed plug-in, other product, HPEQ, push/subtree or subagents.

## Result

Production now builds ten gate-owned translation units and25local files in the
include closure. LayerEngine, FxNodeProcessor, project/graph state and unrelated
effect/panel translation units are excluded. Original M6 and extended ZastavaCore
laws, branch/reset behavior, slot mix, node delta, layer sum and alignment clocks
are preserved in LegacyGateProcessor/LegacyZastavaCore. The independent envelope,
delay and wrapper operations retain their original calculation order.

| Check | Result |
|---|---|
| Primary program/boundary/automation PCM | **229/229 byte exact** |
| Original input-state serialization | **229/229 byte exact** |
| Fresh and edited project restoration | **458/458 state/PCM/metadata matches to actual M0** |
| Latency and tail reports | Exact at recorded block/event schedules |
| Parameter contract | Original15specs+bypass, IDs/order/ranges/defaults/choices/labels preserved |
| Host programs | Default+six original, same order/names |
| Native expectations | **127passed,0failed** (71original+56isolation) |
| Static original programs, blocks1/64/127/512/4096 after prepare256 | Bit identical |
| Prepared audio allocations | **0**, first callback and automated branch/filter/bypass/lookahead cases |
| pluginval | **5..10**, all exit0 |
| Steinberg validator | **537passed,0failed** |
| VST class identity | Matches archived original |
| UI | Existing Black Steel panel inspected; shared top-left PresetScreen unchanged |

Total host PDC remains5ms rounded to samples:240at48k. All six tested Legacy
automation schedules also produced identical PCM hashes across their five host
block sizes; no broader guarantee about untested automation is inferred.
Processor object size falls from844,152to2,176bytes. This is sizeof the C++
object, not a measurement of every heap allocation or process working set.

Original sources/configuration, binary and first laboratory are frozen under
out/s1/original. Initial fixtures are out/s1/reference; final matching-method
fixtures are out/s1/reference_v3. Primary229PCM/state/latency fixtures match the
original v1capture exactly, including after the method extension below.

## Disclosed measurement corrections

V1 assumed that loading a saved state would return exactly the same normalized
parameter readings as before saving. It flagged191cases. The archived original
M0 executable reproduced the same191failures against its own reference. Actual
serialized state bytes remained unchanged; the differences were normalized
parameter readback from the existing float/range mapping. That historical behavior
is preserved, not silently repaired by isolation. It is not counted as a new
state-quality success or hidden as a tolerance adjustment.

The paired method was extended to capture the original M0's actual restored
state bytes, parameter readings, selected-program metadata, latency and audio
on fresh and edited instances. The isolated processor must match those exact
observations. All comparisons remain exact; there is no widened tolerance.

The first v2verification then had a harness bug: File::loadFileAsData appends to
its MemoryBlock, and the load target was already populated. Its doubled size
falsely failed the initial input-state comparison in every case, despite matching
restored state/audio/metadata. Clear the load buffer before reading. Final v3
repeats both frozen M0 and isolated runs with the same final driver and method
fingerprint. No audio algorithm changed during either measurement correction.
V1/v2reports, failed candidate, original self-check and diagnostic trace remain.

These discoveries inform V2 state tests: new modes need stable settings/audio
restoration, while Legacy must keep its original observable behavior. Legacy
compact state still omits selected-program metadata; that also remains intact.

Terminology clarification to the frozen method disclosure: its initial phrase
"serialization-idempotent" described the combined bytes-plus-normalized-readback
assertion. Final measurements show0changed serialized states and191normalized
readback differences. The compatibility comparison itself remains exact.

## Evidence and reproduction

Authoritative files:acceptance.json,before_v3.json,capture_v3.json,after_v3.json,
native.txt,validators_final/,panel.png. Earlier candidate_01,after,v2,initial
capture and m0_self_check are retained historical results, not final acceptance.
Tools/verify_zastava_s1.py audits fixtures, matching method/source identities,
source closure, contract, class IDs, native results and actual validated DLL.

```powershell
cmake --build build --config Release --target HamloZastava_CompatLab HamloZastava_Tests HamloZastava_Snapshot HamloZastava_VST3 -- /m:2 /v:minimal
build/Release/HamloZastava_CompatLab.exe verify E:/HamloZastava/out/s1/reference_v3 E:/HamloZastava/out/s1/NEW_verify.json
build/Release/HamloZastava_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_zastava_s1.py audit
```

Frozen full M0 compilation is optional:configure
-DHAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON, then explicitly build
HamloZastava_M0CompatLab. It uses archived sources/headers and the same final lab;
normal production builds do not require the ignored archive. Do not overwrite
old raw directories or build while validators fingerprint a binary.

Validated DLL SHA256:
ef62bb5f012c8faacf50de92a9fe916828888dd92e6da18d95b3bc3b4522e738.
Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.

## Remaining V2 work

Define numeric task-specific targets before new sound changes, then use one
paired lab. Implement the four approved modes and soft/hard profiles, detector/
external routing and precise QUICK/ADVANCED controls,200genre recipes with
material/DSP/method-bound resumable verification, final VST validation and report.
Attack/quiet-note/tail quality, false triggers, bleed suppression and live FL
performance have not been measured as new V2 capabilities in this stage.
