# Keeping HamloFader in step with HPEQ

M0: every file under `Source/` except `Source/UI/PresetScreen.*` is HPEQ's (b277315), unchanged, apart from
`Source/Standalone/ModulePluginEditor.*` (the preset screen added) and, because HPEQ has no module VST3 for the Gain slot,
`ModuleVariant.*` (ModuleKind::Fader, `makeFader()` with LayerSnapshot.cpp's ranges) and the Fader case of the editor;
`Source/UI/HamloFaderPanel.*` copied as well. The plug-in is HPEQ's HamloFader module VST3: identity
BBLB / HmFd / com.hamloprod.hamlofader, kind 8, state tag HamloFader. The next step (v2) narrows the code to
this module (as HamloComp / HamloUdarnik / HamloRedWall / HamloMag did) and records every change here.
