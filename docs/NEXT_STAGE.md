# HamloRack RACK1 continuation - 2026-11-22

Resumed from `98327f5` after pulling the other computer's saved checkpoint.
The former remote feature branch was deleted; local `feature/hamlo-plugins`
now tracks `origin/main`. No push or installed VST replacement is authorized
as part of this continuation.

Native adapter rounding, Revolution scene masks/codec, removed runtime references
and per-layer reclamation acknowledgements have been repaired. Native Release
and Debug matrices each pass 639 expectations; Debug CRT audio allocations/frees
are zero. The reconstructed same-machine legacy reference passes all 224 exact
audio/latency comparisons. Structural checks pass 250 designs and 13 tests.
Separate Comp/Fader builds pass 764 / 17,432 expectations and build their VST3s.
The complete HPEQ Release suite passes 3,737 expectations after a test-only
8 MiB stack reserve correction for its enlarged snapshot fixtures.

The owner explicitly permits fixing any standalone Hamlo VST along with the
rack. Fix defects at their source, preserve compatibility, and synchronize the
native pinned identity when the imported payload changes. The present adapter
fix did not require a standalone DSP change. No subagents/model changes.

Read `docs/hamlorack/RACK1_RESUME.md` and `RACK1_CHECKPOINT.json` first. Remaining
RACK1 work: full composition/PDC and long-tail bypass/re-enable tests, concurrent
publication/reclamation stress, expanded parameter/transport/block schedules.
Then RACK2 native panels/runtime bank import; RACK3 recipe calibration and DAW
checks. The 250 recipes remain design-only. Preserve references under
`out/rack1-local/legacy-before`; never render the candidate over them.

---

# Historical HamloRack RACK1 WIP handoff - paused by owner 2026-11-20

Owner explicitly stopped implementation and requested commit + push. Save this
as an **unfinished checkpoint**, not a completed RACK1 or a released beta.
Current branch `main`; pre-checkpoint base `45d61b3`. The checkpoint commit is
the commit containing this handoff (`git log -1` after publication). No model
or reasoning setting was changed and no subagents were used.

## RACK1 work saved

- Captured all **224 legacy before-renders**, finite, zero capture failures.
  Same deterministic stereo 48 kHz fixture, prepare(256), blocks 127, BPM 120,
  two seconds of input plus one second of silence. Local executable, raw `.f32`
  references, source SHA256 manifest and original receipt remain under
  `out/rack1/legacy-before/`; do not overwrite these with the new DSP binary.
  The durable metadata receipt is `docs/hamlorack/RACK1_LEGACY_BASELINE.json`.
- Added `HPEQ_RackLab legacy-write / legacy-check`; the renderer and fixture
  stay the same for the pending after-comparison.
- Added build-directory namespace isolation for all ten complete pinned native
  AudioProcessor wrappers. Imported `Plugins/` payloads remain untouched.
  Native editor construction is replaced internally; Revolution uses its
  existing offline constructor. Native parameter/state/output wrappers remain.
- Added native fixed layouts/runtime bindings, a message-thread runtime registry,
  fixed-capacity snapshots and versioned native slots. Absent native revision
  retains legacy DSP; legacy saves still write schema 1, native states schema 2.
- Added native processing in frequency/full-layer nodes, outer MIX ownership,
  wet-only Echo/Verb send rules, alignment and reader-held runtime lifetimes.
  Project publication returns errors rather than silently replacing an invalid
  native module with legacy processing. Final correctness is not verified yet.
- Increased alignment capacity for the native fixed delays and combined
  frequency-node plus whole-layer-chain latency. This needs the pending legacy
  exact comparison and dedicated PDC tests.

## Verified versus still pending

The isolated native libraries and Release `HPEQ_RackLab` / `HPEQ_Tests` compiled
at a checkpoint; existing native-source warnings are in the logs. No tests were
run from those new executables. Native inputs changed after that successful build:
`Source/Rack/NativeApi.h`, `Tests/Unit/StateTests.cpp`, and
`scripts/hamlorack/generate_native.py`. Reconfigure/rebuild before trusting the
current source snapshot. The design-bank formatter also received a trailing
newline correction during commit preflight; its generated files are pinned to
LF in `.gitattributes`. Do not call the latest working tree fully build-verified.

