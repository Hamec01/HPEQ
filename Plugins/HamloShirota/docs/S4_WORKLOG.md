# S4 completed checkpoint (2026-11-02)

Parent d1ef423. Final handoff: NEXT_STAGE.md. Acceptance: reports/s4/REPORT.md and
acceptance.json. All build/lab/validator jobs finished; no pending build sessions.
S4 is the actual VST/UI integration; 200programs/calibration/packages are S5.

The first25case integration and431native checks covered initial routing. Coverage
expanded to31cases and451native expectations, including I07timing, host tempo/
path/engine events, all profile actual FIR taps, Mix0, state and empty-I/O safety.
A new same-method frozen-facade before/final pair replaced neither original raw
runs nor their reports. Final acceptance is stage_final.json, not earlier
candidate_02/final/accepted snapshots. Five identity faults reject stale/corrupt
inputs, including actual embedded process-memory corruption without disk changes.

UI iterations fixed knob angles, model text, footer overlap and arc rendering;
final shared header adds ON and modified state without changing PresetScreen.
All final accepted_page_0..4 and accepted_binaural images inspected. The final
empty-buffer guard preserves all31previous accepted PCM hashes. No S2/S3 accepted
source/data/evidence changed; S4 adaptations and prepared bank are separate.

Final451native/160M0/106lab/5rejection checks and pluginval5..10/Steinberg537 pass.
Final CPU means pass25percent, but maximum1.0062block periods remains open for
live-host verification. No owner FL/K712 audition or final installer claimed.

Pre-commit whitespace cleanup triggered one final fingerprint-bound rebuild and
all acceptance checks. Authoritative lab/validators now use stage_final; prior
accepted_final evidence remains retained. All31PCM hashes remain identical.
