# HamloRack: integration contract and 250-recipe design bank

Date: 2026-11-20. Stage: RACK0. Product name: **HamloRack** (owner suggestion,
adopted as the working product name). Repository and legacy state identity: HPEQ.

## Deliverable and release status

This stage specifies 250 individually authored whole-rack recipes against the
actual ten beta source trees in `Plugins/`. They are **design candidates**, not
calibrated factory programs. The JSON is deliberately outside
`Assets/Presets/Factory`; the current HPEQ loader must not silently ignore beta
parameters and present a different sound under a new name.

The current HPEQ engine still runs its earlier module implementations. Importing
standalone source into `Plugins/` did not connect those implementations. Runtime
adapters, beta panels, audio calibration and the release binary remain next-stage
work. No audio quality, loudness, true-peak, listening or host-validation result
is implied by structural recipe validation.

## Bank allocation

| Bank | Count | Timing reference |
|---|---:|---:|
| Boom Bap | 50 | 90 BPM |
| Trap | 50 | 140 BPM |
| Techno | 50 | 132 BPM |
| Drum and Bass | 50 | 174 BPM |
| Studio | 50 | 120 BPM reference; free-time effects labeled by their native time setting |

Each genre bank has Kick 6, Snare/Clap 6, Hats 5, Percussion 4, Drum Bus 7,
Bass 5, Vocal 5, Keys 4, Samples 4, Master 4. Studio adds lead/backing vocals,
guitar, keys, synth, mix-bus and utility tasks. The same source category across
genres must differ in processing intent and audible behavior, not just its label.

Each recipe has a stable ID, source/problem description, intended audible
difference, rejection condition, ordered EQ and module settings, reference BPM
and a calibration plan. Time constants are in milliseconds or seconds; sync
choices use the exact native enumeration. There is no random recipe generator.
Common helper functions only encode an explicitly authored signal-chain decision.

Read the [overview](PRESET_OVERVIEW.md) for all 250 intentions and chains, or
the [detailed catalog](PRESET_CATALOG.md) for exact parameter choices and explicit
scene events. Machine-readable settings are in
`Assets/Presets/HamloRack/Design/recipes.json`; their used native IDs, bounds,
choice labels, source-file hashes and source-tree identities are in the adjacent
`native-contract.json`. The declaration reader covers the fields used here, not
the entire native host ABI. The future adapter must use each pinned product's
actual complete defaults and state serializer.

Structural review corrected five reverb low-cut candidates above the native
1000 Hz maximum, and changed an inaccurate four-bar title to a bar-end title.
No four-bar scheduler is implied by a one-bar scene. A shadowed helper was removed;
the checker now rejects duplicate top-level function definitions. Pattern steps
in the readable catalog are 1-based; stored array offsets are 0-based. These were
design/data corrections, not audio-calibration results or relaxed audio targets.

## Adapter contract

1. Keep vendor `BBLB`, plugin code `HpEq`, bundle identity, APVTS parameter IDs,
   legacy `HPEQ` root, `.hpeq` files and existing user-preset folder compatible.
   Change the display name to HamloRack in a runtime stage, with a host identity
   comparison before release. Do not rename IDs or rescale choice automation.
2. Old slots without an explicit beta-engine revision retain the existing DSP
   path, defaults, latency and send semantics. Compare saved legacy states and
   renders bit for bit before and after adapters. A new recipe explicitly opts
   into a pinned beta revision; never infer it from the new display name.
3. Build namespace-isolated native DSP adapters from pinned source. Comp has
   `hamlo::dsp`, while several other products retain `hpeq::dsp`; their identical
   class/header names cannot be linked wholesale into HPEQ. Do not overwrite
   shared `ProcessorsM6` with one product's private copy.
4. Reset an instance to the pinned native parameter defaults before applying a
   recipe. The recipe lists physical native host values, not normalized host
   automation or packed DSP slots. Respect transforms: Udarnik `rise`/`decay`
   become log2 in its DSP; `sensitivity` becomes one-minus. Keep source defaults
   pinned even where the schema extractor covers only literal parameter entries.
5. MIX is owned once. For inserts, native MIX is unity and the rack blends the
   latency-aligned processed and dry signal with `amount`. For Echo/Verb, request
   native **wet-only** output and add `amount * wet` to exactly one aligned dry
   path. Verb native `routing=Send`, `keepDry=Off`; Echo Score `scoreRouting=Send`.
   Verify impulse, bypass, MIX 0/1 and tails. A wet-send amount is not a native
   insert crossfade. Rack-wide wet-only utility recipes must suppress the dry
   path explicitly after that contract is implemented.
6. Native output protection, monitor and delta controls remain represented.
   Udarnik `protect`, Mag `match`/`delta`, RedWall `monitor`, Fader measured-match
   state are plugin-level behaviors; copying only their inner kernels drops them.
   Recipes keep diagnostic audition modes off. Modern Zastava uses its `v2_*`
   controls, Shirota its `v2*` controls, Echo its Score controls.
7. Fader Match is an explicit capture/action, not a live rider. Its stored
   `matchedDb` must be measured for the calibration clip. Ride remains a separate
   mode with an activity floor and bounded boost/cut. Rhythm ducking without an
   external bus must be called tempo ducking, not external sidechain.