**Not run:** legacy after-comparison, native-vs-adapter parity, the new runtime
allocation/lifetime/geometry/transport matrix, Debug CRT allocation checks,
validators, UI snapshots and DAW/listening checks. The 250 new recipes remain
design candidates, with no runtime importer or audio calibration. RACK0's 12
structural tests passed in the earlier stage; that is not a native DSP verdict.

Known unfinished items/risks to resolve first:

1. `native::Scene` and a draft `makeRevolutionScene` encoder were added to the
   API/generator after the build checkpoint; regenerate, compile and round-trip
   test that code before using it. The RACK0 scene
   metadata currently lists seven lock/allow arrays, whereas the native pattern
   owns three lock/allow arrays; SHAPE/THROW steps live in native lane state.
   Resolve and disclose that design correction when adding the encoder.
2. Check exact parameter equivalence through the public native APVTS API versus
   the adapter's raw physical snapshots. Native normalization/denormalization
   can round float values; do not assume exact parity or relax its threshold.
3. Prove runtime registry reclamation, bypass/re-enable, tail retirement and
   publication under playback, including reprepare at a different sample rate.
4. Native wrapper nodes explicitly reject `DualMono` until independent native
   mono instances exist; Left/Right and Mid/Side paths need their own probes.
5. `StateTests.cpp` now asserts legacy schema 1 explicitly. Add native schema 2,
   malformed/unknown revision, native blob and undo/state tests; do not treat
   this assertion edit as a substitute for round-trip/audio verification.

## Exact continuation commands

From `E:/HPM/HPEQ` (the old `build/` cache points at a moved `E:/HPEQ` checkout;
keep using the new build directory):

```powershell
cmake -S . -B out/rack1-build -G "Visual Studio 18 2026" -A x64 -DHPEQ_JUCE_DIR=E:/HPM/HPEQ/build/_deps/juce-src
cmake --build out/rack1-build --config Release --target HPEQ_RackLab HPEQ_Tests -- /m:4 /nologo /verbosity:minimal
out/rack1-build/Release/HPEQ_RackLab.exe legacy-check E:/HPM/HPEQ/out/rack1/legacy-before/renders E:/HPM/HPEQ/out/rack1/legacy-after.json
```

Build logs: `out/rack1-configure.log`, `out/rack1-native-build.log`,
`out/rack1-engine-build.log`. Baseline capture log:
`out/rack1/legacy-before/run.log`. The latest source/build metadata receipt is
`docs/hamlorack/RACK1_CHECKPOINT.json`. Do not rerun `legacy-write` with the new
binary into the immutable before directory. Preserve the reference executable
`HPEQ_RackLab_reference.exe` and the 224 raw reference files.

No active jobs at the owner's stop. No installed VST, release package, private
audio or binary is being published. All RACK0 files and the unfinished RACK1
source/docs are included in the requested WIP source commit. Further push or
continued implementation needs the owner's next instruction.

Next action on resume: finish the scene encoder, rebuild the saved checkpoint,
then run the exact legacy comparison and implement the independent parity and
RT probes described in `docs/hamlorack/RACK1_IMPLEMENTATION.md`. Recommend
**Astra / xhigh** for this DSP/state compatibility work; the owner controls
the selector. Continue without subagents.

---

# Historical RACK0 handoff - 2026-11-20

## Objective and scope

Owner wants the beta Hamlo modules connected into HPEQ, a sensible product name
and 250 deliberately designed whole-rack presets. Working product name:
**HamloRack**. This chat completed one bounded design/data stage under
`efficient-staged-work`; runtime integration and audio acceptance are still open.
Do not describe the design bank as 250 finished or calibrated factory programs.

Actual checkout: `E:/HPM/HPEQ`, branch `main`, base commit `45d61b3`
(`R18 / 0.5.1: SAMPLE CHOP whole sample, BARS 1-2-4-8-16`). No commit or push was
made. Ten standalone checkouts are siblings under `E:/HPM`; Revolution's source
is `HamloRevolutionTransitions`, not the other Revolution or generated folder.

## Completed

