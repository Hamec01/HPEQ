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

Layer 1 processes the full PRE-EQ (9 shapes, cuts 6-96 dB/oct, L/R/M/S) and FX nodes (Gain, Compressor, Reverb send) from saved state; there is no
node-editing UI yet, and the frequency core is H3 (DECISIONS D4).

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
| `HPEQ_HostSmoke --plugin <HPEQ.vst3> [--other <HPDG.vst3>]` | Loads the built .vst3 through JUCE's VST3 host: scan, unity, state, 100 instance/editor cycles |
