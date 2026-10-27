# Carrying changes between HamloComp and HPEQ

HamloComp was copied from HPEQ at `b277315` (branch `feature/hpeq-m0-m1`, 2026-10-27). The owner's plan: each
module is sold as its own plug-in and inside the combo (today's HPEQ, to be renamed); a compressor change
made here is carried back into HPEQ.

| HamloComp | HPEQ | Differences |
|---|---|---|
| `Source/DSP/Compressor.h/.cpp` (`hamlo::dsp::CompressorProcessor`) | `Source/DSP/Processors.h/.cpp` (`hpeq::dsp::CompressorProcessor`) | takes `CompressorSettings` instead of `FxSlotParams`; the law and the code are the same |
| `Source/DSP/Ramp.h` | `Source/DSP/Ramp.h` | namespace only |
| `Source/UI/HamloCompPanel.*`, `BlackSteel.*` | same paths | namespace `hamlo::ui`; binary-data namespace `HamloCompAssets` |
| `Source/UI/ModuleHelp.*` | same paths | only the `comp` table |
| `Source/Plugin/Parameters.h`, `Presets.*` | `Source/Standalone/ModuleVariant.cpp` (makeComp), `Source/Core/ModulePresets.cpp` (kCompressor) | the same IDs, ranges, defaults and presets |
| `Source/Plugin/HamloCompProcessor.*` | `Source/Standalone/ModulePluginProcessor.*` | calls the compressor directly instead of a one-slot `LayerEngine` |

To carry a DSP change back: copy the class body into HPEQ's `Processors.cpp`, map `CompressorSettings`
fields to `p.values[slot::k...]` (the field comments name the slots), and run HPEQ's compressor tests and
`HamloComp_Reference` against HPEQ's rebuilt `HamloComp.vst3` (a change in both must give a difference
below -120 dB again).

When the third module is split out, the shared pieces (Black Steel, ramps, the plug-in wrapper) should move
into one library both products use, instead of a third copy.
