# HamloEcho 0.2.2

HamloProd's standalone musical echo machine for Windows x64 VST3. **Default** now starts NOTE SCORE dry, with all effect amounts at zero. Choose Default under Factory / General to clear tails and reset Send/Mix Lock/Freeze; raise Mix and Time to explore the first unpitched echo. Older saved projects retain their stored engine and settings. The [0.2.1 patch report](docs/reports/default_021/REPORT.md) documents this initialisation-only change; DSP algorithms and the 200 genre recipes are unchanged.

- QUICK: a simplified Note Score page with TIME / MOTION / SPACE, eight moves and a separate MIX slider. EDIT IN NOTE SCORE transfers the move into the full editor.
- Eight editable note reflections: text/piano input, sync, Swing and a Same/Step note generator.
- Separate cumulative feedback pitch, Clean/Warm/Tape/BBD/Dust loop character.
- Three complementary bands with independent time, feedback, level and pan.
- Room/Plate/Cloud space before, after or inside feedback; individual reflection sends.
- Reverse, Stutter, Tape Stop and Skip; independent captured Glitch for echoes and/or spatial tails.
- Insert/Send, Mix Lock, Duck, Width, wet Output and stable Freeze.
- 200 new permanent-ID genre recipes;208 host programs including Default and seven original programs.

SOURCE NOTE is a manual reference; the complete input is transposed relative to it. FL Studio labels use C5=MIDI60, with an optional Standard C4=MIDI60 convention. This is not automatic pitch tracking or chord correction. Fixed reflections support +/-24 semitones. Pitched capture needs at least80ms; Reverse/Glitch add their displayed capture window. Dry and host PDC remain zero samples.

Saved states and the 28 original effect controls retain their behaviour. Expanding the library changes the wrapper-generated host Program selector's normalised scale from eight to208choices; existing Program-selection automation needs review. Direct program numbers0..7 remain unchanged. See [compatibility scope](docs/reports/release_020/SDK_PROGRAM_SELECTOR.md).

Read the [user guide](docs/USER_GUIDE.md), [factory catalog](docs/GENRE_PRESETS.md), [0.2.1 patch report](docs/reports/default_021/REPORT.md) and [original 0.2.0 report](docs/reports/v2/V2_REPORT.md). The original [compiled-bank report](docs/reports/presets/verification.json) passes 200/200; the patch verifies identical DSP files and all 200 recipe payloads. The [QUICK report](docs/reports/quick_022/REPORT.md) covers the added controller. Updated 0.2.2 installer/portable ZIP and SHA256 are in `dist/`; the [0.2.2 package manifest](docs/reports/quick_022/package_manifest.json) pins the new binary. The unchanged 0.2.0 A/B archive remains available. Owner FL Studio audition follows delivery.

## Build and verify

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ECHO_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloEcho_VST3 HamloEcho_Tests
build/Release/HamloEcho_Tests.exe
```

Additional targets: `HamloEcho_Lab` (immutable Legacy verification), `HamloEcho_ScoreLab` (integrated DSP), `HamloEcho_ScorePresetLab` (actual compiled-bank renders), `HamloEcho_Snapshot`, `HamloEcho_PrototypeLab`. [SCORE_METHODS](docs/SCORE_METHODS.md) and [PRESET_METHODS](docs/PRESET_METHODS.md) define measurements and retained failures. Calibration resumes by DSP/material/recipe/tool identity and rejects stale sources.

The patch passes 1234 unit expectations and 160 bit-identical M0 state renders, plus pluginval 5–10 and Steinberg 537/537. The unchanged DSP/library retain the 0.2.0 evidence: 59 integrated checks, 2687 renderer hashes and 200/200 independent Insert/Send/depth/state/tail/demo checks. Earlier CPU measurements and their failed/outlier runs remain scoped to their documented workload; this init-only patch does not claim a new performance or calibration campaign.

The raw machine-local archives under ignored `out/m0`, `out/attacks`, `out/score` and `out/presets` must be preserved. Source-only checkouts explicitly skip unavailable old-state archive tests. Provenance: [SYNC_WITH_HPEQ](docs/SYNC_WITH_HPEQ.md). HPEQ/Verb are separate and unchanged. Maintainer handoff: [CURRENT_WORK](docs/CURRENT_WORK.md).
