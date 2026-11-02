# S4: production VST and editor integration

2026-11-02, parent `d1ef423`. S4 is complete as a development integration,
version **0.1.0**, with seven original host programs. The 200 genre programs,
calibration, owner FL/K712 audition and final 0.2.0 packages remain for S5.
No installed plug-in or other product was changed.

## Implemented

The actual VST now provides Legacy / Stereo-Space / Binaural, width 0..4,
low/mid/high widths and crossovers, mono bass, Phase/Micro/Diffuse mono generators,
optional visible Side-energy guard and mono audition. Tight/Open/Scatter early
fields have amount, size, depth, damping and predelay. Position, source spread
and channel balance are independent. Point/Stereo Spread emitters use three
measured horizontal headphone profiles, D1/H3/H4.

Motion has 16 editable nodes, typed positions, Hold/Travel in milliseconds or
beats, shortest/CW/CCW arcs, linear/smooth curves, loop/one-shot and automatable
Restart. Travel Rate 0.25..4 changes speed without rewinding; editing a node
recipe restarts its path. QUICK provides width/space/travel plus essential
switches; STEREO, SPACE, MOTION and LEGACY provide detailed controls and tooltips.
The circular map supports node dragging. Global ON/BYPASSED and MODIFIED/LOADED
are visible on every page; page/node navigation does not modify the audio recipe.
Shared PresetScreen remains unchanged at the top left.

Original parameter IDs, ranges, order, seven programs, state root and class IDs
are preserved. 147 parameters append after the original 12, giving 159 application
parameters (the validator also exposes its wrapper parameter). Old states clear
all new fields on fresh and edited instances. Neutral Legacy serialization is
byte-identical, preserving the historical omission of program metadata; full
modern state stores all nodes, program, page and modified status. Default resets
modern controls; Legacy Default retains its original recipe. A selected modern
engine with Position off and neutral controls produces aligned dry.

PDC remains `round(0.0025 * fs)` in every mode, **120 samples at 48 kHz**. Modern
Mix 0 and bypass align to the same reference. Engine changes use a 15 ms prepared
warmup and 50 ms crossfade. Spatial timing uses two static prepared heads with
30 ms crossfades rather than sliding delay reads; mono-bass uses paired filters.
All buffers and filter preparation are outside audio. Prepared 256 accepts 4096.
Accepted S2/S3 source/data/evidence remain unchanged; V2 adaptations are separate.

## Acceptance

Targets preceded production sound changes in `docs/S4_METHODS.md`.
`acceptance.json` is an executable audit of source/data, recorded evidence and
the exact validated binary; stale identities fail instead of being reused.

| Check | Final result |
|---|---|
| Native expectations | **451 passed, 0 failed** |
| M0 audio/state/latency, original schedules | **160/160 bit exact** |
| Final paired production lab | **106 passed, 0 failed**, 31 cases |
| Frozen pre-integration facade, same method | 94 passed, 0 failed, 3 n/a |
| Identity/corruption rejection | **5/5**, including actual embedded-memory fault |
| Event output, blocks 1/64/127/512/4096 | Identical raw SHA256 |
| Audio allocations in lab | **0**, including tempo/path/engine events |
| Tested rates, finite and correct PDC | 8/32/44.1/48/88.2/96/176.4/192 kHz |
| Width/bands/generated Side/actual binaural FIR | Native checks pass |
| Size-transition out-of-carrier energy | **-43.3906 dB**, target <= -30 dB |
| Size-transition peak / bound | 0.060787 / 0.129961 |
| Mono-bass transition peak / bound | 0.131222 / 0.245651 |
| pluginval | **5 through 10**, all exit 0 |
| Standalone Steinberg | **537 passed, 0 failed** |
| UI | Five pages plus Binaural inspected at 1180 x 900 |

Details: `stage_final.json`, `stage_final_legacy.json`, `guard_tests.json`,
`before_final_v2.json`, `validators_stage_final/`. Raw data: `out/s4/stage_final`;
frozen facade: `out/s4/frozen`. Native state/migration, mono-to-stereo, Mix 0,
empty-buffer and integrated FIR assertions supplement Legacy-default validators.
The actual Point impulse matches independently read embedded taps at both sides
for all profiles. Modern event fixtures also change tempo/unit/node recipes and
switch engines 1 -> 2 -> 0 -> 1.

