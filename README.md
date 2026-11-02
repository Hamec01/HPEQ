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

The S2 offline V2 kernel implements Clean, Drum, Duck and Rhythm with separate
Soft/Hard profiles, detector/key filters, lookahead and exact fixed PDC.
Its paired laboratory passes 275 checks across 66 cases, including block
determinism and a five-minute stress; eight owner-material A/B renders are
prepared. [S2 measurements and limits](docs/reports/s2/REPORT.md).
The production VST remains the accepted S1 Legacy build. V2 host/state/sidechain
integration, QUICK/ADVANCED and 200 genre recipes are still required.
[Scope](docs/ZASTAVA_V2_SCOPE.md), [handoff](docs/NEXT_STAGE.md).

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

Build only the offline kernel laboratory for S2 work:

```powershell
cmake --build build --config Release --target HamloZastava_V2Lab HamloZastava_V2Render -- /m:2 /v:minimal
build/Release/HamloZastava_V2Lab.exe out/s2/NEW_run
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_zastava_s2.py out/s2/NEW_run out/s2/NEW_report.json
```

Captures refuse to overwrite existing runs. Compiled source/method and raw cache
hashes must match. The optional archived S2 neutral control is excluded from
normal builds and enabled with `-DHAMLO_ZASTAVA_BUILD_S2_REFERENCE=ON`.
