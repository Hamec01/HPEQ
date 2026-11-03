# HamloFader: F2 complete, F3 host integration next

2026-11-03. Parent68f98ed; F2completion commit follows this handoff (git log -1).
No active jobs. Apply efficient-staged-work: no subagents, targeted reads,
source/method/input guards and durable stage handoffs. No other products/HPEQ,
installed copy, installer, push/subtree or claimed model/effort switch.

## Settled scope and current deliverable

Owner approved full Trim / Match / Ride / Motion, separate modes; Ride Natural/
Fast and adjustable boost/cut range. Do not ask again. Black Steel/shared
PresetScreen stay. Original8IDs/programs0..7 and class/state contracts preserved;
200newmeaningful recipes will bring208programs in F5. No cross-instance groups,
new MIDI input, DAW curve writing, limiter/saturation or frequency split assumed.
Exact kernel contracts:docs/HAMLOFADER_V2.md; initial audit:FADER_V2_AUDIT.md.

F2kernel is implemented/tested offline. Source/DSP/FaderKernel.h/.cpp and
FaderMeter.h/.cpp are not yet in HamloFader_Shared. Shipping remains F1internal
0.1.0 with the original interface and8parameters/programs. Do not present it as
a new-mode VST or completed0.2.0. Host/UI/library/listening are still pending.

F1from68f98ed remains untouched:9owned units vs32;226M0primary+452actual
restores/678exact PCM,49native,pluginval5..10/Steinberg537;0sample latency.
Archiveout/audit/original,referenceout/f1/reference,immutable original driver
out/f1/reference_lab.exe. Source/UI/frozen F1methods and validated DLL unchanged:
c8489ef44ed8ebbaa7fdf44c284f7e5514876cce01a917dbab5e770f0ed02841.

## F2 acceptance and preserved failures

Final281lab/0failed/0n-a,61cases;59unit/0failed. Matching F1control30pass,
13new-feature categories n/a. Input/output RMS/ITU oracle, separate TP Annex2
convolution and16xsinc comparison, frozen Match/peak limits/silent-short learn,
Ride targets/range/idle, Motion curves/seek/free/stop, routing and exact event
schedules tested. Zero C++audio/setter/action allocations. Five-minute192k
stereo stress and two-hour finite Icapacity tested. 48k/256mean1.717423% of
block duration (limit25%);broader-rate maximum5.386000% on actuali5-10300H.
This is a standalone kernel wall-time observation, not FL CPU approval.

Initial candidate278pass/3fail: rounded free phase increment broke exact cycle
repetition. Integer sample cycle fixed it. Stronger stereo mode probe44pass/1fail:
shared start gain caused a right-channel jump; independently faded channel gains
fixed it. Later cancellation probe58pass/1fail: cancelled Learn could propose
stale gain; off/bypass now clears the unfinished measurement. Final pair v4
recaptured with the same final method. No audio criterion relaxed. Premeasurement
Output/Mix/bypass contract clarifications are hash-verified against first method.
All failed/intermediate evidence remains; final audit detects5regressions and
4stale/corrupt source/method/raw/manifest guards. Valid cache replay is exact.
Official EBU audio/full EBU Mode/LRA/certification and owner music/listening
were not claimed; tests use owned synthetic signals and independent SciPy.

Authoritative docs/reports/f2/:REPORT.md,acceptance.json,before_v4.json,
after_v4.json,unit_v4.txt,guards_v4.json,frozen_tools_v4.json. Raw inputs/outputs:
out/f2/before_v4 andout/f2/after_v4. Frozen tools/sources:out/f2/accepted_v4.
Important:paths before_final/after_final/accepted are intermediate v3evidence.
Do not overwrite them. Regressions atout/f2/candidate_v2 (stereo) and
out/f2/accepted/unit_cancel_probe.exe (cancel);initial repeat failure raw remains.
Final source09214975ed4bf07732709b4ba3e961a5c04446a70959bb2cf8daa1efcab10a7e.
Final methodb0024df55e43b3a5e3810342f91610a32cd09d4bb1fe1044431f39b4e6cc2f34.

## F3 next actions

Record host integration numerical/state contracts and paired baseline before
changing the VST. Append new IDs after original8, preserve old layouts and
programs. Modern default Trim is neutral. Old-state load selects Legacy and
resets every new field. Preserve actual historical old-state/program/audio
behavior on already-prepared instances, not an assumed reset.

The offline kernel APIs are single-audio-thread only. Add prepared audio scratch,
atomic reading snapshots and bounded sample-positioned action handoff. Avoid
message-thread access to kernel internals. Learn Start/Stop returns a proposal;
Apply is an explicit update of settings.matchedDb and persistent host state.
Off/bypass cancellation clears unfinished data. Reset retains configured fixed
Match gain but clears measurements/history/learning. Meter Ihas a visible
72000window cap; distinguish readiness/full flags. RMSlearn includes pauses,
LUFSlearn uses gates. Peak guard is proposal-only, not a limiter.

Store modern settings/fixed Match gain/selected preset/page appropriately;
restore projects exactly on fresh/prepared instances. Project load restores all
settings; factory preset load must clear stale actions/learning. Keep all original
IDs/ranges/choices/order and0sample PDC. Clock supplies PPQ/BPM/sample position;
handle seek/stop/no PPQ. Common enable/bypass fade20ms, routing/polarity10ms.
Independent channel delay is an effect. Full F2contracts and all ranges in design.

Frozen F1lab requires exactly8parameters/programs. Clone it with original-prefix
contract checks for F3; recapture matching original/current with that clone,
preserving old signals/positions/actual restores. Do not silently edit old F1
method or claim its source-wide audit still applies after host changes. F2audit
also asserts F1shipping files unchanged: once F3changes them, archive this F2
acceptance and make a stage-specific audit rather than bypassing its guard.
Then F4QUICK/ADVANCED/Legacy, F5actual-plugin calibration/library/release.

## Commands and environment

cmake --build build --config Release --target HamloFader_F2Current HamloFader_F2Tests -- /m:2 /v:minimal
build/Release/HamloFader_F2Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_f2.py

Reference target is optional:-DHAMLO_FADER_BUILD_F2_REFERENCE=ON, requires local
out/f2/baseline. Normal builds don't need the archive. Reference/current lab
refuse existing capture paths. Analyzer resumes only matching pinned raw data;
choose new report labels for changes. Do not restart valid jobs gratuitously.
JUCE8.0.12read-only pathE:/HPEQ/build/_deps/juce-src; Python/numpy/scipy already
inE:/HamloShirota/build/s3_venv. No environment upgrades. Astra xhigh recommended
for state/clock/measurement diagnosis;medium for routine build/check execution.
No actual switch performed.

Continuation prompt:Continue E:/HamloFader F3 from AGENTS.md and docs/NEXT_STAGE.md.
Connect accepted F2kernel to the VST with compatible appended parameters,
state/clock/actions and actual host-output tests. Work alone; no repeated scope questions.
