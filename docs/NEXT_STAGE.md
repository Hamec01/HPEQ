# HamloVerb continuation — 2026-10-30

## Objective and constraints

Implement the approved standalone 0.2.0 plan in `docs/HAMLOVERB_V2.md`, including 200 calibrated presets. Do not touch HPEQ, integrate subtree or push. Owner now requires one agent, targeted reading and one coherent stage per chat; see personal efficient-staged-work skill.

## Verified stage

Commit `dc6f952` records the design and immutable M0 reference harness, acoustic baseline and fixtures.
Commit `dbd2000` completes the standalone core extraction: nine Verb/UI/wrapper translation units,
no LayerEngine/pools/other effect implementations, independent VerbSettings and LegacyVerbPath.
Final post-refactor M0 comparison: **390/390 bit-exact**, including matching-block automation.
Unit tests: **95/95**, including post-prepare MIX target ramp. Unchanged acoustic lab: four passed,
zero failed; absent V2 features are n/a. Release Reference/Tests/Lab/VST3 targets built.
See `docs/reports/v2/STANDALONE_CORE_REPORT.md` and `standalone_reference_check.md`.

Intermediate standalone VST3 binary SHA256: `f201668a56b329a55be697566f3a4a76f174012cc33fb3d086726e8595a3a811`.
The project version remains 0.1.0 until the planned 0.2.0 release is actually ready.

Original source baseline: `02446a7787643e74c438d584713265b74be9194b`. Original binary SHA256: `e270991f4201250dfbdcfacfb2d5d69807038318f7251bcb57403946837b90ff`. Preserved binaries are in `build/reference/m0`; state/hash/raw fixtures are archived in `docs/reports/v2/m0-fixtures.zip`.

Commands used:

```powershell
cmake --build build --config Release --target HamloVerb_Reference -- /m:3
build\Release\HamloVerb_Reference.exe record build\reference\m0 docs\reports\v2\m0_reference_manifest.md
cmake --build build --config Release --target HamloVerb_Lab -- /m:3
build\reference\m0\HamloVerb_Lab.exe docs\reports\v2\before.md M0
```

## Unfinished work in the working tree

New V2 DSP/parameter/UI/test files and `Tools/` remain untracked. They are partial implementations,
not integrated or verified. Production processor, editor, parameter spec and preset list are now
Verb-only. Inspect `git status --short` and specific interfaces before proceeding. Do not discard
the unfinished files or assume earlier agent summaries establish correctness. VerbV2Settings.h's
include was adjusted to the new standalone VerbSettings.h; this does not establish DSP correctness.

`Tools/genre_presets.py` generated 200 recipes (4x50, approved Vocal10/Drums12/Samples10/Synth8/Atmosphere10 distribution). Zero calibration entries have been applied. The generated bank is not yet connected to factory programs.

No integrated V2 build, V2 acoustic after run, preset calibration/check, redesigned-panel visual QA,
release validators, FL Studio check, installer or 0.2.0 release commit is complete. No background job
is intentionally required to continue; verify process state if any executable is locked.

## Next stage

Review and finish the new DSP implementation, then integrate additive V2 parameters and state migration
while retaining Legacy as default. Preserve all old parameter indices including bypass (append additions
after it), old host programs and old-state reset semantics. Do not connect the 200-preset bank until its
DSP dependencies work. Run the immutable reference check after wrapper changes, plus relevant new DSP
tests and acoustic goals. Existing unfinished sources are not grounds to claim six working engines.

Continuation prompt: "Continue HamloVerb in E:\\HamloVerb using docs/NEXT_STAGE.md. Finish the V2 DSP and
additive parameter/state integration stage, without subagents. Preserve the verified Legacy core."

Recommended effort: xhigh for the extraction/null-test investigation; medium for an understood build/test sequence. The agent cannot change its current-chat model through the tools presently exposed. No subagents.
