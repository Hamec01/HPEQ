# M0: HamloUdarnik extracted from HPEQ into its own repository

Date 2026-10-27. Source: HPEQ `b277315`. Owner: "next Udarnik - I need this VST"; "to me it is the weakest one
now". This stage only moves it, with the sound proven unchanged; the improvement pass is next.

## 1. What was taken

| Part | From HPEQ | Here |
|---|---|---|
| DSP | `UdarnikCore` (HamloUdarnik.*), `TransientProcessor` (ProcessorsM6) | the same code; input `TransientSettings` (the 11 slot values) instead of the engine's `FxSlotParams` |
| Plug-in | `ModulePluginProcessor` with a one-slot `LayerEngine` | `HamloUdarnikProcessor` runs the shaper directly: the slot's dry aligned by its look-ahead, MIX (20 ms), aligned to the fixed 5 ms, ON crossfade (20 ms) |
| Parameters, state, identity | makeUdarnik, BBLB / HmUd / com.hamloprod.hamloudarnik, tag `HamloUdarnik` | the same: HPEQ HamloUdarnik projects open here |
| Page | `HamloUdarnikPanel`, Black Steel, help (transient table) | the same; snapshot [panel.png](panel.png) |
| Presets | `kTransient` (6) | the same 6 as programs |
| Installer | - | Inno Setup 6 as HamloComp's |

## 2. Same sound as HPEQ's HamloUdarnik (`HamloUdarnik_Reference`, limit -120 dB, fixed before the run)

HPEQ's `HamloUdarnik.vst3` rebuilt from `b277315` (no source change) vs this processor: 2 signals x 21 settings
(Default, 6 presets, 3 modes x 3 look-aheads with SNAP +7 / BODY -4, PERESTROIKA RISE / DECAY, sensitivity and
PRESERVE, LISTEN attack / body, OUTPUT with MIX) x 48 / 44.1 kHz x blocks 512 / 97, compared from 0.5 s.

**168 cases, 104 bit-identical, worst -133.3 dB**, 0 over the limit ([reference_check.md](reference_check.md)).
The differences are float rounding of HPEQ's layer delta (dry + (wet - dry)) and of its outer crossfade, which is
exact at ON = 1 here.

## 3. Tests and validation

| Check | Result |
|---|---|
| `HamloUdarnik_Tests` | 141 / 141 |
| No option set: the M6 law is an identity; neutral settings in every mode / look-ahead: the input delayed by L | bit-exact |
| Blocks 1 / 64 / 512 (shaper), 1 / 64 / 127 / 512 / 4096 after prepare(256) (plug-in) | bit-identical |
| LISTEN attack + body = full | <= 1e-6 |
| Fixed latency 240 samples at 48 kHz; fresh instance, MIX 0, ON off = the input delayed by it | bit-exact, every look-ahead |
| pluginval v1.0.4, strictness 5 .. 10 (10 with Steinberg's validator) | SUCCESS |
| Steinberg validator `-e` (SDK 3.8.0) | 537 passed, 0 failed |

## 4. Seen on the way (for the improvement pass, not changed here)

- PRAVDA with SNAP +12 dB raises the test loop's peak from +0.81 to **+12.81 dBFS**: the direct mode has no
  peak protection (UDARNIK has). A user who turns SNAP up in the default mode clips.

## 5. Not done

- Listening in FL Studio (owner); not installed by me (admin rights).
- The improvement pass (the owner: "the weakest one"): research and targets first, as for HamloComp's models.
