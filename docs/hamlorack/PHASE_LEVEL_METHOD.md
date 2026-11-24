# Phase and level repair, method 1 (2026-11-24)

Fixed before candidate DSP changes. The owner permits changed sound in existing
projects and requests low latency; there is no legacy phase selector.

The same HPEQ_PhaseAudit executable source renders before and after. Neutral
frequency regions have a unity Gain processor. Test serial/parallel routing,
one/two differently bounded regions, layer weights 0/.25/.5/.75/1 and Stereo,
Left/Right/Mid/Side. Measure settled impulse-response magnitude at 20 Hz--20 kHz
(below Nyquist), against unity, with a 0.1 dB maximum tolerance. Report phase
separately: an all-pass reference is not an unchanged input. No-region identity
and settled plugin bypass must remain exact. The audit also renders a fixed
noise/percussive fixture and stores its output identity, peak and finite status.

Coefficient transitions, reset, disabled layers, block schedules, real-time
allocation, PDC and the existing suite remain required checks; a static
magnitude pass does not establish transition quality. Native musical effects
and EQ are not phase-inverted to make a neutral test pass.

Archive the old calibration/report/receipts and local reference binary before
replacing them. New DSP invalidates old bank calibration evidence. Reuse existing
bank peak/GR/loudness thresholds without relaxation. Use two independent local
recordings or explicit sample arrangements per category, with recorded hashes.
Before/after comparisons use the same new material. Different material from the
old PC must never be described as an improvement against its old LU figures.

Inspect HR019/059/074/174 first using cumulative chain outputs and module-only
controls; all have zero regions, so region phase is not their root cause.
Review every remaining loudness exception individually against intent/reject_if.
Keep private audio outside Git and export matched A/B with common headroom.
Owner audition remains pending until actually performed.

## Material extension 3 and test-contract change

Before the first new bank calibration, allow an explicit `ID-0.wav` / `ID-1.wav`
pair to override category fixtures. Both names and PCM identities are recorded.
HR019 uses shakers (the category's bongo loses 9.053 LU in its intended 950 Hz
low-cut alone); HR059 uses claps. HR074 and HR174 are Drum Bus, and retain full
drum material. An initially prepared HR074 open-hat selection was rejected before
calibration. This is a new local dataset, not a replay of the unavailable D: set.

The first interim M7 run used old reference equations: seven failures. Replace
that signal model with independent unity-region LayerEngine branches, retaining
the formula gate of -120 dB. Mute/solo formerly required bit identity to an
absent region; the new authorized behavior retains live crossover phase, so
those comparisons now use phase-only branches and the same -120 dB formula
gate. Empty/disabled project identity, block identity and bypass checks remain.

The first transition matrix failed eight block-size comparisons. A second
candidate fixed phase retirement but still delayed tree activation after reset;
the third establishes the tree immediately and splits disable/mode handovers at
the exact ramp endpoint. It passes all 36 transition/RT/wet-only assertions;
all eight block comparisons are sample-identical, not merely under tolerance.

Before the new bank calibration, author -1.05 dB on all 16 final Master recipe
ceilings. This carries the previous measured 0.05 dB safety repair into the raw
design; it does not relax the <= -1 dBTP independent-meter acceptance gate.
The Drum Bus limiter keeps its previous authored ceiling.

## Compressor search repair

First material-v3 batch: HR005 remained outside its unchanged 6--12 dB heavy
parallel band after six ratio-derived Newton steps (best 4.398444 dB at -19 dB
threshold). Archive the first batch and its executable; do not embed its failed
dynamics result. Before restarting the complete bank, replace that slope
assumption with a bracket between -60 and 0 dB, starting at the authored value,
and bisect toward more/less measured GR. Keep at most six measured attempts,
retain the best actually measured setting, and record every attempt. All target
bands, meters, renderers and numerical safety gates are unchanged. New records
must use one frozen executable and `bounded-bracket-v2`; no mixed-method merge.

The tool also prioritizes a valid measured point over a closer invalid point
when selecting the result (heavy target 8 has asymmetric bounds 6--12). This
does not change any already-passing frozen record: the search stops at its first
valid point. If a frozen record fails that selection, it requires an explicit
individual repair with preserved original identity and full remeasurement;
never change its old receipt's executable hash. Regression examples cover
11.9 (valid) versus 5.9 (invalid) at target 8.

## Region retirement and final-source verification

A further regression (neutral EQ keeps the layer active after its region is
disabled) reproduced six failures out of ten route/block combinations, maximum
error 0.004001714. Invalidate the previous block's phase program in update/reset;
only a subsequent actual split can publish a new program. Static authored
recipe graphs never retire their outer regions during these renders, so this
fix does not affect their calibration search or level/tempo matrix. Preserve
the frozen calibrator, its exact audio source snapshot and its original hashes.
Before embedding, re-render every canonical task output through the final DSP
and demand identical input/output PCM hashes. Reverify all 20 Power outputs too.
Any difference invalidates that record and requires remeasurement, not a waiver.

Calibration format 3 records a normalized build-time audio/calibration-source
identity, including native pins, isolation code and pinned JUCE version. Line
endings/BOM and absolute checkout paths do not change it. The runtime rejects
old or mismatched factory calibration. The aggregator requires one final-source
verifier for all 250 canonical proofs and authenticates each unchanged original
record by SHA-256. Run audition preparation before verification, since it adds
audition metadata to those records. Host project-state schema and parameter IDs
remain unchanged.

## Windows artifact version

The first 0.2.2 Release artifact reported 0.2.2 through VST3 but retained
0.2.0 in Windows file properties. JUCE 8.0.12's rcfile custom command does not
depend on its Info.txt input. Append that dependency in the rack CMake project,
then regenerate the resource and relink. This packaging-only correction leaves
the normalized DSP identity unchanged; verify both version representations and
run release validators on the final copied DLL, with its own SHA-256.

## Host program-name contract

The first complete Steinberg 3.8.0 extensive run passed 536/537 tests. The
remaining Scan Programs failure found a pre-existing contract bug: the processor
advertised one program but returned an empty name. Return the stable name
`Default` for that single editable host program; the 270-entry rack browser is
separate. Add a contract regression that every advertised program has a name.
No audio processing or host parameter/state layout changes.

The conservative source identity includes the processor header, so even this
metadata-only correction requires new canonical proofs and re-embedding against
the new identity. Archive the previous proofs, receipts, binary and validator
failure. Keep the original measured calibration records intact. Rebuild the
verifier, demand all 250 task and 20 Power canonical PCM hashes again, then
rebuild and qualify the actual new VST3. Never relabel the old executable hash.

The first final full-suite attempt also retained the preset-browser test's
0.2.1 recipe-byte hash. The browser catalogue's other checks passed; the exact
bank-hash assertion failed because this stage intentionally authored sixteen
Master ceilings at -1.05 dB. Update its pinned baseline from `c72f1d99...` to
the independently verified resource SHA-256 `4f0e1930...`. Keep the exact hash
assertion. Stop that early run, preserve its log, rebuild the test executable
only, and run the complete suite again. Audio source identity and the already
qualified VST3 remain unchanged by this test-fixture correction.
