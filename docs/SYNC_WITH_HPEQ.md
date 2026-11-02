# Keeping HamloRevolution in step with HPEQ

M0: every file under `Source/` except `Source/UI/PresetScreen.*` is HPEQ's (b277315), unchanged, apart from
`Source/Standalone/ModulePluginEditor.*` (the preset screen added) and, because HPEQ has no module VST3 for the Monsta FX
slot, `ModuleVariant.*` (ModuleKind::Revolution = 9, `makeRevolution()` with LayerSnapshot.cpp's keys and
presetBaseModule's defaults, page-only CHARACTER / LISTEN), `ModulePluginProcessor.*` (the pattern / feel / seed
state: atomics into the slot, a REVOLUTION element in the state, programs generate the pattern as HPEQ's
`makeModule`) and the Revolution case of the editor. Copied as well: `Source/UI/HamloRevolutionPanel.*`,
`Source/Core/Revolution.cpp`, and - for the V1 test only, not in the plug-in - `Source/DSP/LayerSnapshot.cpp` and
`Source/Core/FxProjectState.cpp` (HPEQ's own mapping from a module to its slot). The plug-in is HPEQ's HamloRevolution module VST3: identity
BBLB / HmRv / com.hamloprod.hamlorevolution, kind 9, state tag HamloRevolution. The next step (v2) narrows the code to
this module (as HamloComp / HamloUdarnik / HamloRedWall / HamloMag did) and records every change here.

## R1 (lanes, scenes) - changes to HPEQ's files

- `DSP/LayerSnapshot.h`: `FxSlotParams::values` 48 -> 112, `slot::kLane*` (48 .. 92).
- `DSP/HamloRevolution.*`: per-lane length / rate / condition / probability, steps 17 .. 32; `stepAt` / `stepStartQ`
  take the lane's step length. All lane values 0 = HPEQ's engine bit for bit (Tests/GoldenTests.cpp).
- `Core/Revolution.*`: the lane vocabulary (`Lane`, conditions, probability codes, rate names, packing, saved text).
- `UI/HamloRevolutionPanel.*`: lane / step / scene menus, the second half of the cells, probability marks, scene on the LCD.
