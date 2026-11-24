# HamloRack controls stage COMPLETE - 2026-11-24

Owner selected16 assignable macros. Implemented and qualified0.2.3. The owner
subsequently authorized commit/push of the completed HamloRack work. No subagents,
model switch or installed DAW bundle replacement. Stage base commit:
9f8c63729bdaa4d461ee0e79c5c56093822df3c9; publication commit is shown by git log -1.
Pre-existing separate build-relocation changes remain local and uncommitted.
Binaries, private audio and build caches remain ignored.

## Deliverable

`batvst/HamloRack.vst3`, build/HPEQ_artefacts/Release/VST3 and immutable independently
validated out/controls-20261124/qualified-artifact/HamloRack.vst3 have IDENTICAL
complete file inventories. DLL SHA256:
0ef2edf2ef3bb35c4ce5389a863ddf48aef16e75b8fc15394842930ea9ea8023.
WindowsFileVersion/ProductVersion and moduleinfo version all0.2.3. ReleaseLTOON.
Normalized DSP source1c5f3358f8331e4a6f820282b44b97ef9ce5b66a29d2ddf6d3bb5ef6fc2b2d3a.
Current receipt: docs/hamlorack/CURRENT_VERIFICATION.json.
Human report: CONTROLS_REPORT.md; usage: CONTROLS_GUIDE.md; design/gates/failures:
AUTOMATION_SIDECHAIN_OUTPUT.md. Previous0.2.2 archived INTEGRATION_0.2.2_*.

## Completed behavior

Original outputGain/bypass IDs retained, macro01..macro16 and finalTrim appended
(19 total). Module-owned bindings support fanout, reverse physical ranges, move/
copy/delete/undo/save. Stale targets and malformed XML rejected. Clear/Unassign
retain current mapped targets. Native editor knob values and gestures feed host
macros; End remains balanced after routing/module removal. Per-module max64
bindings; native parameters only (rack EQ/region/layer controls outside mapping).
20ms macro ramps on fixed16sample clock; VST3 events arrive at JUCE callbacks.

Optional disabled-by-default mono/stereo key bus, per-Comp ExternalSC for all5
models, full key forSide and independent L/R forDualMono, prepared key delays
track preceding rack latency. Disabled/unavailable bus fallsback; enabledsilent
key produces0GR, including removed send with busstillenabled. Key never mixedin.
Generated isolated Comp gets separate external kernels; original internal kernels
verbatim and Plugins/ untouched. FinalTrim after effects/tubes/legacygain/Master;
peak-only CLIPreset retains integratedLUFS. Positive trim can exceed ceiling;
no new limiter. Settled bypass exactdry. Optional schema3; old1/2 remain readable,
absentnew hostcontrols resetneutral. RackUndo projectedits; DAWundo hostvalues.

## Verification (all complete, no active jobs)

- Final quiet UNFILTERED Release9245/0; log release-full.log, immutable
  HPEQ_Tests-final.exe. Ran after all own builds/validators/replays finished.
- Debug finalcontrols214/0, CRT malloc+new with varying macro/trim+hotkey;
  controls-debug-v13.log. Earlier corrected empty-graph contract205/0 and phase49/0;
  finalRelease repeats all tests. No threshold relaxation.
- Final1c5 phase matrix400/0, phase-qualified/phase.json, immutable
  HPEQ_PhaseAudit-final.exe. Direct phase fixtures don't use preset calibration.
- Raw250+20 exact, final embedded250+20 exact. Calibration entries exactlyequal
  calibration-before.json; no preset settings/trims changed in controlsstage.
  Rawverifier5d8b1cf98006c6c6840c8de080952d1a36f815e63420f0192784a7d3da3567db;
  embeddedverifierbf39086a88aaebd145257b658df0936996ccb454a1e069118121d1518df2f9f1.
  Originalrecords/2000task checks/160Power+20tails keep original calibratorhashes.
  finish_bank --embed authenticated all250 final1c5 proofs, Powerhistory appended.
