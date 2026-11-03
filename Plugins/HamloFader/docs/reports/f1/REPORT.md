# F1: isolated HamloFader Legacy engine

2026-11-03; parent41e0ee9. Approved next product scope is full Trim, Match,
Ride and Motion with separate Natural/Fast riding profiles. This stage isolates
the existing sound; internal version stays0.1.0. New modes are not implemented.

## Result

Production now uses9owned units instead of32: own Gain, Legacy delta-chain,
minimal settings/host mapping, original Fader panel and shared Black Steel/preset
support. Neither LayerEngine/FxNodeProcessor nor other effect/panel processors
are in the reachable production include closure or static library members.
Inactive old source files remain outside that closure. The original Gain body
was copied intact, including settled float pow/multiply and changing double
math, delay interpolation, RMS updates and reset behavior.

The own chain preserves initial20msnode/slot fades, float delta construction,
layer addition and the wrapper's additional bypass interpolation. Solo's
standalone no-op behavior and the UI's RMS MATCH action remain. Eight parameter
IDs/order/defaults/ranges/choices, eight programs/recipes, state/class IDs and
0sample host latency remain. Selected-program restoration retains M0behavior;
modern persistence is a later stage. Tail report remains0for this Gain kind.

| Check | Result |
|---|---|
| Exact primary cases |226/226PCM/state/parameter/meter/latency observations |
| Actual restored observations |452/452fresh or already-prepared/edited original-state restores |
| Raw PCM |678original/current outputs exact; all finite; all measured callback allocations0 |
| Matrix | All8programs at44.1/48/96k mono/stereo,64/127/512blocks; min/max7controls; paired gain/polarity,delay/mix,mute/mix,bypass;8/192krate limits; sample-positioned events at1/64/127/512/4096; reprepare |
| Historical normalized roundtrip differences |0in this matrix; report compares actual original observations |
| Native suite |49passed/0failed; original tests unchanged |
| Build closure |9production units,9static library objects; recursive own include closure contains no other effects/project model |
| Processor sizeof |844152->1816bytes; this measures the object, not allocated heap |
| VST DLL size |12602368->10648064bytes (embedded assets/JUCE still included) |
| UI | Shared UI/Fader panel source unchanged; final original/isolated repeat pixels exact |
| Negative controls | Changed method,changed host contract,corrupt raw PCM,corrupt state all rejected; originals preserved |
| Validators |pluginval5..10exit0; Steinberg537passed/0failed on exact final DLL |

F1_METHODS.md recorded the targets before the lab and isolation. The same
immutable laboratory captured original M0and verified the own engine. Original
Source/Tests/CMake/bundle were already archived by the audit. The reference
executable was copied before replacing production code. It preserves actual
original fresh and edited/restored state/audio rather than assuming a reset.

## Visual measurement disclosure

The first isolated PNG differed from the initial original PNG at688pixels,
all within the two needles (boundsx345..768,y327..370). BlackSteel::meterNeedle
adds a wall-clock-driven tremor. This animation source was not modified.
Repeating both original and isolated captures produced exactly identical final
PNGs (panel_original_repeat.png/panel_isolated_repeat.png), also matching the
original audit image. Keep all initial/repeated images. No audio method or
numerical compatibility goal changed; no UI source was altered for a screenshot.

The lab reached226/226on its first isolated run. There was no DSP correction,
relaxed tolerance, changed audio method or new-state migration during this stage.
Zero-allocation claims cover C++allocation hooks during measured process calls.
Legacy automation is compared under identical block schedules, not asserted
block invariant beyond those observations. Future modes must meet their own
cross-block event goals. CPU/heap savings and new metering accuracy are not
inferred from object size or file size.

## Evidence and continuation

Authoritative:acceptance.json,before.json,after_initial.json,native.txt,
library_members.txt,guards.json,panel*.png,validators_initial/identity.json.
Raw PCM/states:out/f1/reference; original tool:out/f1/reference_lab.exe;
corrupted negative-control copies:out/f1/guards. Original archive:
out/audit/original. Tools/verify_f1.py rechecks all hashes, methods and closure.

Source ID:`921cc23a153cbc12ff77ff65e48e54122bc1d44ac5eb647a9c9b5f94857cc3b9`.
Method ID:`42b060e1c5febf278f2e223657d28cde6ef02ac29a904d1daabd1b3ce1e92d46`.
Validated DLL SHA256:`c8489ef44ed8ebbaa7fdf44c284f7e5514876cce01a917dbab5e770f0ed02841`.

No installed VST, installer, other product/HPEQ, subtree or push changes.
Owner FL listening is pending. Next:F2new kernel/meter numerical contracts and
matching before/after measurements for approved Trim/Match/Ride/Motion,
Natural/Fast, then host/UI/library stages. Original programs0..7remain;
200newmeaningful recipes ultimately give208programs.
