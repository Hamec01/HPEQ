# F3 host integration contracts (before host edits)

Parent 7af28d0, 2026-11-03. F2 offline acceptance remains archived. F3 adds
host integration only; UI/library/release listening stay F4/F5. No installed copy.

- Append host parameters after the original eight. Preserve prefix IDs, names,
  ranges, defaults, choice labels, order, XML root and all eight old program recipes.
  Constructor starts modern neutral Trim; selecting any original program selects
  Legacy, including program 0. Never turn old target-only recipes into auto Match.
- Old unversioned states select Legacy and reset every appended parameter/page,
  retaining historical program selection and Legacy prepared audio history.
  Modern state v3 stores every parameter, program and page; invalid/future formats
  are rejected without changes. Modern restoration resets transient kernel history,
  meters, learning and queued commands, preserving the stored fixed Match gain.
- Fixed PDC 0 samples. Legacy tail 0; modern tail 20 ms maximum channel alignment.
  Native float mono/stereo only. No added buses/MIDI. State is not an audio snapshot.
- Both prepared branches receive raw input. A 20 ms selection fade connects them;
  stable Legacy retains the original arithmetic, initial ramps and block schedule.
  State restoration primes selection; old Legacy histories are never reset.
- Host controls are read at callback boundaries. For identical event sample
  positions compare PCM at blocks 1/64/127/512/4096, prepare 256, including
  oversized callbacks, PPQ/BPM, seek/stop and missing playhead/position.
- Start/Stop/Reset are bounded, nonblocking audio commands (64 entries), absolute
  monotonically ordered processing frames; immediate means next audio opportunity.
  Late commands execute at that opportunity. Return false on busy/full/out-of-order;
  callers must report/retry rather than assume success. State/preset epochs discard
  obsolete commands. UI never reads or mutates the kernel directly.
- Stop produces a published proposal; Apply is a deliberate parameter gesture on
  the control thread, refused for stale epochs, learning, disabled/bypassed or
  non-Match state. It alone changes persistent matchedDb. Reset preserves it.
- Readings use race-free bounded atomic snapshots, including readiness/full and
  proposal/epoch. Concurrent readers may receive not-ready rather than block.
- All processing/commands are allocation-free after prepare. Off/bypass cancels
  unfinished learning. Standard processBlockBypassed follows the same cancellation
  and smooth bypass as the bypass parameter without changing stored host controls.

## Paired Legacy method

F3LegacyLab is a new clone of the frozen F1lab. It uses the same 226 signals,
all sample event positions and three actual restore scenarios (678 renders).
Only the original-eight host prefix and canonical old XML are compared, since
new state fields/parameters are intentional. Old lab/method are not edited.
Capture control before host edits, refuse overwrite, stamp source/method; verify
candidate against the same files byte-for-byte, meters/allocations included.

## Host acceptance

H01 prefix/program/XML exact; H02 678 Legacy renders exact, zero allocations;
H03 neutral modern exact and oversize; H04 every host control maps to F2 settings;
H05 actual plugin vs independently driven accepted kernel exact at event positions;
H06 block invariance and complete host-clock cases; H07 modern fresh/prepared
state restoration and malformed/future rejection; H08 old-load reset of all new
controls/history behavior; H09 commands ordering, capacity, offsets, epochs,
Learn/Stop/Apply/Reset and off/bypass; H10 concurrent snapshots/control commands;
H11 zero audio allocations, 0 PDC, layout/tail; H12 native and vendor validators.
No FL listening, new UI, 200-presets or final-release claim in F3.
