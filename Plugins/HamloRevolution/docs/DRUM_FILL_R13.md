# R13 drum-fill slicing / targets before implementation,2026-11-05

Owner says current fills rearrange too little and repeat similar phrases. Work
only in this clean feature checkout;parent8470eeb/R12. Original Revolution R11
draft remains parked. No subagents,installer,installed overwrite,HPEQ/push.

Reference folderE:/DRUMS/DRUMFILL contains790WAVs. Analyze a stratified subset
of acoustic,kick/snare,roll,glitch and Axel fills across tempi;use as listening/
feature references,not embedded factory audio. Compare on genuine owner loops.
Published musical principles,not copied code/presets:
https://www.native-instruments.com/blogs/blog/drum-fills
Own implementation/names. The article discusses rolls,velocity development,
triplets,groove and phrase endings. Audio slicing cannot perfectly separate
overlapping drums;role analysis is a low/body/air heuristic,not stem extraction.

Append new Fill recipes8Auto and9..20twelve phrase families. Preserve recipe0
Classic andR12recipes1..7,both Rise and Fill,exactly. Existing API default0 and
normal225program/state identity unchanged. New UI Fill defaults to new Auto;
Rise remainsR12. Old recipes stay selectable. No new live host parameter ABI.

New slicer:detect onsets plus grid slices,analyze spectral roles,choose phrases
with actual timing/rest/source-order changes. Use accents,ghosts,roll ramps,
kick displacement,tom-like pitch falls,reverse pickups and distinct endings.
Slice boundaries fade;resampling band-limited. No extra sample library.
New controls:last1/2/4beats(quarter/half/full4/4bar),FXamount and NEWVARIATION
using identical capture. FX adds rising high-pass and selected-hit reverb,not
whole-loop dry attenuation. Four existing macros remain;all-zero is neutral
apart from documented ending/peak protection. Save scratch controls on reopen
and preserve them on sample-workspace regeneration/export. Old files immutable.

Acceptance:
- Same lab source before/after;new capability reportsn/a before it exists.
  Preserve frozen R12 binary/source/state/report references.
- Old0..7recipes andRise PCMexact on corpus;225normalprogram/state comparisons
  exact. Source/DSP/HamloRevolution normal engine unchanged.
- New Auto:>=48unique timing/source/length plans per64seeds and>=24distinct
  quantized32cell onset masks. Each fixed new family>=16masks per32seeds on a
  source with multiple attacks. Ignore pitch,pan,gain,FX when counting rhythms.
- Record edited-region onset-envelope/cosine/source-order diversity vsR12;
  show any regression honestly. Statistics do not certify musical taste.
- New phrase spans last1/2/4beats only;earlier material unchanged on normalized
  inputs except documented peak protection. Full output length remains exact.
- Roll repeats coherent body slices and develops velocity;reverse pickups,
  pitch falls,rests andFX must actually affect their selected fixtures.
  FX0 is dry processing,no hidden reverb. FX must be measurable/audible.
- Determinism,finite,silence,mono,44.1/48/96kHz,70..175BPM,1/2/4bars. Bad
  numbers sanitized. Zero endpoint and soft boundaries. Prepared clips/demos
  independent8xtruepeak<=-1dBTP. No gain-only improvement claim.
- Meaningful UI actions/scratch recall/source reuse and exported-file immutability.
  Native/normal audio allocation+block checks,pluginval5..10,Steinberg537,
  reviewed snapshots,newVSTZIP/SHA with exact validated binary.

Matched-level A/Bs on owner loops include several new variants per source,
not a single selected seed. Keep private audio out of Git/VST ZIP. Report actual
results,failed attempts,render time and manualFL/listening limitations.

Initial preparation matched broad filename fragments and incorrectly labeled
extra sources(including bass/melodic loops). Retained asmaterials(initial),not
used for any audio baseline/acceptance. Corrected to explicit full-drum-loop
filenames with stated90/140/170BPM before the first DSP measurement;materials_v2
is authoritative. The inheritedbreak170clip is explicitly a fill stress input.

First native slicer run:123 pass,2 failures. The roll fixture incorrectly
included loud non-roll accents sharing its source; mark actual roll events and
test their ramp. The hot-source test exposed a real protection bug:when the
whole bar was replaced,the initial3ms crossfade reintroduced unprotected input.
Include that crossfade source in peak metering. Keep original failed evidence.

First paired analysis:all288existing renders exact;48new WAVs<=-1dBTP.
Envelope diversity improved on4/6sources,but regressed on drums140/drums170
(ratios0.833/0.730). Do not hide this initial result. Revise actual phrase gaps,
avoid weak-slice uniform selection,and rotate Auto families through consecutive
seeds. NEW VARIATION advances that seed while reusing capture;full Generate
starts a fresh randomized seed. Re-measure without changing analysis thresholds.
