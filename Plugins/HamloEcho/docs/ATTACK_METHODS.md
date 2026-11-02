# Short-attack and pitched-onset acceptance v1

2026-10-30. Written before the new attack matrix is run and before changing prototype DSP. Preserve the candidate at 05a1445 and its executable `out/prototypes/final.exe`. The VST3 and frozen Legacy lab remain unchanged.

## Policy to investigate

Preserve the leading transient with pitched, band-limited resampling rather than adding unpitched dry audio. Split a detected input attack from the continuously resynthesised body using complementary source-domain weights. Schedule each attack on a common sample timeline, independently of granular-window phase. A stereo-linked detector and prepared event storage must not allocate. Body processing must not replay the extracted attack.

Candidate timing policy: unity forward remains an exact requested delay. Pitched forward has an effective delay of max(requested delay, 80 ms). The additional wait is displayed as effect accumulation, never host/dry PDC. The centre of a band-limited attack aligns to the scheduled timeline; FIR pre-ringing of up to 32/ratio output samples may precede that centre. Do not describe FIR support as an early musical echo. Test causal availability at the minimum requested delay too.

An 80 ms capture budget suggests shorter 40 ms body windows, centred source age, and a separately read short attack. The first 8 ms of the attack should be preserved fully, then the split fades back to body by 12 ms. The final implementation is selected by measurements, not by assuming this candidate works. Complex sustained material, false triggers and multiple/closely spaced attacks need separate checks.

## A01 matrix and independent reference

At 48 kHz test all 49 integer shifts -24..+24 and 16 onsets at samples 16000+239*k. Signals are: a 0.5 impulse; a 2 ms Hann-windowed 800 Hz pulse; and deterministic 6 ms decaying noise. Thus 2352 cases explore a full old 80 ms window and both cycles of the proposed shorter window. Render with requested delay 125 ms, then export a fixed -4 ms..+100 ms interval around the independently specified expected output onset. The detector must not redefine that expected timeline.

Use an offline, independently evaluated 96-tap Kaiser (beta 9) windowed-sinc resampler as the rate-aware reference. Cutoff is 0.45/max(1,ratio); normalise each fractional-phase kernel for DC. Source positions are ratio*t relative to the known signal onset. Source samples outside the finite input clip are zero. The reference does not import C++ interpolation coefficients, detector decisions, attack envelopes or output. Cache it by rate/ratio/input waveform only. It is deliberately a different kernel from the candidate's 64-tap Blackman reader.

Targets, each case: finite output; peak timing within one sample of the reference (unity compares directly with unchanged input); energy within +/-1 dB of the rate-aware reference; normalised waveform L2 error <=0.15 allowing only a +/-1 sample alignment for the comparison; extra output energy more than 5 ms beyond reference FIR support <=-60 dB relative to reference. Report every onset-phase result and failure. Never compare shifted impulse energy to raw unshifted energy as a flat-gain requirement: resampling and anti-alias filtering legitimately change its energy.

The first candidate is expected to fail. Preserve that failure as baseline. After DSP changes run exactly the same matrix/reference/thresholds. Any correction to test methodology must be disclosed, with the original baseline retained and rerun when applicable.

## A02 additional coverage

After A01, add 44.1/96 kHz and 1 ms/4 s delay boundary subsets, stereo-linked impulses, two/rapid attacks, and ring wrap. Retain the original pitch, reverse, CPU and block invariance tests. Test the original pitch FFT method unchanged; use the candidate's documented accumulation bound as before, and disclose any delay-policy change.

The matrix establishes preservation of isolated short attacks; it does not prove complete fragment-duration preservation, pitched Reverse, Freeze, sustained-onset transparency, whole-plugin allocation behaviour or all drum sounds. Audio examples and real snare/vocal material remain listening evidence, not synthetic ground truth.

## Method correction v2, after baseline v1 and before new DSP

First baseline: 48/2352 passed (the unity cases). The original reference aligned a declared clip-start sample. That is not an observable event in continuous audio: a pulse beginning with one exact zero has no marker distinguishing that zero from preceding silence. The intended musical invariant is the position of the physical attack peak. V2 therefore centres resampling on the independently measured maximum absolute input sample, keeping that source peak on the delay timeline. It does not use the candidate detector's chosen peak. A 6 ms peak-search lookahead is available inside the 80 ms capture budget; this is a deliberate timing policy, not automatic note tracking.

