# HamloVerb

The standalone HamloProd reverb, part of the Hamlo plug-in line. The production target now builds only
Verb DSP and its standalone wrapper/UI; it no longer builds HPEQ's layer graph or other effects.
The legacy audio path retains the original additive wet law and parameter/state identity.

The 0.2.0 design and M0 fixtures are recorded in `docs/HAMLOVERB_V2.md` and `docs/reports/v2/`.
New character/effect/UI/preset sources in the working tree are development work, not a released 0.2.0.

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_VERB_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloVerb_VST3`, `HamloVerb_Tests`, `HamloVerb_Snapshot`. Installer: `Installer/Build-Installer.ps1`.
Working rules and the hand-over: AGENTS.md.
