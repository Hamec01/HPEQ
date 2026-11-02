# S1 — immutable M0 and isolated Shirota processor

Completed2026-10-31. This is the first architectural stage of the owner's spatial
V2 request, with AKG K712 as the listening reference. New width characters,
binaural profiles and programmable movement are not implemented in this stage.
No new installer or version release is claimed. Existing audio is preserved.

## What changed

The production library now compiles10Shirota-owned translation units. LayerEngine,
FxNodeProcessor, pooled foreign effects and their panels are excluded. Their
uncompiled historical source files remain in the checkout for provenance.
ShirotaSettings.h carries the original slot indices and lightweight parameter
image. The existing ShirotaCore implementation is byte-unchanged; the M6 stereo
law and core-switch logic are copied without changing arithmetic.

LegacyShirotaCore reproduces the original insert ramp, node delta, layer addition
and three independent conditional alignment histories. The outer plugin's dry/
wet alignment and bypass fade remain. Original state XML, parameters, programs,
latency, class IDs and shared preset-screen files are retained.

Processor object size: **844152 ->3032bytes** (this is `sizeof`, not total heap use).
VST3 DLL size: **12561408 ->10658816bytes**. The validated DLL SHA256 and full source
include closure are recorded in acceptance.json.

## Verification

Goals/method were recorded in docs/HAMLOSHIROTA_V2.md before capture. The laboratory
source was unchanged between capture, old-binary selfcheck and isolated runs.
Archived binaries, original source copies,160raw float renders and binary states
are in ignored out/s1. Capture refuses an existing reference directory.

| Goal | Result |
|---|---|
| S01 original contract | Exact match:12parameters including bypass,7programs including Default, XML root and VST3 class IDs |
| S02 audio | **160/160bit-exact** against M0; programs at44.1/48/96kHz, mono/stereo, boundaries, M6, silence/mono/antiphase/one-sided/impulse and scheduled automation |
| S03 state | Every binary state reserializes exactly and restores identical parameters on fresh and edited instances before prepare |
| S04 timing | All captured latency/tail event values identical; reported PDC110/120/240samples at44.1/48/96kHz |
| S05 isolation |10own production units; include closure has no LayerEngine/FxNodeProcessor/LayerSnapshot/project model or foreign DSP |
| S06 audio allocation | Zero C++ allocations during first and later prepared callbacks, blocks1..4096 and rapid SAFE/style/bypass/mix/filter changes |
| S07 native/validators | Original59expectations pass; extended suite **115/115**. Pluginval5..10 and Steinberg **537/537**, no failures |
| S08 UI | Original and isolated snapshots inspected; layout/preset screen retained |

Every static original program also gives identical results at blocks1/64/127/512/
4096 after prepare256. There is an inherited M0 exception under automation: the
bypass/mix event family produces4distinct results across those5block schedules.
Other tested event families produce1each. The isolated processor matches each
original schedule exactly. The new V2 modes must use sample-clocked transitions
and must not inherit this dependency. No compatibility threshold was relaxed.

Snapshots differ in447pixels in the meter-needle region (928,542)-(984,605). The
unchanged BlackSteel.cpp meter intentionally adds a time-dependent needle tremor.
This is the only pixel difference; the panel and shared screen source are
unchanged. No screenshot was claimed to be byte-identical.

## Width diagnostic for the next stage

This exploratory metric was introduced **after** S1 capture, using existing raw
renders only. It changes neither S1 methods nor acceptance. Reproducer:
`Python311 -I -X utf8 Tools/analyze_s1_width.py` (numpy). Full method and numbers
are in width_diagnostic.json: original48kHz stereo mixed-sine/noise fixture,
discard first100ms, M=(L+R)/2, S=(L-R)/2; ratio10log10(sum(S²)/sum(M²)).

| M0 setting | Side/Mid energy | Correlation |
|---|---:|---:|
| Width100%, SAFE on | -2.4132dB |0.27085|
| Width200%, SAFE on | -1.7399dB |0.19762|
| Width100%, SAFE off | -2.3875dB |0.26811|

On this signal, doubling the Width control increases Side/Mid by only0.6734dB
with SAFE on. This supports investigating the guard as a cause of restrained
width; it is not a universal listening result. The core's existing SAFE side-gain
intervention is confirmed by source inspection. Next-stage targets will separate
artistic width from explicit, visible mono protection.

## Reproduce and continue

Commands/logs: build/s1_lab_build_final.log,s1_capture.log,s1_selfcheck.log,
s1_isolated_build.log,s1_audit_build.log,s1_final_tests.log,s1_after.log and
s1_acceptance.log. Run `build/Release/HamloShirota_Lab.exe verify
E:/HamloShirota/out/s1/reference E:/HamloShirota/docs/reports/s1/after.json`, then
`Python311 -I -X utf8 Tools/verify_s1.py`. Validation raw output is in validators/.
The verifier audits references, original parameter contract, include closure,
unchanged DSP/panel/screen sources, class IDs and the validated VST3 hash.

The build reports an inherited CMAKE_GENERATOR_PLATFORM environment warning; all
targets succeed. No failed audio cases or changed test thresholds were hidden.
No listening audition, personalized HRTF calibration, headphone EQ, model switch,
Verb/Echo/HPEQ modification, subtree, push or subagent work was performed.

Next: define numeric STEREO/SAFE targets and prototype width/mono-to-stereo and
spatial characters, then timed motion and binaural-profile audition. Preserve the
immutable M0. S1 is complete; the requested V2 spatial machine remains in progress.
