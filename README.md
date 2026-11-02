# HamloZastava

Standalone gate/expander/duck processor, internal version0.1.0 with S3 host
integration complete. Production uses twelve Zastava-owned translation units, without LayerEngine, project
state or other effect/panel modules. Original15parameters+bypass, seven programs,
state/class identity and5msPDC(240samples at48k) are preserved.

At S1,229primary PCM/input-state cases plus458fresh/edited restored-state
observations match actual M0 exactly;127native checks, zero prepared audio
allocations, pluginval5..10 and Steinberg537/537. Existing Black Steel and shared
preset screen remain. See [S1 report](docs/reports/s1/REPORT.md) and its disclosed
method corrections, including normalized-readback quirks of the original M0.

The VST now connects Clean, Drum, Duck and Rhythm with separate Soft/Hard
profiles, detector/key filters, lookahead, mono/stereo external key and fixed
5msPDC. The new neutral startup is Clean with processing off. Old seven host
programs retain their Legacy recipes. New parameters append after bypass;
modern state/project restoration and host clock/bypass routing are verified.
283host and219native checks pass, with229M0matches and fresh pluginval5..10 /
Steinberg537/537. [S3 report and limits](docs/reports/s3/REPORT.md).

The editor is still the old Legacy panel; it does not yet control new modes.
QUICK/ADVANCED, detector audition and200genre recipes remain. This is an internal
integration build, not a finished V2 release. The unchanged S2 kernel's offline
275checks/five-minute stress and eight owner-material A/B renders are documented
separately in [S2 measurements](docs/reports/s2/REPORT.md).
[Scope](docs/ZASTAVA_V2_SCOPE.md), [handoff](docs/NEXT_STAGE.md).

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ZASTAVA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloZastava_VST3 HamloZastava_Tests HamloZastava_CompatLab HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_Tests.exe
```

Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.
No installer or installed plug-in change in S3. Frozen M0 lab is optional,
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
