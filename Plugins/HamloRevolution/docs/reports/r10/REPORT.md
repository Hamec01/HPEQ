# R10 / Revolution0.2.2: audible spatial generation

2026-11-04. Parent71545ff. Targets and methods:docs/THROW_GENERATION.md.

## Finding and change

DELAY was already functional. R9 Advanced GEN copied one lane's steps out of a
random SIMPLE recipe without copying the condition or effect controls. An old
EightOfEight condition could leave the new pattern silent through this four-bar
probe. Separately, sends captured the output after SHAPE/GLITCH, so preceding
gates and filters could remove the material that was supposed to echo.

THROW A/B now have independent INPUT/CHAIN selectors in the target's right-click
menu, with visible source captions. INPUT captures original audio in the lane's
selected band and adds the return to the existing chain. CHAIN retains R9's
calculation path. Insert targets retain their chain input. No additional audio
buffers, threads, latency or output gain stage.

SHAPE/THROW GEN now produces target-aware gestures for that lane, resets stale
length/rate/condition/chance, enables its visible controls and keeps target,
time/note/band. Sends always capture the strongest detected/template attack;
other captures and durations vary. Sustained material uses longer regions.
SHAPE has pump, sweep, pan, width and rhythmic drive/crush gestures.
Other lanes, their parameters, engine locks and scene seed are preserved.

Explicit SIMPLE GENERATE, its controls and BOLD add an INPUT delay and reverb
pair, active each bar. The target band's protection stays in force. New controls
are appended after the old host bypass, preserving every old host index.
Old program loads use the frozen R6 generator and CHAIN; all225 slots/names and
the200 previously calibrated recipes remain unchanged. To hear the new recipes
after loading one, press GENERATE or the relevant row's GEN. Stored projects
never regenerate on loading. Missing sources reset on reused instances too.

## Measurements

Same immutable SpatialLab source before and after, SHA256
5664ba5805351e6e52fab204241776e2d8b981821f7533e588d8909899201ddb.
48kHz/120BPM. Synthetic drums, not owner listening/material certification.

| Target | Result |
|---|---|
| T1 |225/225 program PCM hashes and old-state reloads exact against R9; latency unchanged0. |
| T2 |All225 old states reset both sources from INPUT to CHAIN. New source/scene settings roundtrip. |
| T3 |After settling on silence, gated CHAIN energy0; INPUT DELAY125.461381, REVERB551.784131. High-band80Hz rejection -88.48/-90.41dB against6kHz. Zero send preserves exact dry without crossover phase. |
| T4 |16/16 probes pass. Return/dry energy -10.71..-1.94dB versus >=-24dB; peak <=0.470 versus <1; finite and nonzero tails. Includes normal and stale EightOfEight starting states, four genres, DELAY/REVERB. |
| T5 |Every target in both SHAPE/THROW lanes: >=4 distinct structures in8 generations; scheduling reset, other lanes/seed unchanged. Strongest attack retained for every send variant, including sustained input. |
| T6 |Mono/stereo, DELAY/REVERB, Full/Tops, blocks1/64/127/512/4096 after prepare256: exact PCM, zero measured C++ audio allocations. |
| T7 |4365 native expectations pass. Delivery validator/snapshot/hash evidence is in acceptance.json; replay Tools/verify_r10.py. |

The delivered DLL passed pluginval5,6,7,8,9,10 and Steinberg537/537. Final
Advanced/Simple/minimum snapshots inspected; captions fit at opening size.
Existing all-target-pair CPU probe measured4.56% worst block time in the earlier
sound-equivalent release run (48kHz/256); this is not a new INPUT-specific CPU
benchmark. Delivery raw native log retains its own V7 timing.

The return metric subtracts a matched render with just the target THROW disabled;
the preceding chain stays identical, so phase-changing filters are not mistaken
for effect energy. Whole render includes the tail. No normalization or limiter
was used to manufacture the result. Thresholds remain as fixed in the design.

## Failed intermediate runs and limits

Initial T3 fixture failed2 assertions because it fed music during the intentional
10ms SHAPE startup ramp, then expected already-captured echoes to be zero. Added
500ms input silence to settle the controls; DSP and threshold unchanged. Retained
source/executable/log in out/r10/initial-tests.

Initial post-change T4 Techno delay measured -25.38dB, below the -24dB target.
Rotating the entire attack ranking selected only quiet hats in that variation.
Fixed the recipe to always include the strongest attack, varying the rest.
Original after_initial.json and demos retained. The method was not changed.
An early StringRef comparison compile error was corrected before running tests.
Initial screenshots truncated the combined band/source caption; source now has
its own line. The minimum-size editor remains very small; default size advised.

The first receipt replay failed at program32 because its verifier additionally
required program load to equal a state saved after playback and reloaded fresh.
The frozen R9 baseline already has17 such cross-procedure differences (32,34,36,
38,41,45,51,59,60,62,63,64,65,66,67,68,101). Same-procedure R9/R10 comparisons
have zero differences:225 program loads and225 old-state reloads match exactly.
Corrected this extra verifier condition, not the lab, target or measured data.
Within-version exact roundtrip for these17 legacy cases is not certified; cause
not diagnosed in this stage. Failed receipt/tools/archive retained with
failed_verifier names in reports/out/dist. No DSP change for this verifier fix.

Frozen artifacts:out/r10/baseline, before, after_initial, after_release and
accepted. Exact verified VST3/ZIP/SHA256 are in dist/HamloRevolution-0.2.2-r10*.
Samples are still the R9 workspace, including load/play/export/drag. No installer,
installed plugin replacement, HPEQ edits, subtree or push in this stage.
Actual FL listening/OS file drag remains an owner check, not claimed here.
Existing R9 receipt belongs to its frozen R9 source, not current R10 source.
