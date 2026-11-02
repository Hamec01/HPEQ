# Production Score engine integration

## Final checkpoint superseding the earlier candidate below

Final production source is c5dd2066c79ce00c6887862d740a59a4a94be0b3b9aff09a9760ae22770fa969 (code/preset commit e3a5293). 1198 expectations,59integrated checks,160M0cases and2687component hashes pass. The added lifecycle tests reproduce fresh Reverse/Stutter/Tape Stop after nonzero priming; rational subdivision tests cover2..8repeats on a non-divisible fragment. All200compiled recipes pass400actual Insert/Send renders on this source. Pluginval5..10 andSteinberg537/537 pass; the matched VST3 is packaged with SHA256.

Final tone means24.362%/36.732%. Dense controlled repeats24.676%/36.959% and24.855%/36.421%. A first dense run failed33.302%/38.874%, worst738.750%/100.823%, and is retained. The lifecycle reset uses O(1) cursor/clock reset with guarded unwritten history; exact-zero metadata avoids unnecessary processing of retiring silent sources. All original2687component hashes remain exact. Non-divisible Stutter now uses exact rational boundaries.

The complete current evidence, compatibility exception for the208-program wrapper selector, CPU caveats, artifact hashes and pending owner FL Studio audition are in ../v2/V2_REPORT.md. Source-pinned final data are stable_020f, ../presets and ../release_020_final. The e854... checkpoint below is historical and does not supersede this final source.

## Earlier integration checkpoint (historical)

2026-10-30. Candidate 0.2.0, production source e854c1d94ba911cff8a5a13c29a2e86e30a2c3ae4fa6a4e0e3743604063cd76e. This is a completed DSP integration checkpoint, not final release acceptance: factory calibration, compiled-library verification, new-binary validators and owner FL Studio audition follow it.

## Results

- 1188 unit expectations passed, zero failures. Includes 208 program latency/finite checks, append-only parameter layout, complete state, preset Mix Lock, zero prepared-callback C++ allocations during mode/note/space changes, old-state loading on a playing edited Score instance, actual UI note formatting, Freeze infinity, Insert/Send, Width0 and two-path event/block invariance.
- Legacy: 160/160 immutable M0 raw renders match exactly, all archived states load on fresh and edited instances. Original parameter contract and first eight programs retained. Lab now permits only appended parameters/programs and compares XML semantically with every added parameter at its default. Original signals, raw hashes and event schedules are unchanged; original M0 archive remains immutable.
- All 2687 previously analysed prototype/attack/edge/wrap renders hash identically after adding separated streams and epoch reset; prepared allocation counters remain zero. This reuses the independent precision evidence rather than changing its thresholds.
- Independent integrated lab: 59/59 checks pass. Includes unity timing/free and synced across44.1/48/96kHz, complementary-band null, independent-band onset, cumulative feedback pitch, analytic ramp reversal/stutter/tape-stop and texture addressing, fixed100ms combined pitched Reverse at6intervals, nine spatial character/routing tails,60s Freeze energy stability,5min Freeze finite stress/release, adversarial maximum-feedback decay.
- Full-plugin tone workload, i5-10300H,48kHz/256, eight distinct notes/bands/space/glitch/feedback pitch: mean23.893% single /47.109% actual two-path transition; worst40.204%/77.503%. Targets25%/50% met for this workload. Core mean23.420%/47.002%, worst40.305%/81.947%. Dense full-plugin extension remains to be run; these figures do not claim every possible source/settings/host will meet a deadline.

The subsequent QUIET complete-plugin dense workload (two tones plus6ms deterministic noise bursts every8ms) also meets25%/50%:23.849%/47.415% mean,72.448%/79.236% worst block. Production source is unchanged. See stable_020b/plugin_cpu_dense.json. These are measured workloads, not a universal performance guarantee.

Machine-readable evidence: stable_020b/effects.json, stable_020b/plugin_cpu.json, stable_020b/core_cpu.txt, stable_020b/legacy_160.json, prototype_regression.json. Full logs build/stable_020b-*. Source-pinned raw output out/score/stable_020b.

## Failures retained and fixed

First full-feature CPU40.23%/68.52% failed. Cache delay and fragment calculations, avoid repeated stereo calculations and division-based ring wrapping. First optimised core23.95%/49.38%; later complete-plugin figures above.

Initial Plate in feedback sustained unexpectedly. Make space I/O passive, remove its direct term, add internal loss and use a convex loop placement. After Echo is outside feedback. The regression compares its actual output to independently routed base echoes and a separate spatial network.

Two-pole complementary bands nulled when neutral but allowed differently delayed overlap bands to reinforce feedback. Adversarial500ms scales.998/1.00047/1.00183, allfeedback.9, no space grew to second29 energy208194. Replace the analysis crossover with ordered one-pole complementary bands and bound the combined return norm to.9; keep tap/output EQ two-pole. The corrected adversarial response decays. High combined returns receive a disclosed safety trim; ordinary moderate settings remain untrimmed. See SCORE_METHODS for reasoning.

Combined pitched Reverse initially moved its grain clock by the80ms capture compensation. Six analytic comparisons failed. Separate history read compensation from fragment timing; Reverse/Stutter/Tape Stop now retain the effective musical grid. The original reference/tolerance was unchanged and all six now pass.

Preliminary reports and failed captures remain available. Earlier unpinned runs are diagnostic evidence only; final factory calibration must use the source-pinned corrected executable. No numerical acceptance threshold has been weakened. New-feature measurements are n/a on M0.

## UI and library checkpoint

Own Black Steel Score page and five sections are connected; shared PresetScreen.* untouched. Initial screenshots exposed several text/control overlaps, corrected in source. Final screenshots must be refreshed after gain calibration and include Legacy and885px Score.

200 permanent-ID authored recipes are in the compiled bank (208 total with legacy programs), metadata/filter/start-load wired. Initial gain0 is a calibration starting point. Do not call the bank verified until both actual routings and all200compiled states pass the separate library tool. Prepared material metadata pins16owner clips, excerpts and conversion. USER_GUIDE describes manual source-note semantics, capture waits, band safety, spatial routing and output headroom.
