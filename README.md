# HamloVerb 0.2.0

The standalone HamloProd reverb, part of the Hamlo plug-in line. The production target now builds only
Verb DSP and its standalone wrapper/UI; it no longer builds HPEQ's layer graph or other effects.
The legacy audio path retains the original additive wet law and parameter/state identity.

Six new characters: Clean, Warm, Velvet, Crisp, Deep and Bloom; six-band Decay EQ and Post EQ;
Gated, Reverse Swell, true Reverse Audio, Shimmer, stable Freeze, tail-preserving character changes,
Insert/Send and Mix Lock. Legacy is the default for project compatibility; use LOAD START or choose
a new character to begin. The 200 genre recipes follow the original seven presets (208 host programs).

Design: `docs/HAMLOVERB_V2.md`. Measurements, disclosures and remaining manual checks:
`docs/reports/v2/V2_REPORT.md`. Controls/install help: `docs/release/README.txt`.

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_VERB_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloVerb_VST3`, `HamloVerb_Tests`, `HamloVerb_Reference`, `HamloVerb_Lab`,
`HamloVerb_PresetLab`, `HamloVerb_Snapshot`. Installer: `Installer/Build-Installer.ps1`.
Final validation/package helpers: `Tools/Validate-Release.ps1`, `Tools/Package-Release.ps1`.
Working rules and the hand-over: AGENTS.md.
