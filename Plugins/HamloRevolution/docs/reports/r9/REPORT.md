# HamloRevolution 0.2.1 / R9

2026-11-03, parent94051dd. Work only in E:/HamloRevolution; no installed-copy
replacement, HPEQ/HPDG edits, installer, subtree or push. Completed the requested
main sample workflow and audited the perceived lack of effect.

## Why it can sound restrained

No global strength limiter or extra hidden dry blend was found. SIMPLE starts
with TOPS (bass protected), AMOUNT50%, GROOVE intent and larger fills every4bars.
LOCK prevents edits from regenerating the pattern. The opening historical factory
pattern is also not the SIMPLE-generated pattern until GENERATE/a setting edit.
The existing LCD already says to generate; this behavior is preserved for projects.

The useful distinction is modulation, not raw subtraction from dry. On the owned
80Hz bass probe, generated TOPS changes short-window gain by only0.179dB between
p05/p95, while FULL gives6.811dB with otherwise matching SIMPLE settings. TOPS
mean bass gain stays-0.049dB. This supports the protected-bass/rare-event diagnosis
for bass-heavy material; it is not a measurement of the owner's unprovided audio.

A new explicit **BOLD** button on SIMPLE writes a strong recipe: FULL, Chaos
intent, AMOUNT/MIX100%, VARIATION65%, FILL everybar, unlock, GENERATE. Genre and
seed are preserved. It does not add a gain stage or silently change old presets.
Its drum difference ratio4.245dB exceeds the frozen-18dB audibility diagnostic.
That ratio is not a perceptual loudness score and is not an output boost setting.

| Material | Recipe | Gain spread p95-p05 dB | Mean output gain dB | Difference/dry dB |
|---|---|---:|---:|---:|
| drums | Opening factory pattern | 23.742 | -1.042 | -5.293 |
| drums | Generated TOPS defaults | 2.938 | -0.318 | 1.923 |
| drums | Generated FULL defaults | 32.050 | -4.423 | -3.795 |
| drums | Explicit BOLD recipe | 40.388 | 2.856 | 4.245 |
| 80Hz bass | Opening factory pattern | 0.036 | 0.003 | -3.293 |
| 80Hz bass | Generated TOPS defaults | 0.179 | -0.049 | 5.716 |
| 80Hz bass | Generated FULL defaults | 6.811 | -1.530 | -7.834 |
| 80Hz bass | Explicit BOLD recipe | 6.306 | -3.111 | -0.022 |

First baseline disclosure: subtraction alone looked large even on protected bass
because band recombination rotates phase. Before production edits the same lab
was extended with windowed gain/modulation and both sides were recaptured.
before.json preserves the initial method; before_v2.json and after_release.json
use the identical final ImpactLab. All8 complete PCM hashes and numeric rows
match exactly. No original audio engine/generator or recipe was changed.

## Added workflow

The common bottom toolbar appears on SIMPLE/ADVANCED and adapts to RISE/FILL.
Main workflow: LOAD SAMPLE or drop a file anywhere on SIMPLE/ADVANCED -> PLAY,
optionally LOOP -> adjust real controls -> RENDER WAV -> drag **DRAG WAV** into
DAW/Explorer, or EXPORT WAV to a chosen location. USE INPUT returns to the track.
The waveform is the source, not a misleading processed preview. Stop remains
available during a background render. Output readiness is invalidated by source,
settings, bus layout, rate, tempo or selected tail changes.

Main file loading/decoding, anti-alias resampling and rendering are on a lazy
worker thread. Audio only reads prepared immutable slots with bounded reader
claims; there is no audio lock, file IO, allocation or wait. Mono/stereo sources,
8..192kHz, up to60s; unsupported/nonfinite/too-long files retain the prior good
source. Host rate changes stop playback and prepare from cached original PCM.
The sample clock runs while the host is stopped; loop wraps do not reset the
musical pattern. Parameter IDs/order, state root and VST3 class IDs are unchanged.

