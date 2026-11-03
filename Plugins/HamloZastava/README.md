# HamloZastava 0.2.0

Standalone gate, expander, drum envelope, external-key ducker and rhythmic gate.
QUICK and ADVANCED expose four dedicated roles with Soft/Hard profiles. Legacy
keeps the original engine, programs and project behavior. Black Steel and the
shared preset screen retain their established placement.

- **Clean:** gate/expander, hysteresis, Peak/RMS detection and quiet-detail control.
- **Drum:** onset detection with finite Attack/Body/Tail and retrigger protection.
- **Duck:** external sidechain attenuation and recovery; missing key returns dry.
- **Rhythm:** 16–32 editable levels, host-tempo or free timing.

Detector HP/LP, signed LISTEN KEY, lookahead, stereo link, Mix and Output are
available in ADVANCED. Fixed latency is **5 ms** (240 samples at 48 kHz). Modern
startup and INIT have processing off. Factory Default remains the original
Legacy recipe for compatibility. [Controls](docs/ZASTAVA_CONTROLS.md).

**200 new recipes:** 50 each for Boom Bap, Trap, Techno and DnB. Every genre has
16 Clean, 14 Drum, 8 Duck and 12 Rhythm recipes. The six original recipes and
Default keep host positions 0–6: **207 programs** in total. Each new recipe has a
permanent ID, purpose, material, depth and reference tempo. [Library](docs/GENRE_PRESETS.md).

See the [S5 release report](docs/reports/s5/REPORT.md) for measured acceptance,
calibration provenance, compatibility results and remaining owner FL Studio
listening. Annotated test arrangements and natural excerpts are separate checks;
thresholds still need adapting to a recording. Earlier [S1](docs/reports/s1/REPORT.md),
[S2](docs/reports/s2/REPORT.md), [S3](docs/reports/s3/REPORT.md) and
[S4](docs/reports/s4/REPORT.md) reports preserve intermediate evidence.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ZASTAVA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloZastava_VST3 HamloZastava_Tests HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_Tests.exe
```

VST bundle: `build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3`.
Release copy, ZIP and SHA256 manifest: `dist/`. No installer or installed plug-in
replacement in this stage. Copy the complete VST3 directory when installing.
The optional original M0 reference requires local archived sources and
`-DHAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON`; normal builds do not need them.

All 78 parameter definitions/order, VST class IDs, XML state tag and original
compact Legacy states are retained. Modern projects restore the selected recipe
ID, settings, view and tab. [Scope](docs/ZASTAVA_V2_SCOPE.md),
[measurement methods](docs/S5_METHODS.md), [handoff](docs/NEXT_STAGE.md).
Product UI/documentation are English. No HPEQ/subtree/push changes.
