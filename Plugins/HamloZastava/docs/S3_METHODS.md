# S3: host adapter and state contract

2026-11-02. Parent35f6917. Goals and methods recorded before integration.
This is an internal host-integration build, not the final UI/preset release.
S2 kernel and its numerical method stay unchanged. No subagents/installer/push.

## Contract

Append v2_ parameter IDs after the original15parameters+bypass. The Legacy
spec, program0..6, parameter ranges/order/defaults, class ID and XML tag remain.
Fresh instance selects Clean with processing disabled: exactly fixed5ms dry.
Old factory programs select Legacy and reset all modern fields; bypass follows
the historical program behavior. Modern timing/level controls have explicit
intervals so their saved float values survive normalized roundtrips.

Full modern states carry schema1 and selected program. Missing modern schema
or engine field means old state: clear every new field and choose Legacy,
including when loading over an edited/previously processed modern instance.
Ignore invalid root/XML or unsupported future schema. Missing individual new
fields in schema1 reset to defaults. Sanitize nonfinite/out-of-range new values.
Legacy with all modern defaults uses the original16field state encoding; this
preserves original serialization/readback quirks and restored program behavior.
Legacy with retained edited modern controls uses the full schema for lossless
future switching. Full-state load resets modern history at the next callback;
histories are not serialized. Before-prepare and prepared-instance loads tested.

Both engines run on the same input and fixed PDC, without double delay.
Crossfade Legacy/modern over20ms; native modern mode changes use the S2 kernel.
Keep exact Legacy math, including bypass/alignment. Old primary16metadata and
all229PCM/state/latency fixtures must match actual M0 under the same schedule.
Add a disabled optional mono/stereo external key input; main mono/stereo stay
independent. Key samples are never cleared, modified or mistaken for main audio.
Host BPM/PPQ/playing map to S2 Clock; oversized callbacks get correct chunk PPQ.
Modern bypass uses fixed PDC; bypass startup is exact delayed dry. Duck onset
maps user Attack to kernel falling-gain time, recovery maps Release to rising.

## Acceptance

| ID | Goal |
|---|---|
| H01 | Original16parameter contract, seven programs and229primary/restored observations exact to M0 |
| H02 | New clean-disabled, bypass and Mix0 startup equal5ms delayed dry, mono/stereo and8/44.1/48/96/192k |
| H03 | Modern parameter/state/audio restoration exact fresh and edited; all engine/profile combinations; full Legacy+edited-modern state retained |
| H04 | Old state clears every modern field on fresh/edited instances; partial/new, malformed and future states obey contract |
| H05 | Adapter output equals independent S2 Gate output with the same quantized settings/clock/input; all modes/profiles, main and external mono/stereo |
| H06 | Main/key routing preserves key bytes; disabled key fails open/status; changing main with external key cannot change control timing |
| H07 | Same sample-position engine/profile/parameter/bypass/tempo/seek/transport events bit-identical at1/64/127/512/4096 after prepare256 |
| H08 | Zero C++ audio allocations first callback, clock, oversize callbacks, modes, key loss, bypass and prepared state restoration; finite/bounded |
| H09 | Fresh VST passes pluginval5..10 and Steinberg validator; test/report source and DLL hashes match |

Paired method:clone the frozen S1driver into S3LegacyLabMain, changing only its
identity labels and layout setup to retain/disable optional auxiliary buses.
Identical baseline/current driver runs against frozen original M0/current
libraries. Signals, schedules, state comparisons, contract and tolerances remain.
Verify all229new baseline primary hashes equal original S1v3 to prove no changed
fixture. Frozen S1driver/methods are not edited.

S3HostLab uses only public processor APIs and runs against both original/current
libraries. Original build reports unavailable new features n/a. It directly
checks new IDs and state/IO/clock behavior; S2 Gate is the independent numerical
reference. Allocation scope includes processBlock and prepared restoration's
first callback (state parsing itself is a message-thread operation). Source,
driver, method document and verifier hashes bind caches. Preserve baseline
sources/binary before rebuilding shipping targets. Disclose all method changes
and rerun both sides if measurement code changes. No musical quality/library
or finished editor claim in this stage.

*First candidate:266host checks passed,17combined state checks failed; Legacy
229/229 and native206passed. The revised host driver prints separate parameter,
state-byte, program and audio diagnostics without changing any acceptance test.
Rebuild and recapture both sides under the revised method before acceptance.*

The17failures are serialized-byte differences: APVTS raw defaults are initially
unsnapped, e.g.124.999992 instead of125ms. New state loading snaps them; reported
normalized controls and the tested PCM already match. Canonicalize new DSP
controls and full-state encoding to their explicit grids. Do not change Legacy
controls or lower exact state/audio requirements. Retain the failed candidate.

*First native suite used disjoint main/key storage. The added four exact-pointer
alias configurations exposed four failures despite the then283host/207native
passes. The adapter now copies key input before either path overwrites main.
The companion suite has219expectations. The primary drivers/goals are unchanged;
this disclosure changes their method stamp, so recapture matching M0/current
reports under final method v3. Previously validated binaries/reports remain.*
