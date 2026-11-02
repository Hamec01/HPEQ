# HamloShirota 0.2.0: VST-only library acceptance

2026-11-02; parent5765c5e. The owner's latest request excludes the installer.
The delivered Windows x64 VST contains207host programs: Default, six original
presets and200new spatial recipes. No installed plug-in, other product, push or
subtree was changed. FL/K712 listening and live-host deadline checks remain
pending; this report does not substitute offline tests for owner audition.

## Library and measured results

Before:7programs/new0; library checks n/a. After:207programs/new200. The same
native catalog/render driver and final method are used for both. Old first7names,
values and indices are unchanged. Four genres x50: Boom Bap, Trap, Techno, DnB.
Each genre has Kick6, Snare/Clap6, Hats/Cymbals5, Percussion4, Drum Bus7,
808/Bass5, Vocals5, Keys/Piano4, Samples/Melody4, Master4. Each entry has a
permanent ID, purpose/note, genre/group, material, tempo and depth. See
`docs/GENRE_PRESETS.md` and `catalog_final.json` for the complete list/settings.

| Check | Result |
|---|---|
| Calibrated and verified genre programs | **200/200** |
| Unique sound recipes, excluding Mix/Output/UI-only fields | **200/200** |
| Actual saved/restored parameters, selected program and audio | **200/200 exact** |
| Native expectations | **2457 passed,0failed** |
| Original M0 audio/state/latency | **160/160 bit exact** |
| Existing modern31PCM renders versus S4 | **31/31 identical SHA256** |
| Final integration lab | **106/106** |
| Source/cache/actual-embedded-bank rejection | **5/5** |
| Resampled integrated binaural filters | **96/96** |
| Worst preset8xtrue peak | **-1.049999dBTP**, target<=-1 |
| Production owner-material listening files | **47**, common gain within each material |
| pluginval | **5..10, all exit0** |
| Steinberg validator | **537/537** |

Spatial depth is the energy of the output change relative to PDC-aligned dry,
with Output gain removed. This is not a reverb wet/dry or perceived loudness
metric. Gentle:-24..-16dB, Medium:-16..-8, Heavy:-8..0; calibration targets
-20/-12/-4. Each author recipe was rendered at full strength; Spatial Mix was
derived from that residual and then checked in a fresh actual program render.
Output can only trim peaks and cannot manufacture the depth result. Inputs use
the owner's highest-energy10s window plus1s silence, prepared at48k with recorded
source/input hashes and peak cap. Bounds/methods preceded measurement in
`docs/S5_METHODS.md`; none were relaxed. All final audio processing/setter
allocation checks pass. Static block tests exercise every207programs.

Cache records bind sound/parameter/routing/build sources, method, recipe, input
bytes and tempo; each native call records its executable SHA. Generated catalog
and UI are intentionally outside the sound fingerprint, while every compiled
recipe is independently compared with author/calibration settings. Interrupted
native attempts are retained; only complete hash-verified records resume.

## Failures found and corrected

The first BB Bass Answer reached-12.097dB at full strength, narrowly missing
the chosen Medium center(-12). The target band was already reachable, but the
calibration-center rule still failed. Its room amount and beat-related predelay
were increased; other199valid recipes resumed from verified cache. Original
failed output and `calibration_initial.json` remain retained.

The first project-state pass stopped on BB Open Tiles: saved Width3 could show
Width1 after loading into an edited instance, even though DSP audio was already
identical. A silent reset before APVTS state replacement left the parameter and
its adapter out of sync when the pre-load value equaled the saved value. Reset
now notifies the parameter listeners. A regression covers equal pre-load values,
stable-ID selection with a stale numeric program index, and return to the exact
compact Default. Temporary diagnostic instrumentation was restored byte-for-byte
before acceptance; final before/after laboratory method fingerprints match.

Changing this state code conservatively invalidated the sound fingerprint.
All200calibrations,96filter checks and200state/audio validations were repeated.
The200full-strength PCM hashes remain identical to before the state repair.
Prior reports/attempts are retained under `before_state_fix_*` and `out/s5`.

Four-node UI inspection also caught the rear node number overlapping BACK.
Node numbers now sit inside the circle; the initial screenshot is retained.
QUICK/STEREO/SPACE/MOTION/LEGACY snapshots were inspected. Shared PresetScreen
stays unchanged at56,20,336x116; genre/group/note and LOADED/MODIFIED are visible.
Final validators target the corrected UI binary in `validators_release/`;
the earlier `validators_final/` is historical, not the delivered fingerprint.

## Binaural rates and performance

Actual integrated Point impulses cover three profiles and front/left/back/right
at eight rates. Magnitude is compared independently to the native48k bank in
100Hz..min(16k,.4fs), above-40dB source magnitude. ITD is compared with an
independently resampled native reference. All meet<=0.5dB and<=1sample.

| Rate(Hz) | Maximum magnitude error(dB) | Maximum ITD error(samples) |
|---|---:|---:|
| 8000 | 0.337581 | 0 |
| 32000 | 0.052630 | 1 |
| 44100 | 0.004138 | 0 |
| 48000 | 0.000002 | 0 |
| 88200 | 0.000121 | 0 |
| 96000 | 0.000121 | 0 |
| 176400 | 0.000118 | 1 |
| 192000 | 0.000120 | 1 |

The original SADIE II source bank, production bank, S4 DSP kernels and parameter
layout remain unchanged. Full upstream/modified-data notices and license are
included in the complete VST bundle. No runtime data download is needed.

Combined worst-case48k/256CPU, three8s runs after1s warmup, on the available
i5-10300H; all means meet the pre-recorded<=25%block-period target:

| Run | Mean / block period | Largest wall callback / period | Thread cycles at largest wall / median |
|---|---:|---:|---:|
| 1 | 11.332% | 0.24598 | 1,486,618 / 1,406,746 |
| 2 | 11.448% | 0.27313 | 3,593,493 / 1,411,488 |
| 3 | 11.159% | 0.20921 | 2,582,862 / 1,405,497 |

This is not proof of FL callback deadlines. S4 previously recorded a1.0062period
callback, and earlier S3 larger outliers remain unexplained. Final lower numbers
do not erase them. Owner FL Studio/K712 audition is still pending. Horizontal
profiles are not personalized HRTF or headphone EQ; perceived behind-head
placement varies. No elevation/head tracking is provided. Legacy retains its
original block dependence and compact-state omission of program metadata; modern
genre projects preserve their permanent ID/program and exact settings/audio.

## Delivery and reproduction

Delivered bundle: `E:\HamloShirota\dist\HamloShirota-0.2.0-VST3\HamloShirota.vst3`.
DLL SHA256: `49864b3ddcfb8f34608d3201f9ab3e6beba14dd44df59f10156f148417e9b73e`.
Keep/copy the entire HamloShirota.vst3 folder, including Resources. The adjacent
SHA256SUMS.txt covers every file. No installer was built, as requested.
`acceptance.json` and `delivery.json` verify the copied bundle against validators.

Run commands from E:/HamloShirota, using isolated build/s3_venv Python:

```powershell
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py calibrate
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py verify
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/prepare_s5_demos.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/report_s5.py
```

Do not overwrite native run directories or old evidence. Changed DSP/method/
material requires a new matching cache identity. Full build/validator commands,
final commit and open owner checks are recorded in `docs/NEXT_STAGE.md`.
