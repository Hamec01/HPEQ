# HamloMag M0: the saturator extracted from HPEQ

Owner (2026-10-28): HamloMag next, as its own VST, then made competitive (docs to follow: references, targets).

## What was done

- New repository E:\HamloMag, the skeleton of HamloRedWall (build, installer, Black Steel drawing, preset screen).
- DSP copied from HPEQ (b277315) with only the namespace and the settings include changed: Source/DSP/HamloMag.*,
  the SaturatorProcessor of ProcessorsM6 (Source/DSP/Saturator.*), HPEQ's slot layout in Source/DSP/MagSettings.h.
- The page copied from HPEQ (Source/UI/HamloMagPanel.*). One change: the SPEED lever moved right, under OVERSAMPLE -
  it sat at the top left, under the preset screen that is top left on every Hamlo page. The screen covers the top of
  the left tape reel (drawing only); the layout is redone with the new controls.
- Plug-in identity, parameter IDs / ranges / defaults, state tag and the 7 presets as HPEQ's HamloMag module VST3, so
  its projects open here. The plug-in path as HPEQ's module: fixed latency (4x + the TAPE wow delay: 254 samples at
  48 kHz), MIX 20 ms ramp, ON crossfade, the dry path aligned.

## Checks

- Unit tests 184 / 184: the CLASSIC law against the formula (worst 3.7e-8), every model and oversampling finite and
  colouring, latency, blocks 1 / 64 / 127 / 4096 bit-identical (every model at 4x with wow and hiss), MIX 0 and ON off
  = the delayed input bit-exact, HPEQ state loads, mono, programs, page and preset screen.
- pluginval strictness 5 .. 10 (with the Steinberg validator at 10): pass (docs/reports/pluginval/m0); Steinberg
  validator -e: 537 / 537 (docs/reports/vst3validator/validator_m0.txt).
- Not run: the side-by-side reference against HPEQ's HamloMag.vst3 (owner, 2026-10-28: test the plug-ins
  themselves for now, HPEQ gets them later). The DSP files are HPEQ's apart from the namespace and include.

Panel: [panel.png](panel.png).
