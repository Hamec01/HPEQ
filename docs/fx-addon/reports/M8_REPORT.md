# M8 — Validation and beta release (0.1.0)

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M8 — professional validation and beta |
| Date | 2026-10-22 |
| Base | `dfd8406` (M7) on `feature/hpeq-m0-m1`; code and report in the M8 commit |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730, JUCE 8.0.12 |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/Biquad.h`, `CrossoverTree.cpp` | Stereo SSE2 path for the tree's SVF sections (same operations in the same order: bit-identical) |
| `Source/DSP/ProcessorsM6.*`, `FxNodeProcessor.*`, `LayerEngine.cpp`, `ProjectEngine.*` | Alignment / look-ahead buffers sized for the sample rate; oversampling in 64-sample sub-blocks |
| `Source/Analysis/LoudnessMeter.*` | BS.1770-4 loudness (M / S / I) and true peak, on the audio thread, allocation-free |
| `Source/Plugin/FxPluginProcessor.*`, `FxPluginEditor.*` | Input/output loudness meters, true peak, reset; readout in the header |
| `Source/Core/FxProjectState.cpp` | Exact number round trip in the saved state (`std::to_chars` / `std::from_chars`) |
| `CMakeLists.txt`, `CMakePresets.json` | Version 0.1.0; `asan` preset (tests with `/fsanitize=address`); fixtures path |
| `Tests/Unit/M8Tests.cpp`, `Tests/Fixtures/*.xml` | Loudness / true peak, transport edge cases, 1000 edits, state precision, preset compatibility |
| `Tests/Tools/LabT11.cpp`, `HostSmokeMain.cpp` | T11 fixtures and checks through the real `.vst3` |
| `Tests/Tools/LabM7.cpp` | Memory: committed private bytes, per part |
| `scripts/package.ps1`, `docs/release/*` | Beta package, install text, release notes |

## 3. Commands and exit codes

```bash
cmake --build --preset release                                   # 0, no warnings
build/Release/HPEQ_Tests.exe                                     # 0 — 1025 passed, 0 failed
build/Debug/HPEQ_Tests.exe                                       # 0 — 1025 passed, 0 failed (CRT hook: malloc + new, 0 audio-thread allocations)
build-asan/Release/HPEQ_Tests.exe   (asan preset, MSVC ASan runtime on PATH)   # 0 — 1025 passed, 0 failed, no AddressSanitizer report
build/Release/HPEQ_Lab.exe t11 --out <dir> --count 200           # 0 — 237 fixtures
build/Release/HPEQ_HostSmoke.exe --plugin …/HPEQ.vst3 --other …/HPDG.vst3 --t11 <dir> --out reports/m8/host_smoke.json   # 0 — 20/20
HPEQ_Lab bench (A/B: dfd8406 vs M8, alternating, 3 rounds × 2 reps × 1500 blocks)   # reports/m8/cpu/ab_*.json
HPEQ_Lab m7 (T12, both builds)                                   # reports/m8/t12_before, t12_after
HPEQ_MEM_FS=<fs> HPEQ_Lab m7 (both builds, one process per rate)  # reports/m8/memory_private_mb.txt
pluginval.exe --strictness-level N --validate build/HPEQ_artefacts/Release/VST3/HPEQ.vst3   # N = 5..10: 0 — SUCCESS each
powershell -File scripts/package.ps1                             # 0 — dist/HPEQ-0.1.0-beta-win64.zip
```

## 4. CPU (same session, binaries alternated; method of CPU_REPORT)

The new path is **bit-identical** to M7: the M2 (H3 tree + compressor) and M4 (compressor + reverb)
lab renders of both binaries are byte-for-byte equal (`cmp`).

| Bench config (512 / 48 kHz) | p50 before → after | p99 before → after |
|---|---|---|
| tree, 1 node | 66.5 → 26.8 µs (−60 %) | 90.8 → 55.4 µs (−39 %) |
| tree, 5 nodes | 195 → 118 µs (−40 %) | 344 → 204 µs (−41 %) |
| tree, 8 nodes | 306 → 182 µs (−40 %) | 470 → 297 µs (−37 %) |
| compressor, 1 node | 60.5 → 37.1 µs (−39 %) | 147 → 86 µs (−42 %) |
| 8 nodes × 6 compressors | 1112 → 704 µs (−37 %) | 1474 → 1082 µs (−27 %) |
| 8 reverb nodes | 683 → 533 µs (−22 %) | 1096 → 984 µs (−10 %) |
| 24 bells (no tree) | 185 → 177 µs (−4 %, noise) | 328 → 327 µs |
| ceiling: 24 bells + 8 × 6 compressors | 1242 → 911 µs (−27 %) | 1773 → 1411 µs (−20 %) |

T12 at 48 kHz (project engine, p99 of the block time, before → after): 10 × 1 9.1 → 7.4 %, 5 × 5
17.4 → 12.6 %, 10 × 5 31.4 → 24.7 %, 10 × 5 heavy [comp, sat 2x, transient, limiter] 154 → 118 %
(Parallel) and 152 → 110 % (Serial). Full tables: `m8/t12_before/m7_lab.json`, `m8/t12_after/m7_lab.json`.
(The heavy chains still do not run in real time; see §8.)

## 5. Memory (committed private bytes of one prepared engine, fresh process per rate)

| Rate | M7 | M8 |
|---|---|---|
| 44.1 kHz | 100.3 MB | **45.2 MB** |
| 48 kHz | 101.9 MB | **48.0 MB** |
| 96 kHz | 121.1 MB | **79.0 MB** |
| 192 kHz | 159.2 MB | **143.2 MB** |

Per part at 48 kHz: saturator 49 → 20 KB (oversampling in 64-sample sub-blocks), FX node 0.47 →
0.30 MB, layer 4.2 MB before the saturator change. The M7 report's 62 MB was a working-set delta;
the committed figure for the same build is 102 MB (method corrected here).

## 6. Numbers

| Test (threshold fixed before the run) | Result |
|---|---|
| Loudness, EBU Tech 3341 cases 1–5 (synthesized 1 kHz stereo sines) at 44.1 / 48 / 96 kHz, ±0.1 LU | worst deviation **0.031 LU**; M and S on the steady cases within 0.011 LU |
| True peak, fs/4 sine at 45° (samples −3.01 dBFS, true 0 dBTP), +0.2 / −0.4 dB | **+0.111 dBTP** (4x at 44.1/48, 2x at 96); 192 kHz (1x): −2.97, documented |
| True peak of sines 997 Hz … 19 kHz at −6 dBFS | within −6.4 … −5.8 at every rate |
| Meters: 0 allocations; reset request clears I and TP | pass |
| Host transport: no play head, no tempo, stopped, tempo jumping every block, tempo ramp (Delay sync + Monsta) | finite output; synced delay = 60/bpm |
| 1000 random edits (nodes, chains, controls, routing, mode) with audio running | ids unique, sanitize idempotent, 785 undo steps back to the start exactly, redo to the end exactly |
| State precision: 2000 random doubles through XML save/load/save | **exact** |
| Preset compatibility: committed M8 v1 state re-saves identically and renders; the same project as written by M7 loads; a minimal M1-style tree loads with defaults | pass |
| **T11 via the binary** (JUCE VST3 host): 200 random full projects + 36 rate×block items + 1 variable-block item | state identical after load + save **237/237**; latency **237/237**; renders bit-exact to in-process **57/57** (44.1–192 kHz × 32–4096 blocks, random 1–1500 blocks); re-opened instance **57/57** |
| Instance + editor on desktop, 100 cycles; HPDG.vst3 still scans | pass |
| **pluginval v1.0.4** (Tracktion, GitHub release, `pluginval_Windows.zip` SHA-256 `c08e61ce…715ab`), strictness 5, 6, 7, 8, 9, 10 | **SUCCESS** at every level: scan, cold/warm open, editor (also while processing), audio processing, non-releasing processing, state and state restoration, automation, parameter thread safety, background-thread state, buses, fuzz parameters. Steinberg's vst3 validator step skipped (validator not available). Logs: `m8/pluginval/` |
| Release / Debug (CRT allocation hook) / ASan suites | 1025/1025 each; 0 audio-thread allocations incl. loudness meters |

## 7. Package

`dist/HPEQ-0.1.0-beta-win64.zip` (not in git): `HPEQ.vst3` (version 0.1.0, HamloProd), `INSTALL.txt`,
`RELEASE_NOTES.md`; SHA-256 in `dist/…zip.sha256`. Not code-signed (no signing infrastructure).
Release notes: [`docs/release/RELEASE_NOTES_0.1.0-beta.md`](../../release/RELEASE_NOTES_0.1.0-beta.md).

## 8. Gate 8 — acceptance matrix

Every row of `04_ACCEPTANCE_MATRIX.md` is now PASS or BLOCKED with evidence. **BLOCKED:**
Steinberg's vst3 validator (pluginval's step; the validator is part of the VST3 SDK, not built), DAW checks (FL Studio / REAPER,
freeze/render, automation in a host), UBSan/TSan (not available with MSVC). No open high-severity bug
known.

## 9. Findings / limitations

1. **State drifted by one ulp per save/load** for some numbers: JUCE writes doubles with 15–16
   significant digits in some ranges and its parser is not always correctly rounded. The T11 host
   test caught it (1/237). Numbers are now written with `std::to_chars` and read with `std::from_chars`;
   integers in processor parameters stay integers. States written by M7 still load.
2. JUCE's VST3 host stores state as `<VST3PluginState><IComponent>base64…`; the first T11 version fed the
   plugin's raw state and nothing loaded (test bug, not a plugin bug). The test wraps it like a DAW.
3. The first T11 version re-prepared a hosted instance at a larger block size without deactivating it
   and crashed inside the host; DAWs deactivate first, the test now does. In-process, preparing twice
   without release works (tested by the fixtures).
4. Heavy chains (oversampled saturators in many regions × 10 layers) exceed real time; muted layers
   cost full CPU. Candidates: cheaper half-band filters / rational tanh, optional "mute = stop".
5. True peak at 176.4/192 kHz uses no oversampling (BS.1770 allows it); an fs/4 inter-sample peak is
   not seen there.
6. ASan build: CMake fetched JUCE 8.0.12 from GitHub through the project's FetchContent (same tag),
   because `HPEQ_JUCE_DIR` was not set in that shell — a download not agreed beforehand; the normal build
   uses the local checkout.
7. pluginval only exercises host-visible parameters (Output Gain, Bypass); regions and layers are not
   host-automatable, so its fuzzing covers little of the DSP. That part is covered by T11 (237 random
   projects through the binary), the M7 stress test and the unit suites.
8. Not verified: any DAW, mouse gestures of the new controls.

## 10. Next

Owner checks in FL Studio (the zip). Then the UI rework from the owner's feedback (BACKLOG F1–F3: modules on EQ points, module grid with
a back arrow).
