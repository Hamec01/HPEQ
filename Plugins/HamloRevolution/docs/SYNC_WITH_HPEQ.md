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

## R2 (SHAPE lanes, bands) - changes to HPEQ's files

- `DSP/LayerSnapshot.h`: `values` 112 -> 160; `slot::kShape*`, `kShapeLaneBase`, `kBandBase`, `kCrossover*`.
- `DSP/HamloRevolution.*`: `runLane` (FULL = the lane as before; a band = the Linkwitz-Riley split), `shapeStage`,
  `shapeLevel`; every lane through `runLane`, the shapes after the three engines.
- `Core/Revolution.*`: `kLanes`, `ShapeTarget`, `Band`, `Lane::band` / `Lane::first`, engine names for the shapes.
- `UI/HamloRevolutionPanel.*`: 1180 x 1016, SHAPE rows, BANDS footer, band / curve menus and tags.

## R3 (THROW lanes) - changes to HPEQ's files

- `DSP/LayerSnapshot.h`: `values` 160 -> 208; `slot::kThrow*`, `kThrowLaneBase`, `kThrowBandBase`, `laneSettings()`,
  `laneBand()` (where a lane's settings and band live).
- `DSP/HamloRevolution.*`: lanes 5 / 6 (`ThrowLane`, `throwStage`, `throwLevel`, `clearThrow`, `lineAt`); per-lane arrays
  5 -> 7; the throw buffers allocated in `prepare` (2 x 2.5 s tape + a small FDN per lane); `smoothBlend`.
- `Core/Revolution.*`: `kThrows`, `kFirstShape`, `kFirstThrow`, `ThrowTarget`, `throwIsSend`, TIME / NOTE tables.
- `UI/HamloRevolutionPanel.*`: 1180 x 1280 (the editor opens at 75 %), THROW rows, the target window's menu.

## R4 (SIMPLE, CONNECT) - changes to HPEQ's files

- `DSP/LayerSnapshot.h`: `values` 208 -> 228; `slot::kGrooveBase`, `kGrooveSteps`, `kGrooveAmount`.
- `DSP/HamloRevolution.*`: the groove in `stepStartQ` (a boundary on a 1/16 moves by GROOVE x table); `value()` calls
  Core's `laneEventValue` (same arithmetic); band TOPS in `runLane`.
- `Core/Revolution.*`: `Band::Tops`, `laneHash`, `laneEventValue`. New: `Core/RevolutionSimple.*` (the generator; it
  includes `Link/HamloLinkProtocol.h` - HPEQ would need that file too).
- New page `UI/HamloRevolutionSimplePage.*`; `UI/HamloRevolutionPanel.*`: the SIMPLE button.

## R5 (LISTEN timing, key) - changes to HPEQ's files

- `DSP/HamloRevolution.*`: `listenTiming` (onsets at sample resolution, per 1/16 of the bar), `getTiming`.
- `DSP/FxNodeProcessor.*`: `getRevolutionTiming`.

## R6 (presets, painting, help) - changes to HPEQ's files

- `UI/HamloRevolutionPanel.*`: `paintStep` and drag painting, the CURVE drawn on SHAPE rows, tooltips
  (`UI/RevolutionHelp.*`). The 200 presets (`Core/RevolutionPresets.*`) are the plug-in's own programs.
