# HamloVerb0.2.1 / Keep Dry

2026-11-04. Parentfde1a57. Owner reported that the original sample becomes
quieter with standalone HamloVerb. Targets/method:docs/OUTPUT_LEVEL_AUDIT.md.

## Diagnosis and change

Modern Insert uses the originally specified dry/wet crossfade:
`(1-MIX)*dry+MIX*wet`. Before wet arrives,25% MIX gives-2.498775dB on dry,
50% gives-6.020600dB,100% removes dry. All six characters reproduce this in the
actual-processor audit. Legacy instead adds wet over unity dry. The200 modern
presets use MIX.153442..944734,Output0,so the most wet-heavy recipe can reduce
the direct component by approximately25.15dB before its tail arrives.
No hidden compressor or output trim was found to explain that crossfade loss.

Added an explicit **KEEP DRY** button beside MIX LOCK. For modern Insert it gives
`dry+MIX*wet`,then applies Output. MIX adds reverb without reducing the source.
Switching the law uses a20ms sample-clocked ramp. Send is still wet only and
ignores the choice. Legacy is unchanged and disables the button. Default is off;
old project states without the new ID reset off,including edited instances.
The choice persists across preset loads like routing,while projects restore it.
Host parameter ID keepDry is appended; no old ID/range/order/program changed.
No new audio buffer,latency,lock or allocation.

## Evidence

The same KeepDryTests source records208 cold-program render hashes before and
after the implementation. All must match with the original law. New checks
prove exact dry in all24 engine/MIX predelay windows,additive identity against
the actual Send render within2e-6,unchanged Send,state migration/roundtrip,
workflow preservation across presets and exact mono/stereo automation at blocks
1/64/127/512/4096 after prepare256. Existing allocation audit now includes
KEEP DRY enabled. Legacy390 M0 cases,full native suite and validators are checked
on the release build. Exact counts/hashes:acceptance.json; replay
`python -I -X utf8 Tools/verify_keepdry.py`.

The original200 calibrated recipes are retained verbatim. Their old mixing path
is checked by identical program renders; no old calibration cache is relabelled
as a fresh calibration of the new law. KEEP DRY is an alternate workflow,so the
old wet/dry depth labels do not certify a new200-recipe additive calibration.
No owner sample/settings were provided. The numeric audit establishes this
mixing cause,not the exact unsupplied FL project. Manual FL listening unverified.

KEEP DRY preserves the direct coefficient; it is not a limiter or loudness match.
Wet/dry interference can still change the total,Output changes the whole result,
and added wet needs headroom. In Send,the original track belongs outside the
wet-only return. No new installer,installed replacement,HPEQ/subtree or push.

The earlier Revolution investigation was a mistaken interpretation before the
owner clarified the product. Its uncommitted changes are parked separately in
E:/HamloRevolution/docs/R11_PARKED.md; they were not copied into HamloVerb.
This release changes only HamloVerb's optional insert mix law and its interface.
Frozen baseline:build/keepdry_baseline. Final source/binaries/evidence:build/
keepdry_accepted and docs/reports/keepdry. VST3/ZIP/SHA256:dist/KeepDry0.2.1.
