# Carrying changes between HamloUdarnik and HPEQ

HamloUdarnik was copied from HPEQ at `b277315` (branch `feature/hpeq-m0-m1`, 2026-10-27), as HamloComp was.

| HamloUdarnik | HPEQ | Differences |
|---|---|---|
| `Source/DSP/HamloUdarnik.h/.cpp` | same paths | namespace `hamlo::dsp`; `TransientSettings` instead of `FxSlotParams` (same slot indices 0 .. 10, `Source/DSP/TransientSettings.h`) |
| `Source/DSP/TransientShaper.h/.cpp` (`TransientProcessor`) | `Source/DSP/ProcessorsM6.h/.cpp` (its transient part) | the same code |
| `Source/DSP/AlignmentDelay.h`, `Ramp.h` | `Source/DSP/DelayLine.h` (AlignmentDelay), `Ramp.h` | namespace only |
| `Source/UI/HamloUdarnikPanel.*`, `BlackSteel.*` | same paths | namespace `hamlo::ui`; binary data `HamloUdarnikAssets` |
| `Source/UI/ModuleHelp.*` | same paths | only the `transient` table |
| `Source/Plugin/Parameters.h`, `Presets.*` | `Source/Standalone/ModuleVariant.cpp` (makeUdarnik), `Source/Core/ModulePresets.cpp` (kTransient) | the same IDs, ranges, defaults, transforms and presets |
| `Source/Plugin/HamloUdarnikProcessor.*` | `Source/Standalone/ModulePluginProcessor.*` | the shaper directly instead of a one-slot `LayerEngine`; the same fixed latency and alignment |

This is the second copy of Black Steel, the ramps and the plug-in wrapper (HamloComp has the first); a third module
should move them into one shared library.
