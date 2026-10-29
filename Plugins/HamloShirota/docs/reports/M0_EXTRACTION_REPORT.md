# HamloShirota M0: the stereo processor extracted from HPEQ

Owner (2026-10-29): every remaining HPEQ module gets its own folder / product, as HamloComp, HamloUdarnik,
HamloRedWall and HamloMag did.

## What was done

- New repository E:\HamloShirota. `Source/` is HPEQ's module-plug-in stack (the HPEQ_ModuleShared sources of b277315 and the
  headers they include, unchanged), built with HPEQ_MODULE_KIND=7 - the plug-in HPEQ builds as HamloShirota.vst3, so
  its sound, parameters, presets and state are HPEQ's. Identity BBLB / HmSh / com.hamloprod.hamloshirota.
- Added: the preset screen top left (`Source/UI/PresetScreen.*`, the same file as the other Hamlo plug-ins; HPEQ's
  module presets on it), tests, the page snapshot, the installer, the docs, AGENTS.md.

## Checks

- Unit tests: 59 / 59 (every parameter at its default, fixed latency for every program (120 samples at 48 kHz), ON off =
  the delayed input bit-exact, every program finite, blocks 1 / 64 / 127 / 512 / 4096 bit-identical, state round
  trip, the page and the preset screen).
- pluginval strictness 5 .. 10 (with the Steinberg validator at 10): pass (docs/reports/pluginval/m0); Steinberg
  validator -e: see docs/reports/vst3validator/validator_m0.txt.
- Not run: the side-by-side reference against HPEQ's HamloShirota.vst3 (owner: test the plug-ins themselves for now).
- Next (v2, by AGENTS.md): narrow `Source/` to this module, references and targets, the lab, own features, 200 genre
  presets.

Panel: [panel.png](panel.png).
