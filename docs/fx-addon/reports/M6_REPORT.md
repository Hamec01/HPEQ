# M6 — Remaining processors, latency alignment, host tempo

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M6 — Saturator, Transient, Delay, Monsta FX, Stereo, Gate/Expander, Limiter; PDC |
| Date | 2026-10-22 |
| Base | `00c797f` on `feature/hpeq-m0-m1`; code and report in the M6 commit |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730, JUCE 8.0.12 |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/ProcessorsM6.*` | The seven new processors; every law documented in the header |
| `Source/DSP/Pool.h` | `SharedPool<Processor, N>`: layer pools for large memory (Reverb 8, Delay 4, Monsta 4) |
| `Source/DSP/DelayLine.h` | `AlignmentDelay`: integer delay for latency alignment |
| `Source/DSP/FxNodeProcessor.*` | All types in the chain; per-slot latency, dry-band alignment, Delay as wet-only send, pooled Monsta, transport |
| `Source/DSP/LayerEngine.*` | Layer latency = max node latency; dry path and node deltas aligned to it; transport to nodes |
| `Source/DSP/LayerSnapshot.*` | Parameter indices for the new types; state → parameter mapping with defaults and clamps |
| `Source/Plugin/FxPluginProcessor.*` | Host tempo / position from the play head; `setLatencySamples` from the project; dry copy (bypass, capture) delayed by the latency; Delay tails in `getTailLengthSeconds` |
| `Source/UI/NodeInspector.*`, `Theme.h` | Parameters of every type (choice parameters as combo boxes); no "(M6)" types left |
| `Tests/Unit/M6Tests.cpp` | 16 test groups, 126 expectations |
| `Tests/Unit/TestMain.cpp` | Optional filter: `HPEQ_Tests <name part>` runs only matching test classes |
| `Tests/Tools/LabM6.cpp` | `HPEQ_Lab m6`: spectra, tables, CPU |

## 3. Processors (laws as implemented)

| Type | Kind | Law (short; full text in `ProcessorsM6.h`) | Latency |
|---|---|---|---|
| Saturator | insert | `(f(g x + b) − f(b)) / n`, f = tanh / tube (asymmetric) / hard; auto gain n = g f′(b); 10 Hz DC blocker; trim | 1x 0, 2x 49, 4x 61 samples (measured) |
| Transient | insert | fast/slow peak envelopes, d = 20 log(fast/slow); gain = attack·clamp(d/6) + sustain·clamp(−d/6) | 0 |
| Stereo | insert | M/S width, linear balance | 0 |
| Gate / Expander | insert | peak detector (10 ms release); opens > T, closes < T − hysteresis after hold; attenuation min(range, (T − x)(ratio − 1)), smoothed | 0 |
| Limiter | insert | look-ahead L; sliding minimum of the target gain, release, L-sample moving average on audio delayed by L → output sample peak ≤ ceiling | L = look-ahead (0.1–10 ms) |
| Delay | wet-only send | 2 s lines; ms or tempo-synced (1/32 … 1/1, dotted, triplet); feedback with low cut + damping; ping-pong; width; 100 ms glide | 0 |
| Monsta FX | insert (pooled) | slices of a note division on the host timeline (or a free-running counter when stopped); per slice hash(seed, slice) → Pass/Repeat/Reverse/Stutter with probability `chance`; crush, 4-step gate, 64-sample fades | 0 |

**Latency (PDC).** A slot's latency counts while it is in the chain, even bypassed, so bypassing does not
change the reported latency. Node latency = sum of its slots; layer latency = max over nodes (enabled
or not). The dry path and the deltas of shorter nodes are delayed to the layer latency; the plugin
reports it with `setLatencySamples` whenever the project changes, and delays its dry copy (bypass
crossfade, analyser capture) by the same amount. Changing oversampling or look-ahead changes the
reported latency (hosts re-align; DAW behaviour not checked).

## 4. Commands and exit codes

```bash
cmake --build --preset release                     # 0, no warnings
build/Release/HPEQ_Tests.exe                       # 0 — 896 passed, 0 failed
build/Debug/HPEQ_Tests.exe                         # 0 — 896 passed, 0 failed (CRT hook: malloc + new)
build/Release/HPEQ_Lab.exe m6 --out docs/fx-addon/reports/m6   # 0
build/Release/HPEQ_HostSmoke.exe --plugin build/HPEQ_artefacts/Release/VST3/HPEQ.vst3 \
    --other C:/DRUMENGINE/build/HPDG_artefacts/Release/VST3/HPDG.vst3 --out docs/fx-addon/reports/m6/host_smoke.json   # 0 — 16/16
