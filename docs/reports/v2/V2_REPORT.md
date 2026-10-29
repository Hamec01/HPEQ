# HamloMag 0.2.0: V2 release report

Verification date: 2026-10-29. Design and original targets: [MAG_V2.md](../../MAG_V2.md).
The pre-existing V2 implementation was completed for release without changing the DSP or the lab thresholds.

## Delivered behaviour

- SAMPLER: SP-1200, MPC60 and S950 rate / 12-bit colour before the six original models.
- PHONE bass harmonics, SUB protection and envelope-dependent DYNAMICS.
- K-weighted loudness MATCH, aligned DELTA and 8x oversampling.
- OVERLOAD / BUMP / 24 DB switch labels; existing parameter IDs retained.
- 1180 x 940 Black Steel panel, preset screen top left and help for the new controls.
- 200 genre recipes in ten groups, 50 each for Boom Bap, Trap, Techno and DnB, alongside the seven unchanged factory presets.

All new controls default to off. Host-reported latency is fixed at 351 samples at 48 kHz (7.3125 ms), including
the sampler FIR delay, the maximum oversampling latency and tape wow delay. Verified by the Latency unit test.

## Before and after

The same `HamloMag_Lab` produced [before.md](before.md) and [after.md](after.md). New functions are n/a before.
G1 compares the original 18 stored 0.1.0 renders, with latency aligned; all 18 are bit-identical.
G2 was -47.2 dB at 4x before; after it is still -47.2 dB at 4x and -77.8 dB at 8x.
The complete numeric table is in after.md: 13 / 15 pass. G16 is the separate VST3 validation gate.

| Goal | Before | After | Target / result |
|---|---|---|---|
| G1 old-mode renders | 18 written | 18 / 18 bit-identical | pass |
| G2 alias, 4x / 8x | -47.2 / n/a dB | -47.2 / -77.8 dB | 4x unchanged; 8x <= -90 dB: **miss** |
| G3 SP input alias | n/a | -4.7 dB | >= -30 dB: pass |
| G4 MPC / S950 input alias | n/a | numerical floor / numerical floor | <= -50 dB: pass on these tones |
| G5 SP 1 / 15 kHz; MPC 1 / 15 kHz; S950 1 kHz | n/a | -0.1 / -14.2; -0.0 / -3.1; -0.0 dB | pass |
| G6 low-level THD+N, off / SP / MPC / S950 | n/a | -75.8 / -17.6 / -27.4 / -18.1 dB | pass |
| G7 PHONE harmonics / fundamental change / >1 kHz | n/a | -7.6 / -0.11 / -61.1 dB | pass |
| G8 PHONE filtered-bass loudness | n/a | +4.2 / +13.1 LU | >= +4 LU: pass |
| G9 SUB low-band THD, off / on | n/a | 13.77 / 0.00 % (rounded) | <= 1 %, 10x lower: pass |
| G10 SUB magnitude deviation, lab 20 Hz .. 18 kHz | n/a | 0.00 dB | <= 0.2 dB on measured sweep: pass |
| G11 attack/body nonlinear energy, + / - DYNAMICS | n/a | +5.7 / -13.1 dB | >= +3 / <= -3 dB: pass |
| G12 MATCH absolute loudness error | n/a | 0.58 LU | <= 0.5 LU: **miss** |
| G13 MATCH steady-noise gain movement | n/a | +/-0.04 dB | <= +/-0.3 dB: pass |
| G14 DELTA reconstruction error | n/a | -148.7 dB | <= -120 dB: pass |
| G15 block-size independence | bit-identical | bit-identical | pass |

Coverage note: G10's implemented lab sweep ends at 18 kHz, although the design target names 20 kHz. The reported
0.00 dB applies to that measured sweep; it does not establish the final 18 .. 20 kHz interval.

