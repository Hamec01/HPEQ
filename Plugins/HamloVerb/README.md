# HamloVerb 0.2.2

The standalone HamloProd reverb, part of the Hamlo plug-in line. The production target now builds only
Verb DSP and its standalone wrapper/UI; it no longer builds HPEQ's layer graph or other effects.
The legacy audio path retains the original additive wet law and parameter/state identity.

**WET LEVEL** on Main adjusts only the reverb return, from -24 to +24 dB.
If MIX is high but the tail is quiet, try +6 or +12 dB and then adjust MIX.
It follows the effects/EQ/ducking/width and precedes Insert/Send mixing, leaving
the source and the reverb's decay/feedback unchanged. OUTPUT still trims the
total. SHIMMER is in Advanced > Effects. The WET meter includes this return trim.
Factory preset loads reset WET LEVEL to0dB; edited projects save it normally.
Old projects/programs keep0dB and their previous sound. Legacy ignores the trim.
Boost increases return peaks; no hidden normalization or automatic limiter.

0.2.2 VST3/ZIP/SHA256: `dist/WetLevel0.2.2`. See
[wet level report](docs/reports/wetlevel/REPORT.md) and
[handoff](docs/WET_LEVEL_HANDOFF.md). No new installer was produced.

**KEEP DRY** (top right, beside MIX LOCK) keeps the original signal at unity and
adds MIX times the reverb in modern Insert. Use Output0dB to keep unity source gain.
Without KEEP DRY,the original dry/wet balance remains:50% MIX reduces the direct
source by6.02dB before the tail arrives;100% is wet only. KEEP DRY is remembered
across presets and saved in projects. Send stays wet only; Legacy is unchanged.
Old states load with KEEP DRY off. Adding wet can still change total level/peaks.

Historical0.2.1 VST3/ZIP/SHA256: `dist/KeepDry0.2.1`.
1439 native checks,208 unchanged program renders,390 M0 reference cases and
pluginval5-10/Steinberg537 pass. [Report](docs/reports/keepdry/REPORT.md),
[audit](docs/OUTPUT_LEVEL_AUDIT.md), [handoff](docs/KEEP_DRY_HANDOFF.md).

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
The0.2.0 historical validation/package helpers are `Tools/Validate-Release.ps1`
and `Tools/Package-Release.ps1`;0.2.1 uses `Tools/Validate-KeepDry.ps1` and
`Tools/verify_keepdry.py`,with a separate report directory.0.2.2 uses
`Tools/Validate-WetLevel.ps1`, `Tools/finalize_wetlevel.py` and
`Tools/verify_wetlevel.py`. Historical receipts pin their frozen source snapshots;
do not use an older receipt to certify the changed current source tree.
Working rules and the hand-over: AGENTS.md.
