# HamloShirota 0.2.0

Standalone Windows x64 VST3 for stereo width, early space and timed headphone
motion. **207 host programs: Default, six original presets and 200 new recipes**,
50 each for Boom Bap, Trap, Techno and DnB. Genre/group/purpose are shown in the
shared preset display. All 200 new programs pass owner-material depth/peak and
exact project-state/selected-program/audio restoration checks.

- **QUICK:** width, space and travel controls with essential switches.
- **STEREO:** three frequency widths, crossovers, mono bass, Phase/Micro/Diffuse
  mono generation, visible optional Side budget, position/balance/spread.
- **SPACE:** Tight/Open/Scatter early fields, size/depth/damping/predelay.
- **MOTION:**16editable nodes, numerical azimuth/Hold/Travel in milliseconds or
  beats, arcs/curves, loop/one-shot, continuous rate and Restart.
- **LEGACY:** original sound, parameters, programs and state compatibility.

Stereo / Space and Binaural / Headphones are separate render choices. Select Binaural,
enable Position, then compare the three measured profiles. In MOTION configure
nodes and turn Motion on. Azimuth0 front, +90 left, -90 right, 180 back. On AKG K712,
compare the provided profile front/left/back/right/orbit examples by listening;
these profiles are not personalized HRTF or headphone EQ. Owner audition is
still pending. This build has no elevation/head tracking.

Original12 parameter prefix and class IDs are retained;147 controls append for159.
All modes keep original rounded 2.5 ms host PDC(120 samples at 48 kHz). Default restores
original Legacy and clears modern fields; neutral modern settings give aligned
dry. Compact Legacy retains the historical omission of selected-program metadata;
modern genre projects preserve their permanent ID and program. New functionality
is not represented by changing old parameter ranges or normalized choices.

**2457 native,160M0 bit-exact,106 integration,96 resampled-filter checks,5 identity
rejections,pluginval5..10 and Steinberg537/537 pass.** Mean measured worst-case
CPU is11.16–11.45% of a 48 kHz / 256-sample block. This is not FL callback-deadline certification:
earlier wall-time outliers remain disclosed and live-host checks are pending.
See the [S5 report](docs/reports/s5/REPORT.md), [presets](docs/GENRE_PRESETS.md)
and [methods](docs/S5_METHODS.md) for full conditions and limits.

Ready VST: `dist/HamloShirota-0.2.0-VST3/HamloShirota.vst3`.
**Copy the entire folder** to your VST3 directory and rescan in FL Studio;
Resources/license notices are part of the bundle. Adjacent SHA256SUMS.txt covers
every delivered file. No installed plug-in was overwritten. The installer is
intentionally excluded at the owner's request. Production A/B/headphone examples
are indexed in `docs/reports/s5/demos.json` and stored locally in `out/s5/listen`.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_SHIROTA_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloShirota_VST3 HamloShirota_Tests -- /m:2 /v:minimal
build/Release/HamloShirota_Tests.exe
```

Working rules:AGENTS.md. Handoff/build/audit commands:[NEXT_STAGE](docs/NEXT_STAGE.md).
Historical[S1](docs/reports/s1/REPORT.md),[S2](docs/reports/s2/REPORT.md),
[S3](docs/reports/s3/REPORT.md),[S4](docs/reports/s4/REPORT.md) evidence is retained.
SADIE II data/license/processing notices are in Assets/Hrtf and the VST bundle;
no runtime download is needed. S2/S3 listening examples predate production phase
adaptation; use the S5 examples for this build.
