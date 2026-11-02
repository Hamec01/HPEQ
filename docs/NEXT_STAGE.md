# HamloZastava: S3 complete, S4 modern editor next

2026-11-02. Parent35f6917; S3completion commit follows this handoff (git log -1).
No active jobs, unrelated changes, subagents, installed plugin update, installer,
push/subtree, other product/HPEQ changes or model switch. Apply the owner's
efficient-staged-work skill and keep one coherent stage per chat.

## Settled choices and delivery

All four roles:Clean cleanup,Drum tails,Duck external key,Rhythm; separate Soft/
Hard profiles. QUICK/ADVANCED, Black Steel, unchanged shared top-left PresetScreen.
Later200genre recipes (Boom Bap/Trap/Techno/DnB,50each),207host programs total.
No scope question pending; do not repeat owner questions. VST-first, no immediate
installer. Verb/Echo/Shirota/other products are outside this work.

S3 integrates the unchanged S2kernel into VST. Twelve owned production units;
77parameters:old15+bypass unchanged at0..15,61permanent v2_ IDs appended16..76.
Source/Standalone/ModernParameters.h/.cpp owns enum, specs and mapping. Do not
insert/reorder old OR these appended IDs when adding controls later.

Fresh startup:Clean,Modern Processing off, exact5msdelayed dry. Seven old host
programs (Default+six) still select their original Legacy recipes and reset all
modern fields. Default's retained Legacy recipe is distinct from neutral startup;
the new editor must represent the modified/neutral state honestly.

Production build remains internal0.1.0. Bundle:
E:/HamloZastava/build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3
Validated DLL SHA256:
fe8d11b5e7b2ae908b988c6a3a130b5a26b3934c9907ffebdbbb8e427cdd03f3
The editor is still the old Legacy panel. Host-accessible new parameters work,
but do not present the panel as a finished modern interface or request listening
approval for the whole product. No final ZIP/installer/dist copy was made.

## S3 verified

283host checks,219native,229primary M0 PCM/state/latency matches plus458fresh/
edited actual M0 restorations. Same-event PCM exact at1/64/127/512/4096 after
prepare256. Mono/stereo main and independent mono/stereo key;40neutral matrix
cases across8/44.1/48/96/192k.48exact comparisons against independent S2 Gate.
Zero C++ audio callback allocations, including first call, mode changes,
prepared state restoration, oversized callbacks and host direct bypass.
pluginval5..10 allpass; Steinberg537/537. Four exact-pointer main/key alias
layouts now pass. Old seven programs and first16parameter contract unchanged.

Authoritative docs/reports/s3/REPORT.md and acceptance.json, before_v3.json,
after_host_v4.json,after_legacy_v4.json,native_v3.txt,validators_v3/identity.json.
The final primary method is v3; current run suffix v4 is not another method.
M0reference:out/s3/reference_v3; M0sources remain out/s1/original. S1baseline
bundle archived out/s3/baseline/HamloZastava.vst3 before any S3rebuild.
Source ID:c2cd5c52e2fb2cf848903f2bd5c7d09ee50b598b9aff1d393f80bc59a16d600e
Method ID:9069dde9dd70d55f28d043844bd13a20446a35af6561d7c00dab35653c891b26
Kernel files still match S2sourcec08e4c9d...; its275offline checks/five-minute
stress and8owner-material renders are historical, not a new integrated FL test.
No ear/listening verdict or FL integration approval recorded. Object3304bytes
is not heap footprint; do not claim S2kernel CPU as whole-plugin CPU.

## State and audio details to preserve

New schema1 includes v2_schema/v2_program. Old state (missing schema/engine)
selects Legacy and clears every new control on fresh or edited instances.
Missing individual schema1fields reset to defaults; malformed/wrong-root/future
schema ignored. Full load signals an atomic audio-thread modern reset; histories
are not serialized. Both fresh-before-prepare and already prepared/edited loads
are tested. Old normalized-readback quirks191 remain; do not fix them globally.

Legacy plus neutral modern controls encodes only the original16fields, preserving
M0bytes and historical program-restore behavior. Legacy with edited modern
controls uses full encoding so they survive future switching. Modern full states
restore selected program, all new controls, serialized bytes and tested audio.

APVTS initializes raw defaults unsnapped (e.g.124.999992 vs125ms). Canonicalize
only new controls to their declared grid in BOTH modern::settings and full-state
serialization. First candidate failed17byte comparisons despite matching reported
parameters/program/audio. Fix has no weakened tolerances or Legacy changes.
Do not revert this to raw APVTS reads or change global Legacy normalization.

