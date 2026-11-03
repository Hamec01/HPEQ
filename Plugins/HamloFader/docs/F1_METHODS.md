# F1: standalone Legacy isolation

2026-11-03. Numerical targets recorded before the paired laboratory or isolation.
Approved future V2: Trim/Match/Ride/Motion; Natural/Fast. F1changes no sound.

## Required acceptance

1. Preserve8host parameter IDs/order/ranges/defaults/choice labels and all8program
   indices/names/recipes, XML state tagHamloFader and VST class IDs. Host latency
   stays0samples, tail report unchanged. Legacy gain/mix/bypass/delta arithmetic,
   initial fades, parameter smoothing, meters, mute/polarity/delay remain exact.
2. One immutable laboratory captures original M0and verifies isolated output.
   Raw PCM/state/latency/parameter/meter observations match bit-for-bit for the
   same rate, event positions and block schedule. Capture every case's actual
   original fresh and edited/restored state/audio; compare to those observations,
   not an assumption of historical normalized-value idempotence.
3. Matrix: all8programs at44.1/48/96k, mono/stereo,64/127/512blocks; all parameter
   min/max at48k stereo127; representative paired settings (gain/polarity,
   delay/mix,mute/mix,bypass/nonneutral); automation at declared fixed sample
   positions using1/64/127/512/4096schedules after prepare256;8/192kboundaries;
   reset/reprepare and old-state loading onto a prepared/edited instance.
4. Memory allocated in prepare; zero measured C++ allocations in prepared audio
   callbacks. Existing >=49native checks stay passing. No claim that preexisting
   Legacy event schedules are block invariant: compare each to its exact M0pair.
   Future new kernels must independently meet block1/64/127/512/4096invariance.
5. Build closure includes only Fader-owned processor/host/UI plus shared support;
   no LayerEngine/FxNodeProcessor/EQ/other effects/panels linked into production.
   Record object size and production units, without confusing sizeof with heap.
6. Actual original/current screenshots, same UI; shared PresetScreen/BlackSteel/
   Legacy panel source unchanged. pluginval5..10and Steinberg537/537pass on the
   exact final VST DLL; source/method and raw hashes bind the evidence.

## Provenance and baseline policy

Audit's original Source/Tests/CMake/bundle are frozen atout/audit/original with
docs/reports/audit/original.json. Preserve that copy. Source-bound F1reference
executable is copied before production changes. Capture/verify refuse to
overwrite raw evidence; interrupted work uses a new label or identity-checked
resume, never accepts another source/method. New V2goals are n/a in F1.

Do not change the laboratory between before/after. Any measurement correction
must be disclosed, then both sides recaptured with matching final methods.
No target relaxation to hide a compatibility defect. Original normalized float
roundtrip quirks, if measured, are retained and reported separately.
