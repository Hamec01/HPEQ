# Keeping HamloShirota in step with HPEQ

At M0, every file under `Source/` except `Source/UI/PresetScreen.*` was HPEQ's (b277315), unchanged, apart from
`Source/Standalone/ModulePluginEditor.*` (the preset screen added). The plug-in is HPEQ's HamloShirota module VST3: identity
BBLB / HmSh / com.hamloprod.hamloshirota, kind 7, state tag HamloShirota.

## S1 isolation,2026-10-31

Baseline standalone commit93d965f. ShirotaSettings.h replaces the full layer/
project image with the original48-float slot, stereo indices and transport data.
HamloShirota.cpp is unchanged; its header now includes that lightweight image.
LegacyStereoProcessor copies the StereoProcessor class and its two functions
from ProcessorsM6 without changing arithmetic. LegacyShirotaCore preserves the
original single-insert/node/layer sequence and conditional delay histories.

ModuleVariant, ModulePresets and the editor now contain only Shirota's original
definitions and panel. The plugin processor calls the isolated core directly;
outer bypass and alignment remain. CMake compiles10owned production units and no
foreign effects/panels. Historical unused files remain uncompiled for provenance.
No HPEQ source or checkout was modified; no subtree/push was performed.

Proof:160M0cases bit-identical, original contract/class IDs, state reserialization,
latency,115native expectations and validators. See reports/s1/REPORT.md and
reports/s1/acceptance.json. Preserve out/s1 binaries/reference/original_sources;
do not regenerate the M0. The requested new spatial V2 is still in progress.
