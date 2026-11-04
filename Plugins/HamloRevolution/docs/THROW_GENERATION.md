# R10: audible spatial throws and purposeful lane generation

Targets fixed before the first R10 measurement (2026-11-04). Parent:71545ff.
Scope: Revolution only, VST3 delivery, no installer, installed-copy replacement,
HPEQ synchronization or push. Preserve existing projects and all225 program loads.

The R9 send receives the preceding lanes' output. Two strong filters, a gate or
a glitch can remove the attack before DELAY/REVERB captures it. Advanced GEN also
copies steps from a random SIMPLE intent without its condition or effect settings.
These are separate causes; turning up the output cannot repair either one.

Implement an explicit INPUT / CHAIN source for each spatial send. CHAIN is the
existing default and exact old path. INPUT captures the original, latency-aligned
input in the selected frequency band; the return joins the processed chain.
Insert targets keep their existing chain input. Allocate no new audio buffers.
Missing source parameters in old states reset to CHAIN, even on an edited instance.

Advanced GEN builds a target-aware recipe for just that lane. It keeps its target,
time/note and band; it resets stale condition/chance/length/rate,
enables the lane and applies visible, bounded mix/depth/tail/tone settings. Sends
capture actual/template attacks for at least an eighth note, or longer regions on
sustained sources. SHAPE uses target-appropriate musical envelopes, not arbitrary
SIMPLE intent slices. Consecutive generations vary structure, not just loudness.

Explicit SIMPLE generation adds the new spatial recipe after the existing genre
generator. Old program loading uses the frozen generator, so the200 calibrated
recipes are not silently replaced. Existing saved patterns do not regenerate.

| ID | Acceptance and fixed method |
|---|---|
| T1 | All225 R9 program renders and saved states reproduce exact PCM; old IDs/order/ranges and latency unchanged. Use one frozen SpatialLab before/after. |
| T2 | Missing source resets to CHAIN on fresh and edited instance. New sources/state roundtrip. |
| T3 | INPUT DELAY/REVERB survives a fully closed upstream gate; CHAIN remains silent. Band selection still rejects an out-of-band probe by at least30dB. |
| T4 | Generated DELAY/REVERB, four genres, synthetic drum source: return/dry energy >=-24dB, finite output, peak <1. No automatic output normalization. Paired return = same chain with target THROW disabled. |
| T5 | GEN removes stale Every8/chance restrictions, captures hits, leaves other lanes/parameters and TIME/GLITCH/MOTION locks unchanged. At least4 distinct structural patterns in8 generations per target. SHAPE/THROW have no cell locks in the existing product. |
| T6 | New routing PCM exact for blocks1/64/127/512/4096 after prepare256, mono/stereo, zero measured audio C++ allocations. |
| T7 | Existing native suite; pluginval5-10 and Steinberg validator on the delivered DLL; inspect Advanced/Simple snapshots. |

First T3 unit fixture started music during the existing10ms SHAPE mix ramp, then
incorrectly expected its already-captured delayed tail to be exactly zero. It
failed2 assertions. Corrected the input fixture to provide500ms initial silence,
so the gate has settled before music. The zero threshold and DSP are unchanged.
Original test source, executable and raw results retained in out/r10/initial-tests.

The first post-change SpatialLab run failed T4 for Techno DELAY (-25.38dB).
Its variation rotated the entire attack ranking and could select only quiet
hats. Recipe corrected to always retain the strongest attack, with variation
on additional attacks and capture duration. Method and -24dB threshold unchanged.
Original results/demos retained in out/r10/after_initial.

*First receipt verifier incorrectly required program-load PCM to equal the
after-playback saved-state reload PCM, as well as matching versions. The frozen
R9 baseline already differs between those procedures for17 programs. Corrected
the verifier to compare R9 load with R10 load and R9 reload with R10 reload;
both225/225 exact. Laboratory, inputs, hashes and T1 compatibility target unchanged.
This does not certify exact within-version program/state roundtrips for those17
legacy cases; their cause is not diagnosed here. Failed receipt/tools/ZIP retained.*

SpatialLab retains original state files and hashes, renders paired bypass/return
measurements and exports dry/before/after demonstrations. Baseline lacks INPUT
and reports it n/a. These synthetic checks establish function and minimum effect
energy, not a guarantee of taste or perceptual audibility on any music. Owner
listening in FL remains necessary. No third-party reverb algorithm is introduced.
