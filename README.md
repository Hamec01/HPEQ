# HamloMag 0.2.1

The saturator of HPEQ as its own VST3 (HamloProd): six models (CLASSIC, TAPE, TUBE, IRON, GERMANIUM, VINYL), the
HamloLaw harmonics, tape / vinyl movement and noises, the Black Steel page with the preset screen top left.

Build (Windows, Visual Studio 2026, a JUCE 8.0.12 checkout carrying the renderer patch, e.g. HPEQ's):

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLOMAG_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

V2 adds SP-1200 / MPC60 / S950 sampler colour, PHONE bass harmonics, SUB protection, DYNAMICS, loudness MATCH,
DELTA monitoring and 8x oversampling. The seven original factory presets stay; 200 genre presets cover Boom Bap,
Trap, Techno and DnB. New controls default to off. Fixed host-reported latency: 395 samples at 48 kHz (8.23 ms).

Targets: `HamloMag_VST3`, `HamloMag_Tests`, `HamloMag_Lab`, `HamloMag_PresetLab`, `HamloMag_Snapshot`.
Installer: `Installer/Build-Installer.ps1`. V2 measurements and limitations: `docs/reports/v2/TARGETS_FIX_REPORT.md`.
History: docs/reports/M0_EXTRACTION_REPORT.md; keeping the code in step with HPEQ: docs/SYNC_WITH_HPEQ.md.
