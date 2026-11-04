# HamloFader 0.2.0

Windows x64 VST3: precision Trim, deliberate Match, Natural/Fast Ride and
editable rhythmic Motion. QUICK shows the principal controls; ADVANCED exposes
channel routing/alignment, Match limits, Ride timing and Motion detail. LEGACY
retains the original Black Steel panel. Presets stay at the upper left.

200 new recipes: Boom Bap, Trap, Techno and DnB,50 each, appended after the
original programs.208 host programs total. Genre/group filters browse without
changing audio. [Measured preset index](docs/GENRE_PRESETS.md).

A new instance starts in neutral modern Trim. All eight original factory
programs retain their numbers and select Legacy, including Default. Old projects
select Legacy and clear every new control. XML root, original parameter prefix,
VST3 class IDs and zero-sample PDC are preserved. Alignment is an audible effect.

Match separates Learn, Stop/proposal and deliberate Apply. Applied gain is saved
in the project; learning/history are transient. Ride has bounded boost/cut and
activity protection. Motion supports 32 editable points, shapes, free/host time,
phase and independent right-channel offset. RMS, LUFS and true-peak readings
use a bounded atomic bridge; UI never accesses the kernel directly.

Match: choose RMS/LUFS and target, START LEARN, play representative audio,
STOP/PROPOSE, inspect the proposed gain, then APPLY. Stop does not apply gain.
The Peak Guard limits a proposal; it is not an output limiter. Later material
or changed settings can exceed that ceiling. Reset clears histories, not held
Match gain. Ride follows active input and returns to unity in pauses. Motion
has2..32editable cyclic points, custom/sine/triangle shapes and stereo offsets.

RMS/LUFS M/S/I readiness is explicit; WAIT is not a valid zero reading. The
integrating loudness store holds72000windows(about2hours), then displays FULL
until Reset. This is not a complete certified EBU Mode meter.

Release checks:200/200 calibration and independent compiled-bank rerenders,
200/200 independent8x peak checks;224 host checks;678 original-program Legacy
renders exact;native UI/state/library tests;pluginval5..10 andSteinberg537/537.
The audio kernel is unchanged from accepted F3. Earlier281DSP/59kernel proofs
and the Motion regression evidence remain historical. [Release report](docs/reports/f5/REPORT.md).

VST3,portableZIP,SHA256:`dist/HamloFader-0.2.0`. No new installer or installed-copy
replacement. Manual loading/listening in FL Studio remains for the owner;12
private dry/processed audition pairs are kept locally, not embedded in the VST.
[Design](docs/HAMLOFADER_V2.md),[host contracts](docs/F3_METHODS.md),[handoff](docs/NEXT_STAGE.md).

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_FADER_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloFader_VST3 HamloFader_Tests -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
```

Bundle: `build/HamloFader_artefacts/Release/VST3/HamloFader.vst3`.
Full local audit replay:`Tools/verify_f5.py` (requires retained ignored local
captures/delivery). F1..F4 receipts apply to their frozen source snapshots;
do not edit them to accept later source. Normal builds/tests do not need owner
audio or historical captures. The F5 material pass is opt-in as documented in
the handoff. No installed copy was replaced.
