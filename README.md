# HamloEcho

HamloProd's standalone delay, part of the Hamlo plug-in line. The Echo-only Legacy processor now replaces
the original full HPEQ module stack. All 160 archived M0 renders remain bit-identical, including automation;
the original parameter contract, eight programs, state tag and zero-sample latency are preserved.

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ECHO_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloEcho_VST3`, `HamloEcho_Tests`, `HamloEcho_Lab`, `HamloEcho_Snapshot`.
Current version remains 0.1.0; the planned 0.2.0 musical features are not implemented or released yet.

A separate `HamloEcho_PrototypeLab` target now tests eight pitched reflections and unpitched Reverse.
Pitch accuracy, timing, block invariance, memory and CPU are measured; short-attack preservation and pitched onset
remain open design work. The prototype is not connected to the VST3. See the [prototype report](docs/reports/prototype/PROTOTYPE_REPORT.md).

Verified extraction: 590 test expectations (including machine-local archived states), pluginval 5..10,
and Steinberg validator 537/537. See [extraction report](docs/reports/v2/LEGACY_EXTRACTION_REPORT.md),
[V2 design](docs/HAMLOECHO_V2.md), and [handoff](docs/NEXT_STAGE.md).

The raw M0 reference archive lives under ignored `out/m0/`; keep it intact. On a source-only checkout,
archive-dependent state tests explicitly report SKIP. Original DSP provenance is in [SYNC_WITH_HPEQ](docs/SYNC_WITH_HPEQ.md).
Installer tooling is in `Installer/`; packaging 0.2.0 follows completion of the new DSP/UI/library and release checks.
