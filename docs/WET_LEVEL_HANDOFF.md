# HamloVerb0.2.2 / Wet Level complete

2026-11-04. Parent115db13; completion commit follows this handoff (git log-1).
Owner says the reverb return is barely audible. Work only inE:/HamloVerb;
efficient-staged-work,no subagents,no model switch.

Added append-only wetDb(-24..+24dB,default0),20ms sample ramp outside the modern
tank/effects, before Insert/Send mixing. Old arithmetic exact at unity. Legacy
ignores it. Main SHIMMER knob replaced with WET LEVEL;Shimmer still in Advanced/
Effects. Meter includes trim. Old state missing ID resets0;project recalls it;
presets reset it0 and modified marker reflects edits. Shared PresetScreen and
all actual DSP algorithms/200 recipes unchanged. No hidden preset gain boost.

Before/after identicalTests/WetLevelTests.cpp(hash86bb6e554c53a432874da03834802426c8bc86ebc02ea47380c16e49ee19d0d2).
On owner's normalizedkeys2, TRAP Memphis Reverse Horizon/program104:
SendOff-11.552388dB,Reverse-12.540777dB wet/dry full energy. DisplayedWET is
absoluteRMS, notrelative ratio. +12dB gives wet return-0.540777dB;do not claim
the exact owner's unknown FL clip was reproduced or a broken Reverse fixed.

Accepted:1634native/0fail,208oldprogramPCMexact vs0.2.1,390M0exact,
sampleclockedgainautomationmono/stereoblocks1/64/127/512/4096exact,allocation
audit with+12,old-statemigration,Insert/Send/bypass/dry/Freeze/meter verified.
pluginval5..10/Steinberg537pass. Main/Effects/Legacy snapshots reviewed.
Authoritative reports:docs/reports/wetlevel,before.txt,probe_after.txt,
native_final.txt,m0_reference.md,validators,screens,auditions.json,acceptance.json.
No threshold relaxation or failed acceptance rerun. One unsupported Windows
rg glob corrected during inspection;it did not alter a measurement.
First isolated-mode finalizer stopped on a sibling import before file creation;
explicit local tool search path fixed it. No sound/acceptance change.

Private A/Bs:out/wetlevel_demo_final,first0dB,0.4sgap,then+12dB,common gain.
Independent8x peaks<-6dBTP. Not factory samples,not committed or in VST ZIP.
Delivery:dist/WetLevel0.2.2/HamloVerb.vst3 and portableZIP/SHA256.
No installer,installed overwrite,HPEQ/subtree/newpush or manual FL listening
claim. Previous GitHub publication remains0.2.1 until a new push request.

Frozenbaseline:build/wetlevel_baseline (Source copied before code edits,
Tests-original.exe=0.2.1,Tests-probe.exe plus immutable probe source).
Finalsource/tools/binary:build/wetlevel_accepted. Current older verifiers should
not be used on this changed source;their frozen accepted copies are retained.

Replay:python -I -X utf8 Tools/verify_wetlevel.py
Build:cmake --build build --config Release --target HamloVerb_Tests HamloVerb_VST3 HamloVerb_Snapshot HamloVerb_Reference -- /m:2 /v:minimal
Native:build/Release/HamloVerb_Tests.exe
Reference:build/Release/HamloVerb_Reference.exe check build/reference/m0 <UNUSED.md>
Validation:Tools/Validate-WetLevel.ps1 -Label <UNUSED>
Private demo regeneration is opt-in viaHAMLO_WET_DEMO_DIR;output must not exist.
No active jobs at completion. Medium for routine git/build;Astra xhigh for DSP
or ambiguous lab investigation;owner controls model/effort,none switched here.
