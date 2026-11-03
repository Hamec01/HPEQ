# HamloFader

Windows x64 VST3, internal version 0.1.0. F3 host integration is complete:
Trim, Match, Ride (Natural/Fast) and Motion now run in the AudioProcessor.
The original Black Steel interface remains; the new QUICK/ADVANCED interface
is the next stage. Modern controls are currently exposed as host parameters.
This is a development build, not the completed 0.2.0 release.

A new instance starts in neutral modern Trim. All eight original factory
programs retain their numbers and select Legacy, including Default. Old projects
select Legacy and clear every new control. XML root, original parameter prefix,
VST3 class IDs and zero-sample PDC are preserved. Alignment is an audible effect.

Match separates Learn, Stop/proposal and deliberate Apply. Applied gain is saved
in the project; learning/history are transient. Ride has bounded boost/cut and
activity protection. Motion supports 32 editable points, shapes, free/host time,
phase and independent right-channel offset. RMS, LUFS and true-peak readings
use a bounded atomic bridge; UI never accesses the kernel directly.

F3 acceptance: 224 host checks, 678 exact Legacy renders, 281 DSP laboratory
checks, 59 kernel unit checks and 49 original native expectations pass.
pluginval levels 5–10 and Steinberg 537/537 pass on the same final DLL.
A newly exposed Motion first-block defect was fixed; the strengthened test
rejects the saved old kernel. [F3 report](docs/reports/f3/REPORT.md).

Next: modern UI, 200 meaningful new presets (208 programs in total), music
calibration, owner listening in FL Studio and release packaging. None of those
are claimed complete. [Design](docs/HAMLOFADER_V2.md),
[host contracts](docs/F3_METHODS.md), [handoff](docs/NEXT_STAGE.md).

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_FADER_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloFader_VST3 HamloFader_Tests -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
```

Bundle: `build/HamloFader_artefacts/Release/VST3/HamloFader.vst3`.
Full local audit replay: `Tools/verify_f3.py` (requires retained `out/f3` captures
and tested binaries). Frozen F1/F2 whole-shipping guards describe those earlier
stages and intentionally no longer apply to the changed host/kernel. Their raw
results remain archived. Normal builds do not require audit archives or HPEQ
source beyond the read-only JUCE path. No installed copy was replaced.
