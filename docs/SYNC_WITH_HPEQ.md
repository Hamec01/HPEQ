# Keeping HamloMag in step with HPEQ

| HamloMag | HPEQ |
|---|---|
| Source/DSP/HamloMag.h / .cpp | Source/DSP/HamloMag.h / .cpp (namespace and include only) |
| Source/DSP/Saturator.h / .cpp (SaturatorProcessor) | Source/DSP/ProcessorsM6.h / .cpp, the Saturator section |
| Source/DSP/MagSettings.h (FxSlotParams, slot::kDriveDb .. kSatNoiseFollow) | Source/DSP/LayerSnapshot.h (same indices 0 .. 21) |
| Source/UI/HamloMagPanel.h / .cpp | Source/UI/HamloMagPanel.h / .cpp; SPEED moved under OVERSAMPLE (the preset screen is top left) |
| Source/Plugin/Parameters.h | Source/Standalone/ModuleVariant.cpp, makeMag (IDs, ranges, defaults) |
| Source/Plugin/Presets.cpp (the first 7) | Source/Core/ModulePresets.cpp, kSaturator |
| Source/UI/ModuleHelp.cpp | Source/UI/ModuleHelp.cpp, the "saturator" table |
| Source/DSP/MagV2.h / .cpp (SAMPLER, PHONE, SUB, DYNAMICS) | - (HamloMag v2 only, docs/MAG_V2.md); slots 22 .. 25 |
| Saturator.*: oversampling index 3 = 8x | HPEQ has 1x / 2x / 4x |
| Processor: MATCH, DELTA | - (HamloMag v2 only) |
| Plug-in identity: BBLB / HmMg / com.hamloprod.hamlomag, state tag HamloMag | hpeq_add_module_plugin(HamloMag ...) |
