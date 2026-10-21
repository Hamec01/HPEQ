# M1 — Standalone VST3 insert skeleton

## 1. Milestone / Date / SHA / Branch / Platform

| | |
|---|---|
| Milestone | M1 — own insert-FX VST3 |
| Date | 2026-10-21 |
| Code commit | `38b41bf11533a7f700384347145d61b5e2e35d0b` (branch `feature/hpeq-m0-m1`) |
| Platform | Windows 11 x64, i5-10300H, MSVC 19.50.35730 (VS 18 2026), JUCE 8.0.12 |
| Binary | `build/HPEQ_artefacts/Release/VST3/HPEQ.vst3` (7.3 MB, SHA-256 `3548aaf0…89db72`) |

## 2. Changed files

| Path | Why |
|---|---|
| `CMakeLists.txt` | `juce_add_plugin(HPEQ …)`: `IS_SYNTH FALSE`, no MIDI, `PLUGIN_MANUFACTURER_CODE BBLB` (as HPDG), `PLUGIN_CODE HpEq` (HPDG: `BBG1`), bundle `com.hamloprod.hpeq`, VST3 category `Fx|EQ`; test/tool targets |
| `Source/Core/FxProjectState.*` | Project model + schema v1: 10 layers, EQ nodes (9 shapes, slopes 6…96 stored), FX nodes with range and a **1–6 slot chain** (D2), enums stored by name, sanitizer with count limits |
| `Source/Plugin/FxParameterContract.h` | Permanent host parameter IDs: `outputGain` (−24…+24 dB), `bypass` |
| `Source/Plugin/FxPluginProcessor.*` | Pass-through, 20 ms multiplicative gain ramp, host bypass mapped to `bypass`, lock-free block meters, state save/restore |
| `Source/Plugin/FxPluginEditor.*` | Sketch frame: header, meters, 10 layer buttons (2×5), workspace + inspector areas labelled "not implemented yet (M5)", output gain, bypass |
| `Tests/Unit/*` | Contract, state and harness tests (`HPEQ_Tests`) |
| `Tests/Tools/HostSmokeMain.cpp` | Loads the built .vst3 via JUCE's VST3 host |

## 3. Implemented vs scaffolding

**Real:** audio in → out, output gain, bypass, mono/mono and stereo/stereo buses, state with the full
v1 model, meters (block peak / RMS of real input and output PCM), editor open/close.

**Scaffolding only (stated in the UI):** layers 2–10 are disabled buttons ("not yet implemented (M7)");
spectrum workspace and inspector are empty labelled panels; EQ/FX nodes exist in the saved state but
are **not processed**. Latency and tail are 0 because there is no DSP yet.

## 4. Commands and exit codes

```bash
cmake --preset vs2026-x64                                    # 0
cmake --build build --config Release                         # 0, no warnings
cmake --build build --config Debug --target HPEQ_Tests       # 0
build/Release/HPEQ_Tests.exe                                 # 0 — 263 passed, 0 failed
build/Debug/HPEQ_Tests.exe                                   # 0 — 263 passed, 0 failed (CRT alloc hook)
build/Release/HPEQ_HostSmoke.exe --plugin build/HPEQ_artefacts/Release/VST3/HPEQ.vst3 \
    --other C:/DRUMENGINE/build/HPDG_artefacts/Release/VST3/HPDG.vst3 --cycles 100 \
    --out docs/fx-addon/reports/m1/host_smoke.json            # 0 — 16/16
ctest --test-dir build -C Release                            # 2/2 passed
```

Logs: `reports/m1_logs/`. The first host-smoke run reported 1 FAIL ("default bus 2 in / 2 out"): the
check read channel counts from the VST3 scan description, which JUCE leaves at 0/0 for every plugin
(HPDG too). The check was moved to the instantiated plugin's bus layout; both runs are described here
so the change is visible.

## 5. Numbers

