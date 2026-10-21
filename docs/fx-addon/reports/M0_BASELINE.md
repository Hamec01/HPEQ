# M0 — Audit, measurement harness, baseline

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M0 — audit / freeze / baseline |
| Date | 2026-10-21 |
| HPEQ commit measured | `e3a67a57a29aea8719d3f9ac6acf2458b06b7982` (branch `feature/hpeq-m0-m1`) |
| HPDG commit audited | `b87f71119eaf3e09af8f2a9cda56e4195d7be942` (`main`, clean), read-only |
| JUCE | 8.0.12 (HPDG's checkout via `HPEQ_JUCE_DIR`) |
| Platform | Windows 11, Intel i5-10300H (4C/8T, 2.5 GHz), 16 GB, MSVC 19.50.35730, VS 18 2026, x64 |

## 2. Changed files

| Path | Why |
|---|---|
| `CMakeLists.txt`, `CMakePresets.json` | Standalone repo (DECISIONS D1): JUCE via local dir or FetchContent; optional read-only HPDG sources |
| `Tests/Harness/*` | Signal generators, metrics, WAV/SHA-256 I/O, RT allocation probe, machine info |
| `Tests/Tools/HarnessMain.cpp` | `HPEQ_Harness selfcheck / fixtures / baseline` |
| `Tests/Tools/HpdgBlockAudit.cpp` | Measures HPDG compressor, Monsta, EQ pattern (compiled from HPDG `Source/`, nothing copied) |
| `docs/fx-addon/DECISIONS.md` | D1 separate repo, D2 up to 6 FX per node, D3 rewrite processors |

## 3. Implemented vs scaffolding

Implemented and exercised: the whole offline harness (below). **No plugin DSP exists yet** — that is M1/M2.

- Fixture set `fx_addon_v1`, seed `24681357`: silence, DC, subnormal, impulse, step, sines 20…15k,
  exp sweep 20 Hz→0.45 fs, white, pink, SMPTE IMD, gated burst, L/R/M/S-only, synthetic drums.
  Frequencies above 0.45 fs are marked N/A, never aliased (none occur at 44.1–192 k for this set).
- Metrics: level stats (peak/RMS/DC/crest/finite/subnormal count), aligned null `error_dB` (04 §3),
  delay estimate, transfer function H(f) with an a-priori mask (−100 dB below max |X|, fixed before any
  test), dominant frequency, spectral centroid, M/S RMS and L/R correlation, p50/p95/p99 timing.
- RT allocation probe: Debug = CRT hook (sees `malloc` and `new`), Release = global `operator new`
  only. Verdicts on allocations are taken from **Debug**; every JSON states which probe ran.

## 4. Commands and exit codes

```bash
set HPEQ_JUCE_DIR=C:/DRUMENGINE/JUCE & set HPEQ_HPDG_SOURCE_DIR=C:/DRUMENGINE/Source
cmake --preset vs2026-x64                                   # exit 0
cmake --build build --config Release --target HPEQ_Harness  # exit 0, 0 warnings
cmake --build build --config Debug   --target HPEQ_Harness  # exit 0
build/Release/HPEQ_Harness.exe selfcheck                    # exit 0, 35/35
build/Release/HPEQ_Harness.exe baseline --out docs/fx-addon/reports/m0 --commit <sha>  # exit 0
build/Debug/HPEQ_Harness.exe   baseline --out docs/fx-addon/reports/m0 --commit <sha>  # exit 0
build/Release/HPEQ_Harness.exe fixtures --out docs/fx-addon/reports/m0/fixtures --commit <sha>  # exit 0
ctest --test-dir build -C Release                           # 1/1 passed
```

Logs: `reports/m0_logs/`.

## 5. Numbers

### Harness self-validation (35/35 PASS, Release and Debug)

| Check | Measured | Expected |
|---|---|---|
| Identity null | bit-exact, −400 dB (floor) | exact |
| Gain error 1e-3 | −59.9996 dB | −60 dB ± 0.01 |
| Known 37-sample delay | estimated 37, null after alignment −400 dB | 37 |
| H(f) identity / ×0.5 | 0 dB / error 8.7e-8 dB | 0 / −6.0206 dB |
| 1 kHz sine at 44.1/48/96/192 k | 1000.01–1000.06 Hz | ± 0.5 Hz |
| −6 dBFS-peak sine RMS | −9.0103 dB | −9.0103 |
| Pink PSD slope 100 Hz→10 kHz | −2.79 … −3.18 dB/oct | −3.01 ± 0.5 |
| M-only side / S-only mid | −400 dB / −400 dB, corr +1 / −1 | exact |
| Float32 WAV round trip | bit-exact | exact |

Fixture PCM SHA-256 are identical between the Release and the Debug build (26/26 at 48 k):
generators are deterministic across build configurations. 104 WAV (4 rates × 26) = 76 MB, gitignored,
reproducible; `m0/fixtures/manifest.json` is tracked.

### Timing floor of this machine (plain stereo copy, 48 kHz, Release)

p99 of a buffer copy ≤ 0.8 µs for blocks 1…2048 (≤ 0.5 % of the block-1 budget). Any DSP time later is
reported against `budget = block/fs` on this same machine (`timing_floor_copy` in the JSON).

### HPDG blocks — the baseline HPEQ's rewrites must beat (DECISIONS D3)

All at 48 kHz stereo, prepared block 512. Source: `m0/baseline_metrics_{release,debug}.json`.

| Block | Measurement | Result | Verdict for HPEQ |
|---|---|---|---|
| DrumCompressorBlock | allocations, block 256 ≤ prepared | 0 | — |
| | allocations, block 2048 > prepared (Debug probe) | **1** (`ensureScratchCapacity → setSize`) | confirmed RT risk; HPEQ must preallocate for the host's max and chunk larger blocks |
| | static curve T −20 dB, R 4:1 vs hard-knee law | constant **0.61 dB** less reduction above T; no knee parameter | new compressor: documented detector, knee, curve within a stated tolerance (T06) |
| | attack 63 % on −40→−6 dBFS step, nominal 10 ms | **2 ms** measured | displayed times do not match behaviour (Glue scaling + JUCE ballistics); HPEQ must show what it does |
| | release 63 %, nominal 120 ms | **23 ms** measured | same |
| | mix = 0 | bit-exact dry | keep this property |
| | CPU block 256 (Release) | p50 11.3 µs, p99 42.3 µs (0.8 % budget) | reference for the new one |
| MonstaFxBlock | allocations per process call (Debug) | **1 vector per block** (750 blocks → 1500 CRT events incl. debug proxy) | confirmed RT allocation, every block |
| | block-size invariance vs 256 (T10-style) | 32: **−32.8 dB**, 127: −35.9 dB, 512: −44.4 dB error | output depends on host buffer size → fails T10; rewrite must be block-invariant |
| | no transport (bpm 0, timeline frozen) | finite, −18.2 dBFS RMS | runs, but slicing grid is meaningless without a timeline; HPEQ needs a defined fallback |
| HPDG EQ pattern | allocations per coefficient update | **1** (Release, `new`) / 2 (Debug) | coefficients must be built off the audio thread |
| | zipper: bell −12→+12 dB over 1 s, update per 512 samples | HF products > 2 kHz at **−52.1 dB** rel. signal | M3 target: same test, smoothed, lower |
| | Low Cut slope 25–50 Hz (cut 200 Hz) | 11.96 dB/oct | only 2nd order; HPEQ adds 6…96 dB/oct |

The attack/release method (63 % of the final gain change in dB, 1 ms windows) is a fixed measurement
definition, not a calibrated reference compressor. The new compressor will be measured with the same
code, so the comparison is like-for-like.

## 6. Artifacts

- `reports/m0/baseline_metrics_release.json`, `reports/m0/baseline_metrics_debug.json`
- `reports/m0/fixtures/manifest.json` (+ 104 local WAVs, regenerate with the `fixtures` command)
- `reports/m0_logs/hpdg_ctest.log`, `baseline_*.log`, `fixtures.log`

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| M0-A | **PASS** | SHAs, JUCE, CMake, toolchain above; build logs; HPDG ctest log |
| M0-B | **PASS** | `selfcheck` 35/35, manifest with SHA-256, Release/Debug hash equality |
| HPDG | **PASS (no change)** / pre-existing failure noted | HPDG untouched (separate repo). HPDG's own ctest on its existing `build/` (binaries from 2026-10-19, older than HEAD, not rebuilt): 4/5 pass; `HPDG_LaneBoundaryTests` fails 5 cases ("requires loaded Kick samples", silent preview/export) — sample-library environment of the stale build, unrelated to HPEQ |

Not done in M0 by design: CPU/allocation numbers for HPEQ DSP (none exists), 44.1/96/192 k timing
(timing floor measured at 48 k only), spectrum/phase/group-delay plots (tooling exists, no filters yet).

## 8. HPDG regression

No file in `C:\DRUMENGINE` was modified. HPDG blocks are compiled into `HPEQ_Harness` from their original paths.

## 9. Risks / open questions

1. Release allocation probe misses `malloc`: allocation verdicts must come from Debug builds (or a
   future malloc-level hook). Stated in every JSON.
2. The 0.61 dB static-curve offset of the HPDG compressor was not root-caused (likely JUCE's
   ballistics on a sine peak); it is recorded as behaviour, not used as a target.
3. `pluginval` is not installed on this machine — affects M1-B/M1-C/T11 (see M1).

## 10. Next

M1 — standalone VST3 insert skeleton. Gate 0 is met: harness and baseline exist and are real.
