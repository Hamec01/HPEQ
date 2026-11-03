# HamloFader V2: approved modes and numerical contracts

2026-11-03; F1 parent68f98ed. Owner approved Trim/Match/Ride/Motion and separate
Natural/Fast Ride profiles with adjustable correction range. F2 implements an
offline, independently tested kernel. F3 connects it to the host; F4 adds UI;
F5 adds200recipes/208programs and a validated release. F2does not change the VST.
Work alone; no other products/HPEQ, installer, installed copy or push/subtree.

## Signal and lifecycle

Input trim -> role gain -> channel routing/trim/polarity -> alignment -> Mix ->
Output trim -> enable/bypass blend. Input/output meters bracket the whole path. Learn/Ride analyse the
trimmed pre-role input. Mono is supported; stereo analysis is linked. All audio
buffers, loudness windows and integration storage allocate in prepare only.
Host latency0samples: alignment delays audio deliberately; no negative advance.

Settings apply at sample-positioned events. Fresh prepare/reset primes controls
to current settings without a startup gain jump. Later gain/Mix/enable changes
crossfade over20ms, routing/polarity over10ms and delay over20ms. Enabled=false
or host bypass reaches exact dry after20ms; clocks and meters continue. Learn
is cancelled while bypassed/disabled. There is no peak limiter or coloration.
Nonfinite settings fall back to documented defaults; nonfinite input becomes0.
An overflow fence at float's representable maximum prevents invalid output.

F2 kernels are single-audio-thread objects. Readings/actions are synchronous;
F3 must provide atomic host/UI handoff and serialize persistent settings. Reset
clears measurement/history/riding/phase and unfinished learning, preserving the
configured fixed settings including applied Match gain. Full history is not a
project state. Old-state load must select Legacy and reset all new controls.
All8original IDs/ranges/order and programs0..7stay intact; new host IDs append.
Modern default Trim has neutral gains/delays/routing and no effect.

## Controls fixed for the kernel

| Area | Values / defaults |
|---|---|
| Common | enabled=true,bypass=false; input/gain/output-60..+24dB,default0;Mix0..1,default1 |
| Channels | L/Rtrim-24..+24dB,default0; Stereo/MonoSum/LeftBoth/RightBoth; swap=false; polarity bits0..3,default0 |
| Alignment | independent L/R0..20ms,default0; fractional linear interpolation; routing matrix fades continuously on repeated changes |
| Match | RMS or Integrated LUFS;target-40..0,default-18;maxBoost0..24,maxCut0..60,defaults12/24dB; optional proposal peak guard=true,ceiling-24..0,default-1dBTP;appliedGain-60..24,default0 |
| Learn | explicit Start/Stop/Propose; at least400ms and measured level>-60; insufficient/silent input gives invalid proposal and changes no gain; Apply is a separate settings update |
| Ride | Natural/Fast;target-40..0,default-18dBFS RMS;strength0..1,default1;maxBoost0..24,maxCut0..60,defaults6/12dB |
| Ride activity | floor-90..-20,default-55dBFS;3dB hysteresis;hold0..1000,default80ms;idle return1..2000,default100ms |
| Ride smoothing | rise/fall0.1..5000,defaults250/120ms to90%;Natural uses a two-pole gain response and50ms RMS detector;Fast uses one pole and10ms detector |
| Motion | Custom/Sine/Triangle;2..32uniform cyclic points,default16/all1;Linear/Smoothstep;depth0..90dB,default0;phase and Roffset0..1cycles,default0 |
| Motion clock | Sync/default or Free;cycle0.125..32beats,default1;free10..60000ms,default500;edge smoothing0.1..50ms,default1 |

Manual Gain remains available in every mode. Match applies a held gain, never
keeps chasing the source. RMS learning includes its pauses; integrated LUFS
uses the specified gates. Proposal peak guard is conditional on the learned
pre-role signal and current settings; subsequent input/routing/output changes
can exceed its ceiling. Report headroom-limited proposals honestly.

Ride adjusts mean level, not individual peaks or semantic speech. Active target
is strength*(target-detectedRMS), bounded by boost/cut. Idle target is0dB so noise
is not boosted indefinitely. The displayed correction is actual applied gain.

Motion levels0..1map to attenuation depth*(1-level), without overshoot. The
cyclic end joins the start. Sync uses host PPQ and BPM; seek resets phase at the
event sample, tempo changes preserve continuous PPQ phase. Missing PPQ falls
back to a sample clock at supplied BPM (120default). Stopped Sync fades motion
to unity; Free continues. Disabled processing keeps its clock advancing.
Fixed-point phase accumulation avoids block partition drift. A change of synced
cycle/phase reanchors at the next event sample; free period changes retain phase.

## Meter definitions

