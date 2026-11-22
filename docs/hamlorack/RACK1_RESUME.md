# Historical RACK1 resumption receipt

Superseded by [the full source integration receipt](RACK_COMPLETE.md) and
`RACK_COMPLETE.json`. This file retains the earlier native-adapter milestone;
its pending list and 639-expectation counts are not the current product status.

# RACK1 resumed on Windows, 2026-11-22

Continued from `98327f5` after a fast-forward from `ad71826`. The former remote
`feature/hamlo-plugins` branch no longer exists; this checkout now tracks
`origin/main` without renaming the local branch. No push is part of this run.

The owner explicitly authorized fixes in **any standalone Hamlo VST as well as
the rack**. Fix a defect where it originates and keep both builds synchronized.
The older handoff's blanket prohibition on editing imported plugin code does
not apply to these authorized fixes. Preserve old state/audio compatibility,
record source provenance, and update pinned identities when source changes.

## Changes

- Convert physical snapshot values through the same two range conversions as
  JUCE's public parameter/APVTS path. The initial native test run had 24 failures:
  Comp and Fader raw values and audio differed in all six rate/channel cases.
  The exact comparison was retained; no tolerance was relaxed. This was an
  adapter defect: the standalone wrappers already use the public JUCE path.
- Wait for every layer's snapshot generation before reclaiming old native
  runtimes. Root controls alone cannot acknowledge independently published
  layer snapshots. Reader-held runtimes still protect running tails.
- Release a removed native slot's runtime. Previously the removed slot was
  skipped during processing and could keep its reader reference indefinitely.
- Validate Revolution scene actions and add native scene decoding/round trips.
  The seven lanes use three pattern lock/allow masks (TIME, GLITCH, MOTION);
  SHAPE/THROW data resides in native lane state. Corrected the design metadata
  and added rejection tests for invalid masks. Audio settings are unchanged.
- Permit design-bank checks on a second computer without sibling standalone
  repositories, using the recorded source tree only when it exactly matches the
  clean imported tree. A missing or mismatched identity remains an error.
- Reserve 8 MiB of stack for the MSVC test executable. The first complete suite
  stopped with `0xC00000FD` on entry to DSP M2: that test function holds many
  enlarged fixed-capacity snapshots at once. The filtered test reproduced the
  failure before its first log message. Changing only the diagnostic executable's
  stack reserve allowed it to run. Plugin/host stack settings and DSP are
  unaffected; the final complete suite uses the corrected test target.

## Legacy reference reconstruction

The original computer's raw reference files are unavailable here. Reconstructed
the pre-RACK1 source with `git archive 45d61b3` in
`build/rack1-reference-source`, added the unchanged `RackLabMain.cpp` and only
its executable CMake target, then built with the same local JUCE, MSVC, x64 and
Release configuration as the candidate. The renderer SHA256 is
`eb7c991357df454acf73c35d1516a0fd6a9b5768bc37c1155163f6e9f493ce44`
in both source trees. References are under `out/rack1-local/legacy-before`;
never overwrite these with a candidate executable.

The first candidate comparison passed **224/224** presets with exact float
samples and latency, all finite. This is a same-machine/compiler regression
check, not a cross-compiler bit identity claim. The original metadata is retained
in `RACK1_LEGACY_BASELINE.json`. Neither this three-second fixture nor the native
parity test certifies subjective preset quality or complete long tails.

## Validation status

The durable final results are recorded in `RACK1_CHECKPOINT.json`. Local logs and
raw audio remain under `out/rack1-local`. The design-bank structural suite now
has 13 passing tests and still validates 250 unique, uncalibrated designs.

Native Release and Debug runs each passed 639 expectations. Debug's CRT probe
reported zero audio allocations/frees, including malloc/free. Separate standalone
builds passed Comp's 764 and Fader's 17,432 expectations, and both VST3 targets
built successfully. Their source payloads remain unchanged: the discovered
rounding error was in the rack adapter. Standalone private material calibration
and private auditions were not requested by these test runs.
The complete HPEQ Release suite passed **3,737 expectations, zero failures** after
the test-runner stack correction. The final legacy comparison again passed
224/224 exact renders with unchanged latency.

Re-run the local checks from `D:/HPEQ`:

```powershell
cmake --build build --config Release --parallel 4 --target HPEQ_Tests HPEQ_RackLab -- /nr:false
build/Release/HPEQ_Tests.exe
build/Debug/HPEQ_Tests.exe "HamloRack native"
build/Release/HPEQ_RackLab.exe legacy-check D:/HPEQ/out/rack1-local/legacy-before/renders D:/HPEQ/out/rack1-local/legacy-final.json
python -X utf8 scripts/hamlorack/test_bank.py
python -X utf8 scripts/hamlorack/build_bank.py --check
```

Standalone builds use `Plugins/HamloComp` / `Plugins/HamloFader` as their CMake
source roots and `build/standalone-comp` / `build/standalone-fader` as build
directories. Both use the local `build/_deps/juce-src` checkout through
`HAMLOCOMP_JUCE_DIR` / `HAMLO_FADER_JUCE_DIR`; build targets are
`HamloComp_Tests HamloComp_VST3` / `HamloFader_Tests HamloFader_VST3`.

Native tests compare all ten full native processor wrappers against the runtime
adapter at 44.1/48/96 kHz, mono/stereo, prepare(256), and successive host blocks
1/64/127/512/4096, with identical chunking and transport. This verifies adapter
parity on that schedule; it is not an exhaustive parameter sweep or an
independent comparison between all possible block schedules. Revolution uses
an explicit active scene. Extra probes cover native schema 2, failed revisions,
undo/redo, scene state, slot MIX, channel components, removal and reprepare.
Release allocation counters cover C++ new/delete only; Debug CRT checks also
cover malloc/free.

RACK1 is not yet a release gate: full native composition/PDC, long-tail
bypass/re-enable, concurrent publication/reclamation stress and larger
parameter/transport matrices remain. Runtime bank import, native UI panels,
250-design audio calibration, host validators and DAW/listening checks are
separate unfinished stages. Installed VSTs are not overwritten by this work.
