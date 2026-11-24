# HamloRack 0.2.2 — phase and level stage

2026-11-24, base commit `9f8c637`, owner-approved changes to existing-project
sound. The current binary, source identity, executable hashes, test results and
local evidence paths are recorded in [INTEGRATION_0.2.2_VERIFICATION.json](INTEGRATION_0.2.2_VERIFICATION.json).
No extra lookahead was introduced. Native plug-in source payloads, host
parameter IDs and project-state schema remain unchanged.

## Measured outcome

| Check | Result |
|---|---|
| Neutral crossover phase/magnitude matrix before repair | 232/400 failures |
| Same instrument, final DSP | 400/400 pass; worst magnitude error 0.00000310632 dB |
| Mono-input L/R difference in that matrix | zero |
| Final Debug transition, retirement, wet-only, search and RT regression | 49 expectations, zero failures; CRT malloc + new covered |
| Task recipe source/level/tempo measurements | 2,000 renders, all 250 recipes pass their numerical safety gates |
| Task compressor tuning targets | 142/142 pass |
| Task canonical loudness within +/-0.5 LU | 226/250 |
| Rack Power source/level/tempo measurements | 160 checks + 20 silent tails, all 20 pass |
| Rack Power canonical loudness within +/-0.5 LU | 18/20 |
| Rack Power worst 8x true peak / final-second tail | -0.34999924 dBTP / -100.21424 dBFS |
| Final-DSP canonical proof before embedding | 250/250 task + 20/20 Power sample-exact |
| Final compiled embedded bank replay | 250/250 task + 20/20 Power sample-exact |
| Final VST3 Steinberg 3.8.0 extensive validation | 537 passed, zero failed |
| Final VST3 pluginval | strictness 9, SUCCESS; GUI tests included |
| Final VST3 independent host | all checks pass, 100/100 instances and 100/100 desktop editor cycles |
| Final quiet full Release suite | 9,031 expectations passed, zero failed |
| Structural Python / build-relocation checks | 15 tests / 6 checks, all pass |

The phase matrix covers 44.1/48/96/192 kHz, serial/parallel routing, all five
channel modes, five mix values and one/two regions. It tests neutral steady-state
frequency magnitude; transition/retune tests separately compare block sizes,
handover behavior and audio-thread allocation. This does not promise perfect
phase alignment with a separately routed external dry track: an active crossover
retains its all-pass phase, including at region/layer mix zero. Settled global
bypass retains the original aligned dry reference.

## Changes and remaining decisions

The repaired sum uses a common crossover phase reference across parallel layers,
matching dry references in serial chains, and matching phase for unused L/R or
M/S components. Wet-only subtraction uses that reference too. Each signal has
independent filter state but the same coefficient schedule. Disable/mode changes
and filter retirement follow the sample clock; a retired region cannot replay
the previous block's phase program. This repairs the rack's crossover references;
each native effect retains its own phase and timing behavior.

Every task recipe received its own measured threshold/trim calibration and eight
stress renders. Artistic parameters were retained. Sixteen final Master ceilings
were authored at -1.05 dB before measuring, preserving the independent <= -1 dBTP
gate. Rack Power changed output trim only. All gates retain their original
thresholds. Peak-growth safety is a relative task-preset gate; this is not a claim
that all task outputs have a universal mastering ceiling.
Compressor target counts describe the threshold-tuning pass, before final Master
input trim. Actual final Comp/RedWall gain reduction is reported separately for
each render in the bank receipt.

Twenty-three task insert presets remain below the +/-0.5 LU target because more
gain would exceed their peak-growth gate. HR250 is a deliberately wet-only return
and is not an insert loudness match. PW011 and PW019 are Power headroom exceptions.
Each task has its own retained purpose, decision and listening focus in
[PRESET_REVIEW.md](PRESET_REVIEW.md); detailed metrics and rejection conditions
are in [RACK_BANK_REPORT.md](RACK_BANK_REPORT.md) and [RACK_POWER.md](RACK_POWER.md).

The material is a new local dataset with 36 WAV fixtures and two source examples
per category, plus explicit HR019 shaker and HR059 clap pairs. It differs from
the unavailable old-computer dataset, so old versus new match counts are not an
apples-to-apples improvement metric. All 270 loudness-matched A/B pairs have common
attenuation for headroom and recorded file hashes. Source audio and rendered WAVs
remain private under `out/phase-level-20261124/`.

## Evidence and practical limits

The initial Newton calibration, failed transition candidates, superseded M7
reference equations, stale retirement program and Windows resource-version
dependency are documented in [PHASE_LEVEL_METHOD.md](PHASE_LEVEL_METHOD.md).
Steinberg also exposed an empty name for advertised host program 0; the name is
now `Default`, covered by a contract regression. The conservative source identity
was renewed and all canonical proofs repeated after that metadata-only fix.
Original records retain their actual calibrator identity. Separate canonical
proofs authenticate each record and demand identical input/output PCM from the
final DSP before calibration format 3 can be embedded. The runtime rejects a
factory calibration whose normalized source identity differs from the build.

The first full Release run under concurrent work passed 9,013 expectations and
failed two CPU timing gates. Those failures were not waived; the final quiet run
passed 9,031 expectations with zero failures. An early final attempt was stopped
to update the browser test's obsolete recipe hash for the authorized Master
ceiling changes, then the entire suite was rebuilt and rerun. Actual test
executable, log and VST3 validator identities are in the current receipt.
Release allocation coverage sees operator new; the Debug regression also hooks
malloc. Independent host/validator checks do not replace a real FL Studio session.

The unchanged CPU regression gates pass at 48 kHz / 64 samples: Revolution
p99 7.4% (one) / 66.1% (eight), limits 25% / 100%; TubeColour p99 11.4%, limit
25%. The eight-instance maximum was 139.1% of the block deadline. These are the
existing MonstaProcessor legacy-reference and TubeColour DSP benchmarks, not a
measurement of eight complete native racks; passing a p99 gate does not certify
that every callback meets its deadline. Actual DAW performance remains an owner
host check.

Owner musical listening remains pending for every preset. Measurements do not
certify musical intent, genre suitability or taste. Automation, external
sidechain, final-output controls and sample-asset packaging are subsequent work.
The stage does not install, commit or push the changes.
