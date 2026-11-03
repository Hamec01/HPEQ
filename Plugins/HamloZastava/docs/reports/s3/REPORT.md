# S3: VST host adapter and state restoration

2026-11-02. Parent `35f6917`. The four-mode S2 kernel is now connected to the
VST. This is an internal 0.1.0 build with the existing Legacy editor; modern
QUICK/ADVANCED controls and the 200-preset library are still required.

## Accepted behavior

- Appended 61 permanent `v2_` parameter IDs after the original 15 + bypass:
  **77 parameters** total. Original IDs/ranges/order/defaults and XML/class
  identity remain unchanged. The Legacy spec still describes only its old 15.
- Fresh instances select Clean with Modern Processing off, producing exact
  5 ms delayed dry. Host programs Default + six factory recipes retain their
  original Legacy behavior and program numbers; loading one clears modern
  controls, with bypass retaining its historical program behavior.
- Clean/Drum/Duck/Rhythm and Soft/Hard map to the unchanged S2 kernel. Duck's
  user Attack controls onset, Release controls recovery. All new continuous
  settings have explicit parameter grids; DSP and saved values use that grid.
- Optional disabled-by-default External Key input supports mono/stereo,
  independently of main mono/stereo. Prepared scratch preserves key input when
  it shares main memory. Disjoint auxiliary samples are left unchanged.
- Both prepared engines consume the original input, with fixed PDC applied
  once. Legacy/modern transitions use a 20 ms crossfade; outgoing modern mode
  stays active while returning to Legacy. Modern mode changes use S2 behavior.
- BPM, PPQ, transport and chunk offsets feed the modern clock. Both parameter
  bypass and direct host bypass retain PDC. Missing external key exposes status
  and passes delayed dry. Processing and state-reset callbacks allocate no C++
  objects after preparation, including callbacks larger than prepared256.

Production is 12 owned translation units. Foreign effects, project graphs,
LayerEngine and foreign panels remain outside the production build. The
processor object is 3,304 bytes; this is not total heap/working-set usage. Both
engines continue clocking; no new FL CPU-performance claim is made here.

## Verification

| Check | Result |
|---|---|
| Original contract | First16parameter metadata, seven programs, state XML tag and VST class IDs unchanged |
| Legacy M0 parity | 229 primary PCM/state/latency cases +458 actual fresh/edited restored observations exact |
| Modern host laboratory | 283 passed, 0 failed; original M0 control 2 passed, 7 unavailable |
| Neutral startup/bypass/Mix0/key loss | 40 mono/stereo cases at8/44.1/48/96/192k, exact delayed dry |
| Independent DSP comparison | 48 exact comparisons:4modes ×2profiles ×2main layouts ×3key layouts |
| State | All8mode/profile combinations restored fresh and already prepared/edited; exact modern normalized values, bytes, selected program and tested audio |
| Old/partial/invalid state | Modern controls cleared on old load; defaults replace missing new fields; wrong root/malformed/future schema ignored; nonfinite/out-of-range new values contained |
| Event scheduling | Same-position mode/profile/filter/bypass/key/tempo/seek/transport events exact at1/64/127/512/4096 after prepare256 |
| Native | 219 passed, 0 failed, including direct host bypass, all61new parameter state values, four alias layouts and zero callback allocations |
| Validators | pluginval5/6/7/8/9/10 all exit0; Steinberg537passed/0failed |

No final modern editor, host-side listening verdict, genre calibration or final
musical-quality release claim follows from these integration tests. The S2
numerical kernel sources are byte-identical after normalized line endings;
their separate laboratory and eight prepared owner-material A/B renders remain
historical S2 evidence. Integrated musical listening in FL is pending.

## State encoding

Full states have `v2_schema=1` and `v2_program`. Old/missing modern schema or
engine field selects Legacy and resets every modern field, including when an
edited modern instance already exists. Schema1 missing individual modern fields
uses defaults, preventing stale controls. Unsupported future schemas are ignored.
On full load, modern history is reset on the next audio callback; histories are
not serialized. Fresh and already prepared restoration paths are tested.

Legacy with neutral modern controls uses the original sixteen-field state
encoding and preserves its historical program restoration behavior. Retained
edited modern controls while Legacy is selected use full encoding. The original
191 normalized-readback roundtrip quirks remain in Legacy; they are not silently
fixed. No new parameter ID repurposes a Legacy parameter or slot.

## Disclosed failures and method work

The initial same-method M0 capture reproduced all original229primary and458
restored observations. First integration passed266host checks but failed17
combined state checks; Legacy229 and native206 passed. Expanded diagnostics
separated normalized settings, serialized bytes, selected program and audio.
Only the serialized bytes differed in those measured states.

APVTS constructs raw defaults without snapping, e.g.124.999992ms versus125ms;
loading the new state snapped them. DSP controls and full-state serialization
now use the same explicit grid. Legacy values/encoding remain unchanged. No
exactness criterion or tolerance was weakened. A native full-byte check was
also added (207native expectations before alias coverage).

The first native suite used distinct bus memory. Four added alias layouts then
failed, despite283host/207native passes and passing vendor validators. The
parallel Legacy path could overwrite key input before modern read it. Copying
key samples into prepared scratch before either path fixes all four; the same
companion suite now has219expectations. The old failing executable/sources/log
are retained. Vendor validation was repeated on the corrected DLL.

Diagnostics/disclosure changed the primary method stamp without changing its
assertions. The final v3 method was freshly compiled and run against both actual
M0 and current libraries. All229control fixtures still match original S1v3.
Final before: `before_v3.json` with `out/s3/reference_v3`; final after:
`after_host_v4.json` and `after_legacy_v4.json`. The last label is a current-run
revision, not a different method. Native219 and `validators_v3` bind the same
corrected production DLL. Previous failed/intermediate reports remain in this
directory and `out/s3`; they are not final acceptance evidence.

## Artifacts

Authoritative: `acceptance.json`, `before_v3.json`, `after_host_v4.json`,
`after_legacy_v4.json`, `native_v3.txt`, `validators_v3/identity.json`.
Methods: `docs/S3_METHODS.md`, `Tests/S3HostLabMain.cpp`,
`Tests/S3LegacyLabMain.cpp`; companion: `Tests/S3IntegrationTests.cpp`.
Audit: `Tools/verify_zastava_s3.py`; validators: `Tools/validate_s3.ps1`.

S3 source ID:
`c2cd5c52e2fb2cf848903f2bd5c7d09ee50b598b9aff1d393f80bc59a16d600e`.
Method ID:
`9069dde9dd70d55f28d043844bd13a20446a35af6561d7c00dab35653c891b26`.
Validated DLL SHA256:
`fe8d11b5e7b2ae908b988c6a3a130b5a26b3934c9907ffebdbbb8e427cdd03f3`.
The DLL also contains the matching current production-source stamp, checked by
the audit. Baseline S1 bundle remains in `out/s3/baseline/HamloZastava.vst3`.

Current internal bundle:
`E:/HamloZastava/build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3`.
It has host-accessible new parameters but the Legacy panel controls the old
parameters. **Do not present it as the finished modern interface.** No installed
plug-in, installer, ZIP, push/subtree or other product was changed. Next stage:
modern QUICK/ADVANCED editor, contextual help and detector audition; then200
genre presets (207host programs total), final validators and owner FL audition.
