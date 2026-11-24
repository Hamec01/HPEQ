# HamloRack controls stage — design and acceptance

2026-11-24. Owner selected sixteen assignable macros. Baseline is the qualified
0.2.2 DLL, SHA-256 5333ea3926478c24e5ff4d3d173dece0d56289c20f069dd1d7a42ca1d076499f.
This stage preserves its sound when new controls are unused. No native source
payload edits, new lookahead, automatic installation or publication.

## Contract fixed before implementation

- Keep `outputGain` and `bypass` IDs/version/ranges. Append `macro01`..`macro16`
  (normalized 0..1) and `finalTrim` (-24..+12 dB, default 0), parameter version 2.
- Store macro destinations on their module, including parameter ID and normalized
  endpoint range. One macro can address several destinations; one parameter has
  one macro. Move/copy/undo/save retain explicit assignments; deleting/replacing
  a module cannot redirect its assignment to another module at the old position.
- Map native ranges with their actual skew, interval and discrete snapping.
  Audio reads APVTS atomics, never edits the project, allocates, locks or posts
  messages. Smooth macro targets for 20 ms on a sample clock; affected FX nodes
  consume controls on a fixed 16-sample clock, independent of callback sizes.
  VST3 parameter delivery is at JUCE callback boundaries; no claim of arbitrary
  within-callback host event timing. Native processors keep their own smoothing.
- Add a disabled-by-default mono/stereo external key bus. Main input/output stay
  matching mono or stereo. Native HamloComp alone can select External SC per
  module. Disconnected/disabled bus falls back to internal detection; an enabled
  silent key produces no compression. Key audio never enters the program signal.
  Preserve all five models and source-shaping choices. FET external mode feeds
  its feedback detector from the key reduced by its own gain reduction.
- Align the external key to the program's accumulated rack latency at each
  compressor; preserve full-band external key in Mid/Side modes and independent
  left/right key channels in Dual Mono. All key delay memory is prepared up front.
- `finalTrim` follows every rack effect, tube colour and legacy output gain,
  including Master routing. Smooth for 20 ms; settled global bypass remains
  original aligned dry. Existing true-peak/loudness meters read the final output;
  show a latched overload indication with Reset. This is monitoring and trim,
  not an undisclosed clipper or a new mastering limiter.
- Optional new module features require schema 3. Old schema 1/2 files load with
  no assignments/internal SC and new host parameters at their neutral defaults.
  Old preset/calibration data stays intact; renewed source proofs are mandatory.

## Acceptance gates

| ID | Gate |
|---|---|
| C1 | New unused controls: all 250 task + 20 Power canonical PCM hashes exact to 0.2.2; default/bypass identity and reported latency unchanged |
| C2 | All 16 macro IDs present and distinct; same old parameter IDs/order; real hosted VST3 automation changes intended audio, including closed editor |
| C3 | Assignments/ranges survive serialization, undo, reorder/copy and deletion without targeting an unrelated module; malformed targets rejected |
| C4 | Same absolute-time control events: blocks 1/64/127/512/4096 after prepare(256) match sample-for-sample, no audio-thread new/malloc |
| C5 | External key reduces a below-threshold program by >=6 dB; muted key does not; silent program plus key has exact zero output; all five models finite |
| C6 | Mono/stereo main/key layouts; missing key fallback; latency-aligned impulse/ducking after preceding delayed modules; Side/Dual Mono behavior |
| C7 | Final Trim -6 dB measures -6 +/-0.02 dB after the final Master stage; no increase in limiter GR; bypass dry exact; overload latch/reset follows post-trim true peak |
| C8 | Real UI at minimum and normal sizes: sixteen macro controls, assignment/range management, per-Comp SC selector and final output controls visible and usable |
| C9 | Full Release regression, targeted Debug allocation tests, pluginval 9, Steinberg extensive, independent VST3 host lifecycle |

Record the same black-box control audit on baseline and final DLLs; absent new
controls report N/A before. Preserve every failed run and any method correction.
Owner musical listening and actual FL Studio routing/automation remain separate
checks, never inferred from these numerical results.

"Disconnected" key fallback means a disabled/unavailable host bus. The plugin
cannot infer a removed cable from silence on an enabled bus: enabled silent key
must remain silent detection, as required by C5. The UI reports bus enablement.

## Development disclosures

The first Debug control fixture used the legacy compressor's 0.05 ms attack,
outside the pinned native parameter range; project publication was correctly
rejected, so its automation/SC assertions had no active module. Change that
fixture to the baseline audit's valid 1 ms. Audio gates and the frozen black-box
instrument are unchanged; preserve the rejected-fixture log.

The initial key-timing fixture used Fader, whose reported latency is zero, so
its positive-delay assertions correctly failed. Replace that upstream module
with neutral Udarnik (fixed 5 ms) and observe compressor GR once per sample.
Keep both the positive-latency requirement and exact onset-at-key-plus-latency
gate for preceding-slot, serial-layer and pre-MIX-region paths.

Debug v8 added an actual binary/XML save/reopen test and found a production
loader defect: JUCE XML restores numeric properties as strings, while the new
macro index validator accepted typed integers only. Accept strictly parsed integer
strings too; reject fractional, malformed and out-of-range text. The same run
found macro edits sharing one undo transaction. Give Assign/Remove/Clear their
own transactions. Preserve v8's four failed assertions; repeat unchanged audio
gates after both fixes. Release v1 is a development build, superseded before
calibration/qualification.

The old Debug plugin-contract helper constructed a one-input-bus layout, which
JUCE rejects for a processor now declaring two input buses. Its configure failures
left the processor unprepared and invalidated dependent checks. Update the helper
to use the declared layout with the auxiliary key disabled; keep the same complete
rate/block/channel grid, exact identity and zero-allocation gates. The failed old
helper run remains in contract-debug.log; repeat with the corrected layout.

Final routing review found Unassign/Clear reverting a destination to its pre-macro
static value. Retain the current mapped physical target (actual native range,
reverse mapping and snapping) when removing a binding, in the same undo action.
Add a regression for individual and whole-macro removal. This control-thread
change invalidates source proofs: preserve the first 9834ffcd canonical replay,
then repeat all270 replays and embedding with the final source fingerprint.

Embedded-editor review: retain the original host parameter for a gesture's End
when a destination is cleared or its module deleted before mouse-up. Balance
shared-macro gestures and ignore new touches from a stale editor runtime. Add
actual native-parameter/editor regressions for Clear and deletion. This UI bridge
change does not alter the audio-source identity; final VST qualification includes
it. Debug v12 before this bridge refinement passed200/0 with removal retention.

The first frozen ControlAudit1 final-DLL run reported0dB macro gain and0dB external
ducking (two failures). Investigation in JUCE8.0.12 and the existing HostSmoke
adapter showed an instrument defect: it edited VST3PluginState, not the base64
IComponent's HPEQ state. Requested module graphs never reached the processor.
Preserve the original executable/source, before/ and after/ records. ControlAudit2
unwraps/wraps IComponent and requires a module/feature state acknowledgment before
rendering. Rebuild a separate immutable v2 tool and repeat BOTH0.2.2 and0.2.3 with
it; same signal, rates, block sizes and gain/duck/leak/trim thresholds. The original
v1 internal hash describes its default graph and is not evidence of compressor
state restoration. No product DSP change or calibration invalidation is needed.