Also extend the raw post-onset interval from 100 ms to 300 ms. The original candidate can produce fragments later than 100 ms, so the shorter interval undercounted ghosts. V2 strengthens that coverage. All +/-1 sample, +/-1 dB, 0.15 L2 and -60 dB ghost thresholds remain unchanged. The same new raw matrix/reference runs on the original DSP before any new processing is written. Preserve before_v1.json and the original analyzer/method under out/attacks; use before_v2.json and after_v2.json for the actual before/after comparison. The reference skips mathematically zero support to reduce computation without changing its kernel.

## Method coverage correction v3 and candidate refinements

The v2 baseline still passed 48/2352. The first split candidate passed 2336/2352; it met energy and waveform limits, but 16 noise cases selected a different highest sample with the shorter Blackman kernel. The attack branch was upgraded to a 96-tap Kaiser reader (same mathematical response as the independently evaluated reference; no reference coefficients are imported). The body retains its cheaper Blackman reader. Candidate 2 passed 2352/2352.

The added A02 subset has 60 stereo 44.1/96 kHz boundary cases (shifts -24/-12/0/+7/+24, requested delay 1/4000 ms, three signals) and 15 opposite-polarity two-impulse cases at 8/16/30 ms spacing. Two issues appeared: a 96 kHz stretched pulse began before the attack reader was activated, and an 8 ms weaker second hit could be incorporated into the first fragment. Activate readers for the complete captured leading waveform, rearm after a real silent gap, and seal adjacent fragments. These are DSP fixes, not relaxed targets.

Four additional A02 ghost failures were test bookkeeping: the ghost interval incorrectly used a resampled inter-hit gap, even though the reference explicitly preserves each hit's musical time. Correct the ghost start to follow the last scheduled hit. Preserve the old edge report. Also extend pre-onset capture from 4 to 32 ms: peak-centred slow playback can legitimately expand a leading waveform before its original start, in addition to FIR pre-ringing. The original 4 ms crop could miss this. Read the origin from manifest onset+delay-start, so old raw runs remain analysable. All thresholds are unchanged. Rebuild the same harness against immutable 05a1445 DSP (`Tests/PrototypeReference`) and run baseline and candidate on the stronger 32 ms/300 ms window.

Candidate 3 adapts the body window: 80 ms where the requested/effective delay permits causal reads, otherwise 40 ms. The minimum pitched delay remains 80 ms. This restores the longer-window option without hiding additional delay or changing the attack targets. The original pitch/CPU/Reverse/block tests are rerun; tonal tradeoffs and full-plugin limits remain visible.

## CPU diagnostics

The first component run averaged 21.46%/44.13% for one/two renderers, with a two-renderer worst block of 692%. A mathematically identical empty-mask fast path reproduced all 2685 output hashes, but its normal-priority CPU run measured 21.74%/51.29% with worst blocks 551%/840%; the dual mean failed 50%. Preserve both runs. A two-second process-time sample found Chrome using about 22% of all eight logical CPUs plus other desktop/audio activity. This is evidence of concurrent load, not proof of the cause of an individual stall; no owner process is stopped.

Add GetThreadTimes user+kernel deltas outside the existing timed callback region as a diagnostic mean. Its per-block accounting is coarse, so do not interpret individual charged blocks as precise latency measurements. Keep original wall-clock mean/p99/worst as primary and keep failed runs. Repeat the diagnostic at normal and AboveNormal priority for this short-lived benchmark process only, disclosing the priority. This is an audio-thread-like scheduling experiment, not a substitution for eventual real-host CPU/deadline validation.

A stronger dense-input workload repeats a 6 ms decaying stereo noise hit every 8 ms, keeping multiple attack readers active. Initial CPU mean was 41.47%/84.15%, a real component failure. Exact prepared run-length metadata now identifies fully extracted body ranges in O(1); attack FIR work is bounded to its captured fragment, and all-one attack masks avoid redundant multiplies. The intermediate dense run still failed at 26.43%/51.15%; final normal-priority dense mean is 22.32%/46.14%. Preserve every run. Final old and new matrix hashes must agree, plus a new 12 s dense/ring-wrap comparison at 96 kHz with a 4 s reflection and note/tempo automation, exercising the 512-event history repeatedly. No target is relaxed and full-plugin E10 remains pending.