Unit tests: 707 expectations passed, 0 failed, including old-state migration, neutral V2 DSP equivalence,
MIX / ON / DELTA alignment and bit-identical host blocks 1 / 64 / 127 / 4096 after prepare(256).
The lab additionally verifies blocks 1 / 64 / 512 / 4096 against 256 with the new stages enabled.

## Targets not met

- **G2, 8x alias suppression:** -77.8 dB against <= -90 dB. This improves on 4x by 30.6 dB but misses the target
  by 12.2 dB. This is the measured limit of the current oversampling / nonlinear path on the specified 9 kHz test;
  the original 4x path stays unchanged for compatibility. The target remains -90 dB.
- **G12, MATCH on mixes:** maximum absolute integrated loudness error 0.58 LU against <= 0.5 LU after the first
  3 seconds. The finite-time, gated tracker follows changing programme dynamics rather than guaranteeing a
  whole-file normalization. Steady-noise gain movement is only +/-0.04 dB (G13). No constant correction was added:
  it would not establish the target for every mix. The target remains 0.5 LU.

## Disclosures inherited from development

- **G4:** the first test used `device rate - 15 kHz`, which measures a DAC image when the input is below the
  device Nyquist frequency. The corrected input-alias test uses MPC60 21 -> 19 kHz and S950 18 -> 14 kHz.
  The old 15 kHz image results are still reported separately (-33.6 / -36.2 dB). The final -240 dB readings are
  the lab's numerical floor for these particular tones, not a claim of broadband 240 dB rejection.
- **G6:** the first target used the linear 12-bit range (-6 .. -24 dB) for every device. MPC60 uses companding,
  so its low-level THD+N target is -20 .. -40 dB; SP-1200 / S950 retain the linear range. Off remains < -60 dB.
- **G11:** the first measurement used output minus input on a dense drum bus. Linear model filtering obscured
  saturation, and that bus has little isolated attack/body contrast. The final test uses N = y(x) - 2 y(x/2)
  on clean decaying hits; the drum-bus result remains reported (1.7 / -3.1 dB). Targets stay +3 / -3 dB.
- **G13:** the first measurement used output / input, which includes the saturation's movement on noise.
  MATCH gain is measured as output with MATCH / output without MATCH, in the same 400 ms windows.
- **DYNAMICS:** the first design varied drive by +/-12 dB and compensated level. HamloLaw's bounded harmonics
  made the measured effect run backwards. The final design weights the aligned saturated-minus-dry share.
- **PHONE:** first H2 .. H4, then H2 .. H6 because a deep sub gained only about 1 LU through the phone filter.
- **SAMPLER:** the first anti-alias filter was an eighth-order Butterworth (about -20 dB at MPC60's 21 kHz
  input). The final 257-tap Kaiser FIR at 4x replaces it, evaluated at device ticks, with 32 base samples delay.
- **MATCH:** first an ungated 3-second mean-square follower; final K-weighted 100 ms blocks with relative gating,
  1.5-second averaging and 0.5-second smoothing. G12's remaining miss is disclosed above.
- **Automation compatibility:** OVERSAMPLE adds a fourth choice (8x). IDs and integer 1x / 2x / 4x saved state
  values stay, but normalized host automation may map differently when the choice count changes. This is the
  explicitly documented exception to the rule of adding new parameters instead of extending choices.
- No comparison against the HPEQ plug-in was run; only HamloMag and its stored 0.1.0 renders were tested.
- **Preset calibration, 2026-10-29:** the inherited first 48 records used rising-only bisection. Several recipes
  reached 48 dB with insufficient saturation because the transfer is not monotonic. The replacement scans for
  a target crossing before bisection in either direction. The original records are preserved in
  [calibration_first48.tsv](../presets/calibration_first48.tsv). Unused full/noise rendering is skipped only
  during DRIVE search; the saturation calculation and independent final check remain unchanged. All material
  WAV files were verified to be 48 kHz, matching the lab's fixed processing rate.
