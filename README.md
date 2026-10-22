# HPEQ

Frequency-layer FX plugin (VST3 audio insert effect, Windows x64). Working name; the final product
name is not decided yet.

Each layer runs **PRE-EQ → frequency-range FX nodes (up to 6 processors per node) → delta mix**, with
up to 10 layers. The spec, roadmap and acceptance matrix are in [`docs/fx-addon/`](docs/fx-addon/);
owner decisions that change the spec are in [`DECISIONS.md`](docs/fx-addon/DECISIONS.md).

## Status

| Milestone | State | Report |
|---|---|---|
| M0 audit, measurement harness, baseline | done | [M0_BASELINE](docs/fx-addon/reports/M0_BASELINE.md) |
| M1 VST3 insert skeleton | done (pluginval not run) | [M1_REPORT](docs/fx-addon/reports/M1_REPORT.md) |
| M2 frequency-node DSP prototype (H3 core) | done, Gate 2 passed | [M2_REPORT](docs/fx-addon/reports/M2_REPORT.md) |
| M3 PRE-EQ shapes, slopes 6-96 dB/oct, channel modes, M/S | done, Gate 3 passed | [M3_REPORT](docs/fx-addon/reports/M3_REPORT.md) |
| M4 Compressor (T06), Reverb wet-only send (T07), tails, PDC | done, Gate 4 passed | [M4_REPORT](docs/fx-addon/reports/M4_REPORT.md) |
| CPU pass (O(N) tree, compressor, reverb) | done, -46...-60 % on heavy layers | [CPU_REPORT](docs/fx-addon/reports/CPU_REPORT.md) |
| M5 one-layer UI, real waveform/spectrum, undo, A/B | done, Gate 5 passed | [M5_REPORT](docs/fx-addon/reports/M5_REPORT.md) |
| M6 Saturator, Transient, Delay, Monsta FX, Stereo, Gate, Limiter; PDC | done, Gate 6 passed | [M6_REPORT](docs/fx-addon/reports/M6_REPORT.md) |
| M7 10 layers, Parallel / Serial, layer controls | done, Gate 7 passed | [M7_REPORT](docs/fx-addon/reports/M7_REPORT.md) |
| M8 validation, loudness / true peak, CPU and memory pass, beta 0.1.0 | done; pluginval 5–10 pass; DAW checks BLOCKED | [M8_REPORT](docs/fx-addon/reports/M8_REPORT.md) |
| D6 owner feedback: modules on EQ points, module grid inspector | done | [D6_REPORT](docs/fx-addon/reports/D6_REPORT.md) |
| Module pass 1: compressor (Peak/RMS, auto makeup, Clean/Punch/Glue, vintage page), calmer waveform | done | [COMPRESSOR_PASS_REPORT](docs/fx-addon/reports/COMPRESSOR_PASS_REPORT.md) |

Ten layers (Parallel delta or Serial), each PRE-EQ (9 shapes, cuts 6-96 dB/oct, L/R/M/S) then up to 8
frequency-range FX regions with chains of up to 6 processors; the frequency core is H3 (DECISIONS D4),
layer semantics D5. Beta package: `scripts/package.ps1` -> `dist/HPEQ-<version>-beta-win64.zip`.

## Build

Needs CMake ≥ 3.22 and Visual Studio 2026 (or adjust the preset generator). JUCE 8.0.12 is taken from
`HPEQ_JUCE_DIR` if set, otherwise fetched.

```bash
set HPEQ_JUCE_DIR=C:/DRUMENGINE/JUCE
set HPEQ_HPDG_SOURCE_DIR=C:/DRUMENGINE/Source
cmake --preset vs2026-x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

`HPEQ_HPDG_SOURCE_DIR` is optional: it lets `HPEQ_Harness` measure HPDG's DSP blocks read-only.

## Tools

| Executable | Purpose |
|---|---|
| `HPEQ_Tests` | Unit tests: plugin contract, state schema, harness. Run Debug for full RT-allocation coverage |
| `HPEQ_Harness selfcheck / fixtures / baseline` | M0 measurement harness: fixtures (WAV + SHA-256 manifest), metrics JSON |
| `HPEQ_Lab --out <dir>` | M2 frequency-core experiment: H1 / H1-LR8 / H2 / H3-tree / H3-engine, JSON + SVG + WAV |
| `HPEQ_Lab bench [--cold]` | CPU benchmark of fixed engine configurations (warm-up, repetitions, spikes); compare binaries interleaved |
| `HPEQ_Lab ui --out <dir>` | Renders the real editor to PNG after real audio; UI frame cost |
| `HPEQ_HostSmoke --plugin <HPEQ.vst3> [--other <HPDG.vst3>]` | Loads the built .vst3 through JUCE's VST3 host: scan, unity, state, 100 instance/editor cycles |
