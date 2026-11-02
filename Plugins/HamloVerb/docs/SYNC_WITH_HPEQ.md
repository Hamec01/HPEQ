# Keeping HamloVerb in step with HPEQ

M0: every file under `Source/` except `Source/UI/PresetScreen.*` is HPEQ's (b277315), unchanged, apart from
`Source/Standalone/ModulePluginEditor.*` (the preset screen added). The plug-in is HPEQ's HamloVerb module VST3: identity
BBLB / HmVb / com.hamloprod.hamloverb, kind 1, state tag HamloVerb. The next step (v2) narrows the code to
this module (as HamloComp / HamloUdarnik / HamloRedWall / HamloMag did) and records every change here.

## Standalone core extraction (2026-10-30)

The production target now contains only ReverbProcessor, HamloVerb, Verb's presets/parameter spec,
standalone processor/editor, BlackSteel, the legacy Verb panel and the shared preset screen.
LayerEngine, FxNodeProcessor, pools, crossover/EQ processors and every unrelated effect/panel are
excluded from compilation. Their historical files remain in the repository for provenance.

`DSP/VerbSettings.h` carries only Verb's original slot indices, parameter block and transport.
`DSP/LegacyVerbPath.h` implements the single send node formerly embedded in the graph, preserving
the original float rounding boundaries, input/MIX/node ramps and tail retirement rule.
ReverbProcessor.cpp and HamloVerb.cpp sound algorithms are unchanged. Their headers now include
the standalone settings instead of the HPEQ layer model.

The old 22 parameter IDs/ranges/order, bypass position, state tag, eight host programs and VST3
identity are retained. The UI's meters read the standalone core directly. No HPEQ source or
integration worktree was modified. This stage does not connect the new V2 engines or bank.
