# HamloVerb0.2.1 / Keep Dry handoff

2026-11-04. Parentfde1a57; completed commit:git log-1. No subagents. Correct task
is standalone HamloVerb:owner says the source sample gets quieter with the plugin.
The preceding Revolution R11 investigation is parked uncommitted in that repo,
see E:/HamloRevolution/docs/R11_PARKED.md; do not mix it into this release.

Cause verified through the actual wrapper:modern Insert is(1-MIX)*dry+MIX*wet.
At50% direct gain is-6.0206dB; at100% no direct. All6 characters verified before
the wet predelay.200 presets haveMIX.153442..944734,allOutput0. Legacy is additive.

New appended choice ID keepDry,indexKeepDry after the old97 spec parameters,
default0. In modern Insert,KEEP DRY givesdry+MIX*wet atOutput0.20ms sample ramp
between laws,zero new latency/buffer/lock. Old branch retains exact arithmetic;
Legacy bypasses the new law,Send ignores it. Missing old-state ID resets via
the existing defaults merge; saved new state restores it. Preset loading and
modified-state checks treat it as workflow like routing,preserving the choice.
UI top right:MIX LOCK narrowed beside KEEP DRY; generic toggle callback/tooltip;
disabled for Legacy/Send. Shared PresetScreen and actual reverb DSP untouched.

Checks:1439 native expectations pass;208 cold program PCM hashes exact against
build/keepdry_before.txt;390 M0 references exact. Six characters/MIX0/.25/.5/1
keep exact source before predelay; additive identity vs Send,error<2e-6;Send
unchanged;state migration/roundtrip;blocks1/64/127/512/4096 with events exact for
mono/stereo. Existing allocation audit now explicitly enables KEEP DRY. Final
validator identity and build/delivery/source pins:docs/reports/keepdry/acceptance.json.
Replay python -I -X utf8 Tools/verify_keepdry.py. Main/Advanced snapshots reviewed.

Delivery:dist/KeepDry0.2.1/HamloVerb.vst3 and portableZIP/SHA256. No installer,
installed-copy replacement,HPEQ changes,subtree or push. No manual FL listening
claim. KEEP DRY is not a limiter or loudness matcher:Output trims all;wet/dry
interference can still affect total. Old200 calibrated recipes remain verbatim
for the original balance law,not a new additive200-recipe calibration claim.

Baseline source/test/executable inbuild/keepdry_baseline. Final source/tools/
tested VST inbuild/keepdry_accepted. Lab sourceKeepDryTests is identical before/
after;the parameter-absent baseline reportsn/a for new checks. Separate
OutputLevelAudit verifies the diagnosis. Do not overwrite old release evidence.

Commands:
cmake --build build --config Release --target HamloVerb_Tests HamloVerb_VST3 HamloVerb_Snapshot HamloVerb_Reference -- /m:2 /v:minimal
build/Release/HamloVerb_Tests.exe
build/Release/HamloVerb_Reference.exe check build/reference/m0 <UNUSED-REPORT.md>
Tools/Validate-KeepDry.ps1 -Label <UNUSED-LABEL>
python -I -X utf8 Tools/verify_keepdry.py

Next:owner loads0.2.1 in FL,selects a modern character,INSERT,KEEP DRY on,OUTPUT0dB
and adjusts MIX. No active jobs after completed verification. Medium for routine
checks;Astra xhigh for a new DSP/measurement issue. No model switch performed.
