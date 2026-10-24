# Standalone module VST3 technical plan

## Goal and scope

Build one additional VST3 effect for every owner-facing HPEQ module while keeping the existing
`HPEQ.vst3`, its state schema, UI and DSP path unchanged:

| Target | Product | Fixed HPEQ module | VST3 code |
|---|---|---|---|
| `HamloComp` | HamloComp | Compressor | `HmCp` |
| `HamloVerb` | HamloVerb | Reverb | `HmVb` |
| `HamloEcho` | HamloEcho | Delay | `HmEc` |
| `HamloMag` | HamloMag | Saturator | `HmMg` |
| `HamloRedWall` | HamloRedWall | Limiter | `HmRw` |
| `HamloUdarnik` | HamloUdarnik | Transient shaper | `HmUd` |
| `HamloZastava` | HamloZastava | Gate / expander | `HmZa` |
| `HamloShirota` | HamloShirota | Stereo processor | `HmSh` |

The eight products use the existing Hamlo panels and the same DSP classes as HPEQ. MonstaFx and
the utility Gain node are intentionally excluded because they do not have one of the supplied
owner-facing module pages.

## Isolation strategy

1. Do not edit or replace `FxPluginProcessor`, `FxPluginEditor`, the `HPEQ` CMake target, or the
   existing HPEQ plug-in identity.
2. Add a separate `HPEQ_ModuleShared` static library containing only the common standalone runtime
   plus the already existing DSP and eight panel implementations. HPEQ continues to compile and
   link exactly as before.
3. Add eight small JUCE plug-in targets. Their only variant-specific input is a compile definition
   selecting a module kind; each target has a unique bundle ID and plug-in code.
4. Link the already embedded `HPEQ_Assets` target, so every standalone VST3 contains the same owner
   PNG resources and needs no adjacent asset folder.
5. Copy the finished bundles after build to
   `build/HamloModules/<Configuration>/<Product>.vst3`. JUCE's ordinary per-target artefact folders
   remain available as build internals.

## Runtime and real-time design

- A standalone processor owns one `LayerEngine` and publishes a fixed-capacity `LayerSnapshot`
  containing exactly one slot in the whole-layer chain. This is the same processor path used by a
  module inserted on the HPEQ layer mix chain.
- All UI/host controls are JUCE host parameters. Audio processing reads their atomics at each block
  and builds a POD snapshot; it does not allocate, lock, access the filesystem, or touch the message
  thread.
- The panel `ON` control is the inverse of the host bypass parameter. Bypass keeps the DSP warm and
  crossfades to a latency-aligned dry signal.
- Modules whose latency can change with a control (RedWall, Udarnik, Zastava, Mag and Shirota)
  report a fixed per-module maximum. The processed signal is aligned to that fixed value. This
  prevents DAW PDC changes during automation while preserving each setting's DSP behaviour.
- Reverb/delay tail reporting uses the existing tail estimators and the latest host tempo.
- State is the standalone target's APVTS tree. Parameter IDs reuse the existing module keys
  (`thresholdDb`, `space`, `mix`, and so on) and are treated as permanent automation IDs.

## Editor design

- The standalone editor hosts exactly one existing `Hamlo*Panel` at its native aspect ratio.
- Existing hit zones, dragging, wheel, double-click and typed value entry remain in those panels.
- Panel callbacks begin/end host gestures and write the matching host parameter. A 30 Hz editor
  refresh reflects host automation back into the panel without firing callbacks.
- Existing meter, history, scope and tempo callbacks read the fixed module at mix-chain slot zero.

## Verification and rollback

1. Configure and build Debug and Release with the existing presets.
2. Run the existing `HPEQ_Tests.exe` in both configurations to prove the original application is
   unchanged.
3. Add standalone unit coverage for all eight variants: construction, mono/stereo layouts, finite
   audio, parameter/state round-trip, bypass/PDC and editor creation.
4. Scan and instantiate every collected `.vst3` through JUCE's VST3 host smoke tool; render audio,
   restore state and open/close each editor.
5. Check the collected folder contains exactly eight uniquely identified products.
6. Keep the work in a separate local commit. Do not push or merge without the owner's explicit
   approval. Reverting that commit removes all standalone targets and leaves `HPEQ.vst3` intact.
