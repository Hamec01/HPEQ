# HamloZastava: S4 complete, S5 preset library next

2026-11-03. Parent1341976; S4completion commit follows this handoff (git log -1).
No active jobs or unrelated changes. No subagents, other products/HPEQ, installed
plugin update, installer, ZIP, push/subtree or model switch. Apply owner's
efficient-staged-work; one coherent stage per chat, targeted reads.

## Settled scope

All four roles Clean/Drum/Duck/Rhythm and separate Soft/Hard; modern QUICK and
ADVANCED plus Legacy; Black Steel/shared top-left PresetScreen unchanged.
200genre recipes next:50each Boom Bap,Trap,Techno,DnB,207host programs total.
No scope question pending. VST-first; installer not immediate. Product English,
chat Russian. Do not resume other finished products from historical AGENTS.

## S4 delivered and verified

ModernZastavaPanel owns Quick six role-appropriate knobs, mode/profile/power,
neutral INIT, numerical Advanced Envelope/Key-Mix/Rhythm tabs,32editable percent
steps, tempo/free clock, contextual tooltips/footer and status. Inactive controls
are dimmed/disabled (Drum uses Body/Tail, common Release disabled; Rhythm Link
is disabled). Shared PresetScreen.*, BlackSteel.*, HamloZastavaPanel.* unchanged.
Legacy remains separate; original seven recipes/numbers are unchanged.

New v2_listen appends at77:78parameters total, all prior77definitions/order
retained. Pure kernel Settings.listen appends at end. Signed HP/LP-filtered key
is delayed5ms and crossfaded20ms in prepared rings; it ignores gate gain/Mix/
Output/Processing power. Host bypass or missing external key overrides it.
Modern view/tab restore via metadata. Old states clear Listen and select Legacy.
Host playing is read into an atomic on the audio thread for accurate stopped
Rhythm status; do not query host position on the GUI thread. TARGET GR is the
internal envelope before Mix, not a claim of actual reduction while bypassed.

465native,283frozen S3host,275frozen S2numerical checks pass. All66listen-off
PCM files equal freshly rendered immutable pre-S4kernel AND accepted old S2PCM,
including five-minute192k stress.229primary+458restored M0matches remain exact.
Actual host Listen output, prepared restoration and direct host bypass pass
all6main/key layouts; pure kernel tests cover30rate/layout combinations.
Audition HP/LP rejection37.797122/40.025014dB (goal>=30),20msfade endpoints,
blocks1/64/127/512/4096 exact and zero measured C++ callback/setter allocations.
15actual screenshots inspected at.5/1/1.5scales, including Listen/missing key/
stopped transport. pluginval5..10 and Steinberg537/537 on final DLL.

Authoritative docs/reports/s4/REPORT.md,acceptance.json,before.json,kernel.json,
host_complete.json,legacy_complete.json,native_roles.txt,screenshots_final.json,
images_final/,validators_final/identity.json. Controls guide:docs/ZASTAVA_CONTROLS.md.
Raw pair:out/s4/before_kernel and after_kernel; S3archive:out/s4/baseline.

Internal version0.1.0;13owned production units, no foreign graphs/processors.
Current bundle:build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3.
FinalDLL SHA256:
4634d22972057cf64460b46e63609aceca1cd702fd8ddc9cffe6b7438b134404
S4source ID:
dda97ff70588c3f12ebeb682d81847f87901502599affa9ceb1b0d7131287eba
Kernel source ID:
9877b2194c06629e6b0f93535f8beb528dc748a7d201f949176bda3829c206ac
S2sourcec08e4c9d... is historical:audition extends it, Off PCM remains exact.
No owner FL listening verdict, whole-VST CPU benchmark or completed200library.

## State/audio details

ModernParameters.h/.cpp owns fixed enum/spec/mapping. Append future fields after
existing78, never insert before existing steps or alter original parameter data.
Full schema1 includes v2_program, v2_view, v2_tab. Old missing schema/engine
selects Legacy and clears new fields. Missing schema1fields use defaults; invalid
root/malformed/future states ignored. Full load atomically resets modern history
on next callback; buffers are not serialized. Preserve exact normalized grid
canonicalization for new DSP/state to avoid APVTSunsnapped-default byte drift.

Legacy with all modern defaults uses original16field encoding, retaining M0
bytes and historical program restoration (including191readback quirks). Legacy
with retained modern edits uses full schema. Editor/preset state is not allowed
to change old audio. Original Default program is Legacy; INIT is neutral modern
Clean/processingOff. UI labels this distinction and marks recipe differences.
S4Legacy screenshot has retained32step setting, intentionally showing '*'.

Both prepared paths run original main input; fixed5msPDC once per path.20ms
Legacy/modern crossfade, lastModernMode retained during outgoing fade. Duck user
Attack maps to kernel falling gain, Release to rising. KeyAudio scratch must be
copied BEFORE Legacy writes main, including aliased key/main input pointers.
All audio histories and scratch allocated in prepare. Host direct bypass and
parameter bypass retain PDC. Do not reintroduce allocations or double delay.

