# RACK1: native processors and versioned runtime

Base: HPEQ `45d61b3`, preserving the uncommitted RACK0 bank. Date: 2026-11-20.

Status: **paused by owner, unfinished WIP source checkpoint**. Native libraries
and the main Release tools compiled before the last native source/header edits;
runtime/audio acceptance remains pending. See `docs/NEXT_STAGE.md` and
`RACK1_CHECKPOINT.json` for exact limits and continuation commands. This document
records the intended design and acceptance probes, not completed verification.

## Implementation decision, before changing runtime DSP

Use namespace-isolated copies of the complete native AudioProcessor wrappers,
generated in the build directory from pinned `Plugins/` sources. Keep native
parameter/state code, DSP, output protection, fixed latency and handover paths.
Replace only unused editor construction in that internal headless copy, and
disable Revolution's optional workbench timers via its existing offline
constructor argument. Keep imported sources byte-exact. This avoids a separate
approximation of Protect/Match/send behavior.

A versioned native slot retains physical parameters and optional native
structural state. A fixed-capacity snapshot carries prebound physical values
and a stable prepared runtime pointer. Allocation, native state restoration,
topology changes and reclamation happen outside the audio callback. Slots hold
prepared runtimes while their tails are running. Unsupported revisions,
parameters or capacity must fail explicitly; no fallback to legacy DSP. Slots
without a native revision retain the original processing path.

## Acceptance probes fixed before audio runs

- Legacy: all 224 existing embedded factory states on the unchanged deterministic
  fixture; 48 kHz stereo, prepare(256), blocks 127, BPM 120, two seconds of input
  and one second of silence. Save raw float renders, input/state/output SHA256
  and latency before modifying DSP. Afterward require exact floats and latency
  with the same tool, compiler and configuration. This probe does not validate
  preset sound quality or the entire length of long tails.
- Native parity: public native parameter/state API versus prepared raw-value
  snapshot adapter on identical input/transport. Require exact output and
  latency; self-reported meters are not a reference.
- Composition: insert endpoints, aligned dry, additive wet send, slot/node
  bypass, tails and structural state save/load; frequency-node and whole-layer
  positions. Reject unsupported state rather than silently dropping fields.
  Whole-layer reconstruction may round the existing delta sum; its native
  insert/wet-send comparison uses maximum absolute error <=2e-6. Dry/bypass
  identity and the direct native Runtime parity remain exact comparisons.
- Audio safety: 44.1/48/96 kHz, mono/stereo and blocks 1/64/127/512/4096 after
  prepare(256). Debug CRT allocation probe on the audio thread, including target
  publication and retirement. No allocation, free or locks in the new path.

The RACK0 recipe bank stays design-only until native adapters, native panels and
individual audio calibration have passed their separate gates.
