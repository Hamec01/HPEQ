# HamloZastava

Standalone gate/expander/duck processor, version0.1.0 with S1 isolation complete.
Production uses ten Zastava-owned translation units, without LayerEngine, project
state or other effect/panel modules. Original15parameters+bypass, seven programs,
state/class identity and5msPDC(240samples at48k) are preserved.

Verified:229primary PCM/input-state cases plus458fresh/edited restored-state
observations match actual M0 exactly;127native checks, zero prepared audio
allocations, pluginval5..10 and Steinberg537/537. Existing Black Steel and shared
preset screen remain. See [S1 report](docs/reports/s1/REPORT.md) and its disclosed
method corrections, including normalized-readback quirks of the original M0.

V2 is approved but not implemented yet: natural cleanup, drum-tail control,
external-key ducking and rhythm, with separate soft/hard profiles, QUICK/ADVANCED
and200genre recipes. [Scope](docs/ZASTAVA_V2_SCOPE.md), [handoff](docs/NEXT_STAGE.md).
This isolation build does not claim new detection/bleed/tail quality.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ZASTAVA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloZastava_VST3 HamloZastava_Tests HamloZastava_CompatLab HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_Tests.exe
```

Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.
No installer or installed plug-in change in S1. Frozen M0 lab is optional,
EXCLUDE_FROM_ALL and requires local captured sources plus
-DHAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON. Normal builds do not need that archive.
Product UI/documentation are English; chat with the owner is Russian.
Working rules:AGENTS.md; historical [M0 report](docs/reports/M0_EXTRACTION_REPORT.md).
