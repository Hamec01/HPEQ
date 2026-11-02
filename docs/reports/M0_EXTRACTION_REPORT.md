# HamloRevolution M0 (R0): HPEQ's Revolution page as its own VST3

Owner (2026-11-02): HamloRevolution becomes a rhythmic multi-effect sequencer (docs/REVOLUTION_V2.md); "you do it".
R0 is the first stage: the plug-in that sounds exactly like HPEQ's HamloRevolution module, before any new feature.

## What was done

- New repository E:\HamloRevolution. `Source/` is HPEQ's module-plug-in stack (the HPEQ_ModuleShared sources of
  b277315 and the headers they include, unchanged), built with HPEQ_MODULE_KIND=9. Identity BBLB / HmRv /
  com.hamloprod.hamlorevolution.
- HPEQ has **no** module VST3 for the Monsta FX slot (HamloRevolution lives only inside HPEQ), so - as for HamloFader -
  the stack got a new kind (docs/SYNC_WITH_HPEQ.md lists every changed file):
  - `ModuleKind::Revolution = 9` (8 is HamloFader) and `makeRevolution()`: 13 knobs and 11 switches in the page's
    enum order, with the keys of LayerSnapshot.cpp and the defaults of presetBaseModule. CHARACTER and LISTEN are
    page-only (they steer GENERATE, not the DSP; `kPageOnly`). STEPS and RATE snap to whole numbers.
  - The pattern, its locks and allowed actions, the LISTEN feel and the seed are HPEQ module state, not parameters:
    the processor keeps them (message thread), publishes the packed pattern and the humanize seed through atomics to
    the slot, and saves them as one `REVOLUTION` element in the plug-in state. A state without it (parameters only)
    loads the Default pattern.
  - Programs make the pattern as HPEQ applies a Revolution preset (Plugin/Presets.cpp makeModule): from the preset's
    seed and character. The fixed slot values (mode 1, smart join on, the M6 Monsta keys) are set as LayerSnapshot.cpp
    sets them.
  - The page: HPEQ's HamloRevolutionPanel with GENERATE / NEW SEED / GEN / LOCK / step edits going to the processor,
    the live status (step, warming, LISTEN feel, wave screen) read from the module node.
- Added: the preset screen top left (`Source/UI/PresetScreen.*`, the same file as the other Hamlo plug-ins; HPEQ's 24
  Revolution presets on it), tests, the page snapshot, the installer, the docs.

## Checks

- **V1 (Legacy bit-identical):** for Default and all 24 factory presets the plug-in's slot equals, value for value,
  the slot HPEQ's own LayerSnapshot.cpp makes from the same module (presetBaseModule + the preset + the generated
  pattern). LayerSnapshot.cpp and FxProjectState.cpp are compiled into the test only, as the reference. Same slot =
  same engine input, so the sound is HPEQ's.
- Unit tests: 176 / 176. Besides V1: every parameter at its default, latency 0 samples for every program, ON off =
  the input bit-exact, every program finite, host blocks 1 / 64 / 127 / 512 / 4096 bit-identical (stopped) and
  64 / 127 / 4096 bit-identical while playing at 120 BPM, the state round trip (pattern, locks, allowed actions,
  feel, a seed above 32 bits, one REVOLUTION element after saving twice), the empty pattern = dry (-400 dB), the
  page (GENERATE, NEW SEED, a step click reach the processor; the page follows a program).
- pluginval strictness 5 .. 10 (with the Steinberg validator at 10): pass (docs/reports/pluginval/m0); Steinberg
  validator -e: 537 / 537 (docs/reports/vst3validator/validator_m0.txt).
- Installer `dist/HamloRevolution_VST3_Setup_0.1.0.exe` and a zip of the VST3.
- Not run: listening side by side with HPEQ (the owner tests the plug-ins themselves for now).

## Known

- The preset screen top left covers the nameplate and the left part of the wave screen (it is the same place on every
  Hamlo plug-in). The page is redrawn in R6 (ADVANCED) and R4 (SIMPLE); the layout leaves that corner free then.

Panel: [panel.png](panel.png).

## Next

R1 lanes, R2 SHAPE, R3 THROW, R4 generator + SIMPLE, R5 HPDG Link, R6 ADVANCED page + 200 presets (REVOLUTION_V2.md).
