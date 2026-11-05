# R14 drum-kit composition / targets before sound changes

2026-11-05,parentfe46e5c/R13. Owner still finds fills weak and wants sliced
kick/snare/hat plus a formula informed by E:/DRUMS/DRUMFILL,producing phrases
unlike the original loop. No subagents;work only this checkout. No installer,
installed replacement,HPEQ/subtree/push. English UI/docs.

R13 used one full-band attack bank and65%unfiltered grain. New processing uses
three band/time streams with independent onset detection:low kick,body/noise
snare,high hat. This is signal-based extraction,not perfect source separation.
It must not claim isolated overlapping instruments where the signal cannot
provide them. Raw full-band leakage is removed from the new renderer.

Preserve all old0..20recipes and Rise exactly. Append21AUTO COMPOSE and22..29
eight composition families;new UI Fill defaults to21. No old host parameter or
225program/state change. Kit role switches allow hearing kick/snare/hat alone.
Scratch options restore on editor reopen. Existing1/2/4beat scope,FX,export and
same-capture variation remain. All-zero macros keep source except end/protection.

Analyze tempo-labelled reference fills across the supplied790WAV inventory.
Publish source hashes,exclusion reasons,aggregate role-onset phase/count/build
statistics. Generate a small aggregate grammar header,not copied audio or
individual reference patterns. Both reference and source onset analysis are
band/time heuristics;they are not labelled instrument ground truth.

Formula for role r and position t in phrase progress x:

N_r=clamp(round(reference_median_r*length*density*family_weight_r),bounds_r).
w_r(t)=reference_phase_r(t)*build_r(x)*family_r(t)*syncopation_r(t)
       *(1-copy_penalty*input_onset_r(t))*spacing_r(t).

Draw role positions without replacement,insert coherent snare bursts/hat
ratchets/call-response/rest according to family,assign accented velocity and
source variation. Evaluate32seeded candidates using input-pattern dissimilarity,
build contrast,role balance and excess collision penalties. Return best valid
candidate;never substitute loudness or sub-sample timing jitter for new rhythm.
Consecutive Auto variations cycle all8families. Record similarity/kit counts.

Acceptance,do not relax after seeing results:
- Immutable R14 paired lab before/after;unavailable new functions n/a before.
  Old0..20andRise exact on6owner sources,plus225normalprogram/state comparisons.
- Default new Auto:64/64unique ordered rhythm/source plans and>=48/64distinct
  role-aware32cell rhythm masks per source. Each fixed family>=24/32masks.
- Default complete-bar fills use every detected available role;at least3distinct
  kick/snare/hat placements. No requirement to hallucinate missing instruments.
  Synthetic separated kit:3roles detected;role solos suppress other bands by
  >=18dB relative to their own passband at representative65/1200/10500Hz.
- On ordinary owner drum loops,>=90%of64default Auto plans/source have input
  role-mask cosine<=0.65;report dense/percussion/stress exceptions separately.
  Independent rendered band-onset similarity median reduces>=20%vsR13 across
  the same6-source,8-seed corpus. Report each source,not just global average.
- Preserve selected phrase prefix on prepared inputs;exact length,soft endpoint,
  silence/mono/nonfinite/hot/rates44.1/48/96kHz/tempi70..175/bars1/2/4 bounded.
  Independent8xprepared audio/demos<=-1dBTP. No gain-only audibility claim.
- Native coverage:real source reuse/variation,kit solos,scratch recall,file
  immutability,actual effects/controls,normal block/no-allocation checks.
- pluginval5..10,Steinberg537,review full/minimum/legacy UI images. Package exact
  validatedVST3/ZIP/SHA,report/README/AGENTS/handoff,local owned-stage commit.

Prepare several equal-RMS variants per owner source,with source and R13 for
comparison. Private source/reference audio stays outside Git/VSTZIP. Manual
FL/listening quality is for the owner;do not equate statistical goals with taste.

First rendered comparison:444old renders exact;all new structural/kit checks
pass,but independent source-similarity reduction only12.52%,below20%target.
Internal hard32cell scoring misclassified a few-ms shift across a cell boundary
as a different onset. Replace it with continuous96cell Gaussian20ms tolerance,
plus a shared-transient similarity penalty. Independent lab/analyzer and goal
remain unchanged. Retain first full analysis/renders. Kit muting was also
reviewed to preserve the same composed rhythm;it must not trigger a new draw.

Second paired run passes:36.26%independent similarity reduction. Final kit
review restricts coherent snare-roll anchors to hits with>=20%of the strongest
body energy,so quiet hat residue cannot anchor an entire snare roll. Ghost
one-shots retain source variety. Re-measure final sound after that change.
