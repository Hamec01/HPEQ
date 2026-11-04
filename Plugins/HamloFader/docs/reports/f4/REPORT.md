# F4 modern interface / 2026-11-04

Parent3223d0d. Added own FaderV2Panel; preserved shared PresetScreen, BlackSteel
and HamloFaderPanel. QUICK/ADVANCED select Modern; LEGACY uses the unmodified
old panel. Four modes, channels/alignment, all metering and ready/full states,
Match proposal/actions, Ride profiles/range/timing, editable2..32point Motion,
shape/depth/clock/phase/right offset and exact numeric fields are exposed.
Project page persists; opening editor does not edit state. Modified marker
compares all settings against selected factory program. No host IDs added.

UI reads only bounded atomic status. Failed requests show BUSY. Pending commands
wait for audio; Apply is deliberately separate and disabled for stale/pending/
unready/nonMatch/learning/bypassed states. Shared canApplyMatch/applyMatch use the
same existing validity contract; no gain/meter algorithm changed. Peak Guard is
described as a proposal limit, not an output limiter. M/S/I readiness and72000
window/full warning are explicit. Motion graph is editable in Custom; Sine/
Triangle preview their actual shapes. Numeric input and tooltips remain available.

Accepted:163native checks including real slider/combo/button callbacks,32point
editing/reduction,state/editor reopen,Learn/Stop/no-auto-Apply,deliberateApply,
stale refusal,queue capacity and preset geometry at590/1180/2360width.
Unchanged F3Host method224/0; wholeprocessor measured means1.077..1.577% block
duration on this run,below25%,not FL CPU certification. Unchanged F3Legacy
method226cases x3observations=678exact renders/state/meter results,0failures.
Source/DSP files match accepted F3 exactly. pluginval5..10 exit0;Steinberg537/0.

Screens:8Quick/Advanced mode combinations with synthetic live meter/curve data,
Legacy and minimum590x350. Main representative screens visually inspected;
minimum has very small text:1180x700 recommended. No overlaps identified.
Initial snapshots retained:subtitle truncation,overlong decimal readouts and
irrelevant quick knob were corrected. Initial compile stopped on a missing
parenthesis and a const program getter; logs retained inout/f4. No acceptance
method/threshold change. Baseline screenshot before.png and accepted F3 receipt
verification inout/f4/baseline_verification.txt. F4 UI tests are new interaction
tests,not claimed to exist in F3; audio before/after methods remain unchanged.

Internal version remains0.1.0 and8programs at this stage. No200preset/release,
manual FL,installer,installed replacement,HPEQ/push claim. F5 follows with200
recipes/calibration and final release. Frozen sources/tools/bundle:out/f4/accepted.
ReplayTools/verify_f4.py while current tree is this F4 stage; subsequent F5
source changes intentionally invalidate its current-source pins. Raw historical
evidence/frozen copies remain intact.
