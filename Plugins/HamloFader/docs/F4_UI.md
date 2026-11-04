# F4 interface acceptance, before implementation / 2026-11-04

Complete the approved four-mode interface without changing the F3 audio kernel,
host parameter ABI or original eight programs. Black Steel and PresetScreen
remain unchanged, including its56,20,336x116 placement on1180x700 stage.

- QUICK exposes mode, gain,target,mix/output and relevant Match/Ride/Motion
  controls. ADVANCED exposes all channel/alignment, measuring, Match limits,
  Ride timing and32point curve controls. LEGACY displays the original panel.
- Navigation is explicit:QUICK/ADVANCED select Modern;LEGACY selects Legacy.
  Host/program/state changes refresh the visible engine/page without changing
  audio just by opening the editor. Page saved/restored using existing schema.
- Read bounded atomic FaderStatus only. Missing/short readings show WAIT, not
  valid zero. Show RMS/LUFS M/S/I,sample/true peak and integration-full status.
- Match START/STOP/RESET report command rejection; Apply deliberate and disabled
  for waiting/pending/learning/stale/nonMatch/bypassed/disabled states. Display
  proposal and limits; peak guard never described as an output limiter.
- Motion graph supports2..32point dragging, precise selected-point value,
  host gestures, shape/clock/depth/length/phase/right offset and smoothing.
- Live automation must not interrupt a drag. All controls have English help,
  numeric input/defaults, appropriate modes and modified-state feedback.
- Tests exercise real UI control callbacks, page reopen/state, commands/Apply,
  all modes/point editing and resizing. Review Quick/Advanced/Legacy snapshots
  at1180x700 and minimum590x350. No overlaps or hidden active controls.
- Preserve F3 paired Legacy678renders and host224checks using unchanged labs;
  unchanged kernel/F2tests and native suite pass. Validators5..10/Steinberg537.

Baseline:F3commit3223d0d, archived out/f3/accepted_v2. Record the existing
snapshot and verify F3receipt before edits. Use new F4specific source/evidence
pins; never rewrite the accepted F3receipt to accept changed UI/source.
F5 subsequently adds200 meaningful recipes/208programs and release packaging.
