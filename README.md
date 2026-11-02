# HamloShirota

HamloProd's standalone stereo processor, currently version0.1.0 with the S1
isolation checkpoint complete. Production now uses its own Shirota processor;
160M0renders, original parameters/programs/state and latency match bit-for-bit.

S2 offline prototypes now provide measured width, three mono-to-stereo choices
and three early spatial fields:181/181 checks and40owner-material A/B examples.
See the [S2 report](docs/reports/s2/REPORT.md). These prototypes are not yet in
the production VST3. Binaural rendering, programmable movement, UI integration,
200presets and release checks remain in development. Owner listening reference:
AKG K712. See the [scope](docs/SHIROTA_V2_SCOPE.md),
[acceptance plan](docs/HAMLOSHIROTA_V2.md) and [S1 report](docs/reports/s1/REPORT.md).
This checkpoint preserves the existing sound; it is not the spatial V2 release.

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_SHIROTA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloShirota_VST3`, `HamloShirota_Tests`, `HamloShirota_Snapshot`, `HamloShirota_Lab`. Installer: `Installer/Build-Installer.ps1`.
Working rules and the hand-over: AGENTS.md.

Offline prototype targets: `HamloShirota_SpatialLab`, `HamloShirota_SpatialDemo`.
Methods: `docs/S2_METHODS.md`. Listening WAVs: `out/s2/listen` (local, not packaged).