I07 isolates the wet 997 Hz carrier using its known direct coefficient and a
200 ms Hann window around the timing change. This checks for a pitch sweep,
not transparent automation on every input. Six timing cases and final event
coverage extended the original 25-case driver; a fresh matching final before/
after pair was run. Original runs remain retained, not relabeled. Final UI, empty-I/O and formatting fixes preserve all 31 preceding accepted PCM
hashes. A trailing-space cleanup changed the source fingerprint and triggered
a fresh build, native/lab/Legacy run and complete validator pass. First screenshots
caught knob angles, clipped model text, Legacy footer overlap and path chords;
final `stage_final_*.png` contain corrected layouts and the shared global header.

## Prepared data and performance

A separate production bank transforms S3 48 kHz HRIRs using own real-cepstrum
minimum-phase factorization and restores measured ITD around the remaining
56-sample reference; the spatial stage contributes 64 samples. The original
`horizontal.bin` remains untouched. Method context:
[SciPy minimum-phase documentation](https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.minimum_phase.html).
Three profiles x 72 directions x two ears x 512 float taps are embedded, with
SHA256 verification before use and cached sinc resampling during prepare only.
Maximum native magnitude error is **0.0000194904 dB**, ITD error **0 samples**,
maximum discarded tail-energy fraction **1.26361e-9**. Native spectral criteria
and provenance are in `Assets/Hrtf/production.json`. Full upstream license and
original/changed-data notices are bundled and verified against repository copies.

On this i5-10300H at 48 kHz/256, combined binaural/Space/mono-generation/motion
was measured in three eight-second runs, excluding the first second. SIMD
convolution and precomputed angular indices are used.

| Run | Mean / block period | Largest wall callback / period | Thread cycles at largest wall / median |
|---|---:|---:|---:|
| 1 | 12.632% | 0.27195 | 2,667,473 / 1,460,659 |
| 2 | 12.568% | 0.26846 | 2,413,382 / 1,454,419 |
| 3 | 12.415% | 0.33420 | 2,471,039 / 1,455,917 |

Means pass the pre-recorded <=25% target; the final maximum is 0.3342 periods.
The preceding accepted-binary run had means14.90..15.54% and one callback of
**1.0062 periods** (`timing_accepted.json`). This remains an open live-host concern,
not erased by the final lower result. Other S4 runs around11..13% and maximum
0.23..0.27 are retained. Cycle counts do not establish the cause of this or
larger S3 outliers. No deadline-safety claim.

## Open release work and limits

- FL Studio and owner AKG K712 front/back audition remain pending. Horizontal
  measured profiles are neither personalized nor headphone EQ. Perceived
  behind-head placement varies; no elevation or head tracking is provided.
- Magnitude/ITD fidelity is measured at native 48 kHz. Other listed rates have
  finite/PDC/routing checks, not full resampled spectral acceptance. 384 kHz is
  not claimed as tested. Complete this spectral check before final release.
- Path clock events are sample-clocked; smoothing, filter interpolation, HRIR
  acoustics and crossfades affect audible timing. Legacy retains its original
  block dependence. Guard budgets stationary Side energy, not arbitrary-window
  correlation; stereo mono-conservation excludes artistic Space/binaural paths.
- S3 listening WAVs describe the prototype's phase/delay, not the adapted S4
  bank. Prepare production A/B examples during S5 calibration.
- **200 new programs, calibration, final version, installer/ZIP/SHA256 remain.**

## Reproduction and binary

Use the existing isolated S3 Python environment, from repository root:

```powershell
build/Release/HamloShirota_Tests.exe
build/Release/HamloShirota_LegacyCompat.exe verify E:/HamloShirota/out/s1/reference E:/HamloShirota/out/s4/new_legacy.json
build/Release/HamloShirota_IntegrationLab.exe E:/HamloShirota/out/s4/NEW_RUN
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_s4.py out/s4/NEW_RUN out/s4/new_report.json
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/test_s4_guards.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/report_s4.py
pwsh -NoProfile -File Tools/validate_s4.ps1 -Label NEW_LABEL
```

Do not overwrite accepted runs or build during binary fingerprint validation.
Preparation/build targets and continuation are in `docs/NEXT_STAGE.md`.
Source/method/bank changes invalidate dependent measurements and calibration.

Validated bundle: `build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3`.
Exact DLL SHA256:
`10565fadcadcb279831ba8016483743638627ff2a3699d777fddbdcc2c854498`.
Bank SHA256: `ec0a289b2e7e2d609286cc1bd3278762186a88109f1272910541f5319958b965`.