```

## 5. Numbers

| Test (threshold fixed before the run) | Result |
|---|---|
| T08 alias / fundamental, tanh 12 dB drive, 7.03 kHz at −6 dBFS (2x ≥ 12 dB, 4x ≥ 20 dB better than 1x) | 1x **−28.1 dB**, 2x **−65.8 dB**, 4x **−125.4 dB** |
| T08 same, hard clip (lab only, no threshold) | 1x −25.2, 2x −37.6, 4x −50.0 dB (hard clip has unbounded harmonics; 4x does not remove them) |
| Saturator small-signal gain with auto gain, 3 curves × 3 biases × 3 OS (±0.1 dB) | all within ±0.1 dB |
| Saturator latency: reported = measured (1x/2x/4x) | 0/0, 49/49, 61/61 |
| Saturator DC with bias 0.5 (|mean| < 1e−3) | pass, all curves |
| Transient 0/0 dB identity | bit-exact |
| Transient attack +12 dB: crest +≥ 3 dB | 14.61 → 19.14 dB |
| Transient sustain −12 dB: decay 150–250 ms after a hit lowered ≥ 6 dB | **12.00 dB** (see §9.1) |
| Stereo identity / width 0 / balance 0.5 | bit-exact / L == R / max error < 1e−6 |
| Gate range 0 identity; closed attenuation (range 40, ±0.5) | bit-exact; 40.00 dB |
| Gate chatter: tone wandering ±1.5 dB around T for 2 s (hysteresis 4 dB: ≤ 2 changes) | 1 change; hysteresis 0: 800 (4 s lab: 0 dB 1600, 1 dB 504, 2 dB 24, 4 dB 1) |
| Limiter: drums +18 dB (peak +15.6 dBFS), 3 ceilings × 3 releases × 3 look-aheads, peak ≤ ceiling·(1+1e−6) | **27/27**; worst 0.0000 dB over |
| Limiter block 1 vs 512 | bit-exact |
| Limiter below the ceiling = input delayed by L | exact (max error 0) for 5 / 96 / 480 samples |
| Delay sync: 1/4 @ 120 and 1/8 D @ 90 → 24000 samples; echoes at n·D ±2; 2nd/1st = feedback ±1 dB | pass |
| Delay ping-pong: 1st echo L only, 2nd R only (≥ 40 dB) | L 0.0 / R −400 dB; L −400 / R −6.7 dB |
| T07 Delay: after `tailSeconds()` echoes ≥ 60 dB below the 1st; no runaway at feedback 0.95 | 66.8 / 72.2 / 84.0 dB (fb 0.3 / 0.7 / 0.95) |
| Monsta: chance 0 identity; acted share at chance 0.35 (±0.02); seeds differ | bit-exact; 0.3495; differ |
| T10 Monsta, free-running and playing: blocks 64/127/512 (lab: 2048) vs block 1 | **bit-exact** |
| T10 engine with every M6 type (sat 2x, transient, gate, limiter, stereo, Monsta, ping-pong delay): blocks 1/64/127/512 | **bit-exact**, **0 allocations** |
| PDC engine: layer with a 2 ms limiter (idle) = zero-latency layer delayed by 96, second node (compressor) included (≤ −120 dB after 1 s) | **−400 dB** (exact) |
| PDC plugin: reported = 96 + 61 for [sat 4x, limiter 2 ms]; bypassed output = input delayed by it | 157; exact; back to 0 without the node |
| Pools: 5 delays + 5 Monstas in one layer | 4 + 4 run, 2 slots reported starved |

### CPU (512-sample stereo blocks at 48 kHz, 10 s pink noise; budget 10.67 ms)

| Processor / scenario | p50 µs | p99 µs | p99 of budget |
|---|---|---|---|
| Saturator 1x / 2x / 4x | 16 / 143 / 216 | 31 / 220 / 390 | 0.3 / 2.1 / 3.7 % |
| Transient / Stereo / Gate | 24 / 0.2 / 6 | 43 / 0.3 / 9 | 0.4 / 0.0 / 0.1 % |
| Limiter 2 ms / Delay / Monsta | 13 / 15 / 36 | 24 / 33 / 58 | 0.2 / 0.3 / 0.5 % |
| Layer: two mixed M6 nodes | 261 | 468 | 4.4 % |
| Layer: 8 nodes × 6 slots (sat 4x, transient, gate, compressor, stereo, limiter) | 2401 | 7438 | **69.7 %** |

Oversampling dominates: one 4x saturator costs as much as ~13 limiters. The extreme layer (8 × sat 4x)
would not fit 10 layers on this machine. Not optimised in M6 (measure first): candidates are a cheaper
half-band filter for 2x/4x and a rational tanh on the oversampled samples, measured before/after with
this lab.

Plots: `m6/plots/t08_saturator_spectra.svg`, `m6/plots/saturator_laws.svg`. Data: `m6/m6_lab.json`.
Audio: `m6/monsta_drums_seed3.wav`.

## 6. Artifacts

`reports/m6/m6_lab.json`, `reports/m6/plots/*.svg`, `reports/m6/monsta_drums_seed3.wav`,
`reports/m6/host_smoke.json`, `reports/m6_logs/` (unit Release/Debug, lab).

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| T07 (Delay) | **PASS** offline; DAW BLOCKED | §5 |
| T08 | **PASS** (tanh: 2x −37.7 dB, 4x −97.3 dB vs 1x); hard clip measured, documented | §5, plot |
| T10 (all types) | **PASS** (bit-exact, incl. host-timeline Monsta) | §5 |
| PDC | **PASS** offline (reported = effective delay, dry/wet aligned, bypass aligned); DAW BLOCKED | §5 |
| LIMIT (ceiling) | **PASS** (sample peak; true peak is M8) | §5 |
| Gate 6 | **PASS** — every processor has a documented law and its own test; 0 allocations; CPU measured | |

Debug run with the CRT allocation hook: 896/896, **0 audio-thread allocations** with every M6 type (`reports/m6_logs/unit_debug.log`).

## 8. Existing HPDG

Untouched (no file in `C:\DRUMENGINE` changed); HPDG.vst3 still scans next to HPEQ (16/16).

## 9. Limitations / risks / findings

1. **Test criterion changed after the first run (disclosed).** "Sustain −12 dB lowers whole-file RMS
   by ≥ 3 dB" failed with 0.97 dB: the drum fixture's RMS sits in the attacks, so the criterion did
   not measure the sustain law. Replaced by "the decay 150–250 ms after a hit is lowered by ≥ 6 dB"
   on decaying tone hits: 12.00 dB. The attack criterion was kept unchanged.
2. **JUCE latency figure is off by one for 4x.** `Oversampling::getLatencyInSamples()` reports 60 for
   two half-band stages; the measured delay is 61 (the test caught it). The latency table is now
   measured by correlation at prepare time, so PDC uses 61.
3. **Test helper semantics.** `compareAligned(...).bitExact` is defined as exact *with delay 0*; the first
   PDC/limiter tests used it with a delay and failed while the error was exactly 0. The tests now check
   max error 0.
4. **Limiter running sum.** The first version added doubles and removed float-rounded values; the drift
   let peaks exceed the ceiling by 1e−4 dB. Now both sides use the same doubles: worst 0.0000 dB.
5. **Silent test-process exit while the limiter kept its 2 × 2049-sample buffers inside the object**
   (~57 KB per slot, ~2.7 MB per engine): the run stopped at the M4 plugin PDC test without a message.
   After moving the buffers to heap memory sized in `prepare()` the full suite passes. Most likely a
   stack overflow from a temporary engine-sized object; the exact frame was not identified.
6. Gate level detector: the first version used the sample peak, which closes a hold-0 gate at every
   zero crossing of a tone; changed to a peak detector with 10 ms release (documented law) before the
   gate tests ran.
7. Delay time changes glide (pitch bend), by design; tempo changes while playing glide too.
8. Monsta follows the host timeline only while the transport plays; when stopped it uses its own
   counter, so the first slice after Play may be cut short.
9. Not checked in a DAW: tempo sync, latency changes during playback, automation. UI parameters for the
   new types exist but no mouse test was done by the agent.

## 10. Next

**M7** — 10 layers, Serial/Parallel layer routing, per-layer controls; then **M8** (validation,
pluginval, release checks). Owner UI feedback (BACKLOG F1–F3: modules on EQ points, module grid with
a back arrow) follows after M8 as agreed.
