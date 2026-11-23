# HamloRack source integration — 2026-11-22

Historical checkpoint at `c0bd771`. The owner rejected generic replacement panels,
visible legacy modules and manual default-reset upgrades. This receipt describes
that earlier checkpoint only. The replacement integration and its current checks
are in [CURRENT_INTEGRATION.md](CURRENT_INTEGRATION.md).

Source is ready for the owner's VST build and DAW/listening checks. The rack now
uses all ten pinned native module wrappers, with native parameter panels and an
executable 250-recipe browser. This is a source/test receipt, not a claim that
the new VST has passed host validators or owner listening.

## Implemented

- Native Comp, Udarnik, RedWall, Mag, Echo, Verb, Zastava, Shirota, Fader and
  Revolution; complete public parameter defaults, strict physical-value mapping,
  native factory programs and versioned state. Existing projects keep the legacy
  path until explicitly upgraded. Native scenes retain all eight Revolution
  scenes, seven lanes, locks and allow masks.
- Searchable native controls, typed values, default/fine adjustment, module
  programs, live meters, real Fader Learn/Stop/Apply/Undo and scene-step editing.
  Editor rebuilding follows preset load, undo and module changes.
- Independent Dual Mono instances, including Fader right trim, alignment,
  polarity, routing and combined Match measurements. Fixed PDC and the rack's
  insert/send delta law remain intact. Wet-only returns subtract one aligned dry
  reference. Master recipes put final RedWall after the other layers and gain.
- Parameter/scene edits preserve live Revolution history. Runtime retirement is
  protected by layer generations and audio references. Removed modules release
  their runtime. Node bypass preserves Echo/Verb tails, including initial silent
  gaps; self-noising inserts retire. Host tail metadata follows freeze/unfreeze.
- Shared HamloMag 0.2.2 fixes mono oversampler layout assertions/heap logging in
  both standalone and rack. Its exact prefix commit and upstream base are in
  `Plugins/sync/2026-11-22-mag-mono.json`. Other nine payloads retain their pinned
  identities, including Revolution 0.5.1. Comp/Fader already used the correct
  public JUCE path; their earlier rounding discrepancy was in the rack adapter.
- Build/copy/install paths use HamloRack while preserving the HPEQ plugin/state
  identity. The all-plugin BAT builds current standalone sources from `Plugins/`.

## Final verification

| Check | Result |
|---|---|
| Complete Release suite, otherwise idle machine | 7,276 passed, 0 failed |
| Native/product matrix, Release | 4,176 passed, 0 failed |
| Native/product matrix, Debug CRT hook | 4,176 passed, 0 failed; zero instrumented audio malloc/new/free |
| Standalone HamloMag tests | 747 passed, 0 failed |
| Structural bank tests | 13 passed, 0 failed |
| Legacy factory renders/state/latency | 224/224 exact |
| Rebuilt embedded calibrated bank | 250/250 canonical renders exact |
| Native panel snapshots | 30: ten modules at 1000×700, 1280×840, 1920×1200 |

The native public-wrapper comparison covers 44.1/48/96 kHz, mono/stereo and
successive blocks 1/64/127/512/4096 after prepare(256), including the distinct
authored engine signatures. Composition uses the predeclared 2e-6 float tolerance;
wrapper parity and legacy/embedded replay remain exact. This is not an exhaustive
arbitrary-parameter or automation-schedule sweep. Standalone Comp/Fader's prior
764/17,432 passing checks are carried forward only because those trees are unchanged.

The legacy reference was reconstructed from `45d61b3` on this machine, with the
unchanged renderer (SHA-256 `eb7c991357df454acf73c35d1516a0fd6a9b5768bc37c1155163f6e9f493ce44`).
It covers 48 kHz stereo, prepare(256), blocks 127, two seconds input plus one
second tail; it does not certify every full tail or subjective sound. Original
raw references under `out/rack1-local/legacy-before` were never overwritten.

An initial full suite run during six simultaneous calibrators failed two
existing CPU timing gates. The unchanged complete suite passed with those jobs
finished. No timing or audio threshold was relaxed. Release used LTO off on this
machine; Debug provides the CRT allocation verdict. Detailed hashes, source
identities, local logs and scope are in [RACK_COMPLETE.json](RACK_COMPLETE.json).

## Presets and audition

All 250 recipes completed 2,000 numerical verification renders on two local
recordings/sample arrangements per category, three headroomed input levels and
80/120% host tempo checks. All 250 pass the numerical signal/peak guards; all
142 compressor tuning targets were attained. Independent FIR 8x true peak was
used for masters. Five failures were corrected by changing only their final
ceiling from -1 to -1.05 dB; original receipts were retained.

Canonical loudness is within +/-0.5 LU for **129/250**. There are **121 listed
exceptions**, primarily the peak-growth constraint plus the wet-only return.
They remain exceptions, not bank-wide waivers. Individual measured values,
calibration overrides and pending intent verdicts are in
[RACK_BANK_REPORT.md](RACK_BANK_REPORT.md) and
[RACK_BANK_RECEIPT.json](RACK_BANK_RECEIPT.json). The compressor tuning result is
measured before final master input trim; final renders report actual Comp and
RedWall maximum GR as well.

Private audio is under `out/rack-complete/bank-material-v2/audio`. For equal-level
audition compare `HRxxx-matched-dry.wav` with `HRxxx-matched.wav`: both receive
common attenuation when needed for at least 1.5 dB measured peak headroom.
Calibrated `-dry.wav` / `-processed.wav` remain separate. Original disk audio was
read only; neither source nor rendered audio is tracked. Source-separated stems,
single-note arrangements and an accompaniment substitute for synth are labeled;
they do not establish musical suitability for every genre or backing-vocal task.
See [the method and correction disclosures](RACK_BANK_METHOD.md).

The design JSON keeps its original authoring-stage status strings and values
for reproducibility. Executable status and measured overrides are separate;
the importer refuses missing/stale embedded calibration. The current runtime
bank does not require the private audio or any external preset files.

## Owner build/check

Run `build_vst.bat` for the rack, or `build_hamlo_plugins.bat all` for HamloRack
and ten current native standalones. Rack output:
`build/HPEQ_artefacts/Release/VST3/HamloRack.vst3`; copies go to `batvst/`.
Root legacy module targets are compatibility builds; current standalone fixes
come from `Plugins/`. The optional rack install backs up old rack bundles before
replacement. Installation scripts were parsed/reviewed, not run on installed VSTs.

The owner will build the VST, run host validators/FL load-save and lifecycle
checks, then assess each recipe's authored intent/rejection condition on music.
Revolution's offline sample/chop/drum workbench remains its separate asset/render
workflow; this integration exposes its live insert parameters and scenes.
No new VST build, installed replacement, push or published release is claimed.
