# HamloMag 0.2.1: close G2 and G12

Owner request: achieve the two original targets, without changing thresholds or the lab.
Baseline: 0.2.0, G2 -77.8 dB and G12 0.58 LU. Full historical report: V2_REPORT.md.

| Goal | 0.2.0 | 0.2.1 | Original target |
|---|---|---|---|
| G2, 8x alias suppression | -77.8 dB | -105.6 dB | <= -90 dB |
| G12, worst mix loudness error | 0.58 LU | 0.49 LU | <= 0.5 LU |
| G13, steady-noise gain movement | +/-0.04 dB | +/-0.08 dB | <= +/-0.3 dB |
| G1, legacy renders | 18/18 bit-identical | 18/18 bit-identical | bit-identical |

The unchanged lab passes 15/15: targets_fix6.md. No thresholds, clips, measurement windows or lab source changed.

## Changes

8x uses -120 dB half-band FIR stages and first-order antiderivative antialiasing (ADAA) for the original TUBE
curve. The integral is evaluated between consecutive driven samples, with a midpoint fallback near equal
samples. The legacy 1x/2x/4x paths are untouched. ADAA contributes half a high-rate sample of phase delay;
fixed host latency for the complete maximum path is now 395 samples at 48 kHz (8.23 ms). Filter latency is
measured with the same impulse/correlation procedure used by the existing implementation.

MATCH retains its gated 1.5 s feedforward power average and 0.5 s gain smoothing, adding a unity-weight residual
correction from compensated-output power. Compensated power is estimated using K-weighted pre-gain power times
the applied gain squared. It is a realtime tracker, not a guarantee of exact normalization for arbitrary audio.
The MATCH-off path remains unchanged. Feedback states are reset in prepare and clocked per sample / 100 ms
boundary, independent of host block size. Block tests now cover both TAPE and the stateful 8x TUBE path.

## Experiments disclosed

1. Stronger FIR alone: G2 stayed -77.8 dB. Faster 0.5 s / 0.1 s MATCH: 0.57 LU (targets_fix.md).
2. Analytic blend of tube half-waves: -79.0 dB; 0.1 s power / 0.05 s gain MATCH: 1.30 LU (targets_fix2.md).
3. ADAA: -105.6 dB; cumulative open-loop MATCH: 0.99 LU (targets_fix3.md).
4. Fast integral-only feedback MATCH: 1.42 LU (targets_fix4.md).
5. Original feedforward with half-weight residual feedback: 0.51 LU (targets_fix5.md).
6. Unity-weight residual feedback: 0.49 LU, G13 +/-0.08 dB (targets_fix6.md). This is the retained design.

The unsuccessful candidates were replaced, not reported as passes. Targets remain unchanged throughout.
## Release regression

Final 0.2.1 lab: **15/15**, targets_release_lab.md; legacy G1 **18/18 bit-identical**.
All **200/200 presets pass** without recipe or DRIVE changes: ../presets/genre_presets_check_021.md.
Unit suite: **711/711**, targets_unit_tests.txt, including block independence of the new ADAA state.
pluginval strictness **5 through 10 pass**, including external validator at 10; standalone Steinberg validator
**537 passed, 0 failed**. See targets_validation_summary.txt and ../vst3validator/validator_v2.txt.

Installer and ZIP were built and their SHA-256 files verified; ZIP CRC, packaged README and exact binary identity
against the validated build were checked. Evidence: targets_artifact_checks.txt.

- `dist/HamloMag_VST3_Setup_0.2.1.exe` (+ .sha256)
- `dist/HamloMag-0.2.1-win64-vst3.zip` (+ .sha256)
- `dist/HamloMag.vst3`

The installer was compiled, not installed into the owner's DAW. No push was performed.