## Disclosed revisions

Initial202S4checks passed but numeric test only called setValue; strengthened to
actual value-label text commit/percent parsing/gestures. Header/pattern clipping
and zero-valued text using JUCE's old formatter were fixed; units/OFF checked.
Actual host-output/prepared-restore/bypass cases added to close integration gap.
Visual review then found two enabled no-op controls (Release in Drum,Link in
Rhythm), corrected with two regression checks; final native465. Earlier463pass
results, pictures and validators remain intermediate evidence. Corrected DLL
revalidated and current host/Legacy/screenshots recaptured. No target loosened;
frozen S2/S3primary methods untouched. Fifteen final PNGs reviewed:13equal earlier
inspected images,2changed images (Hard Drum/Legacy) inspected again.

## Reproduce current checks

PowerShell E:/HamloZastava. Python/numpy/scipy:
E:/HamloShirota/build/s3_venv/Scripts/python.exe (-I -X utf8). No upgrades.
JUCE local E:/HPEQ/build/_deps/juce-src (read dependency only, don't edit HPEQ).

cmake --build build --config Release --target HamloZastava_Tests HamloZastava_V2Lab HamloZastava_S3HostCurrent HamloZastava_S3LegacyCurrent HamloZastava_VST3 HamloZastava_Snapshot -- /m:2 /v:minimal
build/Release/HamloZastava_Tests.exe
build/Release/HamloZastava_V2Lab.exe out/s4/NEW_kernel
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/analyze_zastava_s2.py out/s4/NEW_kernel out/s4/NEW_kernel.json
build/Release/HamloZastava_S3HostCurrent.exe out/s4/NEW_host.json
build/Release/HamloZastava_S3LegacyCurrent.exe verify E:/HamloZastava/out/s3/reference_v3 E:/HamloZastava/out/s4/NEW_legacy.json
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_zastava_s4.py audit

Captures refuse overwrite. S4audit is pinned to this stage's current sources/
method/binary/results; source changes require new stage evidence, not relabeling.
S2methods remain unchanged; pure-kernel before is the archived accepted executable.
M0sources:out/s1/original, final same S3method M0reference:out/s3/reference_v3.
Optional archives/defaultOFF; current cache enables M0/S2references. Old S1CompatLab
has historical bus assumptions; use S3Legacy driver for current optional key bus.

Snapshot args:output.png program view engine tab scale listen external profile
processing. Tool prepares synthetic input to settle readouts if processing arg
present. Tools/capture_zastava_s4.py captures fixed cases with source/exe hashes;
existing images/metadata may not be overwritten. Source identity includes all
Source files; GUI edits also invalidate global stamps even when DSP is unchanged.

## S5 next coherent stage

Build200distinct recipes,50per approved genre,with stable permanent IDs and
material/tempo/purpose/depth notes. Keep original7host numbers/recipes and append
200programs (207total). No preset list approved by the owner is missing; use
musical judgment and existing scope, don't ask four-role/profile questions again.

Current mono-main/stereo-key mapping uses the first key channel for detection/
audition (documented/tested); stereo main uses both. Review this policy before
release if broader mono-sidechain aggregation is desired; any change is DSP and
needs fresh evidence/cache identity before calibration.

Current setCurrentProgram applies old15values then resets modern fields. Extend
it to apply each modern recipe's v2_ values (default Engine0for original programs,
Listen0for recipes), preserving old7exactly. isProgramModified already compares
modern presetValue overrides; maintain this. Shared PresetScreen must get genre/
group/name/note entries through its public API, without changing its source.
Modern full-state selected program must remain correct; consider stable recipe
ID metadata with index fallback. Old encoded Legacy behavior stays intact.

WARNING for tests:new207programs invalidate S3driver assumptions getNumPrograms==7
and enumeration of all programs. Clone the frozen drivers for S5, explicitly
restrict Legacy contract/cases to original7, retaining exact229fixtures; measure
matching M0/current method, don't silently weaken existing S3files. Add separate
207program/library tests. Source edits make current S4vendor outputs stale.

Record per-role calibration metrics/targets before measuring. Use annotated
synthetic wanted/noise/hits and owner's material; don't impose compressor GR or
reverb wet/dry bands on a gate. Distinct recipes beyond Mix/Output,finite/peak/
state/audio/uniqueness checks,200/200. Cache by DSP/mapping/method/material hashes
and stable recipe identity; support continuation and reject another DSP version.
Owner material was available in C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/
ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav (verify existence).
Do not treat S2owner A/B files as new calibrated library evidence.

After calibration:final source-bound native/compatibility/GUI/VST validators,
README/report,0.2.0 VST distribution/hash and owner FL audition. Installer is not
requested now. No push/subtree without owner request. Recommend Astra xhigh for
metric/recipe design or unexpected failures, medium for known calibration/build/
validation/package runs. Owner controls selector; agent has not switched models.

Continuation prompt: Continue HamloZastava S5 from docs/NEXT_STAGE.md:200genre
presets with guarded calibration, no subagents, original7programs preserved.