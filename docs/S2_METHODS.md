# S2 spatial prototypes — methods fixed before measurement

2026-10-31. Baseline:5ba304c (isolated M0). Prototype only: do not add host
parameters, change Legacy, rebuild/distribute the production VST3 or claim V2 is
ready. Listening reference:AKG K712, not a personalized HRTF. No subagents.

## Design under test

Predictable Mid/Side width0..4, three complementary spectral width bands, optional
fourth-order mono-bass high-pass on Side, and separately enabled energy-budget
guard with an explicit attenuation reading. Guard is off by default. New modes
must not inherit Legacy's block-dependent bypass/mix clocks.

Compare three mono-to-stereo methods: windowed FIR quadrature (Phase), difference
of short delays (Micro), and cascaded delay allpasses (Diffuse). Inject synthetic
Side while retaining the aligned source Mid. Mono fold conservation applies to
STEREO with Space off. Do not promise that mono fold preserves added spatial wet
energy: that is measured separately.

Three early spatial characters: Tight sparse reflections, Open broader multi-tap
field, Scatter reflections plus cascaded diffusers. No imported IR/HRTF or copied
commercial algorithm. Size, damping, depth, amount and explicit predelay are
independent prototype controls. This is not a complete reverb or binaural engine.

All modes have the same prototype reference delay D=2*round(32*fs/48000), giving
64samples at48kHz. It aligns the FIR quadrature generator with dry and existing
stereo. This is a prototype latency, not a new VST3 PDC decision. Parameter ramps
and transitions run per sample; histories allocate in prepare only.

## Acceptance, before the first run

| ID | Criterion | Measurement |
|---|---|---|
| P01 | Disabled/neutral is bit-identical to input delayed by D |44.1/48/96kHz; includes disabled extreme settings |
| P02 | Width2/4 changes Side/Mid by6.0206/12.0412dB, error<=0.02dB, guard off | Synthetic mixed stereo, discard200ms; original Mid unchanged |
| P03 | Width0 yields equal channels; stereo section's mono-fold error<=-130dBFS RMS | Complex stereo and all three mono generators, Space off |
| P04 | Every mono generator yields Side/Mid between-14and+2dB; normalized pairwise Side correlation magnitude<0.95 | Mono colored-noise/tone fixture, Amount0.75, discard200ms |
| P05 | Mono-bass150Hz attenuates60Hz Side at least30dB;1kHz loss<0.5dB | Steady antiphase tones, compare neutral reference |
| P06 | Muted low width band attenuates60Hz Side>=12dB;8kHz loss<0.5dB | Complementary split300/3000Hz |
| P07 | Guard off has unity intervention; guard on reaches requested stationary energy budget within0.5dB after500ms and meter reports actual reduction | Width4 mixed stereo; floor0and0.4; budget is not a universal short-window correlation guarantee |
| P08 | Reflection/predelay timing error<=1sample at all3rates | Isolated impulse after250ms; remove the direct sample before finding first wet arrival |
| P09 | Normalized spatial-tail pair correlation magnitude<0.9; centroid Open>=Tight+10ms and Scatter>=Open+10ms |48kHz impulse, Size1, same Amount/Depth/Damping; exclude direct |
| P10 | Spatial mono-energy change within+/-6dB on fixture; max-setting impulse tail below-100dB relative energy after3s; all samples finite | Colored mono/stereo and tail stress; report color/headroom separately |
| P11 | Bit-identical output for blocks1/64/127/512/4096 at identical macro/mode/bypass events | prepare256, fixed event positions; zero C++ audio allocations including first callback |
| P12 | Mean callback CPU<=25%of a48kHz/256frame period on current i5; no hidden peak normalization in raw measurements | Three controlled worst-case runs; retain means and largest wall-time outliers |
| P13 | Production unchanged | Existing115native expectations,160M0cases, original VST3 hash, source include closure and parameter/program contract |

One laboratory driver and one Python analysis method are used before and after.
The initial unimplemented prototype returns aligned dry; missing feature metrics
are n/a. Preserve that executable, source, manifest and results before DSP work.
Raw renders and source fingerprints are immutable per run. Resume only records
whose DSP/tool/input identities match. Any correction to methods/thresholds after
the first run must be disclosed here and in the final S2 report.

Prepare A/B listening examples on owner's vocal/keys/drumbus/synth material after
DSP tests. Scale each compared group by one disclosed common gain for<=-1dBTP
(8x reconstruction estimate), retaining raw peaks and samples. These are prototype
demonstrations, not200calibrated presets or owner listening acceptance.

## Primary references (mathematics, not commercial recipes)

- Julius O. Smith, window design of FIR Hilbert transformers:
  https://www.dsprelated.com/freebooks/sasp/Filtering_Windowing_Ideal_Hilbert_Transform.html
- Robert Bristow-Johnson/W3C biquad high-pass coefficient formulae:
  https://www.w3.org/TR/audio-eq-cookbook/
- M. R. Schroeder, allpass sections for dense colorless reverberation:
  https://aes.org/publications/elibrary-page/?id=465

Implementations, parameterization and tap structures here are our own. Timed
position paths, speaker/binaural rendering, selectable HRTFs, UI, presets and
shipping remain later stages.

## Disclosures after the first experiment

Candidate01 passed138checks and failed Width0 channel equality during the initial
30ms ramp from neutral. Corrected the implementation: settings loaded before the
first sample apply immediately; live edits still ramp per sample. Candidate02
passes139/139 with the same thresholds, signals, events and analysis windows.

Added a compiled DSP/tool/method fingerprint to the manifest after those runs,
so a run cannot be attributed to sources edited after its executable was built.
This adds provenance checking; it changes no numerical metric or stimulus. Repeat
the aligned-dry baseline and corrected candidate with this same final driver and
analysis method. Compare all repeated baseline PCM bytes to the original before
run. Preserve the first baseline/candidate reports and source snapshots; no
reference or failed result is overwritten.

The first provenance rerun exposed a build-system issue: restoring a file with
its older timestamp let MSBuild reuse the baseline object while recompiling the
driver. Its99-pass/9n-a result is an invalid mixed candidate, not DSP acceptance.
Preserve it as final_mixed.json. Fix restore timestamps and embed/check the same
fingerprint in both the DSP and driver translation units. Baseline analysis now
requires an explicit --baseline role; a candidate cannot silently accept n/a.
Repeat both roles with the final tool. No numeric threshold, signal, event,
window or DSP recipe changed in this provenance correction.

2026-11-02 routing audit: the 139-check candidate applied width, spectral width
and mono-bass only to the original Side, then added generated Side afterwards.
The existing Width0 test did not combine a generator with Width0. This violated
P03's intended scope. Before correcting DSP, add 15 jobs and 12 assertions:
all three generators at Width0 and Width2, and 60Hz mono generated Side with
mono-bass150Hz or the low width band muted, each against its unfiltered reference.
Keep all original 43 jobs and P02/P03/P05/P06 thresholds unchanged. Preserve the
failing combined-routing run. The corrected order is generation -> spectral
width -> global width -> mono-bass -> guard -> spatial field. Repeat the final
paired dry/candidate experiment and source-pinned demonstrations with this
extended method; compare the original 43 dry references byte-for-byte.
