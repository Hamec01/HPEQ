# Standalone Verb core extraction — 2026-10-30

This is an intermediate compatibility stage, not the 0.2.0 release.

## Change

The production target compiles nine Verb/UI/wrapper translation units instead of the full module stack.
LayerEngine, FxNodeProcessor, effect pools, crossover/EQ processors and unrelated effects/panels are excluded.
The standalone VerbSettings block replaces the HPEQ project/layer model dependency. Historical unused sources
remain in the repository for provenance; they are not production dependencies.

LegacyVerbPath retains M0's two input/node startup ramps, MIX ramp, additive send law, float rounding
boundaries, silent-tail retirement and reverb resets on slot reacquisition. The outer bypass remains
the same 20 ms dry/processed crossfade. The slot is primed during prepare, preserving a MIX change
between prepare and the first host block. DSP sound algorithms in ReverbProcessor.cpp/HamloVerb.cpp
are unchanged. Legacy space-switch block dependence and Classic's 250 ms / 20 s limits remain.

All 22 original parameters plus bypass retain their IDs, ranges, order and defaults. State remains
the original HamloVerb XML contract. Default and seven original presets retain host program numbers.
Identity remains BBLB / HmVb / com.hamloprod.hamloverb and zero reported latency.

## Verification

- Release Reference, Tests, Lab and VST3 targets built successfully.
- Unit tests: **95 passed, 0 failed**, including the added post-prepare MIX ramp regression.
- Immutable M0 comparisons: see `standalone_reference_check.md` for the final per-case results.
  Protocol covers 13 cases, three rates, mono/stereo and five host block sizes; Legacy automation
  uses matching block schedules. Fixtures were captured before extraction and were not regenerated.
- Unchanged acoustic lab: **4 checks passed, 0 failed**; see `standalone_acoustic_check.md`.
  New V2 functionality is n/a. CPU measurements are informational, not an isolated before/after benchmark.

Commands (repository working directory):

```powershell
cmake --build build --config Release --target HamloVerb_Reference HamloVerb_Tests HamloVerb_VST3 HamloVerb_Lab -- /m:3
build\Release\HamloVerb_Tests.exe
build\Release\HamloVerb_Reference.exe check build\reference\m0 docs\reports\v2\standalone_reference_check.md
build\Release\HamloVerb_Lab.exe docs\reports\v2\standalone_acoustic_check.md standalone
```

If the local fixture cache is absent, expand `m0-fixtures.zip` into `build/reference/m0` before checking.
Never record new output over the preserved M0 fixtures.

## Remaining

The new engine, EQ/effects, routing/workflow UI, state migration and 200-preset bank are not connected
to the production target. Their existing untracked sources require review and implementation verification.
No calibrated bank, final V2 lab, release validators, FL Studio smoke test, installer or 0.2.0 release
is delivered by this stage. The legacy COLOR/CRT overlap is deferred to the planned panel redesign.
HPEQ, subtree and remote repositories were not modified.
