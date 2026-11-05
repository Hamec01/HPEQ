# R13 / HamloRevolution 0.3.1 / drum-fill slicer

2026-11-05. Parent8470eeb/R12,feature/transition-v2,local checkout
E:/HamloRevolutionTransitions. Owner requested less sparse,less repetitive fills
using E:/DRUMS/DRUMFILL. Original E:/HamloRevolution R11 draft remains parked.

New offline renderer only for Fill recipes8..20. It detects attack slices,
classifies broad low/body/air energy,and selects source fragments with energy
weighting. It does not separate overlapping instruments into stems. Twelve
phrase families alter timing,source order,accents,gaps,roll placement,triplets,
ratchets,reverse pickups and falling pitch. Rolls reuse a body-oriented source
with rising velocity. Selected earlier accents feed a damped room;FX also sweeps
the high-pass. The room send deliberately has time to develop before the ending.
Source boundaries use fades and band-limited resampling;the final rest is soft.

AUTO SLICER rotates twelve families across consecutive seeds. NEW VARIATION
keeps the capture and advances its seed;GENERATE captures and randomizes again.
EDIT LAST selects1/2/4beats;BARS still selects1/2/4bars of output. FILTER + ROOM
has0..100% in5% steps. Amber marks the edited ending. CHOP/ROLL/REVERSE/STOP stay.
Controls restore when reopening the editor in the same instance and survive
sample-toolbar regeneration. This is scratch state,not a new host-parameter bank.
WAV export/drag never overwrites a previously exported clip. Classic/R12 styles
remain in their submenu. Normal engine,225programs,host parameter IDs and state
schema are unchanged. RISE renderer/math are unchanged.

