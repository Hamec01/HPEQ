# S2: four-role DSP laboratory

2026-11-02; parent894abf8. Goals recorded before new sound. Owner approved four
roles and separate Soft/Hard profiles. This stage implements an offline-owned
kernel, not host/UI/preset integration. Preserve the accepted production binary,
S1sources and evidence. No subagents, other products, installer, push/subtree.

## Defined behavior

Common: fixed rounded5ms audio PDC, optional0..5ms opening lookahead; no audio
allocation, all memory prepared. Main or independent external stereo/mono key,
HP/LP key filters, Peak/RMS detector, threshold/ratio/range/knee/hysteresis/hold,
stereo link, Mix/Output and bypass. Missing requested external key fails open,
with visible missing-key status rather than muting the source.

Clean selects gate or continuous downward expansion. Range limits attenuation.
Soft uses two cascaded gain poles; Hard uses one. Attack/Release mean90percent
response time, not full settling. Soft has slower early closing at the same
90percent time. Both are manual profiles, not an instrument classifier.

Drum uses fast/slow key-envelope contrast, absolute threshold, arming and minimum
retrigger separation. Each hit starts a finite Attack->Body->Tail envelope.
Body is a hold after attack. Tail reaches Range floor exactly at its end.
Soft uses quintic smoothstep, Hard linear finite segments. Common Clean release
does not add a second decay to this tail. No semantic bleed classification claim.

Duck inverts the gate target while key is active. Lookahead anticipates duck
onset, not early unducking. Rhythm has16..32level steps, free step milliseconds
or quarter-note beats, host seek/restart handling and tempo synchronization.
Synced Rhythm fails open on stopped transport. Patterns are not randomized.

Opening lookahead is a future maximum-gain window (minimum for Duck), followed
by remaining fixed-PDC gain delay. This advances opening/duck onset while keeping
ordinary closing/unducking on time. Rhythm uses no anticipatory lookahead.
Filters transition through two fixed-state prepared paths; no moving audio read.

## Acceptance before the first run

| ID | Goal |
|---|---|
| G01 | Neutral, bypass, Mix0 and missing external key equal5ms delayed dry byte-for-byte, initial preparation included, mono/stereo and rates8/44.1/48/96/192k |
| G02 | Clean gate open gain0dB, closed gain=-Range, expansion ratio/knee static curve error<=0.05dB after settling |
| G03 | Hard and Soft90percent attack/release timing within max(2samples,3percent); configured Hold within2samples, zero detector-release fixture |
| G04 | Lookahead5ms/Attack1ms loses<=0.1dB on an aligned attack impulse and improves versus0lookahead by>=15dB; floor/peak remain bounded |
| G05 | Drum annotated four strong+four quiet hits above configured threshold all detected exactly, with no extras; finite tail ends at configured Attack+Body+Tail within1sample; Soft retains more energy early in tail than Hard |
| G06 | External Duck reaches its configured range, main input alone does not trigger when external selected; missing key fails open |
| G07 | Rhythm16and32steps: center levels within0.05dB, free/tempo clocks and same-position tempo/seek/setting events block-identical; step boundary error<=1sample excluding declared envelope/PDC |
| G08 | Key HP/LP discrimination>=30dB difference in applied attenuation; linked stereo applies identical gain, unlinked responds independently |
| G09 | Same-event PCM identical for1/64/127/512/4096 after prepare256, including modes/profiles/filter/lookahead/bypass; zero C++ audio/setter allocations |
| G10 | Five-minute max-rate/parameter stress finite and bounded by input peak times Output gain; disabled/key-loss/seek/reset recover without stuck gain |
| G11 | Combined kernel mean48k/256 processing<=25percent of block on available i5; record max/cycles, do not claim host deadline safety |
| G12 | Same source/method/input-bound lab before and after, unavailable functions n/a; corruption/stale-build rejection; S1accepted production DLL remains byte-identical |

Use one frozen driver/analyzer. Initial API-compatible neutral stub represents
unavailable processing; mark corresponding goals n/a, do not pretend it passes.
Retain before source/executable/raw data. Exact raw float/stamp hashes bind all
cache files. Text identities normalizeCRLF. Any changes to methods/targets after
first measurement must be disclosed and receive a fresh matching before/after.
Owner-material auditions supplement synthetic timing/transfer tests; live FL
behavior, new parameter/project state, UI and200presets belong to later stages.

## Method revision before final paired acceptance

*First:49cases/200candidate checks omitted independent goals listed below;
final:66cases/275checks, identical new control/candidate method, unchanged goals.
The final source comments explicitly disclose this expansion. Comment-only
method stamps require a new pair too; v3 retains exact PCM from revision2.*

The initial49case method produced167passes/9n-a for the neutral control and
200passes for candidate01. It lacked independent Hold/knee/RMS/HP assertions,
tempo+seek phase expectations, recovery and worst-callback/cycle observations.
These are coverage additions to the existing goals, not relaxed targets.
The final method adds these checks and monotonic-release lookahead fixtures:
the latter expose a real full-window queue-capacity fault in candidate01.
Candidate02 reserves the insertion slot before expiring the oldest queue entry.
All initial sources, methods, executable, renders and reports remain archived.
Build a new neutral control and candidate with the identical revised driver and
analyzer; do not relabel initial measurements as matching the final method.
The control's old49PCM renders must remain byte-identical to the first control.

Hard/Soft Hold is measured from the first below-threshold sample to the first
gain change >1e-6, accounting for five-ms PDC. Finite hold count closes on the
last counted sample (480configured ->479measured at48k). Static knee probes use
-50/-40/-30dB key, threshold-40,ratio2,knee12: -10/-1.5/0dB expected reduction.
RMS uses a1kHz .05peak sine, threshold-20,ratio2: -9.0309dB gain. HP6000/LP200
must reject the1kHz key. Rhythm event7 at1.4s seeks+3beats, BPM174after1s;
at1.46/1.55s expected levels are .505/1 including -40dB range floor. Lookahead
must preserve the exact ordinary closing/unducking curve away from its turn.
Recovery tests exercise missing/restored key, bypass, stopped/restarted Rhythm
and reset with a fresh fixed-PDC fill. Stress alternates published extrema.
Wall/cycle readings include setter+processor, QueryThreadCycleTime overhead is
outside timed processing. Max wall fractions can include OS preemption and are
reported without changing the mean performance gate or claiming host safety.
