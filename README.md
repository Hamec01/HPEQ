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
| M2 frequency-node DSP prototype | next | — |

The plugin currently passes audio through unchanged (output gain + bypass only). No EQ or FX
processing exists yet.

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
| `HPEQ_HostSmoke --plugin <HPEQ.vst3> [--other <HPDG.vst3>]` | Loads the built .vst3 through JUCE's VST3 host: scan, unity, state, 100 instance/editor cycles |
