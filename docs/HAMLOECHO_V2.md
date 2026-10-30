# HamloEcho 0.2.0 design and acceptance contract

Approved direction: 2026-10-30. Work is confined to E:/HamloEcho. HamloVerb is complete and must not be modified. No HPEQ integration, subtree operation or push is included.

## Musical behaviour

An editable score of eight reflections combines time, note, level, pan, forward/reverse playback and space send. A piano keyboard and text entry expose actual note names and MIDI numbers. SOURCE NOTE is a manually declared reference: MIDI 60 is C4, A4 is 440 Hz. Changing a target note transposes the complete incoming audio relative to that reference. It does not extract individual notes from a chord or force an evolving melody to one absolute pitch. Automatic monophonic pitch tracking is a separate, uncommitted extension; polyphonic pitch correction is outside this design.

The score supports free notes, a repeated target note, and an editable staircase generated from a start note and interval. Targets remain bounded by the supported pitch range. Fixed-note reflections are derived from source history rather than repeatedly shifting already shifted audio. A separate feedback-transposition mode deliberately accumulates pitch and timbral changes; its limits must be explicit.

Rhythmic placement supports free time, tempo divisions, triplets, swing and disabled reflections. Three frequency bands have independent time and feedback. This is new functionality: M0 has filters in a shared feedback loop, not three independent delays. A compact Echo-owned space stage can follow the repeats or sit in the loop. Per-reflection glitch actions include fragment repetition, subdivision, reverse, tape stop and omission. A tail-only action route keeps the echoes readable while transforming their spatial tail.

The ordinary delay remains directly accessible. Detailed score, bands, space and glitch controls live in Advanced. Preserve Black Steel and the shared preset screen at stage coordinates (56,20,336,116); do not edit PresetScreen.*. Product text and documentation are English.

## Identity and Legacy

Preserve BBLB / HmEc / com.hamloprod.hamloecho, XML root HamloEcho, parameter IDs, order, normalisation, ranges, defaults, old choices, and host programs 0..7. Append new parameters after the existing bypass parameter, with new IDs. Default and old-state loading select Legacy and neutral new options. Loading an old state over an edited instance resets every new option. Project loading restores the complete state; preset Mix Lock applies only to preset loading.

M0 comprises Classic (2 s maximum) and Clean/Tape/BBD/Lo-Fi (4 s maximum), with different internal processing. Legacy retains both engines, their original 10 ms switching law, 20 ms input/node/bypass/mix ramps, dry-plus-wet law and exact arithmetic order. The Classic/Echo engine handover is host-block dependent: preserve that behaviour only in Legacy, and test equal schedules separately. New processing must be block invariant for events at identical sample positions. Dry and host latency remain zero samples; wet capture/pitch/reverse delays are shown separately and included in musical scheduling.

## Independent development and references

The specification follows the owner's musical examples, not a reconstruction of another product. We write our own source, routing, control mapping, visual layout and preset recipes. Shared DSP concepts are not claimed as novel inventions. Published references help identify measurable behaviours and quality problems:

