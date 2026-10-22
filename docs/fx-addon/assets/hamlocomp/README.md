# HamloComp - handoff

Working Soviet-style analog compressor UI (vanilla HTML/CSS/JS + Web Audio). Open index.html in a browser (no build step).

## Files
- index.html - layout (panel, nameplate, knobs, switches, meters, bypass lamp)
- style.css - Black Steel styling (worn steel, brass knobs, brass-bezel meters)
- app.js - parameter model, knobs/switches, Web Audio chain, canvas meters, procedural wear overlay
- reference/ - AI-generated mockups (hamlocomp_v1..v3 = style variants, chosen: v2 Black Steel) and asset sheets (soviet_*_en_*)

## Audio chain
source -> inputBus -> [analyser (INPUT meter)]
                   -> DynamicsCompressor -> WaveShaper (Character) -> makeup gain -> wet
                   -> dry (bypass)
Params: Threshold -60..0 dB, Ratio 1..20 (log), Knee 0..40 dB, Attack 0.1..100 ms (log), Release 10..1000 ms (log), Makeup 0..24 dB.
Switches: Detector Peak/RMS, Character Clean/Warm/Tube, Auto Makeup Off/On.

## Known approximations
- Detector RMS: Web Audio has no RMS detector; RMS mode changes INPUT meter ballistics and shifts threshold +3 dB.
- Character: tanh / asymmetric soft-clip curves, not a model of real hardware.
- Auto makeup: -threshold*(1-1/ratio)*0.5, clamped 0..18 dB.

## Ideas / next steps
- Use regenerated mockups (image quota was exhausted) as panel texture.
- Add real RMS detector via AudioWorklet; add output meter, sidechain HPF, mix knob.
