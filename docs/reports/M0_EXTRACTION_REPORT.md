# HamloFader M0: the gain utility extracted from HPEQ

Owner (2026-10-29): every remaining HPEQ module gets its own folder / product, as HamloComp, HamloUdarnik,
HamloRedWall and HamloMag did.

## What was done

- New repository E:\HamloFader. `Source/` is HPEQ's module-plug-in stack (the HPEQ_ModuleShared sources of b277315
  and the headers they include). HPEQ has no module VST3 for its Gain slot (the HamloFader page), so the copy gets
  `ModuleKind::Fader` (kind 8) and `makeFader()` with the ranges of HPEQ's LayerSnapshot.cpp (gain -60 .. +24 dB, delay
  0 .. 20 ms, target -40 .. 0 dB, polarity, mute, solo) and the Fader case of the editor; the DSP is HPEQ's Gain slot,
  unchanged. Identity BBLB / HmFd / com.hamlofader (new: no HPEQ plug-in to open). SOLO has no effect in a plug-in of
  its own (it soloes a band among the layer's bands in HPEQ).
- Added: the preset screen top left (`Source/UI/PresetScreen.*`, the same file as the other Hamlo plug-ins; HPEQ's
  module presets on it), tests, the page snapshot, the installer, the docs, AGENTS.md.

## Checks

- Unit tests: 49 / 49 (every parameter at its default, fixed latency for every program (0 samples at 48 kHz), ON off =
  the delayed input bit-exact, every program finite, blocks 1 / 64 / 127 / 512 / 4096 bit-identical, state round
  trip, the page and the preset screen).
- pluginval strictness 5 .. 10 (with the Steinberg validator at 10): pass (docs/reports/pluginval/m0); Steinberg
  validator -e: see docs/reports/vst3validator/validator_m0.txt.
- Not run: the side-by-side reference against HPEQ's HamloFader.vst3 (owner: test the plug-ins themselves for now).
- Next (v2, by AGENTS.md): narrow `Source/` to this module, references and targets, the lab, own features, 200 genre
  presets.

Panel: [panel.png](panel.png).
