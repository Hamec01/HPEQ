# S3: timed paths and horizontal binaural prototypes

2026-11-02. Parent checkpoint2b7ff77. Offline only; retain the S1 production VST3
and S2 sources/evidence byte-for-byte. No subagents or other product edits.
Numeric targets below precede the first S3 audio/laboratory experiment.

## Design and coordinate contract

Azimuth is degrees:0front,+90left,180back,-90right, positive counterclockwise
viewed from above. A path has up to16nodes. Each node holds its angle, then
travels to the next node. Hold/travel use milliseconds or quarter-note beats.
Shortest, clockwise and counterclockwise arcs are explicit; a directed arc to
the same angle can make a full turn. Linear and smoothstep travel are separate.
Zero-length segments are bounded, and invalid/all-zero paths safely hold.
Playback/reset, tempo changes and path edits are explicit sample events. There
is no host/transport integration yet. Continuous phase must not depend on blocks.

Stereo rendering has independent source rotation/panning and channel balance.
Stereo Spread routes left/right as separate emitters about the center angle;
Point explicitly collapses input to Mid. Stereo center0/spread180 is identity.
The stereo pan law is constant power; its front/back projections coincide.
Binaural uses measured horizontal HRIRs, circular interpolation and a common
per-profile gain, retaining interaural level/time differences. No artificial
front/back cue is invented to guarantee perception. K712 is a listening
reference, not a personalized HRTF or headphone EQ. Vertical rendering is out
of this prototype. Filters, histories and transition buffers allocate in prepare.

Retain the upstream native256tap44.1/48k and512tap96k HRIRs in full, with zero
Data.Delay as verified from SOFA. No minimum-phase conversion or tail truncation.
The common dry reference delay is half the filter length:128/128/256samples.
This prototype alignment is not the shipping PDC decision. Static filter tests
compare convolution against the original undelayed source; dry tests align by D.

SADIE II v2-1 is pinned to https://zenodo.org/records/10886409 . Inspect its exact
LICENSE.txt, archive notices, SOFA coordinates, sampling rate and Data.Delay.
Prototype profiles D1,H3,H4 are audition alternatives, not ranked quality levels.
Download/check hashes and retain upstream attribution. Process horizontal data
with one common timing origin/gain per profile; record transformations and
tail/response loss. Do not copy a commercial implementation or bundle room/
headphone responses. Source/method/data identities reject stale measurements.

## Acceptance fixed before measurement

| ID | Criterion | Method |
|---|---|---|
| M01 | Disabled and neutral Stereo center0/spread180 are bit-identical to input delayed by D | Three rates44.1/48/96k, stereo and mono fixtures |
| M02 | Holds/travel/node changes and loop/reset events accurate within1sample; angular error<=1e-7degree | Independent analytic path oracle; free time, beats at90/140/174BPM, tempo change, directed full circles and zero durations |
| M03 | Same path/audio bytes for blocks1/64/127/512/4096 at identical events | Prepared256, include mode/profile/tempo/reset/enable/path changes; zero C++ audio callback/setter allocations |
| M04 | Pan/Balance and source spread retain the defined routing | Pure left/right input; speaker left/right separation>=100dB at+/-90; center power within0.01dB; antiphase stereo remains nonzero with spread, Point cancels it |
| M05 | Cardinal HRIR render matches independent convolution, relative RMS error<=-90dB; left source leads/has greater high-band ear energy than right ear; mirrored direction reverses ear ordering | Each profile; impulse/noise, grid points and an intermediate angle; ITD sign via cross correlation and high-band energy3-12kHz |
| M06 | Profile export retains>=99.9%energy after common onset trim and FIR length choice; RMS magnitude difference<=0.5dB at100Hz-16kHz where original response>-40dB of maximum | Source versus prepared filters, every exported angle/ear; retain ITD and common profile scaling; record actual worst errors |
| M07 | No hard transition spike; continuous sweep and profile/enable changes finite and bounded | Steady997Hz input; event residual versus frozen pre/post runs bounded by those signals' peak sums+0.01; adjacent interpolation boundary RMS step<=0.5dB for5degree grid; absolute raw peaks retained |
| M08 | Stress finite and histories bounded | Five-minute render with rapid modes/paths/profiles/tempo; invalid numbers and zero-time paths; no allocations after prepare |
| M09 | Mean processing<=25%of48k/256block period on i5-10300H | Three timed stereo-source binaural runs; report outliers and allocations separately |
| M10 | S1 production/S2 source identities and evidence unchanged; provenance guards reject corruption | Current115native/160M0proof, S2 acceptance hashes and unchanged production DLL; negative data/method/cache checks |
| M11 | Prepared listening demos<=-1dBTP(8x estimate) with one common gain per input group | Owner vocal/keys/drumbus/synth; front/left/back/right and moving paths for all three profiles; listening acceptance remains pending |

One laboratory driver/analyzer is used before and after. Capture an unimplemented
dry-pass baseline first and mark unimplemented features n/a. Bind source and
method to each compiled translation unit; never overwrite a run or failed result.
Any changed threshold/method after the first run must be disclosed here and in
the final report. Primary data/licence references above; own renderer/path code.

## Disclosures and exact prototype laws

Before the first lab run, replaced proposed H10 with H4: H10's regular horizontal
grid is10degrees rather than the5degrees required by this experiment. The
exporter rejected the missing5degree measurement instead of synthesizing data.
No acceptance threshold or audio experiment was changed by this data selection.
Final profiles are D1,H3,H4. All native taps and the embedded copyright remain.

The initial same-method dry/candidate pair passed141checks/4n-a and190/190.
A coverage audit then adds six isolated left/right emitter impulses and one
enable/disable tone case. Prove each emitter by independent convolution and
verify the exact20ms enable ramp against frozen wet/dry. Keep all60original
jobs and all original thresholds; no DSP change. Repeat both experiment roles
with the extended tool and compare original PCM bytes. No failed audio run has
been hidden. Initial and extended reports/sources remain separate.

Stereo Spread uses a constant-power2x2pan matrix, with unity identity at
center0/spread180. Point routes Mid with sqrt(2)constant-power pan gains.
Binaural Spread sends L/R to separate emitters at center+/-spread/2, each with
gain1/sqrt(2); Point sends Mid to one HRIR pair. Correlated emitters can sum
coherently; raw levels are never auto-normalized. Channel balance attenuates the
opposite ear, center being unity. Profile/enable/renderer/source/manual controls
ramp20ms. Clock positions are computed each sample; filter and speaker targets
are sampled every16samples and linearly interpolated over the next16. Path/reset
commands are exact at their sample; acoustic control follows this short filter
interpolation and the HRIR delay. Path geometry is horizontal only.
