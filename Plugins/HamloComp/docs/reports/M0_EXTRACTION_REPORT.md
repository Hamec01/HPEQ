# M0: HamloComp extracted from HPEQ into its own repository

Date 2026-10-27. Source: HPEQ `b277315` (`feature/hpeq-m0-m1`). Owner: "pull the compressor out and make it
a separate VST; for now we work only with it" - its own repository, build, version and installer; the code
is copied, HPEQ is not touched; later the improvements go back into HPEQ (and HPEQ gets a new name).

## 1. What was taken, what was left

| Part | From HPEQ | Here |
|---|---|---|
| Compressor DSP | `CompressorProcessor` in `Processors.*` | `Source/DSP/Compressor.*`, same code; input `CompressorSettings` instead of the engine's `FxSlotParams` (which pulls in the whole project state) |
| Plug-in | `ModulePluginProcessor` running a one-slot `LayerEngine` (the full HPEQ engine) | `HamloCompProcessor` calls the compressor directly; MIX as HPEQ's slot mix, ON as its bypass crossfade (20 ms each) |
| Parameters, state, identity | `ModuleVariant.cpp` makeComp; BBLB / HmCp / com.hamloprod.hamlocomp | the same IDs, ranges, defaults, state tag `HamloComp`, codes: HPEQ HamloComp projects open here |
| Page | `HamloCompPanel`, `BlackSteel`, `ModuleHelp` (comp table) | the same files, namespace `hamlo::ui` |
| Assets | the whole hardware pack, tube portraits, 224 factory presets | only what the page draws: 13 hardware PNGs, 24 scratched pictures |
| Presets | `kCompressor` (7) | the same 7 as host programs |
| Installer | (HPEQ ships a zip) | Inno Setup 6, as HPDG's: `Common Files\VST3\HamloComp.vst3`, VC++ runtime when missing, README and notices in `Documentation` |

Binary: `HamloComp.vst3` 9.3 MB (HPEQ's module build: 13.1 MB). Installer 25.1 MB (24 MB of it is the
Microsoft VC++ runtime).

## 2. Same sound as HPEQ's HamloComp (`HamloComp_Reference`, limit fixed before the run: -120 dB)

The tool loads HPEQ's `HamloComp.vst3` (rebuilt from `b277315`, no source change) and this processor and
renders both: 2 signals (6 s synthetic drums, 3 s 220 Hz sine at -6 dBFS) x 20 settings (Default, the 7
presets, 12 combinations of character x detector x auto makeup at MIX 0.7) x 48 / 44.1 kHz x blocks 512 / 97.

| Result | |
|---|---|
| Cases | 160 |
| Bit-identical from 0.5 s | 128 |
| Worst difference from 0.5 s, re the reference's RMS | **-140.8 dB** (0 over the limit) |

The 32 cases that are not bit-identical are the MIX = 1 ones (Default, Vocal Leveler, Drum Punch, Bass
Control): HPEQ adds the module to the layer as dry + (wet - dry) in float, HamloComp outputs the wet
signal; the difference is float rounding.

### Disclosed

1. **The first run compared nothing**: a hosted VST3's parameter ID is a numeric hash, not the string ID, so
   the reference kept its defaults (difference -7.4 .. +9.0 dB). Parameters are now matched by name.
2. **The comparison window was changed after that run**: from the whole render to 0.5 s onward. In its first
   20 ms HPEQ's wrapper fades the module in from the dry signal (the dry kick at sample 0 passes
   uncompressed); HamloComp starts compressing at once. The first 0.5 s is still reported per case
   (-51.1 .. -1.4 dB re the reference), not judged.
