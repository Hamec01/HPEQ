# F2: four-mode offline kernel and independent metering

2026-11-03; parent68f98ed. Trim, Match, Ride Natural/Fast and Motion now have a
prepared, separately tested kernel. It is **not connected to the VST yet**.
Shipping remains the unchanged F1/internal0.1.0. Host parameters/state/clock
handoff, UI,200recipes and release are F3/F4/F5. No installed copy, installer,
other product/HPEQ or push/subtree changed.

## Implemented

Trim: Input/master/Output and L/R gain, continuously faded routing matrix,
MonoSum/LeftBoth/RightBoth, swap, independent polarity and0–20ms fractional
alignment. Gain/enable fades20ms; routing/polarity10ms. Neutral and settled whole
bypass are exact dry. Mix0 is exact dry with neutral Output trim; Output follows
Mix. This is a gain tool with0samples latency and no peak limiter/coloration.

Independent input/output meters provide rectangular400ms/3s RMS and K-weighted
LUFS, session RMS, gated Integrated LUFS, sample peak and Annex2interpolated peak.
The integration has72000preallocated windows (about2hours), then freezes I with
an explicit full flag while moving windows/peaks continue. Reset uses validity
counts, avoiding a large audio-thread clear. Numerical overflow is fenced at
float's representable range; normal audio can exceed0dBFS intentionally.

Match: explicit Start/Stop learning gives a proposal; Stop does not Apply.
RMS includes pauses; LUFS uses complete gated blocks. Proposals respect boost/
cut and optional learned-signal headroom. A conflicting peak guard/range gives
an invalid limited result. The applied settings value stays fixed on later
source changes and survives reset. Silence and less than400ms are rejected.

Ride: linked input-power detection, adjustable range/strength/floor/hold,
Natural two-pole versus Fast one-pole gain movement, separate rise/fall90%times.
Idle returns smoothly to0dB and stops lifting low noise indefinitely. It measures
signal activity; it does not classify words or identify instruments.

Motion:2–32editable cyclic levels, Linear/Smoothstep, Sine/Triangle, dBdepth,
phase/right-phase, tempo/free clocks, seek handling and short edge smoothing.
Free periods round to the nearest sample, then repeat exactly. Stopped Sync
returns modulation to unity; Free keeps running. Mode transitions preserve the
independently applied L/R gains, including repeated changes.

## Numerical acceptance

| Check | Result |
|---|---|
| Same-method F1control |30passed/0failed;13new-feature categories n/a;10neutral rate/layout cases |
| Candidate matrix |281passed/0failed/0n-a;61cases including five-minute192kstereo stress |
| Additional unit probes |59passed/0failed: signed-zero/subnormal neutrality, learning/apply/reset, headroom conflict, idle continuity, meter readiness/reset/2hour capacity, Free/Sync, overflow, per-channel mode fades, first-sample transitions, exact6dBsteps and90%rise/fall times |
| Compatibility boundary |All F1shipping/include files, frozen methods and validated DLL unchanged;678accepted Legacy PCM files retained and hash-checked |
| Gain/channel/delay |All static gain errors<=0.001dB; routing/polarity and fractional reference<=1e-6; integer delay exact |
| Metering |RMS/session/rectangular and sample peak within0.001dB;LUFS M/S/I within0.1LU of independent SciPy processing with published coefficients and gates |
| Calibration |Mono1kHz at-20dBFS peak:-23.003604LUFS;duplicated stereo:+3.010303LU |
| TP |Independent Annex2convolution within0.001dB;test intersample peak3.092948dB above sample peak |
| TP comparison limit |Largest observed difference to a separate16xsinc estimator:0.032917dB;these are distinct interpolation methods |
| Match |All measured RMS/LUFS target and held-gain errors within0.1dB/LU;peak-limited proposal predicts-1.000000dBTP;invalid/silent/short cases change no gain |
| Ride |Settled targets within0.2dB;exact6dBstep unit probes reduce variation>=50%;harder7.9588dBalternations in the main lab reduce settled variation by>99.99% |
| Pause return |Natural0.030397dB/Fast0.000094dBremaining correction in the declared noise-pause window;idle asserted |
| Clock/events |PCM and readings exact at1/64/127/512/4096blocks for the same settings/BPM/seek/learn/reset/bypass event positions;Free repeats exact;seek phase error1.383e-10cycles |
| Allocation/stability |Zero measured C++ allocations in prepared callbacks/setters/actions;finite five-minute192kstress and invalid/extreme probes |
| Mean processing time |Worst48k/256:1.717423% of block duration;broader matrix max:5.386000%;limit25% |
| Provenance |4negative guards reject changed method/source/raw/manifest;valid analysis resumes byte-identically;5archived regressions detected |

