# HamloComp

HamloProd's compressor as its own VST3 plug-in (Windows x64). It was extracted from
[HPEQ](../HPEQ) on 2026-10-27, where the same compressor is a module of the combo plug-in; from here on
the two are developed separately, and improvements made here are carried back into HPEQ
([docs/SYNC_WITH_HPEQ.md](docs/SYNC_WITH_HPEQ.md)).

- **DSP**: five models - CLASSIC (the original: Peak / RMS, soft knee, Clean / Punch / Glue), VCA (auto
  release), FET (feedback, grit), OPTO (two-stage release), TUBE (vari-mu curve, warmth) - and a SOURCE that
  shapes the detector for DRUMS, SAMPLES (melodic parts) or VOC; auto makeup (`Source/DSP/Compressor.*`, every
  law in the header; design and targets: [docs/COMPRESSOR_MODELS.md](docs/COMPRESSOR_MODELS.md)).
- **Page**: the owner's Black Steel design (1180 x 820, resizable 0.5 .. 2 x): six brass knobs plus MIX,
  three toggles, the MODEL and SOURCE levers, the input CRT with 4 s of history and the gain-reduction needle
  (`Source/UI/`).
- **Plug-in**: host parameters with the IDs of HPEQ's HamloComp module VST3, the same plug-in identity
  (HamloProd / BBLB / HmCp), so its projects open here (as CLASSIC); MIX, ON (bypass), 51 factory presets as programs,
  mono and stereo, latency 0 (`Source/Plugin/`).

## Build

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 [-DHAMLOCOMP_JUCE_DIR=<JUCE 8.0.12 checkout>]
cmake --build build --config Release --target HamloComp_VST3 HamloComp_Tests HamloComp_Reference
build\Release\HamloComp_Tests.exe
```

Without `HAMLOCOMP_JUCE_DIR` JUCE 8.0.12 is fetched and patched (`cmake/PatchJuceWindowsRenderer.cmake`:
the GDI renderer switch). A local JUCE must already carry that patch (HPEQ's `build/_deps/juce-src` does).

The plug-in: `build/HamloComp_artefacts/Release/VST3/HamloComp.vst3`.
The installer: `powershell -ExecutionPolicy Bypass -File Installer\Build-Installer.ps1` ->
`dist/HamloComp_VST3_Setup_<version>.exe` (Inno Setup 6, installs to `Common Files\VST3`, adds the VC++
runtime when missing).

## Checks

- `HamloComp_Tests`: the compressor's laws (static law, identity below the knee, meters, RMS, auto makeup,
  Punch, Glue, block-size independence) and the plug-in (parameters, HPEQ state compatibility, MIX / ON,
  host block sizes, mono, programs, page).
- `HamloComp_Reference <HamloComp.vst3>`: hosts a reference build (HPEQ's module VST3) next to this
  processor on the same signals and settings and reports the difference
  ([docs/reports/reference_check.md](docs/reports/reference_check.md)).

`pluginval` (v1.0.4, strictness 5-10) and Steinberg's VST3 validator (SDK 3.8.0, 537 tests) pass; see the report.

- `HamloComp_Lab [report.md]`: the model and source targets T2 .. T14 ([docs/reports/MODELS_REPORT.md](docs/reports/MODELS_REPORT.md)).

Status and measurements: [M0_EXTRACTION_REPORT](docs/reports/M0_EXTRACTION_REPORT.md), [MODELS_REPORT](docs/reports/MODELS_REPORT.md).
