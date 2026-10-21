# M3 — PRE-EQ shapes and slopes, channel modes, M/S, B-ORDER, T03/T05

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M3 — PRE-EQ and exact filtering |
| Date | 2026-10-21 |
| Base | `594a094` (M2, Gate 2); code and report in the M3 commit on `feature/hpeq-m0-m1` |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730, JUCE 8.0.12 |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/EqDesign.*` | Band designs (all 9 shapes, Butterworth cuts 6…96 dB/oct) as TPT sections + exact analytic response `bandResponse()` |
| `Source/DSP/PreEqProcessor.*` | Rewritten: up to 8 sections per band, channel modes, per-sample coefficient interpolation, entry/exit per shape kind, structure crossfade |
| `Source/DSP/Biquad.h` | `TptSvf` with per-sample frequency interpolation (crossover tree) |
| `Source/DSP/CrossoverTree.*` | Uses the interpolating SVFs |
| `Source/DSP/FxNodeProcessor.*`, `Processors.*` | FX node channel modes (L/R/M/S, Linked, Dual Mono), per-channel compressor detectors |
| `Source/DSP/LayerSnapshot.*` | Slope and channel mode in the snapshot; every EQ shape is now processed |
| `Tests/Unit/EqTests.cpp` | RBJ equivalence, slopes, T03 grid, Nyquist, Tilt, T05 (EQ, nodes, compressor link), B-ORDER, transitions, T10/T04 |
| `Tests/Tools/LabM3.cpp` | `HPEQ_Lab m3`: zipper structures, slope family, shapes, group delay, impulse/ringing, CPU |
| `Tests/Harness/AudioMetrics.*` | `TransferPoint::binFrequencyHz` (see §5.6) |
| other tests / lab | engines and processors moved to the heap (§9.3) |

## 3. Implemented vs scaffolding

**Implemented and processed:** Bell, Low Shelf, High Shelf, Low Cut and High Cut (6, 12, 18, 24, 36, 48,
72, 96 dB/oct, Butterworth), Notch, Band Pass (0 dB peak), Tilt (first order, ±G/2), All Pass.
Channel modes on PRE-EQ bands and FX nodes: Stereo, Left, Right, Mid, Side, Linked Stereo, Dual Mono.

**Decided semantics (documented, not hidden):**
- Q is not used by Low/High Cut: cuts are Butterworth of order slope/6 (no resonant cut in V1).
- **Brickwall is not offered** (spec §6.3: needs its own FIR R&D with transition width and latency).
- For a linear EQ, Linked Stereo and Dual Mono equal Stereo; they matter for the compressor
  (linked: one detector on max(|L|,|R|); dual mono: one detector per channel).
- Mono input: Right and Side bands/nodes have nothing to act on and leave the signal untouched;
  Stereo/Left/Mid/Linked/Dual process the single channel.
- M/S: M = (L+R)/2, S = (L−R)/2, L = M+S, R = M−S (01 §4).

## 4. Commands and exit codes

```bash
cmake --build build --config Release                        # 0, no warnings
build/Release/HPEQ_Tests.exe     # 0 — 644 passed, 0 failed (73 s)
build/Debug/HPEQ_Tests.exe       # 0 — 644 passed, 0 failed (CRT hook: malloc + new)
build/Release/HPEQ_Lab.exe m3 --out docs/fx-addon/reports/m3          # 0
build/Release/HPEQ_Lab.exe --out <tmp>                                # 0, M2 core unchanged (§5.7)
build/Release/HPEQ_HostSmoke.exe --plugin …/HPEQ.vst3 --other …/HPDG.vst3   # 0 — 16/16
```

## 5. Numbers

### 5.1 Designs are the RBJ cookbook

SVF sections vs independent RBJ biquads (Bell ±, shelves ±, Notch, Band Pass, All Pass, 12 dB/oct cuts;
4 rates × 3 frequencies × 120 points): worst **complex** difference **1.1e-10** (threshold 1e-9).

### 5.2 T03 — measured response = analytic response of the used coefficients

460 configurations: 7 shapes + 16 cut variants × fc {30, 80, 200, 1000, 6000} × fs {44.1, 48, 96, 192 k},
1.5 s sweeps through the engine, 160-point log grid.

| Mask (fixed before the run) | Threshold | Worst measured |
|---|---|---|
| analytic ≥ −60 dB | ≤ 0.05 dB | **0.0032 dB** |
| −100 ≤ analytic < −60 dB | ≤ 1 dB | **0.315 dB** |

### 5.3 T03 — slopes (fit where the analytic response is −30…−100 dB; tolerance ±6 dB/oct)

| Slope | 6 | 12 | 18 | 24 | 36 | 48 | 72 | 96 |
|---|---|---|---|---|---|---|---|---|
| Low Cut 1 kHz, fitted | 6.01 | 12.04 | 18.06 | 24.08 | 36.13 | 48.19 | 72.30 | 96.42 |
| High Cut 200 Hz, fitted | 7.59 | 12.92 | 18.59 | 24.25 | 36.18 | 48.20 | 72.29 | 96.37 |
| sections | 1 | 1 | 2 | 2 | 3 | 4 | 6 | 8 |
| Low Cut 200 Hz, analytic at 100 Hz | −6.99 | −12.31 | −18.13 | −24.10 | −36.13 | −48.17 | −72.25 | −96.34 |
| Low Cut 200: group delay at 400 Hz | 0.16 ms | 0.32 | 0.46 | 0.58 | 0.84 | 1.11 | 1.65 | 2.19 ms |
| Low Cut 200: impulse rings to −60 dB | 2.6 ms | 4.0 | 5.5 | 5.2 | 8.7 | 12.0 | 17.0 | 19.9 ms |
| High Cut 2 kHz: impulse rings to −60 dB | 0.6 ms | 0.9 | 1.2 | 1.6 | 2.3 | 3.1 | 4.6 | 6.1 ms |

The High Cut fits sit above 6.02 N at low orders because the bilinear transform steepens the response
towards Nyquist (the fit is capped at fs/4); all are within tolerance. 96 dB/oct one octave below fc:
−96.33 dB analytic (Butterworth 16th order), −96.48 dB at 1 kHz in the design test.
Near Nyquist (cuts at 20 kHz @ 44.1/48 k, bell at 23 kHz Q 8 +12 dB, shelf at 30 kHz): finite,
bounded, frequencies clamped to 0.45 fs.

### 5.4 T05 — channel modes

| Check | Result |
|---|---|
| Left band/node: R untouched; Right band/node: L untouched | **bit-exact** |
| Right band on a left-only signal | **bit-exact** |
| Mid band on side-only signal, Side band on mid-only signal | **bit-exact** |
| Mid band keeps L == R on a mid signal; Side band keeps L == −R on a side signal | **bit-exact** |
| Mid band vs independent M/S reference (filter M alone, rebuild L/R) | −144.2 dB |
| Mono input with Right/Side band | **bit-exact** |
| FX node Mid/Side: delta confined to M (ΔL = ΔR) / S (ΔL = −ΔR) | < 1e-6 |
| Compressor, loud L / quiet R: Dual Mono vs Linked | quiet R: passive −43.02, dual −43.02, linked −62.95 dBFS |

### 5.5 B-ORDER with Low Cut

80 Hz sine at −6 dBFS, compressor node 100–250 Hz (T −30, R 4): **GR 9.22 dB** without PRE-EQ,
**0.07 dB** behind Low Cut 200 Hz / 24 dB/oct (thresholds fixed: > 5 dB and < 1 dB).

### 5.6 Smoothness

| Test | Result |
|---|---|
| PRE-EQ automation, the M0/M2 bell sweep (HPDG pattern −95.0 dB) | **−131.6 dB** |
| Range jump of an FX node (M2 test) | −114.0 dB (metric floor −114.3) |
| Topology changes (M2 test) | HF −86.9 dB, dip 0.55 dB |
| Shape/slope/mode changes, enable/disable, 8 scenarios (dip ≤ 3 dB, HF ≤ −60 dB) | worst dip **2.81 dB** (Low Cut slope 24→96 at a 1 kHz tone), worst HF **−90.2 dB** |

Zipper structure study (`m3_lab.json → zipper`, same sweep): RBJ TDF-II stepped per 32 samples −106.9,
SVF stepped −72.9, SVF stepped every 8 samples −79.9, **SVF interpolated per sample −150.6** /
engine −131.6 dB. The structure change of M3 (to SVFs) first **regressed** this test from −106.9 to
−72.9 dB; the cause (output-mix coefficients stepping) was measured before it was fixed.

### 5.7 M2 core unchanged

`HPEQ_Lab` (M2) rerun on M3: H3 engine identical (no false boost in any scenario, engine vs offline
H3 −146…−155 dB).

### 5.8 CPU (Release, 48 kHz stereo, block 512, budget 10 667 µs)

| Configuration | p50 | p99 | p99 / budget |
|---|---|---|---|
| 24 Bells, static | 158 µs | 311 µs | 2.9 % |
| 24 Bells, automated every block | 249 µs | 519 µs | 4.9 % |
| 24 cuts at 96 dB/oct (192 sections per channel) | 1 061 µs | 1 441 µs | 13.5 % |
| M2 layer ceiling (24 bells + 8 nodes × 6 compressors), now | 1 837 µs | 2 631 µs | **24.7 %** (M2: 19.1 %) |

## 6. Artifacts

`reports/m3/m3_lab.json`, `reports/m3/plots/{low_cut_200_slopes, high_cut_2k_slopes, low_cut_200_group_delay,
eq_shapes, all_pass_1k_group_delay}.svg`, `reports/m3/host_smoke.json`, `reports/m3_logs/`.

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| T03 | **PASS** | §5.2, §5.3 |
| T05 | **PASS** | §5.4 |
| B-ORDER | **PASS** (Low Cut 200 Hz) | §5.5 |
| T01/T02/T04/T10/MIX-0 (M2 tests + new full-EQ T04/T10) | **PASS** | unit log |
| Gate 3 | **PASS** — T03/T05, phase/impulse/sweep evidence, predictable CPU measured | |

Debug run with the CRT allocation hook: 644/644, **0 audio-thread allocations** in every test that probes them (`reports/m3_logs/unit_debug.log`).

## 8. Existing HPDG

Untouched; HPDG.vst3 still scans next to HPEQ (16/16).

## 9. Limitations / risks / findings

1. **Measurement fix:** the first T03 run reported 3.6 dB at a Notch 80 Hz: the analytic value was
   evaluated at the requested frequency, the measurement at the nearest FFT bin (0.05 Hz away, next
   to the null). The harness now reports the bin frequency and the analytic value is taken there.
   No threshold was changed.
2. **Design fix:** the first Tilt (two one-pole shelves) was +0.51 dB at fc; replaced by the single
   first-order tilt `(√g s + 1)/(s + √g)`: exactly 0 dB at fc, ±3.00 dB at 20 Hz / 20 kHz for +6 dB.
3. **Stack:** `LayerEngine` is ~115 KB (24 bands × 2 sets × 8 sections + tree); several engines as
   locals overflowed the 1 MB test-thread stack. In the plugin it is a heap member; tests and lab now
   allocate engines on the heap.
4. CPU: the per-layer ceiling rose to 24.7 % (per-sample coefficient interpolation). T12 risk grows;
   optimisation before M7 (interpolate only while gliding is already the case — the next targets are
   shared foreign all-passes in the tree and the compressor's per-sample `pow`/`log10`).
5. Changing a cut's slope crossfades two filters of different phase: up to 2.81 dB dip on a passband
   tone (within the fixed 3 dB). A phase-matched morph is possible later.

## 10. Next

**M4** — Compressor (T06 against its documented law, already partly measured) and Reverb as region
processors (wet-only send, tails T07, PDC), before the M5 UI.
