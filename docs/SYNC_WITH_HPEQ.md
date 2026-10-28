# Carrying changes between HamloRedWall and HPEQ

Copied from HPEQ `b277315` (2026-10-28).

| HamloRedWall | HPEQ | Differences |
|---|---|---|
| `Source/DSP/HamloRedWall.h/.cpp` | same paths | namespace `hamlo::dsp`; `LimiterSettings` instead of `FxSlotParams` (slot indices 0 .. 9) |
| `Source/DSP/Limiter.h/.cpp` (`LimiterProcessor`) | `Source/DSP/ProcessorsM6.h/.cpp` (its limiter part) | the same code |
| `Source/UI/HamloRedWallPanel.*`, `BlackSteel.*`, `ModuleHelp.*` | same paths | namespace `hamlo::ui`; binary data `HamloRedWallAssets`; only the `limiter` help table |
| `Source/Plugin/Parameters.h`, `Presets.*` | `ModuleVariant.cpp` (makeRedWall), `ModulePresets.cpp` (kLimiter) | the same IDs, ranges, defaults, presets |
| `Source/Plugin/HamloRedWallProcessor.*` | `Source/Standalone/ModulePluginProcessor.*` | the limiter directly instead of a one-slot `LayerEngine`; the same fixed latency |

The third copy of Black Steel, ramps and the wrapper: they should move into one shared library.