- Corrected immutable ControlAudit2 ran BOTHbaseline0.2.2 andfinal0.2.3:0fail;
  internalPCM537a22ef0c830adaf30e4849c39fd5865902b30edae0186dee80e72f53516f72 exact;
  macro+6.00000030dB, key-15.09779625dB, trim-6.00000049dB, no keyleak.
- Same DLL: Steinberg3.8.0 extensive537/0; pluginval9 GUI SUCCESS/noexclusions;
  independent originalHostSmoke100/100instances and100/100desktopeditors.
- Final Pythonbank/adapter16/0; structural--checkpass. Final UI at1000x700 and
  1280x840 plus nativeCompSC inspected. Private screenshots/audio ignored.

Core commands (cwd E:/HPM/HPEQ):
cmake --build --preset release --parallel 4 --target HPEQ_VST3 HPEQ_RackBank HPEQ_Tests -- /p:CL_MPCount=2
& out/controls-20261124/HPEQ_Tests-final.exe
& out/controls-20261124/HPEQ_Tests-debug-v13.exe 'Rack controls stage'
python scripts/hamlorack/finish_bank.py out/phase-level-20261124/bank-bracket out/phase-level-20261124/material-v4 --embed
ControlAudit2: HPEQ_ControlAudit-v2.exe <absolute bundle> <NEW absolute outputdir> [require]
PhaseAudit: HPEQ_PhaseAudit-final.exe <NEW absolute outputdir>
Raw bank: HPEQ_RackBank-final-raw.exe verify-external <absolute bank-bracket> <absolute material-v4> first count
Embedded bank: HPEQ_RackBank-final-embedded.exe verify <same folders> first count
Ranges0/83,83/83,166/84; Power: power-verify <absolute power> <absolute material-v4>.
Steinberg validator.exe -e <bundle>; pluginval --strictness-level9 --timeout-ms120000
--output-dir<privatefolder> --validate<bundle> (actual command uses separate args,
GUI enabled). HostSmoke --plugin<bundle> --cycles100 --out<privateJSON>.
Full CLI provenance/results/hashes in current receipt and private evidence.

## Failures retained and measurement corrections

Invalid attack0.05ms fixture replaced with same valid1ms as baseline; zero-delay
Fader timingfixture replaced with neutral Udarnik240sample while preserving exact
onset gate. Debugv8 XML numeric-string index rejection and Undo transaction bugs
fixed (177/4 before, final214/0). Old contract helper's one-bus layout was rejected;
corrected to declared buses/keydisabled, same coverage. Obsolete invalid-fixture
run stopped; preserved log and stopnote. Unassign/Clear retention and gesture-End
regressions added after review. Candidate1(9834source)270/400 passed but superseded;
all270+400 repeated forfinal1c5, candidateproofs preserved.

ControlAudit1 had two real failed assertions from an INSTRUMENT bug: edited outer
JUCE VST3PluginState, so requested module graph never reached IComponent. Original
exe/source/before/after preserved. Corrected ControlAudit2 unwraps/wraps IComponent
and checks restored module+feature state. SAMEv2 tool reran both binaries with
unchanged signal/blocks/thresholds; bothpassed. Originalv1 defaultgraph hash is not
Comp-restoration evidence. No productDSPchange or new calibration cycle needed.
Receipt generation initially used a strict parser on SDK moduleinfo (trailing
comma); actual versions verified with PowerShell parser and identical bundle
inventories. No SDK/product metadata edits or validator gate changes.
Private progress/history: out/controls-20261124/control-progress-notes.md and
active-jobs-in-progress.json. Current active-jobs.json explicitly has no jobs.

## Next stage (owner/manual)

Actual FL Studio: sidechain enable/mute/disable, macro automation with editorclosed,
save/reopen and audio audition. Every preset's musical-intent verdict still owner
pending; existing23insert+HR250wet and PW011/PW019 loudness exceptions remain.
Rack sample-asset packaging separate. Do not silently claim owner listening or
install the bundle. Commit/push was subsequently authorized; installation remains a separate action.
Recommend medium for routine host/package follow-up; Astra/xhigh for unexpected
DSP/compatibility diagnosis. No model/effort switch performed. A new chat can
continue from this handoff; none created automatically.

