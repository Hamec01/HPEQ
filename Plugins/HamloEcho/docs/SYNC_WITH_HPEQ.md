# Keeping HamloEcho in step with HPEQ

M0: every file under `Source/` except `Source/UI/PresetScreen.*` is HPEQ's (b277315), unchanged, apart from
`Source/Standalone/ModulePluginEditor.*` (the preset screen added). The plug-in is HPEQ's HamloEcho module VST3: identity
BBLB / HmEc / com.hamloprod.hamloecho, kind 2, state tag HamloEcho. The next step (v2) narrows the code to
this module (as HamloComp / HamloUdarnik / HamloRedWall / HamloMag did) and records every change here.

## Echo-only extraction, 2026-10-30

Only this repository was changed. HPEQ and HamloVerb were neither modified nor rebuilt. There is no subtree update or push.

- `DSP/HamloEcho.cpp` is unchanged from M0; its header now includes the standalone `EchoSettings.h`.
- `DSP/LegacyDelay.*` contains exactly the original `ClassicDelay`, `DelayProcessor`, interpolation helper and division table extracted from `ProcessorsM6.*` at ede0db8. The 10 ms engine switch and filter/pitch/Freeze laws are unchanged.
- `DSP/EchoSettings.h` retains the original parameter-slot indices, transport and slot shape without a project/layer dependency.
- `DSP/LegacyEchoCore.*` replaces the single mix-node send slot and its pool, preserving input/mix/node ramps, silent-tail retirement, float delta calculation and dry addition order. It owns one delay instead of preparing unrelated processors and pools.
- `Standalone/ModuleVariant.cpp` and `Core/ModulePresets.cpp` retain only Echo's original spec and seven recipes, preserving their values and order. The plugin wrapper calls the own core; the editor constructs only HamloEchoPanel.
- 51 unused, unchanged copied M0 source/header files were removed; Git's M0 commit retains their history. List: `docs/reports/v2/REMOVED_M0_SOURCES.txt`. Source now has 26 files; the shared library compiles 10 cpp files instead of 35.
- `UI/PresetScreen.*` was not changed. Product identity, APVTS root, original 28 host parameters (including bypass), eight programs and reported zero latency remain unchanged.

`Tools/isolate_echo_m0.py` documents/reproduces the textual extraction from the pinned local Git commit. It is an M1 migration utility, not a generator to run over future V2 work. Quality is established by the independent frozen lab, not by the extraction script itself.

Result: 160/160 raw renders bit-identical to archived M0, complete host contract identical, all 160 old states restored byte-identically on both fresh and edited instances, tests 590/590, pluginval 5..10 pass, Steinberg 537/537. Exact methods, limitations, commands and hashes are recorded in `docs/reports/v2/LEGACY_EXTRACTION_REPORT.md`.
