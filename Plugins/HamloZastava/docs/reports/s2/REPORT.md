# S2: four-role offline gate kernel

2026-11-02. Parent: `894abf8` (accepted S1 isolation). Production remains
HamloZastava 0.1.0. This stage adds a separately built kernel and laboratory;
the new engine is **not connected to the VST or editor yet**.

## Result

Clean, Drum, Duck and Rhythm have separate behavior and Soft/Hard envelopes.
The kernel includes gate/expansion, knee, range, hysteresis, hold, Peak/RMS,
key HP/LP, stereo link, optional external key, 0–5 ms lookahead, Mix/Output and
fixed rounded 5 ms audio delay. Drum has finite Attack/Body/Tail and onset
retriggering. Rhythm has 16–32 editable levels with free/host tempo clocks.
Missing external key and stopped synced Rhythm pass delayed dry with status.
The kernel API's common attack/release fields describe rising/falling gain.
For the later Duck UI, map onset to falling gain and recovery to rising gain
explicitly; the audition recipe already uses 2 ms onset and 100 ms recovery.

The final identical-method pair has **66 cases**: neutral control **218 passed,
0 failed, 9 unavailable**; implemented kernel **275 passed, 0 failed**.
Every measured audio/setter call allocates zero C++ objects, including reset,
first call, filters, lookahead and event changes. All five block schedules
1/64/127/512/4096 produce identical PCM for the specified events after prepare256.

| Goal | Final observation |
|---|---|
| G01 Neutral/PDC | Exact delayed dry at 8/44.1/48/96/192 kHz, mono/stereo; 240 samples at 48 kHz |
| G02 Transfer | 0/−40/−10 dB points within 0.000002 dB; 12 dB knee center −1.500001 dB; RMS −9.022706 versus −9.030900 dB target (0.008194 dB error, limit 0.05) |
| G03 Timing | Soft attack/release 191/959 versus 192/960 samples; Hard 192/960; both Hold 479 measured samples, expected 479 |
| G04 Lookahead | Soft attack peak loss <0.000001 dB, improvement 35.918 dB; Hard loss 0.000082 dB, improvement 24.979 dB; closing/unducking curves exact with/without lookahead |
| G05 Drum | All eight annotated strong/quiet hits, no extra triggers; both finite tails reach floor at configured end; Soft retains more energy at quarter tail |
| G06 Duck/key loss | −40/0 dB external-key results, missing-key status and restoration pass |
| G07 Rhythm | 16/32 level errors <0.000001 dB; measured grid error 1 sample; independent tempo/seek gains .505000055 and 1, as expected |
| G08 Key/stereo | HP/LP fixtures each produce 40 dB discrimination; linked output channels exact, unlinked response differs by 40 dB |
| G09 Determinism | Four alternate schedules exact to block1; zero measured C++ allocations |
| G10 Stress/recovery | Five minutes at 192 kHz with parameter extrema, finite and bounded; key loss, bypass, transport and reset recover |
| G11 CPU | Worst mean at 48 kHz/256: 1.20825% of audio duration, limit 25%; worst mean over broader non-stress/non-block1 cases: 1.319%; 192 kHz stress mean 1.87305% |
| G12 Provenance | Five stale/corrupt experiments rejected; all 49 original control PCM files unchanged; all 132 control/candidate PCM files unchanged by final comment-only method revision; accepted S1 source files and production DLL retained |

Hardware: Intel Core i5-10300H, 4 cores/8 logical processors. The worst measured
48 kHz/256 wall fraction was 14.7281% (recovery2). The one-sample event test had
a 204.48% wall-time outlier with 14,976 thread cycles at that callback; this is
reported, not discarded. `after_v3.json` records mean/max wall, cycles at max
wall, median and maximum thread cycles for every case. These are offline
observations, not a claim of host deadline safety or measured FL Studio load.

## Disclosed method and implementation changes

First run: 49 cases, 167 control passes/9 unavailable, 200 candidate01 passes.
That method omitted independent Hold, knee, RMS, HP, tempo/seek phase, recovery,
worst callback and monotonic-release window checks. The 66-case expansion adds
coverage of the original goals. **No numerical target was loosened.**