Both prepared paths run on original main input; one fixed5msPDC per path.20ms
Legacy/modern crossfade; lastModernMode retains outgoing family while fading to
Legacy. Duck UI Attack maps to kernel falling-gain time, Release to rising.
All buffers, including keyAudio, allocated in prepare. Copy external key BEFORE
Legacy writes main:hosts may share their input pointers. Do not rely on distinct
bus memory. Direct processBlockBypassed and parameter bypass both retain PDC.
BPM/PPQ/transport/chunk offsets map to S2 Clock. Missing key status:
processor.modernReading().missingKey. Legacy panel still reads moduleNode meters.

## Disclosed revisions and retained evidence

Initial283host checks:266pass/17state-byte failures; Legacy229/native206passed.
Diagnostic host method splits parameter/byte/program/audio cause, no changed
criterion. Grid fix then283host/207native and vendor validators passed.
The initial native suite omitted shared bus memory; added4alias configurations
failed on that candidate. Prepared key copy fixes them; native219 passes. Retain
out/s3/alias_regression_tests.exe, candidate_01/candidate_02 sources/logs and
all docs/reports/s3 earlier runs (including after_host.json17failures).

Final disclosure changes method stamp, so both M0 and current primary drivers
were recaptured using the identical final sources. All229baseline fixtures and
458restored observations equal S1v3. Final H01/H02... checks unchanged. Raw
failed/intermediate files are diagnostic history, not final acceptance. Vendor
validators_v3 test the corrected DLL, not earlier validators folder's DLL.
The unchanged S2zero-fill literal causes benign C4244; don't casually edit the
kernel merely for that warning and invalidate all source-bound evidence.

## Commands and continuation

PowerShell at E:/HamloZastava. Python/numpy/scipy:
E:/HamloShirota/build/s3_venv/Scripts/python.exe (-I -X utf8). No upgrades.
JUCE local E:/HPEQ/build/_deps/juce-src; don't edit HPEQ. Current cache enables
HAMLO_ZASTAVA_BUILD_M0_REFERENCE=ON and S2reference; optional controls are
EXCLUDE_FROM_ALL, defaultOFF for fresh production builds.

cmake --build build --config Release --target HamloZastava_S3HostCurrent HamloZastava_S3LegacyCurrent HamloZastava_Tests HamloZastava_VST3 -- /m:2 /v:minimal
build/Release/HamloZastava_S3HostCurrent.exe out/s3/NEW_host.json
build/Release/HamloZastava_S3LegacyCurrent.exe verify E:/HamloZastava/out/s3/reference_v3 E:/HamloZastava/out/s3/NEW_legacy.json
build/Release/HamloZastava_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_zastava_s3.py audit

Captures refuse overwrite. Frozen S1method untouched; S3Legacy clone only adjusts
identity stamps and optional disabled auxiliary layout, preserving all fixtures.
S3primary methods:Tests/S3HostLabMain.cpp,Tests/S3LegacyLabMain.cpp,
docs/S3_METHODS.md. Supplemental native:Tests/S3IntegrationTests.cpp. Audit binds
source/method/raw controls, current compiled stamp inside DLL and validator SHA.
Source edits invalidate accepted current reports. For S4save fresh reports/audit
rather than overwriting S3acceptance or relabeling S3vendor output. Existing
unchanged-method M0reference may be reused; changed methods require matching pair.

## S4 next coherent stage

Build modern QUICK/ADVANCED editor on Black Steel, keep Legacy panel accessible
and shared PresetScreen files/placement unchanged. Add explicit4mode selection,
Soft/Hard, useful quick controls, numerical Advanced settings,32step editor,
external/missing-key status, meters and English contextual help. Modern power
must reflect both v2_enabled and bypass, so fresh neutral startup looks inactive.
Persist appropriate editor/preset state and modified marker; preserve all host
program recipes/IDs. Current seven programs are not the eventual200library.

Detector audition is still pending for modern modes. Legacy keyListen does not
control modern DSP. If implementing new listen output requires a kernel/API
extension, append the new parameter after existing77 and prove listen-off old
PCM exact with fresh paired evidence; do not reuse stale S2/S3numerical caches.
Test PDC/main-key routing and state for audition too. Do not copy foreign panel
logic or change other product repos to implement this.

After UI:200calibrated distinct genre recipes (50each;207host programs total),
resumable source/method/material cache guards, final VST/report/visual snapshots/
validators and owner FL audition. No installer requested now. Recommend Astra
xhigh for UI architecture/key-audition DSP ambiguities, medium for known layout/
build/check work. Owner changes model/effort selector; no agent switch occurred.

Continuation prompt: Continue HamloZastava S4 from docs/NEXT_STAGE.md: modern
QUICK/ADVANCED editor, no subagents, keep all four approved modes and profiles.