| Reference | Published behaviour considered | Our decision |
|---|---|---|
| [Soundtoys Crystallizer](https://www.soundtoys.com/product/crystallizer/) | Pitch-shifted reverse echoes | Test pitch, reversal and capture delay independently; do not reproduce its controls or voicing |
| [Valhalla delay diffusion](https://valhalladsp.com/2019/06/13/valhalladelay-the-diffusion-section/) | Diffusion changes discrete echoes into a spatial texture | Give reflections an explicit space send and separate tail-only transformations |
| [FabFilter Timeless manual](https://www.fabfilter.com/downloads/pdf/help/fftimeless3-manual.pdf) | Feedback filtering and multitap editing | Use a musical note score with a declared source reference; evaluate feedback stability independently |

## Frozen methods and initial targets

Written before changing DSP or running the V2 lab. Tests/EchoLabMain.cpp is the common capture/verification tool. Its capture manifest pins the baseline executable, source revision, states, layout, schedules and raw float render hashes. Preserve the original executable and VST3 under out/m0. New-feature measurements report n/a on M0, never a fabricated pass. Extend the lab with each new measurement before implementing the measured feature; record and justify any subsequent methodological change in both this document and the release report, and rerun the original baseline wherever applicable.

| ID | Acceptance target and method |
|---|---|
| E01 | Legacy output is bit-identical to archived M0 for every captured case at identical sample rate, channel count, transport, state and block/event schedule; all saved old states load on fresh and already edited instances. Preserve the full host parameter contract and programs. |
| E02 | New modes are bit-identical at blocks 1/64/127/512/4096 after prepare(256), with events split at identical sample positions, including tempo, note, bypass and mode automation. Zero audio-thread allocations after preparation, including switching. |
| E03 | An unpitched, forward, non-diffused reflection starts within one sample of the requested free/synced time at 44.1/48/96 kHz. Wet processing accumulation is explicitly accounted for, not hidden by shifting the measurement window. |
| E04 | Fixed target notes and feedback pitch increments: <=5 cents on isolated 110/220/440/880 Hz tones for shifts -24..+24 semitones where output is 55..7040 Hz, measured after a 250 ms settling interval over >=500 ms. Report sidebands and transient performance separately; a dominant correct tone alone does not establish audio quality. |
| E05 | Reversing a captured non-palindromic ramp produces the expected reverse order within the documented overlap regions. Fixed-pitch playback preserves fragment duration within one sample; repeat/stutter counts and rhythmic boundaries are exact. |
| E06 | Neutral three-band recombination matches its declared phase-matched reconstruction reference with maximum absolute error <=1e-6. Measure crossover response, leakage and independent impulse timing; do not compare a phase-rotating crossover to unshifted dry as a false null test. |
| E07 | Freeze: energy drift <=1 dB over 60 s after settling; five-minute maximum-feedback stress has finite output, no unbounded growth, allocation, hang or runaway state. Release and mode switching remain controlled. |
| E08 | Width 0 gives identical wet channels. Report correlation and mono energy loss for each new factory recipe on appropriate stereo material; do not call intentional anti-phase content universally mono safe. |
| E09 | Zero dry/host latency at every supported sample rate and mode. Insert MIX=0 is exact dry; Send contains only wet, and returning to Insert restores its previous MIX. Mix Lock and full project state round-trip are tested separately. |
| E10 | At 48 kHz/256 on the owner's i5-10300H, mean processing time <=25% of block duration for one complete instance and <=50% during transitions. Report worst blocks, settings, CPU identity and contention; do not discard failed runs. Prototype eight pitch voices before finalising the implementation. |
| E11 | 200 new recipes: 4 genres x 50, permanent IDs, purpose, description, source-note assumptions, material, tempo and depth. Wet/dry full-render energy: Gentle [-24,-16), Medium [-16,-8), Heavy [-8,+6] dB. Independent Insert/Send renders, true peak <=-1 dBTP for prepared demos, finite output and state round-trip; recipes differing only by Mix/Output are duplicates. Calibration resumes by recipe/material/DSP identity and rejects stale DSP results. |
| E12 | Unit and compatibility checks pass; pluginval levels 5..10, Steinberg validator, main/Advanced/Legacy screenshots, FL Studio VST3 audition and prepared A/B examples. Installer VST3 must hash-identically match the validated binary. Report every unverified or failed requirement. |

Factory library allocation per genre: Vocal 10, Drums 12, Samples/Keys 10, Synth 8, Atmosphere/FX 10. Keep the seven original recipes and Default in their positions: 208 host programs total.

## Stages

1. Capture M0, host contract and baseline measurements; isolate the Legacy Echo processor and prove E01 without adding new sound.
2. Prototype score playback, pitch quality/cost and reverse capture; settle measured wet latency, buffers and feasible voice budget.
3. Implement new score/feedback/bands/space/glitch paths, state migration and DSP lab checks.
4. Build and inspect the UI and help, then author/calibrate/independently verify all 200 recipes.
5. Validate, audition, report, package 0.2.0, publish local SHA256 files and commit each coherent stage. No push.

No algorithmic goal is considered achieved merely because the implementation exists or a smoke test passes.
