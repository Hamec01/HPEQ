# HamloFader

Standalone Windows x64 VST3 gain utility, internal0.1.0. F1isolation is complete:
production has9owned units and no LayerEngine/project model/other effects.
Original gain, polarity, delay, mute, RMS meters, one-shot MATCH and mix/bypass
behavior remain. Eight parameters and eight host programs retain their original
contracts; reported latency is0samples. Delay0–20ms is an audible alignment effect.

Paired original/current lab:226primary cases plus452actual restored observations,
678exact PCM outputs, zero measured callback allocations. Original native suite:
49/49. pluginval5..10and Steinberg537/537pass. Shared UI source and final repeat
screenshots match. [F1report](docs/reports/f1/REPORT.md).

**Approved V2, still to implement:** separate Trim, Match, Ride and Motion modes;
Ride Natural/Fast with adjustable correction range. Modern metering/learning,
precise channels, editable volume curves, host/state/UI and200newrecipes follow.
No finished V2or FL Studio listening approval is claimed.
[Audit/scope](docs/FADER_V2_AUDIT.md), [handoff](docs/NEXT_STAGE.md).

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_FADER_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloFader_VST3 HamloFader_Tests HamloFader_F1Lab HamloFader_Snapshot -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_f1.py
```

VST bundle:build/HamloFader_artefacts/Release/VST3/HamloFader.vst3.
Frozen reference captures remain inout/f1/reference; new report paths must be
unused. The archived original source/bundle/tool are local audit evidence, not
a build dependency. Normal builds do not need HPEQ sources beyond the JUCE path.
No installed plug-in replacement, installer, push or subtree in this stage.
