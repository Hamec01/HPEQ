# S1: own Zastava processor, unchanged sound

2026-11-02; audit69901fb, original production sourcef7330fd. Owner approved all
four new roles(cleanup,drum tails,external-key ducking,rhythm) and separate soft/
hard profiles. This first stage isolates Legacy; no V2 sound or new controls yet.
No subagents, other products/HPEQ, push/subtree or installer. VST first.

Before code changes capture existing M0 binary, full source/config identity,
parameter/program/state contract and deterministic raw fixtures. Freeze one lab
for capture/verification; unavailable future features are n/a. Never overwrite
the reference. Synthetic alternating bursts/quiet beds, silence, mono/antiphase,
one-sided and impulse inputs cover original programs, boundaries, both M6 and
extended branches, lookahead/filter/output branch switches, automation and bypass.
Keep initial ramps, conditional delay clocks, prepare reuse and reset sequences.

| ID | Acceptance |
|---|---|
| L1 | Every captured PCM byte identical at the same block/event schedule; original16parameter contract,7program names/order, XML state bytes/class identity and host latency unchanged |
| L2 | Every old state restores the same parameters, reserialized bytes, program metadata and PCM as actual M0 restoration on fresh and edited instances; preserve historical behavior rather than silently repairing it |
| L3 | Native tests pass; static original programs identical at blocks1/64/127/512/4096 after prepare256; characterize Legacy automation dependence without imposing a new M0 behavior |
| L4 | Own prepared processing allocates zero memory, including first callback, bypass/lookahead/branch/filter automation, block4096 after prepare256 |
| L5 | No LayerEngine, FxNodeProcessor, graph/project state, unrelated processors/panels in production translation units or include closure |
| L6 | Same16parameter order/ranges/defaults/labels/choice normalization, VST class IDs; pluginval5..10 and Steinberg validator pass on final binary |
| L7 | Existing Black Steel page and shared top-left PresetScreen retained, snapshot inspected; finite gate meters in extended branch |

Preserve total5msPDC(240samples/48k) and original sound. Legacy's M6 and extended
gate laws are distinct; resetting/output-zero/lookahead branch behavior is part
of its contract. Own allocation occurs in prepare only. Compare source-pinned
capture and final native binaries, retain raw reports and every failed candidate.
No claim about acoustic gate quality/timing/noise/bleed before V2 measurements.

S1 laboratory digest covers its source, this method and the audit script. Text
hashes normalize CRLF to LF; raw audio/binary/state bytes remain exact. Source
fingerprints differ across isolation as expected; the method and fixture schedule
must not. New core/facade units check a matching compiled source identity.

## Disclosed method correction before final paired run

The first229case capture/verification(v1) found all PCM and latency identical,
but191state checks failed. Running the archived original M0 executable against
its own reference reproduced the same191failures. The original check incorrectly
required float/skew parameter restoration to be serialization-idempotent; that
is not true of this M0/APVTS state. It did not measure isolation compatibility.

V2 of the lab captures actual M0 fresh/edited restored states, normalized values,
selected-program metadata, latency and rendered audio, then compares the isolated
processor against those exact observations. Original input-state bytes and main
PCM remain strict byte comparisons. No tolerance was widened; all comparisons
remain exact. Historical idempotence is recorded separately and is NOT repaired
or declared passed in S1. New-engine state accuracy is a later requirement.

Keep v1reference/candidate/self-check immutable. Build the frozen original source
with the same final v2lab, capture reference_v2, then verify the current isolated
source with that same lab. Assert all229primary PCM/state hashes still equal v1.

The first v2verification harness incorrectly loaded reference bytes into a
non-empty MemoryBlock. JUCE File::loadFileAsData appends, so it doubled that
buffer and falsely failed all initial input-state size comparisons. Diagnostic
checks showed the restored-state bytes, PCM and metadata themselves matched.
Clear that load buffer before reading; final v3capture/verification repeats the
same229fixtures with one matching driver. Keep v2failed reports and diagnostic
trace. No audio algorithm or acceptance tolerance changed.
