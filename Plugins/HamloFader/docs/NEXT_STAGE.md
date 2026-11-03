# HamloFader: F3 accepted, F4 modern interface next

2026-11-03. Parent7af28d0; completion commit follows (git log -1). No active jobs.
Work alone in E:/HamloFader, apply efficient-staged-work. No other products/HPEQ,
installed copy, installer, push/subtree or claimed model switch. Owner scope is
settled: full Trim/Match/Ride/Motion, Natural/Fast, adjustable range. Do not ask again.

## Completed and verified

F3 host integration is in production (12owned units),68newIDs append to original8.
New constructor modern neutral Trim. Loading any original program0..7 selects
Legacy, including Default. Original IDs/ranges/order/recipes/root/VST3CIDs retained.
Modern schema3 parameters/applied Match/program/page; old states reset all new
controls/page and selectLegacy, while retaining old program/preparedLegacyhistory.
Both branches always receive input; Engine selection20ms; state/preset primes it.
PDC0, modern tail20ms alignment effect, Legacy tail0. Old shared UI unchanged;
editor meters now consume atomic status. Product still internal0.1.0,8programs.

224host checks;678exactLegacyrenders;281DSPchecks;59kernelunit;49native;
pluginval5..10 and Steinberg537 pass. Wholeprocessor48k/256maxmean1.457655%
blockduration (not FL CPU certification). Zero measured C++audio allocations.
A real previously missed Motion Reset/first-call smoothing fault was fixed:
prime only first sample, not wholefirstcall. Final regression with frozenoldkernel
fails53checks. Same F2 method rerun281/0; do not claim old F2 proof coveredthiscase.
All failed/intermediate artifacts remain. Reports:docs/reports/f3/REPORT.md,
acceptance.json,host_final_v2,legacy_final,kernel_after_v1,kernel_unit_v1,
native_final,regression_final_v2,validators_final. Replay:Tools/verify_f3.py.
F1/F2 whole-shipping guards intentionally fail now; don't edit/bypass them.
Source 535ed71398e2ffcf87e700a5845cb464732481039aefa206878974e02a57e47c.
Host method 330e8a2688064b5678a6f80282b18d2a1e5310986858ebf4a32fdea1fe4db9b1.
Kernel source db89bee1334fb56617984276aaba2c3d60f2066cafad4e200f58eac9c033f9b5.
DLL 8cf87da40406aa53ce9804264f6f15a1aa467d6e0bf2699ad565c5326237b46a.
Frozen final sources/tools/bundle:out/f3/accepted_v2;controlout/f3/baseline;
rawout/f3/legacy_reference andkernel_after_v1. Pre-fixout/f3/candidate_v2.
Original F1/F2 archives remain. No calibration results exist for the new DSP.

## F4 implementation contracts

Read only needed pieces of Source/Standalone/FaderHostControls.*,
FaderAudioBridge.h and ModulePluginProcessor.* plus old editor/shared UI APIs.
Controls array/enum is the appended ABI: don't reorder or changeold8. Reuse
old gainDb/mix/targetDb/polarity; mute disables modern; old delayMs/solo stay
Legacy-only. Engine Legacy0/Modern1; Mode Trim0/Match1/Ride2/Motion3.
68newfields include32levels; points2..32, unused levels still saved.

Keep Black Steel and PresetScreen bounds56,20,336,116 at1180x700. Do not edit
shared PresetScreen/BlackSteel/HamloFaderPanel. Add an own modern panel and
QUICK/ADVANCED/Legacy navigation; page state is0..2. Old panel is only Legacy.
F4 must expose modes, gain/routing/alignment, RMS/LUFS/readiness/full/TP,
Match proposal/limits and deliberate Apply, Ride activity/correction/profiles,
Motion curve/phase/sync/free/right-offset, output/mix/power, help/modified state.
Never imply peak guard is an output limiter or LUFS meter is fullEBUMode.
72000integratingwindows cap is visible; unready readings are not validdata.

Processor readStatus(FaderStatus&) is bounded/atomic and mayreturnfalse;
show waiting/not-ready rather than use mutable kernel/moduleNode fromUI.
FaderStatus hasreading/proposal/epoch/frame/commandSequence/legacy/bypassed
and Legacy input/output/longmeter values. requestAction(StartLearn/StopLearn/
Reset, optional absolute processing frame) returnsfalse onfull/busy/order;
UI should retry or visibly reportfailure. Immediate default means nextaudio
opportunity; timeline is processedframes, not host transport. Prepare/state/
preset invalidatesoldqueuedcommands. applyMatch() control-thread-only explicitly
sets matchedDb withhostgesture, refusesstale/pending/disabled/bypassed/nonMatch.
Stop onlyproposes. Reset clears histories/meters/proposal while retainingmatchedDb.
No control-thread access to kernel. Apply should be unavailable while pending.

Host parameters apply atcallback boundaries, commands at exactsampleframes.
State/preset loads are lifecycle operations; not atomic sample-accurate automation
of a wholeproject. Modern restore resets transient DSP; full audiohistory is not
serialized. Constructor modernTrim vs program0Legacy is intentional.

Before UI edits record F4visual/interaction targets; verify Quick/Advanced/Legacy
snapshots at supported sizes, clipping/overlap and interactions. OldUI labels
mustn't controlhiddenmodernparameters. Save page/modified status and selected
program correctly. Original8recipes stayuntilF5 adds200meaningfulnewones.
Don't ship default as a curatedpreset replacement or claim200exist.

## Commands

cmake --build build --config Release --target HamloFader_VST3 HamloFader_Tests HamloFader_F3Host HamloFader_F3Legacy -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_f3.py

F3Host/Legacy audit binaries refuseexistingoutput paths and requirelocalarchives.
Normal VST/native builds don't needarchives. Optional local regression enabled
inbuildcacheHAMLO_FADER_F3_REGRESSION=ON; defaultOFFinCMake. F2reference option
also needslocalout/f2/baseline if enabled. No dependency update needed; read-only
JUCE8.0.12 atE:/HPEQ/build/_deps/juce-src;VS18x64, /m:2. Python/numpy/scipy from
E:/HamloShirota/build/s3_venv. New source/UI invalidates F3wholeSource acceptance;
archive it and make F4specific guards instead of changing accepted results.

Recommended reasoning: medium for the decision-complete UI implementation/builds;
Astra xhigh if DSP/compatibility or ambiguous measurement issues appear. No switch
performed. Commit only owned stage changes withCodexcoauthor when checks pass.
