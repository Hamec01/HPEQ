# Current HamloRack integration

HamloRack **0.2.2**, 2026-11-24: phase-consistent layer/region mixing and a
new measured calibration of all 250 task and 20 Rack Power presets. The current
release checks and artifact identity are recorded in
[CURRENT_VERIFICATION.json](CURRENT_VERIFICATION.json). Historical 0.2.0 and
0.2.1 receipts retain their own binary identities and are not current results.

## Audio behavior

Parallel layers now share a common crossover all-pass reference. Serial dry/wet
mixing, unused L/R or M/S components and wet-only subtraction use the matching
phase response. Each signal has independent filter history and replays the
same coefficient clock. Disabling regions/layers retires phase at the sample
clock, including when another EQ keeps a layer active. No additional lookahead
or linear-phase latency is introduced.

An active crossover at zero wet mix still has its all-pass phase response;
settled global bypass retains the original delay-aligned dry signal. External
parallel processing must account for this phase response. The owner authorized
new behavior for existing projects: old sound can change. Host parameter IDs
and the project-state schema are unchanged. See
[PHASE_LEVEL_METHOD.md](PHASE_LEVEL_METHOD.md) for equations, failed candidates,
regression contracts and measurement provenance.

## Preset bank

All 250 task presets were measured on two suitable local examples per category,
with explicit shaker/clap material for HR019/HR059, at multiple levels and tempos.
All pass their numerical safety gates and all 142 compressor tuning targets pass.
Canonical loudness is within +/-0.5 LU for 226/250. The remaining 23 insert
exceptions and HR250 wet-only return remain explicitly documented. Artistic
settings were retained; measured thresholds/trims were adjusted, and final
Master ceilings were authored at -1.05 dB before calibration to preserve the
independent <= -1 dBTP gate.

The twenty Rack Power demonstrations retain all authored effect parameters and
topology; only output trim changed. All pass 160 source/level/tempo and 20
silent-tail checks; 18/20 meet +/-0.5 LU. PW011/PW019 remain headroom exceptions.
Worst independent 8x true peak is -0.34999924 dBTP and worst final-second tail
is -100.21424 dBFS.

Each preset has an engineering decision, individual metrics and a matched A/B
pair with common headroom attenuation. These are numerical results; musical
approval, owner listening and an actual FL Studio session remain pending.

- [Individual task decisions](PRESET_REVIEW.md)
- [Task metrics and exceptions](RACK_BANK_REPORT.md)
- [Rack Power chains and metrics](RACK_POWER.md)
- [Current stage report](PHASE_LEVEL_REPORT.md)

Calibration format 3 requires the compiled normalized DSP source identity.
Aggregation authenticates every original record and requires exact canonical
replays through the final DSP. Static calibration records keep their original
calibrator hashes; later verification has a separate executable identity.
Private source audio and rendered WAVs remain outside Git.

## Existing integration

All ten pinned native plug-ins retain their original processors and editors,
including sample/workbench features and native state. Native editor gestures
feed the rack project state, undo, host save and runtime bindings. The searchable
270-preset library, arrows, filters, favourites and user tags remain available.
Native payloads were not changed in this stage. Previous integration details
and verification are archived in `INTEGRATION_0.2.1_NOTES.md`,
`INTEGRATION_0.2.1_VERIFICATION.json` and `INTEGRATION_0.2.0_VERIFICATION.json`.

Automation, external sidechain, final-output controls and rack sample-asset
packaging are separate work. This stage does not certify them as completed.

Build using `build_vst.bat`. It configures the current checkout safely, preserves
a relocated stale build, and writes `build/HPEQ_artefacts/Release/VST3/HamloRack.vst3`
and `batvst/HamloRack.vst3`. Production LTO stays enabled. A normal build does not
replace the installed DAW bundle. The standalone build menu now builds each
product's Assets target before its VST3 target to prevent a BinaryData race.
