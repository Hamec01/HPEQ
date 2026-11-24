# HamloRack 0.2.3 controls report

2026-11-24. All acceptance checks completed on Release 0.2.3 with LTO enabled.
The previous 0.2.2 checkpoint and its evidence are archived separately.
No installation, commit or push was performed.

## Resulting behavior

Sixteen permanent host macros (`macro01`..`macro16`) can address embedded module
parameters, including multiple destinations and reversed physical ranges.
Assignments belong to the module and survive movement, copying, undo and saved
projects. Unassign/Clear retain the mapped physical targets. Native editor knobs
forward values and balanced gestures to their assigned host macro, including
when a destination or module is removed before gesture End.

HamloComp has a per-module External SC selector and an optional mono/stereo key
bus, disabled by default. All five models use the external detector without
mixing key audio into the program. Prepared delay lines align the key to the
program at each compressor; Side processing retains the full key and Dual Mono
uses the respective key channel. Original internal kernels remain verbatim in
the generated isolated copy. Pinned `Plugins/` payloads were not edited.

Final Trim follows effects, tube colour, the existing Output gain and Master
limiting. Final meters include it. The CLIP reset clears peak history while
retaining integrated loudness. Settled global bypass remains exact dry. New
features use state schema 3; legacy states clear absent new controls to defaults.

See [the usage guide](CONTROLS_GUIDE.md) and
[the contract and development disclosures](AUTOMATION_SIDECHAIN_OUTPUT.md).

## Measured checks

| Check | Result |
|---|---|
| Current-source raw canonical task bank | 250/250 PCM hashes exact to the qualified 0.2.2 calibration |
| Current-source raw canonical Power bank | 20/20 exact |
| Final embedded task/Power bank replay | 250 + 20 exact |
| Final phase matrix | 400 passed, 0 failed |
| Debug controls, native editor and allocation checks | 214 passed, 0 failed; CRT malloc + new covered |
| Corrected Debug plugin contract | 205 passed, 0 failed; rate/block/channel grid, 100k mutations, 100 editors |
| Additional Debug phase regression | 49 passed, 0 failed |
| Compressor makeup macro, same absolute-time events, blocks 1/64/127/512/4096 after prepare(256) | Sample-for-sample equal |
| External key timing after preceding slot / serial layer / pre-MIX region | Exact GR onset at key sample 8192 + latency 240 = sample 8432 |
| Hosted macro makeup gain | +6.00000030 dB; gate +6 +/-0.03 dB |
| Hosted external-key reduction | -15.09779625 dB; gate at least 6 dB |
| Hosted silent program plus key | Exact zero output |
| Hosted Final Trim | -6.00000049 dB; gate -6 +/-0.02 dB |
| Before/after internal detector PCM | Exact same SHA-256 with corrected ControlAudit2 |
| Steinberg 3.8.0 extensive validator | 537 passed, 0 failed |
| Independent VST3 host lifecycle | 100/100 instances and 100/100 desktop editors |
| pluginval strictness 9, GUI enabled, no exclusions | SUCCESS |
| Python bank/adapter checks | 16 passed; structural bank check passed |
| Final unfiltered Release suite | 9245 passed, 0 failed; all other build/validator/replay jobs finished first |

The allocation check changes macro/trim targets and supplies a hot external key
inside the run; only the actual audio callback is probed. Tests also cover all
five detector models, missing/disabled key fallback, Side/Dual Mono, saved audio
reopen, malformed XML, stale destinations, copy/fanout/removal, post-Master trim
with unchanged limiter GR, peak-only reset retaining loudness and exact bypass.

## Artifact and provenance

Qualified VST3: `batvst/HamloRack.vst3`. The independently validated copy is
`out/controls-20261124/qualified-artifact/HamloRack.vst3`; every bundle file matches
the build output and public copy. Machine-readable evidence is in
[CURRENT_VERIFICATION.json](CURRENT_VERIFICATION.json).
Windows FileVersion/ProductVersion and VST3 module metadata are all 0.2.3.
Release LTO remains ON. DLL SHA-256:
`0ef2edf2ef3bb35c4ce5389a863ddf48aef16e75b8fc15394842930ea9ea8023`.
Normalized DSP source SHA-256:
`1c5f3358f8331e4a6f820282b44b97ef9ce5b66a29d2ddf6d3bb5ef6fc2b2d3a`.

The 0.2.2 binary, verification and original calibration records are preserved.
Current calibration format 3 requires the final compiled source identity; all 250
original records are authenticated by separate exact current-source proofs.
The raw verifier is `5d8b1cf98006c6c6840c8de080952d1a36f815e63420f0192784a7d3da3567db`.
The final embedded verifier is
`bf39086a88aaebd145257b658df0936996ccb454a1e069118121d1518df2f9f1`.
Private audio and evidence remain under `out/controls-20261124/` and the preceding
phase-stage output folder. Original 2000 task checks and 160 Power checks plus 20
tails retain their original calibrator identity; this stage replays canonical
PCM with neutral new features. It does not claim a new rerender of those variants.

The initial ControlAudit1 edited JUCE's outer host XML instead of its IComponent
state and failed two checks. Its executable, source and records are preserved.
Corrected ControlAudit2 verifies module/feature state restoration before audio;
the same immutable v2 executable passed on both archived 0.2.2 and final 0.2.3.
Signals, durations, blocks and numerical thresholds were unchanged. The original
v1 default-graph hash is not evidence of compressor-state restoration.

## Practical limits

- Owner listening, individual musical-intent approval and an actual FL Studio
  routing/automation/save-reopen session remain pending.
- Existing loudness exceptions remain: 23 task inserts plus HR250 wet-only,
  and PW011/PW019 Power headroom exceptions. Preset settings were retained.
- Macros address exposed native module parameters. Rack EQ/region/layer controls
  are outside this mapping system. Macro ramps are 20 ms with a 16-sample control
  clock; VST3 events are delivered at JUCE audio callback boundaries.
- A disabled/unavailable key bus falls back internally. An enabled silent bus
  remains an external key, including when a DAW removes a send but keeps that bus on.
- Positive Final Trim can exceed the Master limiter ceiling. Monitoring and trim
  do not add another limiter or clipper. Rack sample-asset packaging is separate.
