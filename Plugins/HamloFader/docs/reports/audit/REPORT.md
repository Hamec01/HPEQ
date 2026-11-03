# Initial Fader audit

2026-11-03; parenta7d26e8. Owner approved full Trim, Match, Ride and Motion as
separate modes; Ride Natural/Fast with adjustable correction range.

Source/Tests/CMake and original binaries were archived before rebuilding.
Unchanged targets built successfully; Tests49/49. Rebuilt VST, Tests and Snapshot
are byte-identical to their archived originals. DLL SHA256:
43cf82d8e3287d83f0c5a94a9fa7e35d81ba4f359a32e8124d6e9d2f7feb2fc5.

Actual1180x700snapshot inspected: top-left preset screen and controls are
readable, no overlap found. Eight parameters,8programs; reported latency0samples.
No DSP/UI/host source changes and no installed copy/installer changes.

Findings:32production units include all unrelated effects; MATCH is one-shot
RMS adjustment, not LUFS or continuous gain riding; Solo has no useful action
in the standalone; factory Match recipes only set target; selected program does
not persist. Existing tests do not prove all-rate/event/state compatibility,
allocation freedom or meter/MATCH accuracy. These remain explicit unmeasured
goals. Historical M0validators are not newly claimed release validation here.

See docs/FADER_V2_AUDIT.md for source observations and official published
references. F1_METHODS.md records the isolation goals before its first render.
Original evidence:out/audit/original and original.json; native.txt, panel.png,
acceptance.json. No numerical criterion was relaxed.