- **Calibration candidate ranking:** after the first scan exposed high-DRIVE descending branches, the search
  also prefers candidates meeting the existing +/-6 dB level and +3 dBFS peak limits on the quiet render. Final
  acceptance still uses the complete preset with its noises. CAL records from this repair stage also include
  quiet peak and RMS change for diagnosis; depth bands and all check criteria are unchanged.

## Release gates and artifacts

The first complete calibrated preset check passed **138 / 200**; 62 failed depth, output level or uniqueness.
It is preserved in [genre_presets_check_first.md](../presets/genre_presets_check_first.md). Recipe repairs add
dry blend for overly coloured Gentle presets, move selected SUB crossovers lower for more saturated low-mid
energy, increase selected heavy blends, give genres distinct active tone / dynamics / model settings, and
compensate heavy-preset output loss. Selected heavy buses use AUTO off and fixed OUTPUT so their saturated high
band remains audible alongside the protected sub. These are preset settings; the DSP and acceptance bands stay.
The exact original-to-final recipe fields are in [recipe_changes.md](../presets/recipe_changes.md).

The second full check reached **195 / 200**, preserved in
[genre_presets_check_repair.md](../presets/genre_presets_check_repair.md). The last five were Underground Crush
Bus (depth), Dark Parallel Smash (level), Peak Drum Drive (duplicate), Jump-Up Snare Bite (depth / level) and
Amen Break SP-1200 (depth / level). The last repair recalibrates the selected manual-gain recipes, raises Dark
Parallel Smash's fixed OUTPUT by 2 dB, gives Peak Drum Drive its own DYNAMICS value, removes OVERLOAD from the
Medium Jump-Up Snare Bite, and lowers Amen Break's fixed OUTPUT before recalibration. All acceptance limits stay.

`Tools/run_genre_check.py` runs the same executable once per genre, then combines all 200 rows. Each filtered run
still fingerprints all preceding presets, so cross-genre duplicates fail just as in an unfiltered run. No
checks are skipped: finite output, depth band, +3 dBFS peak, +/-6 dB RMS level and within-group uniqueness.

Final preset check: **200 / 200 pass**. Rounded table extrema: peak +1.60 dBFS (limit +3), RMS change
-5.92 .. +4.60 dB (limits +/-6). Final table: [genre_presets_check.md](../presets/genre_presets_check.md);
all 200 DRIVE records: [calibration_final.tsv](../presets/calibration_final.tsv). 63 recipes changed; depth labels
and acceptance bands were not relaxed. Factory presets retain their original values.

Final VST3: pluginval 1.0.4 strictness **5, 6, 7, 8, 9, 10: SUCCESS**, including the external Steinberg validator
at 10. Standalone Steinberg extensive validation: **537 passed, 0 failed**. Logs:
`docs/reports/pluginval/v2/`, `docs/reports/vst3validator/validator_v2.txt`; summary and tested binary hash alongside
this report. Final unit tests: **707 / 707**.

Distribution in `dist/` (local build artifacts, not checked into Git):

- `HamloMag_VST3_Setup_0.2.0.exe` and `.sha256`:
  `22dac13aaf286ab5b8f7f8539ec9194049b0d48a1f209e069a7c86699d570ebc`.
- `HamloMag-0.2.0-win64-vst3.zip` and `.sha256`:
  `e1989acff1449149c208851ca677a6b1ba9083542772a33a065cce66ef391a8d`.
- `HamloMag.vst3`, including README, third-party notices and the JUCE license; ZIP contains the same bundle.

ZIP CRC integrity, exact binary identity against the validated build and distribution copy, packaged README /
license identity, and both published SHA-256 files were checked successfully.

The installer was compiled successfully with Inno Setup; it was not installed into the owner's DAW during
verification. Rebuild / validate / package with `Tools/Validate-Release.ps1` and `Tools/Package-Release.ps1` after
the standard CMake Release build. No push was performed.
Panel inspected: [panel_v2.png](panel_v2.png); no control overlaps found.
