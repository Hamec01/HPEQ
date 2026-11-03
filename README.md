# HamloZastava

Standalone gate/expander/duck/rhythm processor, internal version0.1.0 with S4
modern editor and key audition complete. Production uses thirteen owned units, without LayerEngine, project
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

QUICK and ADVANCED expose the four roles, precision controls,32editable steps,
contextual help, neutral INIT and a separate Legacy page. LISTEN KEY auditions
the signed filtered key at the same fixed delay. The78thparameter appends after
the old77; pages and settings restore in modern projects. [Controls guide](docs/ZASTAVA_CONTROLS.md).

S4passes465native,283host and275kernel checks, with all66listen-off PCM renders
exact to the accepted old kernel. Legacy229primary+458restored cases still
match. Fresh pluginval5..10/Steinberg537pass and15screenshots were inspected.
[S4 report](docs/reports/s4/REPORT.md). The200genre recipes remain; this is not
the final V2release. Earlier S2owner-material renders are historical evidence,
separate from the [current extended kernel](docs/S4_METHODS.md).
[Scope](docs/ZASTAVA_V2_SCOPE.md), [handoff](docs/NEXT_STAGE.md).

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ZASTAVA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloZastava_VST3 HamloZastava_Tests HamloZastava_S3HostCurrent HamloZastava_S3LegacyCurrent HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_Tests.exe
```

Bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.
No installer or installed plug-in change in S4. Frozen M0 lab is optional,
EXCLUDE_FROM_ALL and requires local captured sources plus
-DHAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON. Normal builds do not need that archive.
Product UI/documentation are English; chat with the owner is Russian.
Working rules:AGENTS.md; historical [M0 report](docs/reports/M0_EXTRACTION_REPORT.md).

Kernel regression laboratory (captures refuse to overwrite existing runs):

```powershell
cmake --build build --config Release --target HamloZastava_V2Lab HamloZastava_V2Render -- /m:2 /v:minimal
build/Release/HamloZastava_V2Lab.exe out/s4/NEW_run
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_zastava_s2.py out/s4/NEW_run out/s4/NEW_report.json
```

Captures refuse to overwrite existing runs. Compiled source/method and raw cache
hashes must match. The optional archived S2 neutral control is excluded from
normal builds and enabled with `-DHAMLO_ZASTAVA_BUILD_S2_REFERENCE=ON`.
