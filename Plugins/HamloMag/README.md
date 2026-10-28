# HamloMag

The saturator of HPEQ as its own VST3 (HamloProd): six models (CLASSIC, TAPE, TUBE, IRON, GERMANIUM, VINYL), the
HamloLaw harmonics, tape / vinyl movement and noises, the Black Steel page with the preset screen top left.

Build (Windows, Visual Studio 2026, a JUCE 8.0.12 checkout carrying the renderer patch, e.g. HPEQ's):

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLOMAG_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloMag_VST3`, `HamloMag_Tests`, `HamloMag_Snapshot`. Installer: `Installer/Build-Installer.ps1`.
History: docs/reports/M0_EXTRACTION_REPORT.md; keeping the code in step with HPEQ: docs/SYNC_WITH_HPEQ.md.
