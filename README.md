# HamloEcho 0.2.0

HamloProd's standalone musical echo machine for Windows x64 VST3. Choose **NOTE SCORE** in Engine or load a genre preset. Default and older projects retain **Legacy**, with its original sound and parameter/program identities.

- Eight editable note reflections: text/piano input, sync, Swing and a Same/Step note generator.
- Separate cumulative feedback pitch, Clean/Warm/Tape/BBD/Dust loop character.
- Three complementary bands with independent time, feedback, level and pan.
- Room/Plate/Cloud space before, after or inside feedback; individual reflection sends.
- Reverse, Stutter, Tape Stop and Skip; independent captured Glitch for echoes and/or spatial tails.
- Insert/Send, Mix Lock, Duck, Width, wet Output and stable Freeze.
- 200 new permanent-ID genre recipes;208 host programs including Default and seven original programs.

SOURCE NOTE is a manual reference; the complete input is transposed relative to it. FL Studio labels use C5=MIDI60, with an optional Standard C4=MIDI60 convention. This is not automatic pitch tracking or chord correction. Fixed reflections support +/-24 semitones. Pitched capture needs at least80ms; Reverse/Glitch add their displayed capture window. Dry and host PDC remain zero samples.

Saved states and the 28 original effect controls retain their behaviour. Expanding the library changes the wrapper-generated host Program selector's normalised scale from eight to208choices; existing Program-selection automation needs review. Direct program numbers0..7 remain unchanged. See [compatibility scope](docs/reports/release_020/SDK_PROGRAM_SELECTOR.md).

Read the [user guide](docs/USER_GUIDE.md), [factory catalog](docs/GENRE_PRESETS.md), [design/acceptance contract](docs/HAMLOECHO_V2.md) and [release report](docs/reports/v2/V2_REPORT.md). The [compiled-bank report](docs/reports/presets/verification.json) passes 200/200 recipes. Installer, portable VST3 ZIP, A/B examples and SHA256 files are in `dist/`; the [package manifest](docs/reports/release_020_final/package_manifest.json) pins the validated binary. Owner FL Studio audition follows delivery.

## Build and verify

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ECHO_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloEcho_VST3 HamloEcho_Tests
build/Release/HamloEcho_Tests.exe
```

Additional targets: `HamloEcho_Lab` (immutable Legacy verification), `HamloEcho_ScoreLab` (integrated DSP), `HamloEcho_ScorePresetLab` (actual compiled-bank renders), `HamloEcho_Snapshot`, `HamloEcho_PrototypeLab`. [SCORE_METHODS](docs/SCORE_METHODS.md) and [PRESET_METHODS](docs/PRESET_METHODS.md) define measurements and retained failures. Calibration resumes by DSP/material/recipe/tool identity and rejects stale sources.

The corrected engine passes 1198 unit expectations, 59 integrated checks, 160 bit-identical M0 renders and 2687 retained renderer hashes. The 200-recipe bank passes independent Insert/Send, depth, state, finite-output, tail and demonstration-peak checks. Pluginval 5–10 and Steinberg 537/537 pass. Controlled tone/dense CPU runs meet the 25%/50% mean budgets at 48 kHz/256 on the owner's i5-10300H; one failed dense run and wall-time outliers remain disclosed in the report.

The raw machine-local archives under ignored `out/m0`, `out/attacks`, `out/score` and `out/presets` must be preserved. Source-only checkouts explicitly skip unavailable old-state archive tests. Provenance: [SYNC_WITH_HPEQ](docs/SYNC_WITH_HPEQ.md). HPEQ/Verb are separate and unchanged. Maintainer handoff: [CURRENT_WORK](docs/CURRENT_WORK.md).
