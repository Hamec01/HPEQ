# F3: host integration accepted

2026-11-03. Parent 7af28d0. Work only in E:/HamloFader; no installed-copy
replacement, installer, push/subtree or other product changes. Internal version
remains 0.1.0. This accepts host integration, not the finished 0.2.0 product.

## Result

Trim, Match, Ride Natural/Fast and Motion now run in the real AudioProcessor.
The production target contains 12 owned translation units (F1 had 9), adding the
kernel, meter and host control mapping. There is no LayerEngine or other effect.
68 new parameters append to the original 8; original IDs, names, ranges, defaults,
choices, order, XML root and the two VST3 class IDs remain. Native parameter count
is 76; the VST3 validator also sees the wrapper's program parameter (77 total).

A new instance is neutral modern Trim. All original programs 0..7 still select
Legacy and retain their old recipes; the two old Match programs set Target only.
Selecting Default explicitly therefore selects Legacy. There are still 8 programs;
the 200 new recipes and their host indices belong to F5.

Modern projects store parameters, applied Match gain, selected program and page
in schema 3. History/learning are transient. Old unversioned projects clear all
68 new controls and page, select Legacy, retain the historical current-program
behavior and preserve the live Legacy audio history. Modern restore resets its
own transient history; no control thread touches either kernel's mutable data.
Malformed, incomplete, duplicate, nonfinite, out-of-range and future modern
states are rejected before mutation. Numeric validation is locale independent.

Both prepared branches receive input. Selecting Engine crossfades for 20 ms;
a state/preset restoration primes selection. The stable Legacy arithmetic remains
unchanged. Host PDC is 0; Legacy tail is 0 and modern tail reports the maximum
20 ms alignment effect. All audio buffers are prepared before callbacks.

Learn Start/Stop/Reset use a nonblocking, 64-entry command queue. Accepted frames
are monotonic processing positions, independent of host transport position;
immediate/late commands run at the next audio opportunity. Full, busy or
out-of-order requests return false. Callers must handle that result. Epochs
invalidate stale commands on state, preset and prepare changes. Atomic snapshots
publish readiness/full flags, readings, proposals and command progress. Apply
is a deliberate control-thread host parameter gesture; it refuses stale/pending,
learning, Legacy, disabled, muted or bypassed states. Reset retains applied gain.

## Measurements and acceptance

| Check | Result |
|---|---|
| Original-prefix/program/XML contract | Exact, paired control/current method |
| Legacy PCM | 226 cases × 3 actual observations = 678 exact renders |
| Old project over modified prepared modern instance | Exact original-control PCM |
| Host integration | 224 passed, 0 failed |
| Kernel laboratory after DSP fix | 281 passed, 0 failed, same F2 method |
| Kernel unit suite | 59 passed, 0 failed |
| Original native suite | 49 passed, 0 failed |
| Sample event/block schedule | Exact PCM at 1/64/127/512/4096, prepare256 |
| Layout/rates | Mono/stereo; 8/44.1/48/96/192 kHz probes |
| Host clock | BPM, PPQ, sample position, seek, stop, absent PPQ/samples/playhead |
| Learn interval inside oversized callbacks | Exactly [123,26001), 25878 samples |
| Callback/action allocation scopes | Zero measured C++ allocations |
| Concurrent snapshots/three command producers | Coherent, accepted commands conserved |
| Live AudioProcessor/control stress | Passed, zero measured audio allocations |
| Whole processor 48 kHz/256 mean, six configurations | Maximum 1.457655% of block duration, target 25% |
| pluginval | Levels 5,6,7,8,9,10 all exit0 |
| Steinberg validator | 537 passed, 0 failed |

The performance number is standalone wall time on the existing i5-10300H,
including both branches and atomic publication, not FL Studio CPU approval.
The six configurations cover Legacy, nonneutral Trim/Match, Natural/Fast Ride
and nonneutral Motion. Concurrent stress is not a ThreadSanitizer proof.
The F2 method rerun includes its five-minute 192 kHz stress and independent
RMS/LUFS/true-peak oracles; full EBU Mode/official certification is not claimed.

## Failure found and retained

Initial host tests had 44 failures: first-callback Motion smoothing after Reset
changed with callback/prepared chunk length, plus an incorrectly exact -12 dB
test assumption. The original preset actually stores -11.99999618530273 dB due
to its historical float normalization; acceptance now compares the original
stored float exactly. No audio tolerance was loosened. An intermediate fixture
change to read raw host floats did not solve the DSP fault.

The actual DSP bug was `first` remaining true for every sample of the first
process call, bypassing Motion edge smoothing for that entire call. Priming only
the first sample fixes it. F2 had not exposed this Reset/first-segment case;
its earlier acceptance is preserved as historical evidence, not treated as proof
against the newly found defect. The same final F2 laboratory was rerun and passes.

The strengthened final host method run with the saved pre-fix kernel yields
53 failures (171 passes); the corrected implementation yields 224/0. Intermediate
host_v1/v2/v3/v4/v5/v6 and earlier final reports remain. host_v1 used the Legacy
method label before a separate host-method stamp was added; it is not final
provenance. The final host fixture explicitly covers Fast Ride, LUFS Learn,
nonneutral CPU settings and exact command boundaries. No criterion was relaxed.

## Evidence and identity

Authoritative: host_final_v2.json/.txt, legacy_final.json/.txt,
kernel_after_v1.json, kernel_unit_v1.txt, native_final.txt,
regression_final_v2.json/.txt and validators_final/. Earlier files are retained
intermediate observations. acceptance.json pins reports, tools, local raw data
and frozen binaries. `Tools/verify_f3.py` replays provenance/acceptance;
`guards_v2.json` exercises stale source/method/raw/binary receipt rejection.

Source: `535ed71398e2ffcf87e700a5845cb464732481039aefa206878974e02a57e47c`.
Host method: `330e8a2688064b5678a6f80282b18d2a1e5310986858ebf4a32fdea1fe4db9b1`.
Kernel source: `db89bee1334fb56617984276aaba2c3d60f2066cafad4e200f58eac9c033f9b5`.
Kernel method: `b0024df55e43b3a5e3810342f91610a32cd09d4bb1fe1044431f39b4e6cc2f34` (unchanged F2 method).
Tested DLL: `8cf87da40406aa53ce9804264f6f15a1aa467d6e0bf2699ad565c5326237b46a`.

Control source/bundle and prefix lab: out/f3/baseline; raw Legacy control:
out/f3/legacy_reference. New DSP raw: out/f3/kernel_after_v1. Failed pre-fix
sources/tools: out/f3/candidate_v1 and candidate_v2. The earlier out/f3/accepted archive is retained before audit-tool line-ending normalization. Final sources/tools/bundle:
out/f3/accepted_v2. No archive was overwritten. Original F1/F2 whole-shipping guard
scripts intentionally describe older source boundaries; use the F3 audit now.

## Remaining work

F4 builds the modern QUICK/ADVANCED interface, mode controls, readable meters,
Learn/Stop/Apply/Reset actions, curve editor, help and modified-state display.
The old panel/preset layout remains, with meter reads switched to atomic status;
modern parameters are currently accessible through host automation/generic UI.
Shared PresetScreen/BlackSteel/HamloFaderPanel source was not changed.

F5 adds 200 meaningful recipes (208 total programs), owner-material calibration,
full new UI screenshots, A/B audio, owner FL Studio listening and release
packaging. No presets, listening, modern visual acceptance or final release is
claimed here. State serialization is a lifecycle operation, not sample-accurate
automation of an entire project; parameter events are tested at host callback
boundaries and commands at explicit processing sample positions.