Reference principles:audio chopping,velocity development,triplets and section
endings from [Native Instruments](https://www.native-instruments.com/blogs/blog/drum-fills).
Own code/names/recipes;no reference-library audio embedded in the plugin.

## Paired measurement

Targets were written in docs/DRUM_FILL_R13.md before baseline. The same unchanged
Tests/DrumFillLab.cpp was built before and after;before reports new features n/a.
Original source/executable preserved locally in out/r13/DrumFillLab-before.*.
Before288rows,after384rows. Materials:790reference WAVs inventoried,32stratified
fills analyzed,0decode rejects. Six48kHz owner-source preparations:90BPMdrums,
90BPMtoms,120BPMdrums,140BPMdrums,170BPMdrums,and a170BPMfill stress input.
Full paths,original/prepared hashes and reference descriptors:materials.json.
These private WAVs remain outside Git and the portable ZIP.

| Check | Result |
|---|---|
| Old Rise/Fill0..7 plus previous Auto corpus |288/288PCMexact,including192fixed compatibility cases |
| New Auto rendered corpus |96/96finite;earlier bars exact on48two-bar renders |
| Auto structural diversity |64/64ordered timing/length/source plans and64/64rhythm masks on each of6sources;targets48/24 |
| Fixed new families |32/32plans;minimum31/32rhythm masks on each source;target16 |
| Prepared new audio,independent8xtruepeak |48/48pass;worst-2.528640dBTP,target<=-1 |
| Native suite |4897expectations,0failures |
| Existing programs + old state reloads |225/225load PCM,state-reload PCM and latency exact vsR12 |
| Normal audio blocks/allocation |1/64/127/512/4096exact,mono/stereo,zero measured C++audio allocation |
| VST3 validation |pluginval5..10exit0;Steinberg537pass,0fail |
| Reviewed images |Fill full/minimum,Classic Fill,Rise;no control overlap;preset screen retained |

New native fixtures exercise1/2/4beat boundaries,44.1/48/96kHz,70/175BPM,
1/2/4bars,determinism,mono/silence,hostile nonfinite input/settings,hot sources,
spectral roles,coherent roll ramps,actual control influence,pitch/reverse/rest,
room-accent placement,and real capture reuse after replacing live audio history.
All-zero macros keep source PCM apart from the5msending fade/peak protection.
Hot original input can require common attenuation;normalized earlier material
stays exact. Rendering allocates offline;the new slicer is not run in processBlock.

Previous225program load and restore comparisons use the same separate procedures
as R12. The17historical load-vs-restore differences within one version remain;
no new difference was introduced. See R12 report for that pre-existing behavior.

## Audible-shape diversity and limitations

Each row compares8seeds using the last-bar128cell RMS envelope and mean pairwise
1-cosine distance. It ignores overall gain. Higher means more different envelopes;
it is not a musical-quality score or a guarantee that every fill sounds better.
Timing/source-order-inclusive plan uniqueness is measured separately above.

| Source | R12 Auto | R13 Auto | Ratio |
|---|---:|---:|---:|
| break170,fill stress input |0.2532|0.3488|1.378 |
| drums120 |0.3659|0.4497|1.229 |
| drums140 |0.4253|0.4611|1.084 |
| drums170 |0.4404|0.3451|0.784 |
| drums90 |0.3484|0.4763|1.367 |
| toms90 |0.2936|0.5476|1.865 |

Five sources improve on this metric;dense drums170 remains21.6%lower. This is an
open result,not hidden by a global average or selected-seed demo. The new families
offer more kinds of gesture;they do not universally increase every diversity
metric. All original styles stay available for this reason too. Final judgement
requires owner listening in FL Studio;no manual FL approval is claimed here.

24private matched-level A/B files use seeds1..4on all6sources,not hand-selected
successful cases. Each file is R12,300mssilence,R13,both at the same RMS with a
shared true-peak-safe attenuation. Metadata records original loudness difference
in rms_difference_db and the common matched target in matched_rms_db. All demo
peaks<=-1dBTP. These files are for local comparison,not redistribution.

Offline corpus render times mean122.65ms,max441.20ms for1/2bars. Runs overlapped
native/compatibility checks,so these are observed render times,not an isolated
CPU benchmark. The synchronous offline render can briefly occupy the UI. Sample
loading uses host BPM and repeats the source;it does not detect or stretch tempo.
Match project BPM to the source. The generated ending and its room are trimmed
to the selected bar line. No claim of perfect instrument isolation or infinite
tail export is made.

## Retained first attempts

Initial material search used broad fragments and mislabeled extra non-drum loops.
It was corrected to explicit full-loop filenames before any baseline DSP run.
Unused initial preparation retained in out/r13/materials;materials_v2 is final.

First native slicer run123pass/2fail. One fixture mistakenly mixed non-roll accents
into its ramp test;actual roll events are now marked. A real bug let hot original
audio leak through the initial3mscrossfade when replacing a whole bar;source in
that crossfade now participates in protection. Original failure log retained.

First paired analysis regressed on drums140/drums170(0.833/0.730ratios). Actual
phrase gaps,weighted source choice and Auto-family rotation improved that to
1.084/0.784final. No threshold was relaxed. Intermediate analysis_first.json and
analysis_second.json retained,along with local raw renders. A later room review
moved the send off late roll ticks onto an earlier accent and lengthened decay;
final measured source/DSP is the one packaged. Independent analysis method and
before/after laboratory remained unchanged. The R12 TransitionLab is untouched.

## Delivery and replay

Portable bundle/ZIP/SHA256:dist/HamloRevolution-0.3.1-r13*. The exact DLL hash in
acceptance.json equals the validated build,delivery and frozen accepted copy.
PEFileVersion/ProductVersion0.3.1 verified;CMake now tracks generated Info.txt
for incremental resource-version updates. Source,binaries,methods and private
materials are pinned in the local acceptance receipt. Git preserves source and
non-audio reports. No installer,installed-copy overwrite,HPEQ/subtree or push.

Replay:python -B -I -X utf8 Tools/verify_r13.py. Re-render commands and continuation
state:docs/R13_HANDOFF.md. The owner can start with SIMPLE > FILL > AUTO SLICER,
one bar,NEW VARIATION,and audition the12STYLEfamilies on their own track.