8. Revolution patterns are explicit seven-lane, 16-step scene data. Store lane
   locks, allow masks, engine selection and the active scene, using the native
   scene serializer. Do not regenerate from a seed at load. Block GLITCH action
   numbers differ from Legacy GLITCH action numbers. Unused SHAPE/THROW lanes
   must be cleared, even if their native factory defaults are enabled. Offline
   sample/chop/drum-composer workbench features need a separate sample asset and
   render-job contract; they are not an always-running insert effect.
9. Allocate processors, pools, delay memory, oversamplers and scratch outside the
   audio callback. Extend the fixed snapshot capacity intentionally: the old
   48-float block cannot hold modern Echo/Shirota/Verb and Revolution state.
   Report one deterministic latency per active topology, including dry alignment,
   node/layer alignment and transitions. Check pool exhaustion explicitly.
10. Preserve the ten-layer serial/parallel delta law. An inserted band processor
    changes only its selected frequency region; a parallel dry copy must not
    double the signal. Bass paths remain centered. Final true-peak limiting must
    be after all additive sends/layers and output gain; a band or branch limiter
    cannot promise a rack-wide ceiling.

## Acceptance fixed before audio runs

| Gate | Required result |
|---|---|
| S1 Source identity | All ten payload trees match clean standalone HEAD trees; record hashes and source versions. The registry's old Revolution 0.4.0 entry is historical if the actual tree is 0.5.1. |
| S2 Recipe structure | Exactly 250 stable unique IDs and names; allocation above; every parameter ID exists in the pinned declaration and every value is finite, in range and discrete where required. |
| S3 Meaningful difference | No duplicate processing topology/settings within or across banks after removing names, notes and IDs. Each recipe has its own source, intent and rejection test. This does not prove audible uniqueness. |
| S4 Routing | At most ten layers, eight frequency nodes per layer and six modules per chain; valid EQ/dynamic settings and frequencies; no unintended diagnostic mode, frozen feedback or unseeded generator. |
| A1 Legacy | All 224 existing factory states and representative user states retain exact renders with beta opt-in absent. Keep the existing bank intact and stable. |
| A2 Adapter parity | For every module and every used engine: native vs rack adapter, aligned, same source/transport/settings; exact output where the wrapper permits, otherwise predeclare and justify tolerance before measuring. |
| A3 Audio safety | No NaN/Inf, missing pool, stuck tail or unintended silence. Run 44.1/48/96 kHz, mono/stereo, blocks 1/64/127/512/4096 after prepare(256); no audio-thread allocations; verify reported PDC. |
| A4 Calibration | Each track/bus recipe: same-input BS.1770 loudness difference within +/-0.5 LU after rack output calibration, unless the intentional effect or peak constraint prevents it; report the exception, never mask it. Peak growth <=3 dB over input for noncreative track recipes; masters independent 8x true peak <=-1 dBTP. |
| A5 Dynamics | Comp threshold calibrated against intended GR (gentle 1-3, medium 3-6, parallel heavy 6-12 dB). Master limiter work <=3 dB unless the individual mastering intent explicitly allows more; lower the loudness target rather than destroy transients. |
| A6 Intent | Each recipe's authored expected-difference and rejection test assessed on its appropriate material, then a second independent clip; test at -24/-18/-12 dBFS nominal input with input peak <=-3 dBFS. Do not treat a numerical topology difference as an audible result. |
| A7 Mono and timing | Low-band mono/correlation and mono-fold loss measured for spatial recipes. Tempo recipes checked at reference BPM and +/-20%; an unsynced free delay must be labeled and intentional. |
| A8 Listening | Render an input, processed and loudness-matched A/B for all 250. Record an individual verdict and revision, not one verdict for the bank. Owner FL listening remains separate from objective checks. |
| A9 Product | Real HamloRack editor snapshots inspected at supported scales; pluginval 5-10, Steinberg validator, DAW load/save, instance/editor lifecycle, preset undo and factory-browser navigation. |

Private input/output audio stays outside tracked source. Calibration receipts
must include source and fixture hashes, native version, recipe hash, engine
revision, BPM/sample rate, measured loudness/peak/intent results and failures.
Changing DSP invalidates dependent calibration. Do not silently alter these
thresholds after a run.

The three input levels must all remain unclipped. Prepare a sufficiently
headroomed canonical clip and attenuate it for the level ladder; do not force
every sparse transient clip to an RMS target that violates the -3 dBFS peak
limit. Record achieved RMS and peaks along with the nominal input target.

## Reproducing the structural gate

From the HPEQ repository root:

```powershell
python -I -X utf8 scripts/hamlorack/build_bank.py
python -I -X utf8 scripts/hamlorack/build_bank.py --check
python -I -X utf8 scripts/hamlorack/test_bank.py
```

Build writes the JSON, native contract, readable catalogs and structural receipt.
Check recomputes from the authored decisions and actual pinned sources and fails
on stale artifacts without writing. The 12 tests include rejection cases for
unknown/out-of-range/nonfinite parameters, fractional choices, duplicate recipes,
invalid geometry, dry/wet routing, diagnostics, scene actions, Fader capture and
the final master ceiling stage. They are not DSP or listening tests.

## Next stages

- RACK1: engine revision/state/snapshot model and native adapters; preserve legacy
  renders and prove module parity before exposing the new bank.
- RACK2: integrate beta module panels and HamloRack identity/browser; translate
  the already authored recipes into executable native states without losing keys.
- RACK3: per-recipe calibration, independent verification and listening renders;
  repair failures individually, then package the beta after release checks.

The repository's one-stage-per-chat preference applies. No source subtree,
installed VST, old factory state or release binary is changed by RACK0.
