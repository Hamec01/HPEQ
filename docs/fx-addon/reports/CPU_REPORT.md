# CPU pass between M4 and M5 — p99 with 8 reverbs, O(N) tree, compressor and reverb hot loops

## 1. Scope / date / base

2026-10-21, base `2cb6084` (M4). Owner request: "сначала разберись с CPU: p99 при 8 реверберах, общие
allpass в дереве, компрессор без pow/log10 на сэмпл; те же замеры до/после".
Machine: i5-10300H laptop, Windows 11, MSVC 19.50.35730 Release (`/Ox`, no `/fp:fast`), 48 kHz stereo,
block 512 (budget 10 667 µs).

## 2. Method (new tool, fixed before any change)

`HPEQ_Lab bench`: 9 fixed engine configurations; per configuration a fresh engine, 200 warm-up blocks,
then 1500–2000 timed blocks; repetitions interleaved across configurations; process at HighPriority;
reports median p50/p95/p99 over repetitions, worst max, and "spikes" (> 3 × p50) with their spacing.
`--cold` repeats the M2–M4 lab method (no warm-up, normal priority).

**The laptop's speed drifts by up to ~1.7× between sessions** (same binary: tree 8 nodes 284 µs in one
session, 476 µs twenty minutes later). Absolute numbers from different sessions are therefore not
comparable, so the before/after comparison below runs **both binaries alternately in one session**:
the baseline commit built in a separate worktree with the identical bench code, then
before, after, before, after, before, after (3 rounds × 2 repetitions × 1500 blocks). Raw JSON:
`reports/cpu/ab_before_{1,2,3}.json`, `ab_after_{1,2,3}.json`.

## 3. The p99 with 8 reverbs (45 % in the M4 lab)

| Measurement of `reverb_8nodes` at base `2cb6084` | p50 | p99 | spikes > 3×p50 |
|---|---|---|---|
| M4 lab, single cold run (reported in M4) | 1 363 µs | 4 799 µs (45 %) | — |
| bench, warm, 3 reps | 1 311 µs | 1 876 µs (17.6 %) | 0 |
| bench `--cold` (old method), 3 reps | 1 421 µs | 2 171 µs (20.4 %) | 0 |

The 45 % did not reproduce, not even with the old method: no periodic or code-path outliers, p99/p50 ≈
1.4. It was a one-off of that run (thermal/OS). The real cost was structural: **the tree alone was
812 µs for 8 nodes**, and each reverb ~60–100 µs.

## 4. Changes

| Area | Change | Correctness evidence |
|---|---|---|
| Crossover tree | O(N²) → O(N): band'_j = LR4_j(prefix of the other nodes' all-passes), each node's delta all-passed by the later nodes via **Horner** recombination (`acc = AP_j(acc) + d_j`). 8 nodes: 160 → 64 SVF sections per channel | linear response identical: engine vs offline H3 formula −142.7…−155.4 dB, no false boost in any M2 scenario; T01/T02 vs all-pass reference unchanged |
| Tree timing | the recombination chain replays the split chain's coefficient retunes at the same sample positions | first attempt was not block-invariant (T10: −114 dB); fixed, T10 bit-exact again |
| Compressor | per-sample `log10`/`pow` (double) → chunked float loops (level, dB, law, dB→gain via `exp2`) around the serial double recursion | T06 vs the independent law model: −135.3 dB (threshold −120; was −147 dB) |
| Reverb | no integer division in the inner loop (conditional wraps), interpolation skipped for whole-sample lengths | RT60 table, bypass/tail and wet-only tests unchanged (bit-identical criteria still pass) |

**Behavioural consequence (non-linear only), disclosed in DECISIONS D4:** a node's compressor/reverb
now sees its band without the all-pass phase of the *earlier* nodes' edges (previously: of all other
nodes' edges). Same band magnitude, different phase, so detectors on transient material read
different peaks. M4 scenario (drums): first node's peak GR 14.19 → 15.70 dB.

## 5. Before / after (interleaved, same session; medians)

| Configuration | p50 before | p50 after | Δ | p99 before | p99 after |
|---|---|---|---|---|---|
| tree, 1 node, gain | 90.5 | 79.9 | −12 % | 142.2 | 142.2 |
| tree, 5 nodes, gain | 587.5 | 301.8 | −49 % | 768.0 | 439.0 |
| tree, 8 nodes, gain | 1 209.9 | 488.0 | **−60 %** | 1 461.9 | 664.7 |
| compressor, 1 node | 121.7 | 94.8 | −22 % | 199.1 | 198.7 |
| compressor, 8 nodes × 6 | 2 728.4 | 1 325.9 | **−51 %** | 3 145.6 | 1 611.0 |
| reverb, 1 node | 205.5 | 147.7 | −28 % | 322.5 | 275.1 |
| reverb, 8 nodes | 2 205.9 | 1 089.2 | **−51 %** | 2 608.5 | 1 367.9 |
| EQ, 24 bells (unchanged code) | 366.5 | 364.0 | −1 % | 468.8 | 504.3 |
| layer ceiling: 24 bells + 8 × 6 compressors | 3 130.7 | 1 685.4 | **−46 %** | 3 541.9 | 2 007.5 |

(µs per 512-sample block; this session ran in the laptop's slow state: the same "after" binary measured
tree 8 nodes at 284 µs earlier the same day.) The unchanged EQ case moving by −1 % shows the A/B
method is fair.

## 6. Tests

Release 698/698; Debug with the CRT allocation hook 698/698, **0 audio-thread allocations** (`reports/cpu_logs/unit_debug.log`); host smoke 16/16.

## 7. What is left (T12, M7)

- A layer at its ceiling still takes ~16–19 % of a block (slow state); ten such layers do not fit.
  Realistic layers (a few nodes) are 1–5 %. Lazy processing of inactive layers (M7) and a per-plugin
  budget/warning are needed; SIMD across channels in the tree and in the EQ sections is the next lever.
- EQ sections were not optimised in this pass (24 bells unchanged).
- Laptop timing drift: CPU acceptance numbers (T12) must come from interleaved A/B on one machine, or a
  desktop with fixed clocks.
