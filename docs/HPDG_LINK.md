# HPDG Link: HPDG -> HamloRevolution (and later the other Hamlo plug-ins)

Owner (2026-11-02): "a CONNECT TO HPDG button in the plug-in: when HPDG is running it gives the plug-in tempo, MIDI,
genre and everything needed, and Revolution makes new patterns and groove from it (also controllable)".

## 1. Requirements

- Works when HPDG and Revolution run in **one host process or in different ones** (FL Studio can bridge plug-ins into
  their own processes): so no in-process singletons - a named, file-backed shared-memory slot.
- **Read-only for the client**, local only, no network (no firewall prompt), nothing on the audio threads that can block.
- **Several HPDG instances**: each has its own slot; the client lists the live ones (name, genre, tempo) to pick.
- **Versioned**: a client refuses a newer major version cleanly (lamp shows "update"), accepts newer minor fields.
- **Liveness**: a heartbeat; no beat for 2 s = gone; the client keeps the last snapshot and falls back to LISTEN.
- **HPDG unchanged otherwise**: the publisher is additive (CLAUDE.md of HPDG: genre engines untouched); it writes only
  after a generation / mutation / parameter change and once a second for the heartbeat.

## 2. Transport

`%LOCALAPPDATA%\HamloProd\Link\hpdg-<instanceId>.slot`: a fixed-size file mapped with `juce::MemoryMappedFile`
(read-write for HPDG, read-only for clients). `instanceId` = a random 64-bit id made when the HPDG instance is created
and kept in its state (so a reopened project relinks to the same HPDG). Old files with a dead heartbeat older than
1 hour are removed by any publisher at start.

Consistency: a **seqlock**. HPDG increments `sequence` to odd, writes the payload, increments to even. A client copies
the slot, then re-reads `sequence`: equal and even = a consistent copy, else retry (at most a few times, on a
background thread). No locks, no waits on the audio threads.

## 3. Snapshot (little-endian, packed; version 1.0)

| Field | Type | Meaning |
|---|---|---|
| magic | char[8] | "HAMLOLNK" |
| versionMajor, versionMinor | uint16, uint16 | 1, 0 |
| sequence | uint32 | the seqlock counter |
| heartbeatMs | int64 | `juce::Time::currentTimeMillis()` of the last write |
| instanceId | uint64 | see above |
| instanceName | char[64] | UTF-8, e.g. "HPDG - Drums 1" (the host track name when known) |
| bpm | float32 | HPDG's tempo (= the host's when synced) |
| genre, substyle | int32, int32 | HPDG's GenreType and the substyle index of that genre |
| swingPercent, timing, humanize, density, velocityAmount | float32 x 5 | GeneratorParams |
| intensity | int32 | the rack intensity 1 .. 5 |
| keyRoot, scaleMode | int32, int32 | 0 = C .. 11 = B; 0 minor / 1 major / 2 harmonic minor |
| bars, phraseLengthBars | int32, int32 | the pattern's length, the phrase length |
| seed, generationCounter, mutationCounter | int32 x 3 | changes when HPDG makes a new pattern |
| eventCount | int32 | <= 1024 |
| events[1024] | struct | lane (uint8: TrackType 0 .. 10), flags (uint8: ghost, slide, fill, accent), velocity (uint8), roleId (uint8), tick (int32: 0 .. bars x 3840, PPQ 960, with the timing offset applied), lengthTicks (int32) |
| roleNames | char[32][24] | roleId -> the semantic role text ("backbeat", "fill", "anchor" ...) |

About 13 KB. `bpm`, the pattern and its timing offsets carry the groove: Revolution's GROOVE knob moves its steps from
the straight grid towards these offsets; its genre rules read `genre` / `substyle` and place events on the lanes'
ticks (hats for rolls, the snare for stutters, the phrase end for fills).

## 4. Sides

- **HPDG (`C:\DRUMENGINE`)**: `Source/Services/HamloLinkPublisher.{h,cpp}` - built from `PatternProject` on the message
  thread after generate / mutate / parameter changes; a 1 s timer for the heartbeat; the instance id in the plug-in
  state; a small "LINK" indicator. Tests: the snapshot equals the project (every field), the seqlock under a reader
  thread, no work on the audio thread. This is a change to HPDG: its own commit there, and only with the owner's go.
- **HamloRevolution**: `Source/Link/HamloLinkClient.{h,cpp}` - lists slots, maps the chosen one read-only, polls at
  10 Hz on a background thread, hands a copy to the generator (message thread) and the "linked" state to the page.
  The generator's result reaches the audio thread as a whole new pattern (an atomic swap), never piecewise.
- The same client can later serve HamloEcho (delay times / tonal key), HamloVerb (pre-delay on the grid), HamloComp
  (sidechain-free pumping on the kick lane).
