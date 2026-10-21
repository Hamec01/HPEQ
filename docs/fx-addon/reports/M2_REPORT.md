# M2 — PRE-EQ + frequency FX node prototype: H1 vs H2, H3 engine, Gate 2

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M2 — DSP prototype of the frequency core (STOP-GATE) |
| Date | 2026-10-21 |
| Commits | `85da164` (first engine on H1 + H1/H2 lab, Gate 2 blocked) → the commit adding this revision (H3 engine, Gate 2) |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730, JUCE 8.0.12, 48 kHz unless stated |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/Biquad.*` | RBJ designs in double (peak, LP, HP, all-pass), TDF-II sections; **TPT state-variable filter** for modulated filters |
| `Source/DSP/CrossoverTree.*` | **H3 core (D4):** order-free phase-coherent tree, edge birth/parking at 1 Hz, per-sample node presence |
| `Source/DSP/LayerEngine.*` | Layer: PRE-EQ → tree → node deltas; tree not run when no edge is allocated |
| `Source/DSP/FxNodeProcessor.*` | Node chain of up to 6 slots (D2) on the band given by the tree |
| `Source/DSP/PreEqProcessor.*`, `Ramp.h`, `DspConfig.h`, `LayerSnapshot.*` | PRE-EQ Bell with glides; exact-landing ramps; 32-sample control clock; lock-free snapshots |
| `Source/DSP/Processors.*` | Gain (reference) and the new documented compressor (D3) |
| `Source/DSP/RegionSelector.*` | **removed** (H1 selector, refuted) |
| `Source/Plugin/FxPluginProcessor.*` | Layer 1 through the engine; bypass = 20 ms crossfade to an exact dry copy |
| `Tests/Unit/DspTests.cpp` | T01, T02, MIX-0, FX-off return, T04, T10, D2 chain, delta math, topology/edge transitions, compressor law, decision B |
| `Tests/Tools/LabMain.cpp`, `Tests/Harness/SvgPlot.*`, `AudioMetrics.*` | H1 / H1-LR8 / H2 / H3-tree / **H3-engine** lab, cross-checks, corrected HF metric |

## 3. Implemented vs scaffolding

**Real, in the plugin (layer 1):** PRE-EQ Bell bands; FX nodes on the H3 tree with any (overlapping)
ranges; chains of Gain and Compressor; node/slot mix; node births/retirements without crossfades;
crossfaded plugin bypass. **Not processed yet:** other EQ shapes (M3), other FX types (M4/M6),
channel modes other than Stereo, node Solo, layers 2–10, node UI (M5).

## 4. Commands and exit codes

```bash
cmake --build build --config Release                       # 0, no warnings
cmake --build build --config Debug --target HPEQ_Tests     # 0
build/Release/HPEQ_Tests.exe   # 0 — 526 passed, 0 failed
build/Debug/HPEQ_Tests.exe     # 0 — 526 passed, 0 failed (CRT hook: malloc + new)
ctest --test-dir build -C Release                          # 2/2
build/Release/HPEQ_Lab.exe --out docs/fx-addon/reports/m2  # 0
build/Release/HPEQ_HostSmoke.exe --plugin …/HPEQ.vst3 --other …/HPDG.vst3  # 0 — 16/16
```

## 5. Numbers

### 5.1 Frequency-core comparison (`reports/m2/m2_lab.json`, plots in `reports/m2/plots/`)

Gain −6 dB per node, 4 s exponential sweep. **Boost** = the largest positive gain anywhere while every
node cuts (must be 0). **Passive** = nodes running at 0 dB (H3's dry all-pass audible).

| Scenario | Metric | H1 (spec) | H1-LR8 | H2 | H3 tree (lab) | **H3 engine** |
|---|---|---|---|---|---|---|
| A 1 node 250–700 | boost / deepest | +1.31 / −4.33 | +2.81 / −5.72 | 0.00 / −4.33 | 0.00 / −4.33 | **0.00 / −4.33** |
| B 2 contiguous | boost / deepest | +1.94 / −3.87 | +2.82 / −5.70 | 0.00 / −5.81 | 0.00 / −5.81 | **0.00 / −5.87** |
| C 3 contiguous | boost / deepest | +2.28 / −4.46 | +2.98 / −5.80 | 0.00 / −5.99 | 0.00 / −5.99 | **0.00 / −6.08** |
| D 5 contiguous | boost / deepest | +1.78 / −3.89 | +2.84 / −5.74 | 0.00 / −6.00 | 0.00 / −6.00 | **0.00 / −6.26** |
| D | node-centre depths | −3.2…−2.3 | −5.6…−5.4 | −4.9…−6.0 | = H2 | −5.1…−6.3 |
| E overlap 250–700 + 450–1200 | boost / deepest | +1.40 / −2.71 | +3.87 / −6.84 | N/A | 0.00 / −10.96 | **0.00 / −9.81** |
| all | FX off | exact | exact | all-pass | all-pass | **bit-exact (tree not run)** |
| A / D | passive: magnitude, group delay max | 0 / 0 | 0 / 0 | 0.00 dB, 2.9 / 11.2 ms | = H2 | 0.00 dB, **2.9 / 15.2 ms** |
| drums, compressor on 250–700 | power change 300–600 / 40–100 / 3k–8k Hz | −1.81 / −0.003 / −0.002 dB | — | −2.95 / −0.017 / −0.002 | = H2 | **−2.95 / −0.017 / −0.002** |

- Engine vs the offline order-free H3 formula (plain RBJ cascades): **−146…−155 dB** in all scenarios.
- H1 cuts with false boosts because its band rotates in phase against the dry; H3's band/dry ratio is
  `|HP4_lo|·|LP4_hi|`, a real number, so a cut never adds energy.
- The overlap is cut twice (deltas add, as the delta model defines: −9.8 dB with two −6 dB nodes).
- Engine group delay is a little higher than the lab tree (15.2 vs 11.2 ms, 5 nodes): touching nodes
  each keep their own all-pass of the shared edge. De-duplicating shared edges is a later optimisation.

### 5.2 Engine invariants and transitions (unit tests, thresholds fixed in the source before the runs)

| Test | Result |
|---|---|
| T01 "FX off" (empty, node disabled, bell 0 dB) × 44.1/48/96/192 k × blocks 1/64/512 | **bit-exact** |
| T01 passive running node (mix 0, slots bypassed, slot mix 0, Gain 0 dB, compressor below threshold) | vs documented all-pass reference: worst **−196 dB** (threshold −100) |
| T02 2–8 overlapping nodes: disabled | **bit-exact** PRE-EQ output |
| T02 mix 0 / bypassed slots | vs AP(PRE-EQ) worst **−128.6 dB**; RMS vs reference 0.0000 dB (no dry doubling) |
| Delta math vs independent reference `AP_hi AP_lo x + (g−1) LP4_hi HP4_lo x` | −147.0 dB |
| MIX-0 with a compressor still in 1+ dB reduction | bit-exact to the passive-node render |
| FX off again (disable all running nodes) | **bit-exact** PRE-EQ output 100 ms later, tree inactive |
| T10 blocks 1/32/127/256/1024 | **bit-exact** |
| T04 100k mutations, 20k snapshots, 10k random blocks | finite, worst peak +10.1 dBFS, **0 allocations** (Debug probe) |
| Node on, node added, range moved, all off (1 kHz tone) | HF > 5 kHz **−70.3 dB** (≤ −60), worst level dip **0.52 dB** (≤ 3) |
| Range jump 250–700 → 800–2000 Hz | HF > 5 kHz −82.1 dB (≤ −60) |
| PRE-EQ automation vs HPDG pattern, same sweep and metric | −106.9 dB vs −95.0 dB |
| Compressor law / attack / decision B | within 1e-4 dB; 63 % at exactly 10.0 ms; GR 14.3 → 1.3 dB behind a −18 dB bell |

### 5.3 What failed on the way (kept, because each changed the design)

| Attempt | Measured | Consequence |
|---|---|---|
| H1 engine (spec default) | false boosts +1.3…+2.3 dB | H1 refuted → D4 |
| H3 with a crossfade between old and new tree on every structure change (as first planned in D4) | **17.76 dB level dip** on a 1 kHz tone when a node switches on; T10 −157 dB from a float blend at c = 1 | replaced by edge birth/parking at 1 Hz: no crossfade at all |
| Tree in RBJ TDF-II biquads with edges gliding from 1 Hz | **T04 runaway: +101.9 dBFS** (time-varying instability near z = 1) | tree filters moved to TPT state-variable filters: +10.1 dBFS, stable |
| T02 level check "windowed RMS within 0.1 dB of the PRE-EQ output" | failed by 0.13–0.48 dB although the output equals the all-pass reference bit-for-bit | the metric was wrong (an all-pass moves energy across the 1 s window edge); replaced by RMS vs the reference, old value logged (0.537 dB) |

### 5.4 CPU (Release, 48 kHz stereo, block 512, budget 10 667 µs)

| H3 engine | p50 | p99 | p99 / budget |
|---|---|---|---|
| 1 node, compressor | 52.3 µs | 86.6 µs | 0.8 % |
| 5 nodes, compressor each | 411.9 µs | 561.4 µs | 5.3 % |
| Layer ceiling: 24 bells + 8 nodes × 6 compressors | 1 617 µs | 2 039 µs | **19.1 %** |

The tree costs `N` own bands plus `N(2N−2)` foreign all-passes per channel (8 nodes: 144 SVFs). Ten
ceiling layers would need ~190 % of the budget: **T12 is at risk**. Optimisation targets before M7:
shared foreign all-pass products, compressor `pow`/`log10`, SIMD over channels.

## 6. Artifacts

`reports/m2/m2_lab.json` (incl. 240-point response curves per hypothesis), `reports/m2/plots/*.svg`,
`reports/m2/wav/` (local: drums input and compressor renders for H1/H2/H3-tree/H3-engine),
`reports/m2/host_smoke.json`, `reports/m2/hpdg_remeasure/`, `reports/m2_logs/`.

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| T01 | **PASS** | bit-exact when FX are off; passive nodes ≤ −196 dB vs the documented all-pass result (04 §3) |
| T02 | **PASS** | disabled: bit-exact; mix 0 / bypassed: ≤ −128.6 dB vs AP(PRE-EQ), level equal to reference |
| T04 | **PASS** (M2 parameters) | 100k mutations, finite, bounded, 0 allocations |
| T10 | **PASS** | bit-exact across block sizes |
| MIX-0 | **PASS** | bit-exact with hidden compressor state |
| B-ORDER | **PASS** (Bell variant) | GR 14.3 → 1.3 dB; LowCut-200 variant needs M3 |
| H1/H2 comparison | **DONE** | H1 refuted; H3 (D4) implemented and measured |
| **Gate 2** | **PASS** | no dry doubling; no phase catastrophe at unity/bypass (FX off bit-exact, passive = documented all-pass); no non-finite; T01/T02/T04/T10; active response without false boosts |

## 8. Existing HPDG

Untouched. HPDG.vst3 still scans next to HPEQ (host smoke 16/16).

## 9. Limitations / risks

1. **Phase:** while any FX node runs, the layer's dry passes through all-passes (flat magnitude,
   up to ~15 ms group delay at low edges with 5 nodes). Linear Phase mode (FIR, latency) is future work.
2. Narrow single regions reach −4.3 dB for −6 dB (LR4 edges at −6 dB); the UI must draw the real
   response (M5).
3. Births/retirements take 60 ms glide + 20 ms fade: a node is heard ~80 ms after it is enabled.
4. CPU ceiling per layer 19 % (§5.4) — T12 risk.
5. Erratum to M0 stands (HPDG zipper metric; corrected −95.0 dB).

## 10. Next

Gate 2 passed. **M3** — PRE-EQ shapes and slopes (Low/High Cut 6…96 dB/oct, shelves, notch, band-pass,
tilt, all-pass), channel modes and M/S, B-ORDER with LowCut 200 Hz, T03/T05; plus the tree/compressor
CPU work flagged above before M7.