---

# HamloRack phase and level repair COMPLETE - 2026-11-24

Completed the owner-approved phase/level stage. Owner chose changed sound for
all existing projects and low latency. No subagents, model/effort switch,
commit/push or install. Branch main, base/HEAD unchanged:
9f8c63729bdaa4d461ee0e79c5c56093822df3c9. All changes remain in the working tree.
Preserved the pre-existing build-relocation changes described below.

## Deliverable and provenance

Release LTO ON, VST3/Windows version0.2.2:
`batvst/HamloRack.vst3` (also build/HPEQ_artefacts/Release/VST3).
DLL SHA256:5333ea3926478c24e5ff4d3d173dece0d56289c20f069dd1d7a42ca1d076499f.
Immutable independently validated copy:
`out/phase-level-20261124/qualified-artifact/HamloRack.vst3`; identical DLL hash.
Compiled normalized source identity:
5324bfad090c07e9b02e64d66a07461852d53ec02e81454889b7366ffcdda9c3.
Calibration format3 checks this identity plus actual recipe resource hash.
Native plugin payloads, host parameter IDs and state schema unchanged.
Evidence and private audio: out/phase-level-20261124/ (ignored, do not commit WAVs).
Authoritative current receipt: docs/hamlorack/CURRENT_VERIFICATION.json.
Human report: PHASE_LEVEL_REPORT.md; failures/method: PHASE_LEVEL_METHOD.md.
0.2.1 receipt/notes archived as INTEGRATION_0.2.1_*; 0.2.0 remains historical.

## Completed behavior and measurements

Common crossover phase for parallel layer delta sums; phase-aligned serial dry
references, unused L/R and M/S components and wet-only subtraction. Independent
histories replay the actual coefficient clock. Disable/mode/region retirement
is sample-clock based, with stale phase programs invalidated. No added lookahead;
active mix0 retains all-pass phase, settled global bypass original aligned dry.
External parallel dry routing needs phase assessment. Old project sound can change.

Final phase audit400/400, zero mono channel difference, max magnitude error
0.000003106316754dB. Same original instrument failed232/400 before repair.
Debug phase/transition/search/RT49/49, CRT malloc+new covered. That Debug binary
precedes only the host program-name metadata fix; audio code unchanged, identity
transition proven by reconstructing entire previous source hash from that one line.

All250 task recipes individually measured:2000 source/level/tempo renders,
safety250/250,142/142 compressor tuning targets (before final Master input trim),
loudness226/250 within+/-0.5LU. Actual final Comp/RedWall GR also recorded per render.
23 insert peak-growth exceptions plus HR250 intentional wet-only remain explicit.
Power20/20 safe:160 checks+20tails,18/20 loudness matches; PW011/PW019 headroom
exceptions. Worst Power8xTP -0.349999236944497, tail -100.21424225552187dBFS.
All270 canonical PCM hashes exact through final DSP before embedding AND final
embedded bank. Original frozen bracket calibrator/records retain original hashes;
separate current-source proofs authenticate unchanged records. No relabelled EXEs.
Dataset material-v4 has36 WAV fixtures, category pairs + HR019 shakers/HR059 claps;
old-computer dataset unavailable, so no direct old/new loudness-count comparison.
All270 loudness-matched A/B pairs ready, private bank-bracket/audio and power/audio.
PRESET_REVIEW.md has250 individual engineering decisions. Owner listening pending.

## Completed acceptance checks

- Final unfiltered quiet Release suite:9031 expectations,0 failures,
  `HPEQ_Tests-final.exe`, full-release-final.log/.exit0 (session51319 complete).
