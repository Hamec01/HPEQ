# HamloUdarnik

HamloProd's transient shaper as its own VST3 plug-in (Windows x64). Extracted from [HPEQ](../HPEQ) on 2026-10-27
(the same shaper is a module of the combo plug-in there), as HamloComp was; from here on the two are developed
separately and improvements are carried back ([docs/SYNC_WITH_HPEQ.md](docs/SYNC_WITH_HPEQ.md)).

- **DSP**: the M6 transient law (fast / slow envelopes) and the HamloUdarnik core: modes PRAVDA (direct SNAP /
  BODY gains), UDARNIK (loudness-neutral body, peak-safe snap), PERESTROIKA (rise and decay reshaped), hit
  detection with confidence and sensitivity, PRESERVE, look-ahead 0 / 2 / 5 ms, LISTEN, OUTPUT
  (`Source/DSP/`, every law in `HamloUdarnik.h`).
- **V2 engine** (default for new instances): hits from a fast onset detector, weights from the hit's phases, so SNAP /
  BODY deliver their value, level independent, steady tones untouched; PROTECT keeps the added level under 0 dBFS
  ([docs/reports/V2_REPORT.md](docs/reports/V2_REPORT.md)). CLASSIC keeps the original laws for older projects.
- **Page**: the Black Steel design (1180 x 820, resizable 0.5 .. 2 x): ten knobs, LOOK-AHEAD and LISTEN levers, ENGINE and PROTECT,
  the gain CRT with the last hit, the IMPACT needle (`Source/UI/`).
- **Plug-in**: host parameters with the IDs of HPEQ's HamloUdarnik module VST3 and the same identity
  (HamloProd / BBLB / HmUd), so its projects open here (as CLASSIC); MIX, ON (bypass), 18 presets as programs, mono and stereo,
  a fixed latency of 5 ms (`Source/Plugin/`).

## Build

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 [-DHAMLOUDARNIK_JUCE_DIR=<JUCE 8.0.12 checkout>]
cmake --build build --config Release --target HamloUdarnik_VST3 HamloUdarnik_Tests HamloUdarnik_Reference
build\Release\HamloUdarnik_Tests.exe
```

The plug-in: `build/HamloUdarnik_artefacts/Release/VST3/HamloUdarnik.vst3`. The installer:
`powershell -ExecutionPolicy Bypass -File Installer\Build-Installer.ps1` -> `dist/HamloUdarnik_VST3_Setup_<version>.exe`.

## Checks

- `HamloUdarnik_Tests`: the shaper (identities, look-ahead, block sizes, LISTEN, OUTPUT, SNAP) and the plug-in
  (parameters, latency, MIX / ON, alignment, HPEQ state, host blocks, mono, programs, page).
- `HamloUdarnik_Reference <HamloUdarnik.vst3>`: HPEQ's module build vs this one on the same signals and settings
  ([docs/reports/reference_check.md](docs/reports/reference_check.md)).
- pluginval v1.0.4 strictness 5-10 and Steinberg's VST3 validator pass.

Status: [docs/reports/M0_EXTRACTION_REPORT.md](docs/reports/M0_EXTRACTION_REPORT.md).