Performance was measured on this Intel Core i5-10300H (4cores/8threads), using
wall time around the standalone kernel, including its meters. This is not a
FL Studio instance benchmark or a maximum-callback deadline guarantee.
The oracle uses independently generated synthetic signals and SciPy filtering/
convolution/gating; agreement is a defined numerical test, not a claim of full
EBU Mode/certification. LRA and official EBU audio test sequences are outside
this scoped implementation. No official EBU audio was downloaded or redistributed.
Primary references are linked in HAMLOFADER_V2.md; the ITU PDF was read locally
for its published filter tables, stored only in ignored out/f2/references.

## Recorded iterations and measurement boundaries

Initial targets and the whole F1source/bundle were archived before measurement.
Two signal-contract clarifications occurred before the first control run:
enable/bypass blends after Output, and Mix0 requires neutral Output for exact
dry. The audit reconstructs that measured target version and verifies its hash
against the first compiled method stamp. These were not post-run relaxations.

First candidate:278passed/3failed, all exact Motion cycle repeats. Rounding one
48bitphase increment per sample accumulated a small phase error. Integer free
cycle positions fixed exact repetition, within the existing one-sample period
goal. Initial Motion primes to its intended curve value; idle return starts at
the currently applied gain; meter reset uses validity rather than bulk clearing.
The first reports/raw files remain. The first candidate binary/source version
was not separately frozen; the retained raw/report identity is the historical
regression evidence for this iteration.

The main v2matrix passed281/281, but a stronger per-channel transition unit probe
then passed44and failed1. It found a shared left-channel start gain when leaving
stereo Motion with a right-phase offset. Independent L/Rstart values fixed that
actual discontinuity. The pre-fix v2kernel source and lab/unit binaries are
archived; rerunning that probe still detects its failure. Final59unit checks
include that probe and additional contract tests. The cancellation probe subsequently passed58and failed1: bypass stopped Learn
but left a valid unfinished fragment for Stop. Cancellation now clears that
measurement and proposal. The pre-fix v3source and probe binary remain at
out/f2/accepted; its failure is reproduced by the audit. Both main control/candidate
were recaptured with the final matching method after the disclosures. No audio
oracle, tolerance or numerical goal was weakened.

F1's exhaustive historical compatibility proof is retained, not replaced by the
small F2neutral control. F2does not test a new VST wrapper, APVTS schema or GUI;
those do not exist yet. Learning/Ride act on trimmed pre-role input; later
manual/channel/routing/Output changes can alter output level/headroom. Peak
proposal protection is not an output limiter. The public kernel/actions/readings
are single-audio-thread APIs; F3must add explicit host/UI synchronization.

## Evidence and next stage

Authoritative:acceptance.json,before_v4.json,after_v4.json,
unit_v4.txt,guards_v4.json,frozen_tools_v4.json. Keep baseline.json,
before/after_initial, before/after_v2 and all unit probe logs. Raw inputs/outputs,
method pins and manifests:out/f2/before_v4 and out/f2/after_v4.
Accepted sources/tools:out/f2/accepted_v4. Regression kernel/binaries:
out/f2/candidate_v2. Corrupt-reference copies:out/f2/guards_v4.

Kernel source ID:`09214975ed4bf07732709b4ba3e961a5c04446a70959bb2cf8daa1efcab10a7e`.
Final method ID:`b0024df55e43b3a5e3810342f91610a32cd09d4bb1fe1044431f39b4e6cc2f34`.
F1VST DLL remains
`c8489ef44ed8ebbaa7fdf44c284f7e5514876cce01a917dbab5e770f0ed02841`.

Next F3:append parameters after the original8;preserve programs0..7;connect
sample-positioned clock/actions and prepared buffers;serialize settings/fixed
Match gain/selected preset;old-state load explicitly restores Legacy and clears
new fields;prove fresh/prepared restoration, bypass and actual plugin output.
Then F4QUICK/ADVANCED/Legacy and F5library208programs/calibration/release.
Owner FL listening remains pending.