| Measurement | Result |
|---|---|
| Unity, default state: 4 rates × mono/stereo × 9 block sizes (1…2048), drum fixture | **72/72 bit-exact** |
| Unity through the VST3 binary, blocks 64/512/2048 | bit-exact |
| Bypass with Output Gain −12 dB | bit-exact |
| Output Gain −6 dB after ramp | error 2.4e-7 dB; via VST3 binary −6.000 dB |
| Gain ramp smoothness (largest per-sample step) | 0.0125 dB |
| Heap allocations in `processBlock` (Debug, CRT hook incl. malloc): all unity cases, gain ramp, blocks 4096 > prepared 64, 10 000 random blocks | **0** |
| T04 scope M1: 100 000 random parameter mutations (incl. out-of-range), 10 000 blocks of random size 1…2048 | no NaN/Inf, peak within the +24 dB bound, 0 allocations |
| Editor cycles | 100/100 in-process (while processing); 100/100 instance + editor on desktop via VST3 binary |
| State save → load (in-process and via binary) | parameters, 10 layers, nodes, 6-slot chain, selected layer restored; renders bit-identical |
| HPEQ vs HPDG identity | UID −1663069447 vs 881007846; identifiers `VST3-HPEQ-2677d61e-9cdf92f9` vs `VST3-HPDG-cc721422-34831ce6`; HPEQ scans as effect `Fx|EQ`, HPDG as instrument |

CPU is not reported: the only work is a gain multiply; it becomes meaningful with M2 DSP.

## 6. Artifacts

`reports/m1/host_smoke.json`, `reports/m1_logs/unit_release.log`, `unit_debug.log`, `host_smoke.log`,
`ctest_release.log`.

## 7. Acceptance matrix

| ID | Status | Evidence |
|---|---|---|
| M1-A | **PASS** | 72/72 bit-exact in-process; bit-exact via binary |
| M1-B | **PASS** (JUCE VST3 host) / pluginval **BLOCKED** | scan, unique UID, effect category via `HPEQ_HostSmoke`; pluginval is not installed |
| M1-C | **PASS** (in-process + JUCE host) / DAW **BLOCKED** | 100 editor cycles both ways; FL Studio/REAPER not run by the agent |
| STATE | **PASS for M1 scope** | parameters + project + sound restored; automation gestures and later DSP state come with M2+ |
| T04 | **PASS for M1 scope** | 100k mutations of the M1 parameters; re-run when DSP parameters exist |
| HPDG | **PASS** | HPDG untouched; HPDG.vst3 still scans, IDs differ |
| T11 | **BLOCKED** | needs pluginval (strictness ≥ 5) and a DAW |

## 8. Existing HPDG

No change to `C:\DRUMENGINE`. The HPDG.vst3 binary (2026-10-20 build) loads in the same host process
next to HPEQ; identifiers do not collide.

## 9. Limitations / risks / decisions needed

1. **pluginval** is not on this machine. Running it needs a download of the official release from
   `github.com/Tracktion/pluginval` — asking the owner before downloading.
2. No DAW test by the agent. Manual check suggested: load `HPEQ.vst3` on an audio track in FL Studio,
   confirm it appears as an effect, passes audio, gain/bypass automate, project reopens.
3. Mono→stereo is deliberately rejected until an upmix rule is defined (01 §10).
4. The model allows 10 × (24 EQ + 8 × 6 FX) objects; host-automatable parameter slots for them are an
   M2/M3 design task (01 §10), not decided here.

## 10. Next

Gate 1 is met (separate .vst3 really builds, unity proven, host-stable in the tests run).
**M2** — one PRE-EQ + one frequency FX node prototype (H1 delta bands vs H2 crossovers, T01/T02/T04/T10),
with the chain semantics of D2 tested for K = 1 first. The HPDG baselines from M0 are the bar for the
rewritten processors in M3 (EQ), M4 (compressor) and M6 (Monsta and the rest).
