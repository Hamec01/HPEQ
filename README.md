# HamloShirota

HamloProd's standalone stereo and spatial processor. The S4 development VST now
includes **Legacy / Stereo-Space / Binaural**, frequency-dependent width, three
mono-to-stereo generators, three early spatial fields and editable 16-node
motion paths. QUICK exposes width/space/travel; STEREO, SPACE and MOTION provide
numerical detail. LEGACY preserves old projects. Owner reference: AKG K712.

Current version is **0.1.0 development**, with seven original host programs.
The 200 genre presets, calibration, FL/K712 listening and final 0.2.0 packages
are the next stage. See the [S4 report](docs/reports/s4/REPORT.md) for measured
results and open performance/listening limits.

S4 passes 451 native expectations, 160 bit-exact M0 audio/state/latency cases,
106 integration checks, five provenance rejection tests, pluginval 5..10 and
Steinberg 537/537. Original IDs/order/ranges and class IDs remain; 147 controls
append after the original 12. PDC remains 2.5 ms rounded to samples in every mode.
Legacy is the default; modern controls reset to neutral.

Choose **Stereo / Space** for widening. For headphone placement choose
**Binaural / Headphones**, enable Position and compare the three profiles.
In MOTION set nodes, Hold/Travel in milliseconds or beats, arc and curve, then
enable Motion. Positive azimuth is left, zero front, 180 back. Behind-head
perception varies by listener; profiles are not personalized headphone EQ.
Type exact values in the fields. Default restores original Legacy and clears
modern settings; selecting a modern engine starts its neutral path.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_SHIROTA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloShirota_VST3 HamloShirota_Tests HamloShirota_Snapshot -- /m:2 /v:minimal
build/Release/HamloShirota_Tests.exe
```

VST bundle: `build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3`.
Keep the entire bundle including Resources/license notices. No installed plug-in
was overwritten. `Installer/Build-Installer.ps1` exists, but the final V2
installer has not been built or certified.

Scope: [SHIROTA_V2_SCOPE](docs/SHIROTA_V2_SCOPE.md). Acceptance:
[HAMLOSHIROTA_V2](docs/HAMLOSHIROTA_V2.md), [S4 methods](docs/S4_METHODS.md).
Working instructions: `AGENTS.md`; continuation: [NEXT_STAGE](docs/NEXT_STAGE.md).
Historical [S1](docs/reports/s1/REPORT.md), [S2](docs/reports/s2/REPORT.md) and
[S3](docs/reports/s3/REPORT.md) evidence are retained unchanged. Listening WAVs in
`out/s2/listen` and `out/s3/listen` precede the production phase adaptation and
are not S4 auditions. SADIE II data/license/attribution are in `Assets/Hrtf` and
the VST bundle; no runtime download is needed.
