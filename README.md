# HamloFader

The gain utility of HPEQ as its own VST3 (HamloProd), part of the Hamlo plug-in line. M0: HPEQ's module-plug-in stack
(`Source/` is HPEQ's, verbatim, built with HPEQ_MODULE_KIND=8) plus the preset screen top left.

    cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_FADER_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
    cmake --build build --config Release

Targets: `HamloFader_VST3`, `HamloFader_Tests`, `HamloFader_Snapshot`. Installer: `Installer/Build-Installer.ps1`.
Working rules and the hand-over: AGENTS.md.
