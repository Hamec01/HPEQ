# HamloFader V2: initial audit and scope proposal

2026-11-03. Parent a7d26e8; clean checkout at entry. Owner chose Fader next.
Apply efficient-staged-work: one agent, targeted reads, durable handoff.
No HPEQ changes, subtree/push, installed plug-in replacement or installer.
Historical AGENTS sections about unfinished Mag/Revolution are stale.

## Audit-stage targets, before measurement

This stage audits the unchanged M0; it does not implement a new audio path.

| Check | Acceptance / boundary |
|---|---|
| Archive | Preserve original Source, Tests, CMake, existing tools and complete VST3 bundle before rebuilding; SHA256 manifest |
| Current suite | Fresh build of unchanged production/tests; at least the historical49expectations, zero failures |
| Current UI | Actual1180x700snapshot, inspect preset/header/control overlap |
| Contract | Record8parameters (seven plus bypass),8programs (Default plus seven), class IDs/state tag, current0sample host latency |
| Changes | Source, Tests, CMake and shared PresetScreen unchanged in this audit |
| Limits | Existing tests do not establish exhaustive bit-exact compatibility, automation scheduling, allocation freedom, MATCH accuracy or metering conformance |
| Future sound | New features n/a until scope answers, fixed numerical design/methods and matching before/after laboratory exist |

## Observed implementation

- CMake compiles32production source units: LayerEngine, FxNodeProcessor,
  crossover/EQ, unrelated effects and their panels. prepare also initializes
  shared processing pools. Standalone Fader needs its own isolated closure.
- ModuleVariant::makeFader defines Gain-60..+24dB, Delay0..20ms,
  Target-40..0dBFS, Mix0..1, PolarityOff/L/R/Both, Mute and Solo; bypass follows.
  IdentityBBLB/HmFd/com.hamloprod.hamlofader, state tagHamloFader.
- GainProcessor lives beside compressor/reverb utilities in Processors.cpp/h.
  Its settled path deliberately uses float pow/multiply; its changing path uses
  double gain smoothing and fractional linear delay interpolation. Isolation
  must preserve both paths and surrounding delta/mix arithmetic exactly.
- Gain/mute/polarity/delay smoothing is documented as20/10/10/50ms respectively;
  these numbers describe existing implementation, not newly validated targets.
  Delay is an audible effect and host latency remains0samples. Changing delay
  bends pitch; no negative-time advance is available at0samples host latency.
- Input meters are channel-averaged one-pole mean-square300ms/3s; output300ms.
  They are RMS/dBFS measurements, not LUFS or true peak. MATCH is a UI action:
  gain=clamp(target-inputLong,-60,+24), ignored below-100dBFS. It is not a
  continuous level rider or a proven integrated-loudness normalization.
- Solo exists to isolate frequency bands in HPEQ. In this standalone whole-layer
  instance it has no useful solo action. Preserve its ID and Legacy behavior;
  avoid presenting it as functional cross-track solo in the modern interface.
- Seven factory recipes:Unity,Pad-6,Pad-12,Boost+6,PolarityFlip,Match-18,Match-14.
  The last two set only the target; loading them does not execute MATCH.
- State stores APVTS values; selected program is not serialized. Existing tests
  roundtrip values on a fresh instance, not prepared/edited audio history.
- Current tests enumerate defaults/programs, bypass, finite bounded audio,
  one program across block sizes and basic state/layout. The test named44.1/48k
  uses only its48kconstant. Do not infer two-rate coverage from its title.

## Published behavior references

These sources inform the problem space, not names, layouts, proprietary code or
claims of matching a competitor. No competitor audio/binary has been measured.

| Official reference | Relevant published behavior | Own proposal / boundary |
|---|---|---|
| [Blue Cat Gain Suite manual](https://www.bluecataudio.com/Doc/Product_GainSuite/) | Gain utilities, channel control and automation; linked instances are a separate feature | Precise local channel gain/automation. Cross-instance groups require explicit separate scope; not assumed |
| [dpMeter5 manual](https://www.tbproaudio.de/assets/content/manuals/dpMeter5%20manual.pdf) | RMS/loudness/true-peak readings, reference matching and explicit reset | Separate named units, learning/reset/freeze and bounded match. Measurement compliance must be tested independently |
| [Vocal Rider manual](https://www.waves.com/1lib/pdf/plugins/vocal-rider.pdf) | Bounded automatic gain movement with target, activity and timing controls | Own optional level follower with explicit boost/cut limits and pause policy; no semantic voice-detection claim |

## Approved scope

Owner answered2026-11-03: full Trim/Match/Ride/Motion, separate modes. Ride has
Natural and Fast profiles with adjustable correction range. These decisions are
settled; do not ask again. Detailed numerical contracts follow per stage.

1. **Trim:** large precise gain fader, fine numeric entry, Input/Output trim,
   stereo balance/per-channel level, polarity and clearly defined alignment.
2. **Match:** deliberate Learn -> measured result -> Apply/Frozen Gain, with
   explicit RMS versus LUFS mode, peak readout and silence/insufficient-data
   handling. Fixed matched gain does not keep chasing the music.
3. **Ride (optional):** continuous bounded gain adjustment. Proposed Natural
   and Fast profiles; adjustable range/timing/activity floor so pauses do not
   ramp noise upward. Show actual gain correction and idle state.
4. **Motion (optional):** own editable volume contours, fades, tremolo and
   rhythmic curves, tempo/free timing, depth and phase. Remains a focused gain
   tool; no assumed new saturation, reverb, glitch or frequency split engine.

Retain Black Steel, the shared top-left PresetScreen and Legacy compatibility.
Modern neutral startup/Default policy, learned-state persistence, exact ranges,
transport/seek policy and integration gates must be fixed in the numerical design
before implementing new DSP. Host MIDI input and automatic writing of DAW
automation are not assumed. Parameter automation is supported through host IDs.

The standing library target is200newrecipes/4genres. Its role allocation depends
on approved Ride/Motion scope. For a utility-only tool,200gain offsets would not
establish meaningful recipe diversity; resolve that explicitly rather than pad
the library with renamed levels. Preserve original programs0..7 in either case.

## Proposed implementation stages

- **F1:** source-bound M0archive and exhaustive paired compatibility lab; then
  isolate own Legacy processor. No new sound. Preserve8IDs/programs/state/class,
  delta arithmetic, ramps, meter updates and same-block Legacy output.
- **F2:** approved kernel/meter/control algorithms with numerical goals and the
  same before/after instrument; unsupported baseline goals receive n/a.
- **F3:** host state/clock/automation/bypass integration, neutral startup, actual
  fresh/prepared restoration, block1/64/127/512/4096 and zero audio allocations.
- **F4:** QUICK/ADVANCED/Legacy, precision readouts/context help, actual screenshots.
- **F5:** meaningful library/calibration, guarded resume, final validators, VST3/
  ZIP/SHA256, report and local commits. Owner FL listening stays explicit.

Before F1changes, fix its exact numeric matrix/method and capture every original
program, parameter boundaries, relevant paired settings, events and actual
fresh/edited state restorations. Do not treat this audit's49checks as that proof.
