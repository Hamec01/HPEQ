# HamloZastava: initial V2 audit

2026-11-02. Owner selected Zastava after Shirota. Baseline commit:f7330fd;
working tree was clean. This is an audit and proposal, not a completed V2.
Work only in E:/HamloZastava, without subagents, HPEQ/other product changes,
push or subtree. Deliver VST first; installer is not an immediate requirement.
Apply the owner's efficient-staged-work preferences. No model switch occurred.

## Current implementation

- Version0.1.0/M0. Shared build still contains LayerEngine, other effects and
  their UI panels. Processor owns LayerEngine; isolate the gate first.
- Fifteen original parameter specs plus bypass: threshold, range, attack, hold,
  release, mode, ratio, hysteresis, key HP/LP, mix, output, lookahead, key listen,
  flip. Gate, expander and duck laws exist as ZASTAVA/PATRUL/TREVOGA.
- Threshold-90..0dB, default-40; range0..90dB; attack0.05..100ms; hold0..500ms;
  release1..2000ms; ratio1..100; hysteresis0..24dB. Original Default is active,
  not a neutral new-engine recipe. Preserve its legacy project behavior.
- Core lookahead0/2/5ms; wrapper fixes total latency at5ms(240samples/48k).
  Key filters operate on input-derived detector, not a separately routed bus.
  Audio buses currently expose main input/output only, mono/stereo.
- Linked peak detector with fixed10ms detector release. Gain uses attack/
  release smoothing in dB; hold/hysteresis govern the open flag. PATRUL uses a
  continuous downward-expansion target; TREVOGA ducks while open.
- GateProcessor selects the basic M6 law or extended ZastavaCore according to
  optional style/lookahead/key/listen/flip/output values. Switching that branch
  clears gate state. Legacy compatibility must preserve both branches and their
  transitions, not just copy ZastavaCore. New processing should not reset because
  an unrelated Output value passes through zero.
- Black Steel1180x780, shared preset screen at the standard top-left rectangle.
  Existing archived panel snapshot inspected; no request to redesign its style.

Files inspected:Standalone/ModuleVariant,ModulePluginProcessor;DSP/HamloZastava,
ProcessorsM6;CMake source closure;Tests/PluginTests;M0 extraction report.

## Verification and missing evidence

The available native test executable was run again: **71passed,0failed**,
build/zastava_audit_native.log. This checks the existing binary, not a new
source-fingerprinted clean rebuild. Historic M0 report also records71checks,
pluginval5..10 and fixed240sample latency. Those historic validator results are
not fresh V2 validation and are not a substitute for new acceptance.

Current tests exercise parameter defaults, fixed latency, bypass, finite output,
block sizes, state and editor/preset placement. They do not yet quantify attack
preservation, tail retention, false triggers, chatter near threshold, bleed
rejection, external-key timing, coefficient automation, or all real-time memory
allocations. No acoustic superiority, noise reduction, click level or timing
accuracy is claimed before measurements.

The repository AGENTS section9 is stale: Mag is complete per owner, and
Verb/Echo/Shirota/Revolution have progressed elsewhere. Do not resume that old
Mag task or edit those products. Current handoff:docs/NEXT_STAGE.md.

## Proposed product direction — owner answers pending

One processor with separate natural cleanup, drum-tail control, external-key
ducking and creative rhythmic gating. Keep precise typed controls and a QUICK
page with a few macros; ADVANCED exposes detector, timing and response shape.
Retain Legacy and original IDs/order/ranges/program indices/state/class identity.
Append new IDs. Old states must reset every new field on an edited instance.
New functions default neutral. Preserve shared PresetScreen unchanged.

Proposed priorities:
1. Reliable gate/expander, adjustable opening/closing behavior and stereo link,
   key filters/audition, visible detector and applied gain, optional external key.
2. Preserve attacks and quiet wanted details; separate control of body/tail
   instead of simply chopping the whole signal. Quantify ghost-note retention.
3. Ducking with independently tuned response and optional tempo-related timing.
4. Rhythm source/shape editing only after the owner selects the creative scope.
5.200meaningful genre presets with calibrated, purpose-specific checks, stable
   IDs, resumable DSP/material/tool-bound records, state/audio verification.

Do not define a single generic preset-depth metric yet: cleanup, drum shaping
and ducking have different desired signals. Choose material annotations and
metrics first, then record numerical goals before sound changes. Measure with
one before/after lab, mark unavailable features n/a and disclose failures.

Owner questions sent: include all four tasks or prioritize cleanup/drums versus
creative effects? Preserve quiet details by default, remove them aggressively,
or separate soft/hard profiles? No response assumed at this audit checkpoint.

## Published behavior used only as references

- [FabFilter Pro-G expert mode](https://www.fabfilter.com/help/pro-g/using/expertmode):
  sidechain filtering/routing and configurable stereo triggering are useful
  reference problems. Do not copy names, control layout or implementation.
- [FabFilter Pro-G timing](https://www.fabfilter.com/help/pro-g/using/timecontrols):
  separate opening/hold/release/lookahead controls inform the test matrix.
- [Sonnox Oxford Drum Gate manual](https://dload.sonnoxplugins.com/pub/plugins/UserGuides/Oxford_Drum_Gate_User_Guide.html):
  detection versus wanted decay/quiet hits are distinct evaluation problems.
  Its instrument classification is not implemented or promised here.

Our implementation, names, appearance and measured decisions remain our own.

## Immediate independent next stage

Before changing sound, save the M0 binary, source/contract identity, states and
raw renders of original programs, both legacy branches, boundary settings and
automation. Then isolate a Zastava-only processor/build and prove identical
PCM/state/latency at identical event/block schedules. Preserve the archived M0,
fixed PDC and allocation discipline. Confirm feature scope from owner answers
before designing V2 DSP and freezing numerical targets.

## Follow-up checkpoint

Owner answered:all four roles, separate soft/hard profiles. S1isolation now
complete; see reports/s1/REPORT.md and ZASTAVA_V2_SCOPE.md. Earlier pending/audit
statements above are historical. New V2algorithms have not been implemented yet.
