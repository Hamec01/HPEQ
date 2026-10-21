# M4 — Compressor (T06) and Reverb as wet-only send (T07), tails, PDC

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M4 — compressor + reverb on FX nodes |
| Date | 2026-10-21 |
| Base | `ab1be6e` (M3); code and report in the M4 commit on `feature/hpeq-m0-m1` |
| Platform | Windows 11 x64, i5-10300H (laptop), MSVC 19.50.35730, JUCE 8.0.12, 48 kHz |

## 2. Changed files

| Path | Why |
|---|---|
| `Source/DSP/ReverbProcessor.*` | New wet-only FDN reverb (8 lines, Householder, Jot absorption, pre-delay, width, wet low cut) |
| `Source/DSP/FxNodeProcessor.*` | Send slots (`s_k = s + mix·W(inGate·s)`), reverb pool, bypass with spill-over (input gates), starvation flag |
| `Source/DSP/LayerEngine.*` | Owns the per-layer `ReverbPool` (8 instances, allocated in `prepare`) |
| `Source/DSP/LayerSnapshot.*` | Reverb parameters (pre-delay, decay, HF ratio, size, width, low cut) |
| `Source/Plugin/FxPluginProcessor.*` | `getTailLengthSeconds()` = longest active reverb (pre-delay + T60) |
| `Tests/Unit/DynamicsReverbTests.cpp` | T06 vs an independent law model, ratio/attack/release, T07 RT60/damping/width, wet-only, spill-over, pool, T10/allocations, stability, PDC/tail |
| `Tests/Tools/LabM4.cpp` | `HPEQ_Lab m4`: compressor curves, EDC plots, octave-band RT60, IR and drum WAVs, CPU |

## 3. Implemented vs scaffolding

**Implemented and processed in FX nodes:** Compressor (insert), Reverb (wet-only send), Gain.
Send/insert slots mix freely in one chain (D2). Bypass keeps tails:

| Action | Behaviour |
|---|---|
| Slot bypass (insert) | output mix fades out (20 ms) |
| Slot bypass (send) | the slot's **input** closes (20 ms); the tail rings out; the reverb returns to the pool after 50 ms below −120 dBFS |
| Node bypass | the node's **input** closes (20 ms); inserts give exactly 0 for 0; sends ring out; then the node and its tree edges retire |
| Node mix 0 | removes the contribution immediately and exactly (a mix setting, not a bypass) |
| Plugin bypass | 20 ms crossfade to the exact dry signal (tails are cut: bypass means dry) |

**Not yet:** Delay, Limiter and the rest (M6); wet output "Restricted" band-limit (the reverb is linear,
so its wet stays within the band it is fed; the option matters for non-linear FX in M6); DAW tests.

**Limits (documented):** 8 reverbs per layer. A 9th reverb slot stays silent and reports
`slotStarved()` (for the UI in M5). Pool memory ≈ 2 MB per layer at 48 kHz, ≈ 8 MB at 192 kHz.

## 4. Commands and exit codes

```bash
cmake --build build --config Release                         # 0, no warnings
build/Release/HPEQ_Tests.exe     # 0 — 698 passed, 0 failed
build/Debug/HPEQ_Tests.exe       # 0 — 698 passed, 0 failed (CRT hook: malloc + new)
build/Release/HPEQ_Lab.exe m4 --out docs/fx-addon/reports/m4         # 0
build/Release/HPEQ_Lab.exe --out <tmp>                               # 0, M2 core unchanged
build/Release/HPEQ_HostSmoke.exe --plugin …/HPEQ.vst3 --other …/HPDG.vst3   # 0 — 16/16
```

## 5. Numbers

### 5.1 T06 — compressor against its documented law

| Check (threshold fixed before the run) | Result |
|---|---|
| Sample-by-sample vs an independent model written from the documented law: 4 settings (incl. knee 12 dB, R 100, makeup ±) × burst / drums / unbalanced stereo × linked / dual-mono | worst **−147.0 dB** (≤ −120) |
| Static slope above threshold for R 2 / 4 / 10 (DC levels) | 1/R within 1e-4 |
| Attack 63 % (M2) / release 36.8 %, 200 ms | 10.0 ms exactly / **9600 samples = 200.0 ms** (±1 sample) |
| Linked vs Dual Mono (M3) | quiet channel −43.02 dual, −62.95 linked |
| HPDG baseline (M0): displayed 10/120 ms → measured 2/23 ms | HPEQ: the displayed time constants are the measured ones |

Plots: `m4/plots/compressor_static_curve.svg` (knee 0/6/12 dB), `compressor_gain_envelope.svg`.

### 5.2 T07 — reverb

RT60 from the Schroeder energy-decay curve (T20 × 3), impulse response, 48 kHz:

| Setting | broadband | 125 Hz | 500 Hz | 2 kHz | 8 kHz | L/R corr. |
|---|---|---|---|---|---|---|
| 0.5 s, HF 1.0 | 0.501 | 0.495 | 0.506 | 0.499 | 0.500 | 0.04 |
| 1.8 s, HF 1.0 | 1.788 | 1.819 | 1.823 | 1.785 | 1.783 | 0.01 |
| 1.8 s, HF 0.5 | 1.510 | 1.819 | 1.819 | 1.742 | 1.395 | 0.01 |
| 1.8 s, HF 0.2 | 1.500 | 1.818 | 1.795 | 1.515 | 0.740 | 0.02 |
| 4.0 s, HF 1.0 | 3.995 | 3.948 | 3.973 | 4.002 | 4.006 | 0.00 |
| 4.0 s, HF 0.2 | 3.356 | 3.946 | 3.935 | 3.605 | 1.928 | 0.01 |

