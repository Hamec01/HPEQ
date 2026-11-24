# Current HamloRack integration

HamloRack **0.2.3**, 2026-11-24: sixteen assignable host macros, an external
HamloComp key and final output trim/monitoring. Release qualification and artifact
identity are in [CURRENT_VERIFICATION.json](CURRENT_VERIFICATION.json), with
[the human report](CONTROLS_REPORT.md) and [usage guide](CONTROLS_GUIDE.md).
Previous receipts retain their own binary identities as `INTEGRATION_0.2.0_*`,
`INTEGRATION_0.2.1_*` and `INTEGRATION_0.2.2_*`.

## Controls

The old Output Gain and Bypass IDs remain. Sixteen permanent macro IDs are
appended, followed by Final Trim: 19 host parameters total. Assignments belong to
their module, support multiple destinations and reverse physical ranges, and
survive movement/copying/undo/save. Removing an assignment retains its current
mapped target. Original editor knobs forward values and balanced host gestures.
Macros map exposed native module parameters; rack EQ, region and layer controls
are outside this mapping. A 20 ms ramp and 16-sample control clock are independent
of callback size; JUCE delivers VST3 automation at callback boundaries.

The optional mono/stereo Sidechain bus starts disabled. Each HamloComp can select
External SC, with all five models and SOURCE detector shaping. The rack aligns
the key to preceding declared latency; key audio never enters the program.
Disabled/unavailable bus means internal detection. An enabled silent key remains
external, including when a host removes a send but keeps the bus enabled.

Final Trim follows every effect, tube colour, existing Output Gain and Master
limiter. Final meters include it; CLIP reset retains integrated loudness. Positive
trim can exceed the limiter ceiling. No extra clipper/limiter was added. Settled
global bypass remains exact dry. Optional new features require state schema 3;
legacy schema 1/2 states load with absent new controls reset neutral. Rack Undo
covers project/assignment edits; host parameter values use DAW parameter undo.

## Audio behavior and presets

The phase behavior qualified in 0.2.2 remains: parallel layers share a crossover
all-pass reference; serial dry/wet, unused channel components and wet-only
subtraction use matching phase. Coefficient changes and retirement follow the
sample clock, with no additional lookahead. Active crossover mix 0 retains its
all-pass phase. External parallel routing needs phase as well as latency checks.
The owner authorized that earlier phase change for existing projects.

With new controls unused, 0.2.3 reproduces all 250 task and 20 Power canonical PCM
hashes exactly. Final embedding was independently replayed with the same result.
Calibration format 3 authenticates final normalized DSP source and every original
record. No preset settings or calibration entries changed in the controls stage.

Original task calibration covers 2000 renders, 250 numerical safety passes and
142 compressor targets. Canonical loudness meets +/-0.5 LU for 226/250: 23 insert
exceptions plus HR250 wet-only remain documented. Power retains 160 source/level/
tempo checks and 20 tails, with 18/20 loudness matches; PW011/PW019 retain their
headroom exceptions. Those variant measurements retain the 0.2.2 calibrator
identity. This stage proves canonical replay equivalence rather than claiming a
new measurement of every variant. Private audio remains outside Git.

- [Individual task decisions](PRESET_REVIEW.md)
- [Task metrics and exceptions](RACK_BANK_REPORT.md)
- [Rack Power chains and metrics](RACK_POWER.md)
- [Historical phase method](PHASE_LEVEL_METHOD.md)
- [Historical phase 0.2.2 report](PHASE_LEVEL_REPORT.md)

## Qualification and remaining work

Final Release LTO ON: 9245 assertions passed, 0 failed. Debug controls: 214/0 with
CRT malloc + new coverage while macro/trim change and a hot key is active.
Final phase matrix: 400/0. Real hosted VST3 macro gain: +6.00000030 dB, key
reduction: -15.09779625 dB, Final Trim: -6.00000049 dB; internal PCM unchanged from
archived 0.2.2. The same corrected ControlAudit2 executable ran on both binaries;
the original tool's host-envelope mistake and failed run are preserved/disclosed.
Steinberg extensive: 537/0; pluginval 9 with GUI: SUCCESS; independent host:
100/100 instances and 100/100 desktop editors. Thresholds were not relaxed.

All ten pinned native plug-ins retain original processors, editors and resources.
The generated isolated Comp copy adds separate external-key kernels and keeps
original internal kernels verbatim; `Plugins/` payloads were not edited.
The searchable 270-preset library, native sample/workbench features, favourites,
tags, user presets and module actions remain available.

Owner listening, individual musical-intent approval and actual FL Studio
sidechain/automation/save-reopen checks remain pending. Rack sample-asset
packaging is separate. No installed DAW bundle replacement, commit or push was
performed. Earlier uncommitted phase/level and build-relocation work is preserved.

Build using `build_vst.bat`. Output bundles are
`build/HPEQ_artefacts/Release/VST3/HamloRack.vst3` and `batvst/HamloRack.vst3`. Production LTO stays enabled. Every file of the final
build bundle, independently validated copy and public bundle matches. DLL SHA-256:
`0ef2edf2ef3bb35c4ce5389a863ddf48aef16e75b8fc15394842930ea9ea8023`.
