# M2 — PRE-EQ + frequency FX node prototype, H1 vs H2

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M2 — DSP prototype of the frequency core (most important phase, STOP-GATE) |
| Date | 2026-10-21 |
| Measured at | working tree on top of `460d73a`; code commit listed in the commit that adds this report |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730, JUCE 8.0.12, 48 kHz unless stated |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/Biquad.*` | RBJ designs in double (peak, LP, HP, all-pass), TDF-II sections; 0 dB bell = exact identity |
| `Source/DSP/Ramp.h`, `DspConfig.h` | Exact-landing ramps; control clock of 32 samples counted on a running sample clock (block-size invariant) |
| `Source/DSP/LayerSnapshot.*` | Trivially-copyable layer image + lock-free triple buffer (message thread → audio thread) |
| `Source/DSP/PreEqProcessor.*` | PRE-EQ (M2 scope: Bell), glides of f/gain/Q, fade-in/out of bands |
| `Source/DSP/RegionSelector.*` | H1 selector: LR4 HP(low) · LR4 LP(high), gliding edges |
| `Source/DSP/Processors.*` | Gain (reference "mock") and a **new compressor** (documented detector/knee/ballistics, D3) |
| `Source/DSP/FxNodeProcessor.*` | Node with a chain of up to 6 slots (D2), delta to the layer |
| `Source/DSP/LayerEngine.*` | Layer: PRE-EQ → nodes → `y = xE + Σ mix·Δ`, chunking, no allocation |
| `Source/Plugin/FxPluginProcessor.*` | Layer 1 runs through the engine; bypass now a 20 ms crossfade to an exact dry copy |
| `Source/Core/FxProjectState.*` | `FxType::Gain` (utility/reference processor) |
| `Tests/Unit/DspTests.cpp` | T01, T02, MIX-0, T04, T10, D2 chain, delta math, smoothness, compressor law, decision B |
| `Tests/Tools/LabMain.cpp`, `Tests/Harness/SvgPlot.*` | `HPEQ_Lab`: H1 / H1-LR8 / H2 / H3 on the same signals, JSON + SVG + WAV |
| `Tests/Harness/AudioMetrics.*`, `Tests/Tools/HpdgBlockAudit.cpp` | Corrected HF metric (see §9.1) |

## 3. Implemented vs scaffolding

**Real and running in the plugin (layer 1):** PRE-EQ Bell bands, FX nodes with H1 selectors, chains of
Gain and Compressor slots, node/slot mix and on/off fades, crossfaded bypass.
**Not processed (carried in state, inactive in the snapshot):** other EQ shapes (M3), other FX types
(M4/M6), channel modes other than Stereo, node Solo, layers 2–10. No UI to create nodes yet (M5);
nodes come from saved state / tests.

## 4. Commands and exit codes

```bash
cmake --build build --config Release                       # 0, no warnings
cmake --build build --config Debug --target HPEQ_Tests     # 0
build/Release/HPEQ_Tests.exe   # 0 — 529 passed, 0 failed
build/Debug/HPEQ_Tests.exe     # 0 — 529 passed, 0 failed (CRT hook: malloc + new)
build/Release/HPEQ_Lab.exe --out docs/fx-addon/reports/m2 --commit <sha>   # 0
build/Release/HPEQ_HostSmoke.exe --plugin …/HPEQ.vst3 --other …/HPDG.vst3  # 0 — 16/16
build/Release/HPEQ_Harness.exe baseline --out docs/fx-addon/reports/m2/hpdg_remeasure  # 0
```

## 5. Numbers

### Engine invariants (unit tests, thresholds fixed in the test source before running)

| Test | Result |
|---|---|
| T01: 8 "no audible change" configurations × 44.1/48/96/192 k × blocks 1/64/512 (empty layer, node disabled, node mix 0, all slots bypassed, slot mix 0, Gain 0 dB, compressor never above threshold, bell 0 dB) | **all bit-exact**, 0 allocations |
| T02: 2…8 overlapping nodes (disabled / mix 0 / bypassed slots) over an active PRE-EQ | **bit-exact = PRE-EQ output**, no level rise |
| MIX-0: compressor in heavy reduction mixed to 0 | after the 20 ms ramp **bit-exact** xE while GR > 1 dB is still running |
| Delta math: engine vs `x + (g−1)·Select(x)` computed independently | −157.2 dB |
| D2 chain: −3 dB then −3 dB vs −6 dB | < −120 dB; order of compressor/gain matters; a bypassed middle slot is bit-identical to removing it; 6-slot chain finite, 0 allocations |
| T10: blocks 1 / 32 / 127 / 256 / 1024, 2 bells + 3 nodes incl. compressors | **bit-exact** to block 1 |
| T04: 100 000 mutations, 20 000 snapshots, 10 000 blocks of 1…2048 | no NaN/Inf, worst peak +9.5 dBFS (bound 1e4), **0 allocations** |
| Range jump 250–700 → 800–2000 Hz under a 1 kHz tone | HF > 5 kHz at **−88.2 dB** (threshold −60, metric floor −114.3) |
| PRE-EQ automation, same sweep as the HPDG audit | **−106.9 dB** vs HPDG **−95.0 dB** (same signal, same metric) |
| Compressor static law (DC levels −30…0 dB, T −20, R 4) | within 1e-4 dB of the formula; attack 63 % at 480 samples = 10.0 ms exactly |
| Decision B (80 Hz tone, node 60–120 Hz compressor) | GR 14.32 dB without PRE-EQ → 1.25 dB with a −18 dB bell before it |
| Plugin bypass with active node | crossfade, then bit-exact dry |

### H1 vs H2 (and two additional variants) — `reports/m2/m2_lab.json`, plots in `reports/m2/plots/`

Each node processed with Gain −6 dB, measured from a 4 s exponential sweep. "Boost" = the largest
**positive** gain anywhere when every node is set to **cut**: it should be 0.

| Scenario | Metric | H1 (spec) | H1-LR8 | H2 | H3 |
|---|---|---|---|---|---|
| all | bypass with nodes present | **bit-exact** | bit-exact | all-pass (null +0.9…+3.0 dB, magnitude 0.00 dB) | = H2 |
| all | bypass group delay max | 0 | 0 | 2.9 ms (1 node) … 11.2 ms (5 nodes, 80 Hz edge) | = H2 |
| A 1 node 250–700 | boost / deepest | **+1.31** / −4.33 | **+2.81** / −5.72 | 0.00 / −4.33 | 0.00 / −4.33 |
| B 2 nodes | boost / deepest | **+1.94** / −3.87 | **+2.82** / −5.70 | 0.00 / −5.81 | = H2 |
| C 3 nodes | boost / deepest | **+2.28** / −4.46 | **+2.98** / −5.80 | 0.00 / −5.99 | = H2 |
| D 5 nodes | boost / deepest | **+1.78** / −3.89 | **+2.84** / −5.74 | 0.00 / −6.00 | = H2 |
| E overlap 250–700 + 450–1200 | boost / deepest | **+1.40** / −2.71 | **+3.87** / −6.84 | N/A | 0.00 / −10.96 (deltas add in the overlap) |
| D, middle node only | neighbour centres max | 0.56 dB | 0.04 | 0.58 | 0.58 |
| Compressor 250–700 on drums | power change 300–600 Hz / 40–100 Hz / 3–8 kHz | −1.81 / −0.003 / −0.002 dB | — | −2.95 / −0.017 / −0.002 | = H2 |

Engine-vs-offline cross-check of H1 (LR4): −149…−155 dB in all scenarios (the lab's offline formula
and the plugin engine agree).

**Why H1 fails when processing:** `y = x + (g−1)·b` with a minimum-phase band `b` whose phase rotates
across the band relative to `x`. Where `b` is in phase, the cut works; where it rotates, the "cut" adds
energy. Steeper edges (LR8) give deeper centres but more rotation, so larger boosts. An earlier metric
("depth at node centre") hid this, because the geometric centre is exactly where the phase lines up;
the full curves (`all_minus6_curve_db`) show it.

**H3** (added in M2 because H1 was refuted): one Linkwitz-Riley tree over the union of all node
edges; each node's band is the sum of the elementary bands inside it; `y = AP(x) + Σ (P(M) − M)`. On
touching, non-overlapping nodes it is numerically H2; unlike H2 it handles overlap.

### CPU (Release, 48 kHz stereo, block 512, budget 10 667 µs)

| Configuration (H1 engine) | p50 | p99 | p99 / budget |
|---|---|---|---|
| 1 node, compressor | 29.0 µs | 31.9 µs | 0.3 % |
| 5 nodes, compressor each | 145.6 µs | 223.4 µs | 2.1 % |
| Layer ceiling: 24 bells + 8 nodes × 6 compressors | 940 µs | 1 414 µs | **13.3 %** |

The ceiling × 10 layers would be ≈ 133 % of the block budget: T12 risk, see §9.

## 6. Artifacts

`reports/m2/m2_lab.json` (incl. per-hypothesis response curves on a 240-point grid), `reports/m2/plots/*.svg`,
`reports/m2/wav/drums_{input,H1,H2,H3}_compressor_250-700.wav` (local, regenerate with `HPEQ_Lab`),
`reports/m2/host_smoke.json`, `reports/m2/hpdg_remeasure/`, `reports/m2_logs/`.

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| T01 | **PASS** (H1 engine) | bit-exact in all 8 configurations × 4 rates × 3 blocks |
| T02 | **PASS** (H1 engine) | 2–8 overlapping nodes, bit-exact to PRE-EQ |
| T04 | **PASS** (M2 parameters) | 100k mutations, finite, bounded, 0 allocations (Debug probe) |
| T10 | **PASS** | bit-exact across block sizes |
| MIX-0 | **PASS** | bit-exact after ramp with hot compressor state |
| B-ORDER | **PASS (Bell variant)** | GR 14.3 → 1.3 dB; the LowCut-200 variant needs M3 cut shapes |
| H1/H2 comparison | **DONE — H1 refuted for active processing** | table above |
| Gate 2 | **NOT PASSED** | invariants pass, but the default frequency core (H1) produces up to +2.3 dB of false boost on a cut. Per the STOP-GATE, no UI work until the core is replaced. |

## 8. Existing HPDG

Untouched. HPDG.vst3 still scans next to HPEQ (host smoke 16/16).

## 9. Limitations / risks / decisions

1. **Erratum to M0:** the HPDG EQ "zipper −52.1 dB" was the floor of a 2nd-order measuring high-pass
   (the 100 Hz tone leaking through), not zipper. Corrected metric: 8th-order high-pass, onset
   excluded, floor reported. HPDG pattern re-measured: **−95.0 dB**, metric floor −154.6 dB.
2. **Decision needed (DECISIONS D4, proposed):** replace H1 by H3 as the frequency core. Consequences:
   - no false boosts, correct depth for adjacent nodes, overlap supported;
   - the price is phase: while any FX node is active, the whole layer passes through the tree's
     all-pass (flat magnitude, 3–11 ms group delay at the low edges, like minimum-phase multiband
     plugins). With **no active node the tree is not run**, so T01 stays bit-exact for "FX off".
     A node at mix 0 / bypassed while others are active gives `AP(x)`, so T01/T02 for that case are
     checked against the documented all-pass result (allowed by 04 §3), not bit-exact identity;
   - a later Linear Phase mode (FIR, reported latency) can remove the phase shift.
3. Narrow single regions do not reach the full set gain with −6 dB-at-edge crossovers (1.5 octaves:
   −4.3 dB at the centre for −6 dB). Inherent to LR4 edges in both H2 and H3; the UI must draw the real
   response (M5), not the set value.
4. CPU: the per-layer ceiling (24 bells + 48 compressors) is 13 % of the block budget; ten such layers
   would not fit. Compressor `pow`/`log10` per sample is the first optimisation target.

## 10. Next

Owner decision on D4. With D4 accepted: implement H3 in the engine (shared tree per layer, edge glides,
crossfade when the edge set changes), rerun T01/T02/T04/T10 and the lab on the engine, then Gate 2.