Main export snapshots parameters/scenes, tempo, source and mono/stereo bus layout.
An independent processor renders one source pass plus chosen0/2/4/8s tail, with
fixed latency trimmed. IEEE32-bit float WAV preserves headroom: no normalizing or
new limiter. Invalid writes do not replace destinations; source overwrite is
refused. Temporary sample workspace audio is not embedded in project state;
project restoration stops it. Load the source again after reopening a project.

On RISE/FILL the same toolbar operates on the transition source/clip, not the main
sample. Existing transition rendering/resampling still runs on its historical
control-thread path and retains24-bit WAV/normalization behavior; its direct
sample load/export is now visible. Each rerender uses a new filename, preserving
files already dragged into a host. A failed transition write is no longer valid.

## Acceptance

- Full native suite:1286passed/0failed, including190new sample-workspace checks.
-8old paired renders exactly identical, plus existing Golden/compatibility suite.
- Sample playback at blocks1/64/127/512/4096afterprepare256: exact PCM;
  zero measured C++audio allocations. Repeated load/cancel/audio stress finite.
- Export decoded32f PCM equals an independent actual AudioProcessor render,
  including2s tail. Mix0 preserves >0dBFS samples exactly; mono host stays mono.
-44.1->48kHz pitch/duration/stereo identity pass.96->48kHz32kHz input residual:
  -112.887dBFS (additional probe target-70dBFS). Input/source failures preserve data.
- Worker cancellation/editor teardown, prepared-rate changes, stale readiness,
  source protection, write failure, transition file immutability and toolbar
  event routing pass. Closing during a queued render meets the2s bound.
- pluginval5,6,7,8,9,10 exit0; Steinberg537passed/0failed, one final DLL throughout.
- Main/Advanced/Transition/actual minimum snapshots inspected. Preset screen stays
  at the same design coordinate. The half-size minimum has very small text and
  can abbreviate the tail combo label; menu/tooltip and default size remain usable.

Intermediate evidence is retained. The first new test fixture called an assertion
before beginTest and stopped before its cases; fixed the harness ordering without
changing DSP or goals. Its stdout is retained; that first executable was not
separately frozen. Stereo-only export was found during final review; mono layout
and its explicit test were added before the first mono measurement. Earlier
validator/native runs remain intermediate, not release evidence.

No manual FL Studio/Explorer drag session or owner listening is claimed. Incoming
file-drop handlers and outgoing file readiness/routes are exercised; actual OS
cross-application drag remains an owner/host check. Full official codec or
psychoacoustic certification is not claimed. Long tails can exceed the selected
export extension; choose the tail appropriate to the recipe.

## Identity and reproduction

Source map SHA256: `977af3c9f03fdf645b49e1d60df370c57fdd27f84d34778633c6fb881da967d3`.
Final DLL: `528935bce31a497cedbfce9b113b5d19a218c5541dd101ba17488c99223acf58`.
Authoritative: native_release.txt, before_v2.json, after_release.json,
validators_release/, screens/*_release.png. acceptance.json pins sources,
methods, reports and frozen binaries/package. Tools/verify_r9.py replays the
acceptance and four rejection guards. Intermediate *_final* files are history.
Raw source/bundle before production changes:out/r9/baseline; final archive:
out/r9/accepted. Normal builds do not need those audit archives.

Build: cmake --build build --config Release --target HamloRevolution_VST3
HamloRevolution_Tests HamloRevolution_Snapshot HamloRevolution_ImpactLab -- /m:2 /v:minimal.
Run native suite, then the unchanged ImpactLab to a NEW JSON path. Validator
script Tools/validate_r9.ps1 refuses an existing label. No dependency upgrades.

Delivery:dist/HamloRevolution-0.2.1-r9/HamloRevolution.vst3 and
HamloRevolution-0.2.1-r9-win64-vst3.zip withSHA256. Packaged DLL equals tested DLL.