- Root README adopts the working HamloRack name and explains current build/state
  identity. The actual binary/UI is still HPEQ.
- `docs/hamlorack/RACK0_DESIGN.md`: native adapter/state/routing contract, source
  integration traps and structural/audio gates fixed before future audio runs.
- `scripts/hamlorack/recipes.py`: 250 individually authored recipes, HR001-HR250;
  five banks of 50 (Boom Bap, Trap, Techno, Drum and Bass, Studio). Every recipe
  has an ordered chain, native physical parameters, EQ, intent and rejection test.
- `Assets/Presets/HamloRack/Design/recipes.json`: generated design bank, clearly
  marked `HamloRackDesign` / design-only / no runtime importer. It is outside the
  executable legacy factory bank. Fader Match explicitly requires a capture.
- Adjacent `native-contract.json`: used parameter declarations, source hashes and
  all ten exact native source-tree identities. Actual Revolution is **0.5.1**;
  old `Plugins/sync/` registries describing 0.4.0 are historical. Do not downgrade.
- Readable `PRESET_OVERVIEW.md` and `PRESET_CATALOG.md` in `docs/hamlorack/`.
- Builder/checker and 12 failure-oriented tests. Explicit 20 Revolution scenes,
  21 frequency-region designs, 618 total module instances and 10,550 explicit
  parameter settings before future calibration. No random preset generation.

## Verification

Commands from `E:/HPM/HPEQ`:

```powershell
python -I -X utf8 scripts/hamlorack/build_bank.py
python -I -X utf8 scripts/hamlorack/test_bank.py
python -I -X utf8 scripts/hamlorack/build_bank.py --check
git diff --exit-code HEAD -- Source CMakeLists.txt Assets/Presets/Factory Plugins
```

Build/check: 250 candidates, 250 unique exact processing signatures, exact bank
allocation, all ten modules covered; IDs/ranges/discrete grids/capacities/scenes
and native send contracts validated. Source payload trees equal clean standalone
HEAD trees for all ten; imported working payloads checked for local edits.
Unit checks: **12 passed**. Frozen result: `docs/hamlorack/STRUCTURAL_VALIDATION.json`.
Every audio status remains `not_run`. Exact signature uniqueness does not prove
audible uniqueness. No C++ build, audio render, calibration, listening, host
validator, UI screenshot or package verification was done in this design stage.

Structural review fixed invalid reverb low-cut values (native maximum 1000 Hz),
an inaccurate four-bar label and a shadowed helper. No audio thresholds were
changed or measured failures hidden. Check the current structural receipt for
final bank/contract hashes rather than copying a previous intermediate hash.

## Uncommitted owned files and jobs

- `README.md`
- `docs/NEXT_STAGE.md`
- `docs/hamlorack/` (design, two catalogs, structural receipt)
- `scripts/hamlorack/` (authored recipes, build/check, tests)
- `Assets/Presets/HamloRack/Design/` (two JSON files)

No active jobs. Existing DSP, CMake, the 224 legacy `.hpeq` presets, plugin
payloads and standalone source trees were not changed. Do not discard this work
or regenerate a different 250-item bank when starting the adapter stage.

## Next action: RACK1

Read the design contract and implement a versioned beta slot/state/snapshot
representation with native DSP adapters and unchanged legacy paths. Establish
before-renders for all 224 legacy factory states before modifying runtime DSP.
Prove native/adapter parity and PDC/MIX/tail semantics before translating or
exposing the new recipes. Do not load the design JSON through the old `.hpeq`
loader, silently ignore unfamiliar keys or overwrite common DSP headers with
one standalone's copy. Native output wrappers matter, especially sends,
Udarnik Protect, Mag Match/Delta and Fader Match/Ride.

Recommend **Astra / xhigh** for this DSP/state compatibility stage. No model or
effort switch has been performed; the owner controls the app selector. Continue
without subagents. Current owner request authorizes integration; old standalone
instructions saying to leave HPEQ untouched describe the earlier extraction
stage and do not require asking for that authorization again.

Continuation prompt: "Continue HamloRack RACK1 from docs/NEXT_STAGE.md: implement
versioned native beta adapters, preserve exact legacy renders and keep the
authored HR001-HR250 bank for subsequent calibration."