| Check (threshold fixed) | Result |
|---|---|
| RT60 within ±15 % for 0.5 / 1.8 / 4 s × size 0.5 / 1 / 1.5 | 0.495…0.501, 1.788…1.797, 3.990…3.995 s |
| Damping 0.3: 500 Hz T60 within ±20 %, 8 kHz < 0.8 × 500 Hz | 2.004 s / 1.177 s |
| Width 1: tail |corr| < 0.3; width 0: corr = 1 | −0.072…0.012 / 1.000000 |
| Wet-only: node output before the first possible reflection = passive render | **bit-exact** (no dry doubling, no added latency) |
| Bypass (slot and node) after the input has stopped | tail **bit-identical** to the un-bypassed render; reverb back in the pool; node leaves the path |
| Bypass while the input plays | remaining tail RT60 0.978 s (setting 1.0, ±20 %) |
| Stability: T60 20 s, no damping, size 1.5, 30 s noise; 20k mutations of reverb/compressor parameters | finite, bounded, 0 allocations |
| 9 reverb slots on one layer | 8 run, the 9th reports starvation |

### 5.3 The M4 scenario (compressor 250–700 Hz + reverb 3–8 kHz + an overlapping node, drums)

GR 14.19 dB, 2 reverbs running, finite; render `m4/wav/drums_comp250-700_reverb3k-8k.wav`
(input −18.3 dBFS RMS, output −18.5 dBFS RMS, peak −3.3 dBFS).

### 5.4 T10, allocations, PDC

- Blocks 1 / 64 / 127 / 512 with compressor + two reverbs: **bit-exact**; 0 allocations.
- Latency **0** for every configuration (EQ 96 dB/oct + compressor + reverb): every processor is
  zero-latency, dry and wet need no compensation (`setLatencySamples(0)`).
- Host tail: pre-delay 40 ms + T60 3.5 s → `getTailLengthSeconds()` = **3.54 s**; 0 when the node is off.

### 5.5 CPU (Release, 48 kHz stereo, block 512, budget 10 667 µs; two lab runs, laptop)

| Configuration | p50 | p99 |
|---|---|---|
| 1 reverb node | 104 / 201 µs | 239 / 769 µs |
| M4 scenario (compressor node + reverb node) | 172 / 319 µs | 426 / 742 µs |
| 8 reverb nodes (pool full) | 1 363 / 2 227 µs | **4 799 / 3 015 µs (45 % / 28 %)** |
| Reverb acquire + clear (audio thread, 48 kHz) | 4.5 / 8.4 µs | |

The run-to-run spread on this laptop is up to 2× (turbo/thermal); numbers are comparable only within
one run. The p99 of the 8-reverb case is far above its p50 and is not explained yet (T12/M8 item).

## 6. Artifacts

`reports/m4/m4_lab.json`, `reports/m4/plots/{compressor_static_curve, compressor_gain_envelope, reverb_edc}.svg`,
`reports/m4/wav/` (local: drums input and scenario render, reverb IRs), `reports/m4/host_smoke.json`, `reports/m4_logs/`.

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| T06 | **PASS** | §5.1 |
| T07 | **PASS** (engine/offline); DAW **BLOCKED** | §5.2 |
| PDC | **PASS** (latency 0 by construction, wet-only alignment bit-exact); DAW **BLOCKED** | §5.4 |
| T10 / allocations with reverb | **PASS** | §5.4 |
| Gate 4 | **PASS** — T06/T07; kick/snare/full-mix behaviour on the synthetic drum fixture; mono/stereo kept | |

Debug run with the CRT allocation hook: 698/698, **0 audio-thread allocations** in every probed test, including reverb acquire/release (`reports/m4_logs/unit_debug.log`).

## 8. Existing HPDG

Untouched; HPDG.vst3 still scans next to HPEQ (16/16).

## 9. Limitations / risks / findings

1. **Reverb bug found by T07:** RT60 was ~13 % short at every setting (fractional delay lengths read
   with linear interpolation low-pass every loop pass). Lengths are now whole samples once settled:
   errors < 1 %.
2. **Test fixes, disclosed:** (a) release check used 0.368 instead of e⁻¹ (4-sample offset);
   (b) the first bypass criteria ("tail within 30 dB 0.3 s later", "no 3 dB step between 2.5 ms
   windows") were unsuitable for a noise-like decaying tail and failed on correct behaviour; they were
   replaced by stricter ones (bit-identical tail when bypassed after the input; RT60 of the remaining
   tail within ±20 %). Both versions are in the git history.
3. **Bug found while writing T07:** a bypassed node never stopped while it held a reverb (the slot's
   own input gate stayed open). The slot now also treats a closed node input as closed.
4. **Headroom:** long decays build up level (T60 20 s on −12 dBFS noise: wet +2.1 dBFS RMS, peak
   +16 dBFS). No hidden limiter by design (01 §4); metering/output trim in M5, limiter in M6.
5. CPU and the unexplained p99 of many reverbs (§5.5); reverb memory per layer at 192 kHz (~8 MB).

## 10. Next

**M5** — the one-layer UI: spectrum workspace with real EQ curve and FX regions, real waveform
input/processed/delta, node and slot inspector (incl. GR meters, pool starvation), Freeze, undo, A/B.
