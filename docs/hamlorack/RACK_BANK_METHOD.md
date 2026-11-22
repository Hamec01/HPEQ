# Rack bank laboratory: method v1

Declared before the first calibration run, 2026-11-22. The acceptance thresholds
in RACK0 A4-A8 are unchanged. This is a reproducible engineering calibration;
owner material, individual musical-intent verdicts and FL listening remain owner
checks. Synthetic vocal and instrument signals are not recordings of performers.

`HPEQ_RackBank calibrate <absolute output directory> <zero-based first> <count>`
renders the complete processor at 48 kHz, stereo, prepare(256), host blocks 127.
The category-specific deterministic fixture has two independently varied seeds,
pitches and envelopes, lasts at least two bars / three seconds, and is capped at
-9 dBFS sample peak before the -6/0/+6 dB ladder. Record achieved RMS, LUFS and
peaks; sparse material may not attain its nominal -18 dBFS (-14 bus) RMS target.
Transport supplies reference BPM and PPQ. The independent second fixture also
runs at 80% and 120% tempo. PDC is removed by the processor's reported latency.

Measurements: the separate BS.1770 K-weighting/gated loudness meter, sample peak,
independent JUCE FIR 8x true peak, RMS, side RMS and L/R correlation. Native Comp
and RedWall GR are captured after every callback. The native true-peak display
is not the reference for the -1 dBTP ceiling. Audio callback allocation/parity,
other sample rates, mono and block sizes are covered by the unit-test matrix.

For each compressor, tune its threshold on the canonical fixture, at most six
bounded iterations, retaining the best result. Heavy parallel (amount < .75,
ratio >= 4) targets 8 dB / acceptable 6-12 dB GR; ratio >= 3 targets 4.5 / 3-6;
the remaining gentle settings target 2 / 1-3. Report any failure individually.
These are canonical-clip targets; all ladder GR measurements are retained.
Fader Match uses the real Learn/Stop/Apply controls and stores measured matchedDb.

Track output trim aims at +/-0.5 LU on the canonical clip, constrained by <=3 dB
peak growth across both fixtures and all three levels. Use .001 dB calculation
headroom for float rounding; the verification threshold stays exactly 3 dB.
Wet-only returns retain their authored amount and report loudness separately.
Master trim is before the final RedWall, never after it, and gives priority to
<=3 dB limiter GR at the hot level over a loudness increase. Master ceiling stays
<=-1 dBTP. Explicit loudness/GR exceptions remain visible; they do not become
passes through a bank-wide waiver. No automated score certifies musical intent.

Each record contains definition/executable hashes, initial and final metrics,
parameter overrides, all verification results and a separate pending owner
verdict. Local dry, processed and loudness-matched WAVs support audition. The
tracked aggregate contains the numerical receipts and overrides, not audio.
Never reuse records from a different executable or definition hash. Recompile
with the final embedded calibration and verify it before reporting completion.

## Material extension v2

First: one synthetic HR001 spot run used the v1 fixture generator. After the
owner authorized the entire D: disk, actual isolated/source-separated recordings,
sample libraries and two mix sources were found. No acceptance threshold changed.
Use a fresh output folder, not cached synthetic records. `prepare_material.py`
reads an explicit private manifest, resamples with polyphase FIR, places authored
sample onsets, applies 10 ms edge fades and writes eight-second 48 kHz stereo
float fixtures. Original files remain unchanged. Exactly two distinct source
recordings/arrangements per category are required; a premixed source is used
only for mix/master/sample/utility tasks. Source-separated stems and single-note
sample arrangements are labeled as such; they are not owner-recorded raw stems.

Pass that fixture folder as the final argument to `HPEQ_RackBank calibrate`.
Its `sources.json` stores private paths, source and fixture SHA-256 and selection
offsets. Numerical receipts contain its hash and per-render PCM hashes. Input
normalization, -6/0/+6 ladder, thresholds and calibration algorithm are unchanged.
The independent 80/120% host-tempo test uses the same audio at its recorded speed:
it tests effect timing/stability, not time-stretching or genre/style suitability.

The full-band mono fold is RMS((L+R)/2) relative to stereo RMS. Additional low
band metrics use two cascaded 120 Hz one-pole low-passes, then the same RMS fold
and correlation calculation. These are diagnostics, not an invented universal
stereo-width pass threshold. Measurements and audition files cover the active
eight-second window; long native send retirement is checked separately by the
20-second bypass test. They do not certify every complete preset tail or owner
musical-intent/listening gate.

## Individual ceiling repairs

First: HR048's independent 8x hot-clip peak was -0.999579735 dBTP with an authored
-1 dB ceiling; the strict <=-1 gate correctly failed. Do not round it into a pass.
Correct failing master recipes with a small ceiling margin, re-render their full
level/clip/tempo matrix and retain original receipts. The measurement threshold
and independent meter stay unchanged. Repair-tool executable hashes are recorded
explicitly; a final exact replay through the embedded bank validates all 250
canonical outputs after rebuilding. No audio-processing code changes during
the calibration/recipe-repair phase.

The host tail metadata cache was updated during the final source pass; it does
not change sample arithmetic. All 250 canonical outputs were then replayed
exactly through the rebuilt embedded bank. Five ceiling repairs used a separate
executable sharing the original renderer/meters; the aggregator verifies each
immutable original receipt and that only the final ceiling override changed.

Audition export initially appended a second RIFF when a repair replaced a WAV.
The writer now truncates before replacement. The ten affected derivative files
were recovered losslessly from their final RIFF segments, after verifying the
preserved original prefix, declared length, 48 kHz/stereo format and frame count.
Numeric render hashes/metrics were unaffected. Final matched A/B pairs use
`prepare_auditions.py`: match the measured BS.1770 difference, then apply common
attenuation to both reference and processed audio for >=1.5 dB measured 8x peak
headroom. Compare `ID-matched-dry.wav` with `ID-matched.wav`; calibrated dry and
processed WAVs are retained separately. Original audio is never overwritten.