3. **One new test was wrong as written**: the static-law check with a sine and the Peak detector (measured
   -17.04 dB against the law's -18.00 dB): a peak detector does not hold a sine's peak between peaks. The
   signal was changed to a square (constant |x|); it reads -17.98 against -17.98 dB. The ported HPEQ tests
   kept their thresholds.
4. Behaviour difference by design: while ON is off the compressor keeps running (HPEQ stops the slot and
   restarts it cold), so switching back on does not start from an empty detector.

## 3. Tests (`HamloComp_Tests`, Release): 112 / 112

Compressor (ported from HPEQ `CompressorPassTests`, same thresholds): static law (square, Peak) -17.98 /
-17.98 dB; identity below the knee for Clean / Punch / Glue (bit-exact); detector 0.2 rounds to Peak
(bit-exact); input meter Peak -20.00, RMS -22.98 dB (limit +-0.1); RMS law -15.76 / -15.74 dB (+-0.2); auto
makeup -0.08 dB after 10 s (+-0.5); Punch 3rd harmonic -37.2 vs Clean -62.9 dB (>= 20 dB more); Glue GR
150 ms after a short burst 0.07 vs Clean 4.00 dB, after a long one 6.60 vs 3.97 dB; blocks 1 / 64 / 127 / 256 /
512 bit-identical with every option on.

Plug-in: parameters (IDs, ranges, defaults, bypass, latency 0); a state in HPEQ's HamloComp format loads and
round-trips; MIX 0 and ON off give the dry signal bit-exact, MIX 1 the compressor alone; host blocks 1 / 64 /
127 / 512 and 4096 after prepare(256) bit-identical; mono; programs (Default + 7, Default restores); page
1180 x 700, edits write the parameters, host changes come back.

## 4. Not done / limits

- **Not installed, not opened in a DAW** (FL Studio): the installer was built, not run (it needs
  administrator rights and writes to `Common Files\VST3`). Listening is the owner's.
- Not code-signed.

## 5. Neutral defaults (owner, 2026-10-27)

"Make the defaults neutral, as on HPEQ's module page: 0 dB, 1:1, no auto makeup." As in HPEQ, two sets of
starting values: a fresh instance and the Default program are neutral (threshold 0 dB, ratio 1:1, auto
makeup off; the rest unchanged); a factory preset starts from the preset base (-30 dB, 4:1, auto makeup on,
HPEQ's `presetBaseModule`), so the 7 presets set exactly what they set before. Old projects are not affected:
a saved state holds every parameter.

| Check | Result |
|---|---|
| A fresh instance on the drum loop | output = input, bit-exact (new test) |
| Programs: a preset's unnamed values come from the base (e.g. New York Parallel: auto makeup on) | pass |
| `HamloComp_Tests` | 117 / 117 |
| `HamloComp_Reference`: + a "Default (neutral)" case, presets from the base | 168 cases, 136 bit-identical, worst -140.8 dB, 0 over -120 dB |

## 6. pluginval and the stand-alone build (owner: "download everything needed")

**pluginval v1.0.4** (Tracktion, GitHub release `pluginval_Windows.zip`, SHA-256
`c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab`, the same file as HPEQ's M8; the exe is not
signed) on `HamloComp.vst3` (Release, neutral defaults), strictness 5, 6, 7, 8, 9, 10: **SUCCESS at every
level, 0 failures**. Sections: scan, cold / warm open, plugin info, programs, editor (also while processing),
audio processing, non-releasing processing, state and state restoration, automation, editor automation,
parameters, background-thread state, parameter thread safety, buses (list, enable all, disable non-main,
restore), fuzz parameters. The vst3 validator step was skipped in these runs (see section 7). Logs:
`docs/reports/pluginval/strictness_<n>/`.

Disclosed: the first attempt started the six levels at once (pluginval is a GUI program and PowerShell did not
wait for it), which left empty logs; they were deleted and the levels run one after another.

**FetchContent build**: a clean configure without `HAMLOCOMP_JUCE_DIR` fetched JUCE 8.0.12, applied the renderer
patch (3 switches in place), built `HamloComp_VST3` and `HamloComp_Tests`; tests 117 / 117. So the repository
builds on its own, without HPEQ's build folder.

## 7. Steinberg's VST3 validator (owner allowed the download, 2026-10-27)

VST3 SDK `v3.8.0_build_66` (github.com/steinbergmedia/vst3sdk, the SDK version JUCE 8.0.12 carries: "VST 3.8.0"),
`validator` built from it (Release, VSTGUI and samples off). On `HamloComp.vst3`:

| Run | Result |
|---|---|
| `validator -e` (extensive) | **537 tests passed, 0 failed**; "64bit Audio Processing not supported" is information (the plug-in processes 32-bit float) |
| pluginval strictness 10 with `--vst3validator` | **SUCCESS**, its vst3 validator step exited 0 |

Logs: `docs/reports/vst3validator/validator_extensive.txt`, `docs/reports/pluginval/strictness_10_with_vst3validator/`.