- Final VST Steinberg3.8.0 extensive:537/537; pluginval strictness9SUCCESS (GUI
  included, no disabled tests); independent binary host all pass,100/100 instance
  and100/100 desktop editor cycles. Each final log/.exit0 in evidence root.
- Final PhaseAudit: phase-qualified/phase.json,400/400; canonical embedded replays
  three verify-embedded logs83+83+84 exact, Power20 exact, all .exit0.
- Python structural15/15; build_bank.py --check0; configure_build_tests.ps1
  6checks pass; git diff --check0 (only line-ending warnings).
- Existing legacy MonstaProcessor/TubeColour CPU regressions passed unchanged
  p99 gates:Revolution1/8=7.4/66.1%, Tubes11.4%. Eight-instance max139.1% deadline;
  this is not a timing guarantee for eight full native racks. DAW check pending.

Failures preserved/documented: original phase232/400; earlier transition and
retired-region failures; initial Newton HR005 failure replaced by entire bounded
calibration; earlier full Release9013/2CPU under load. First qualified Steinberg
536/1 found advertised program0 blank -> Default + contract regression, renewed
source guard and ALL canonical proofs. JUCE rcfile lacked Info.txt dependency,
leaving Windows0.2.0 with VST0.2.2 -> CMake dependency fixed. An early final suite
was stopped at obsolete browser recipe hash; exact pin updated for authorized
16 Master ceilings, test-only rebuild, then entire suite rerun9031/0. No gates
relaxed. Archived *before-program-name*, *before-resource-fix*, original logs/tools.

## Reproduction and remaining scope

Build: build_vst.bat (no install); Release production LTO stays ON.
Tools:

```
cmake --build --preset release --parallel 4 --target HPEQ_RackBank HPEQ_Tests HPEQ_PhaseAudit -- /p:CL_MPCount=2
```

Replay: HPEQ_RackBank-final.exe verify <bank-bracket> <material-v4> [first count];
power-verify <power> <material-v4>. PhaseAudit requires a NEW output directory.
Finalize exact receipt: python out/phase-level-20261124/finalize_receipt.py after
all real .exit0s. Source changes invalidate dependent calibration: frozen original
record hashes must stay intact; reverify before embedding. Do not run or overwrite
an EXE while its build target links. All jobs completed; no active task process.

Uncommitted work: Source/DSP phase paths/engines, Source/Rack calibration guard,
Source/Plugin program name, CMake, tests/tools, 250/20 assets, numerical receipts,
method/review/current integration docs, scripts/hamlorack and preserved BAT/helper
relocation changes. No Plugins/ changes. Private files only in ignored out/.

Next coherent stage: owner audition in FL Studio, then the already requested
host automation, external sidechain and final-output controls. Include automation
with frequency moves and delayed native branches in the next validation design.
Rack sample-asset packaging also remains separate. Do not claim musical approval.
Recommended for next DSP/automation design: owner's Astra/xhigh; no switch made.
Continuation prompt: "Continue from docs/NEXT_STAGE.md. Phase/level stage is
complete; implement the next agreed automation/sidechain/output stage."

---


# HamloRack source integration - 2026-11-22

Current receipt: `hamlorack/RACK_COMPLETE.md`, `RACK_COMPLETE.json` and
`RACK_BANK_REPORT.md`. Native panels, executable 250-recipe browser, measured
calibration, real Fader actions, editable Revolution scenes and dual-mono/native
runtime handling are integrated. Existing 224 factory programs retain the legacy
path. HamloMag's mono oversampler defect was corrected in the common standalone
and rack source, with new provenance and strict native parity/CRT tests.

The owner will build the VST and check FL. Host validation, DAW state/lifecycle
and individual musical-intent/listening verdicts belong to that build/check;
do not present them as completed automated source tests. Numerical loudness
exceptions stay listed individually. Preserve the immutable references under
`out/rack1-local/legacy-before`. No push or installed plugin replacement was done.
No subagents/model changes. Older milestones below are historical.

---

# Historical HamloRack RACK1 continuation - 2026-11-22

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
