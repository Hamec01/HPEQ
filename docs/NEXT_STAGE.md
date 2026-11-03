# HamloFader: F1 complete, F2 new modes next

2026-11-03. Parent audit41e0ee9; F1 completion commit follows (git log -1).
No active jobs. Apply efficient-staged-work: no subagents, targeted reads,
source/input/method guards, one coherent stage with durable handoff. No other
product/HPEQ change, installed copy, installer, push/subtree or model switch.

## Settled owner decisions

Full Fader: separate Trim / Match / Ride / Motion. Ride has Natural and Fast,
adjustable boost/cut correction range. Owner answered both; do not ask again.
Black Steel and shared top-left PresetScreen remain. Original8IDs/programs and
state/class identity preserved; future208programs with200meaningful recipes.
Scope/official references:docs/FADER_V2_AUDIT.md. Avoid copying other products'
names/implementation. No cross-instance groups, MIDI input, automatic DAW curve
writing or new nonlinear/FX engine assumed. Host parameter automation remains.

## Completed evidence

Initial audit: unchanged49native pass, actual panel inspected; original/rebuilt
VST/Tests/Snapshot byte-identical. Audit reportdocs/reports/audit/REPORT.md.
Original Source/Tests/CMake/bundle/tool archiveout/audit/original; hashes in
original.json. Parent a7d26e8. Do not overwrite it.

F1 now builds9owned units vs32. FaderGain body/member layout copied intact from
original Processors; LegacyFaderCore preserves slot fade/node delta/layer sum;
host wrapper retains its own bypass interpolation. Minimal FaderSettings avoids
project graph. Old unrelated source remains uncompiled/unreachable. Static
library lists9objects; recursive include closure has no foreign effect/model.

Final226/226primary and452/452actual fresh/edited-prepared restored observations
match original,678exact PCM, meters/state/parameter/program/PDC preserved;
zero measured C++callback allocations.49native,pluginval5..10/Steinberg537pass.
Four method/contract/raw/state corrupted references rejected. Original and
isolated repeat PNGs exact. Initial isolated image's688needle pixels differed
because unchanged BlackSteel has wall-clock tremor; all captures retained.
sizeof844152->1816 is object storage, not allocated heap. Internal version0.1.0;
new Trim/Match/Ride/Motion not yet implemented, owner FL listening pending.

Final reportdocs/reports/f1/REPORT.md; acceptance.json; before.json /
after_initial.json; native.txt; library_members.txt; guards.json; panel*.png;
validators_initial/. Source921cc23a153cbc12ff77ff65e48e54122bc1d44ac5eb647a9c9b5f94857cc3b9.
Method42b060e1c5febf278f2e223657d28cde6ef02ac29a904d1daabd1b3ce1e92d46.
DLLc8489ef44ed8ebbaa7fdf44c284f7e5514876cce01a917dbab5e770f0ed02841.
Raw referenceout/f1/reference; immutable original exeout/f1/reference_lab.exe.
Corrupted copiesout/f1/guards; never reuse those as controls. Current lab
build/Release/HamloFader_F1Lab.exe. Frozen driver/method remains unchanged.

## Next action: F2

Before new audio, write HAMLOFADER_V2.md with exact controls/ranges/defaults,
state/reset/clock/pause policies and numerical goals, then a single paired lab.
Baseline is this isolated F1; unsupported new features report n/a. Preserve
Legacy and verify actual state history, not only nominal current settings.
Define precise Trim/channel/polarity/alignment; independent named RMS/LUFS/TP
meters; deliberate Learn->Apply/Frozen match; bounded Natural/Fast riding with
activity floor/no noise boost during pauses; deterministic editable Motion
curves/fades/tempo/free timing. Design all feature buffers for prepare and0sample
host latency. Negative delay cannot advance time under that latency policy.
Do not silently promise a peak limiter: Fader gain/metering is its own task.

Original Match is a UI action target minus3sRMS, not LUFS/riding. Solo has no
standalone effect and its ID stays. Original selected program is not serialized.
Keep modern learned/preset state separate and explicitly reset new controls
when loading old states. New IDs append after original8; keep original ranges,
order and program positions0..7. Modern neutral Default must have no effect.

F1 lab contract requires exactly8parameters/programs. Once F3/F5append them,
clone a prefix-aware compatibility driver and recapture both original/current
under matching methods, as Zastava did; do not edit/pretend old F1method passes.
F2 new kernels require sample-positioned block1/64/127/512/4096determinism.

## Commands

cmake --build build --config Release --target HamloFader_VST3 HamloFader_Tests HamloFader_F1Lab HamloFader_Snapshot -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_f1.py

Frozen reference/report capture paths cannot be overwritten. For revalidation
choose NEW paths/labels. Source/DSP/method changes invalidate dependent evidence.
Normal build doesn't require the archive. JUCE8.0.12local read-only path:
E:/HPEQ/build/_deps/juce-src. No environment upgrades. Recommended Astra xhigh
for F2DSP/meter design or unexpected results, medium for routine execution;
no model/effort switch was performed.

Continuation prompt:Continue E:/HamloFader from AGENTS.md and docs/NEXT_STAGE.md.
F1is accepted. Implement F2 for the approved full Trim/Match/Ride/Motion scope,
Natural/Fast, with numerical targets and paired baseline first. Work alone.