The added window checks expose a real candidate01 bug. Insertion before expiry
could fill the monotonic queue and make its head equal its tail. Candidate02
reserves one more slot. Rebuilding archived candidate01 with the final driver
reproduces all four failures, with maximum PCM differences 0.0319876/0.0241326
(Clean Soft/Hard) and 0.00410262/0.00500800 (Duck Soft/Hard). Candidate02 has zero
difference in the same closing/unducking comparisons.

Revision2 captured matching controls/candidates after expanding the method.
Final v3 adds the required disclosure comments in driver/analyzer and italic
disclosure in the methods document. Their method stamp consequently changes:
both sides were freshly captured again, with all 66+66 PCM files byte-identical
to revision2. Initial and intermediate evidence is retained; it is not relabeled.

The build reports a nonfatal MSVC C4244 at `V2Gate.cpp` reset's `std::fill` of
float storage with integer literal zero; zero is exactly representable. The
archived neutral stub has the same warning. No warning suppression was added.

## Owner-material A/B renders

Eight renders (four roles × two profiles) use the owner's `vocal_lead1.wav`,
`drumbus1.wav`, `keys.wav`, `synth.wav`, and a separate `kick.wav` duck key.
Preparation resamples to 48 kHz, takes at most 12 seconds, scales to −6 dBTP
using 4× polyphase interpolation, and adds 250 ms to drain the audio delay.
Delayed dry and the prepared external key are included beside processed WAVs.
These demonstration recipes are not part of the eventual 200-preset library.

| Recipe | Soft / Hard low-level energy gain | Soft / Hard high-level energy gain |
|---|---:|---:|
| Clean | −1.49 / −2.07 dB | approximately 0 / 0 dB |
| Drum | −35.94 / −34.55 dB | approximately 0 / 0 dB |
| Duck | −1.21 / −1.13 dB | −1.22 / −1.18 dB |
| Rhythm | −4.15 / −4.19 dB | −5.40 / −5.44 dB |

The level groups are input-energy quartiles, **not annotated wanted/noise
labels**. The drum loop yields 289 trigger events for each profile; without
manual hit annotations this is not a detection accuracy score. Output sample
steps do not exceed the corresponding prepared dry's largest sample step in
these eight renders; this does not prove absence of all audible clicks.
All eight are finite, allocation-free, and peak from −7.80145 to −6.00000 dBTP.
Source/material/prepared-input/renderer/output hashes are in `auditions.json`.
An owner listening judgment has not been recorded.

WAVs: `E:/HamloZastava/out/s2/auditions_01/`. Example pairs:
`Clean_dry.wav` / `Clean_Soft.wav`, `Drum_dry.wav` / `Drum_Hard.wav`,
`Duck_dry.wav` / `Duck_Soft.wav`, `Rhythm_dry.wav` / `Rhythm_Soft.wav`.

## Evidence and continuation

Authoritative: `acceptance.json`, `before_v3.json`, `after_v3.json`,
`regression_v3.json`, `auditions.json`, and `docs/S2_METHODS.md`.
Final raw/executable pairs: `out/s2/before_v3`, `out/s2/after_v3` and sibling
`.exe` files. Regression: `out/s2/regression_v3`. Original neutral/candidate01
sources and v1 methods are archived under `out/s2`.

Kernel source ID:
`c08e4c9de2b82a7db9eeffaff95dd74d2b8584a596a1e03d442294086081a084`.
Final method ID:
`0567998ff0e306a19cf7a981d2902b05d304b5a3d954f926bc8406964dcb1ff9`.
Unchanged production DLL SHA256:
`ef62bb5f012c8faacf50de92a9fe916828888dd92e6da18d95b3bc3b4522e738`.

No new VST, parameter/state adapter, external host bus, key audition button,
QUICK/ADVANCED editor or genre library is claimed here. They remain required,
along with host bypass/PDC routing, Legacy/new state and program restoration,
mono-key host routing, UI inspection, final pluginval/Steinberg and FL audition.
The S1 validators apply to the unchanged Legacy binary only. There is no
installer, push/subtree or change to other products in S2. Next: `NEXT_STAGE.md`.