Mono/stereo only. RMS is channel-average power; LUFS sums the two K-weighted
channel powers. Rectangular400ms/3s M/S windows have explicit readiness flags.
Integrated LUFS uses complete400ms windows at100mssteps, absolute-70LUFS gate
then relative-10LU gate. Preallocated capacity is72000windows (about2hours);
once full, I freezes and reports full until Reset, while M/S/peaks continue.
Session RMS and peak holds reset explicitly. Sample peak and interpolated true
peak have separate names. TP uses the published4phase12tap-per-phase BS.1770
Annex2filter; finalization drains its history for peak measurement only.
No LRA or complete EBU Mode/certification claim is made in this scope.

Primary specifications: [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en)
and [EBU Tech3341](https://tech.ebu.ch/files/live/sites/tech/files/shared/tech/tech3341v4_0.pdf).
48kK coefficients follow ITU tables; other rates preserve that response through
bilinear remapping. Tests use independently generated signals and an offline
oracle; official EBU audio is not downloaded/redistributed or claimed tested.

## Acceptance, fixed before first F2measurement

| ID | Goal |
|---|---|
| K01 | Neutral and settled bypass exact dry; Mix0 exact dry with neutral Output trim; latency0; all supported rates8/44.1/48/96/192k |
| K02 | Static gain error<=0.001dB over-60..24; routing/polarity expected sample error<=1e-6; integer delay exact and fractional delay<=1e-6to independent linear reference |
| K03 | Gain/enable/routing transitions continuous: first changed sample advances at most1/rampLength of its commanded step; neutral endpoints exact after declared ramps |
| K04 | RMS vs offline rectangular/session power error<=0.001dB; LUFS M/S/I vs independent ITU oracle<=0.1LU;1kmono-20dBFS peak reads-23LUFS within0.1LU; duplicate stereo adds3.0103LU |
| K05 | TP vs independent Annex2convolution<=0.001dB; intersample-peak cases exceed sample peak; compare separately with high-resolution sinc oracle (report difference, no hidden equivalence) |
| K06 | Match on steady and varying test excerpts reaches chosen RMS/LUFS target within0.1dB/LU when range/peak guard permit; freeze unchanged on different later audio; silence/short learn produces no proposal |
| K07 | Peak-limited Match proposals respect measured predicted ceiling within0.02dB and set limited flag; limits always respected |
| K08 | Ride steady error<=0.2dB after settling; programmed6dBlevel alternations reduce long-term variation>=50%;correction always within configured range;Natural/Fast produce distinct measured early response |
| K09 | Following active signal, low-noise pause correction<=0.1dB within hold+5*idleReturn;silence stays0;idle flag asserted |
| K10 | Motion frequency/period and seek phase within1sample;custom full/floor levels within0.05dB after edge settling; no out-of-range interpolation;stopped Sync reaches unity,Free continues |
| K11 | Identical PCM and readings at blocks1/64/127/512/4096for identical sample-positioned events, including BPM/seek,settings,learn/reset and bypass;prepare256 |
| K12 | Zero measured C++ audio allocations, including setters/actions during prepared processing; finite five-minute192kstereo stress, invalid inputs/settings and extreme controls |
| K13 | 48k/256mean single-kernel wall time<=25% of audio block duration;report benchmark environment and per-mode results |
| K14 | Legacy F1production/include files,shared UI,frozen methods and validated VST DLL unchanged in F2; all678accepted raw outputs/hashes retained |

Any failed goal stays explicit. Measurement changes require disclosure and a
matching before/after recapture. The same F2driver/analyzer is used for both;
F1control returns n/a for new features. No new VST/UI/library release in F2.

### First-run disclosure

Initial candidate:278passed,3failed (Custom/Sine/Triangle exact cycle repeats).
The free clock rounded a48bitphase increment per sample, accumulating sub-sample
phase error. Free periods now round to the nearest sample (<=0.5sample period
error), with integer cycle position and a recomputed fixed phase, so repeated
cycles are exact. No tolerance or analyzer criterion changed. Also prime initial
Motion to its curve value, begin Ride idle return at its applied gain, and reset
meter windows through validity counts rather than clearing large buffers.
Initial reports/raw files remain. This disclosure changes the method stamp;
recapture the unchanged F1control and new candidate under the matching final
stamp. Peak-guard/range conflicts produce an invalid limited proposal instead
of silently violating either constraint.

The expanded transition unit probe then passed44checks and failed one: mode
switching from stereo Motion used a shared left-channel start gain. Store and
crossfade each channel's actual gain independently. The pre-fix v2sources and
lab/unit binaries are archived. Keep the failing unit log; repeat all current
tests and the unchanged control under the final matching method stamp.

The cancellation probe passed58checks and failed one: bypass stopped capture
but retained a proposal-capable unfinished measurement. Active Learn cancellation
now clears that measurement/proposal; later Stop cannot apply stale gain. Keep
the pre-fix source/binary and failing probe, then recapture the final pair.
