# Revolution0.3.0 / R12: source-aware RISE and DRUM FILL

2026-11-04,parent5e89bae. Owner rated previous generation10/100 and requested
improvement against E:/DRUMS examples. Design/targets:docs/TRANSITIONS_V2.md.

## What changed

Analyzed28 local references:16 drum fills at95/120/140/170BPM and12 riser/FX
examples. Source paths,hashes,onset contours,energy and brightness:materials.json.
Some reference files include an arrival or outro; their whole-file energy slope
is not mislabelled as a pure rising-envelope target. Originals are read only.
Three actual owner inputs were prepared at48kHz,-18dBFS RMS subject to-3dBFS peak:
DECAP drum loop120BPM,BoomBap jazz toms90BPM,MusicRadar acoustic fill170BPM.
These are measurement inputs,not factory samples embedded in the plugin.

The old Rise varied mainly two capture/acceleration choices. Old Fill had three
block orders and a nearly fixed ending roll. New recipes select actual source
attacks,bandlimit their resampling,shape accents/ghosts/pauses and vary phrase
structure and pickup sources. RISE families:LIFT,PULSE,SWELL,RUSH,ARC,PRESSURE.
DRUM FILL families:POCKET,ROLL,BREAK,TRIPLETS,SCATTER,TURNAROUND. AUTO picks a
family by seed;GENERATE varies timing and sources inside it. A STYLE menu exposes
all families plus exact CLASSIC. Existing four knobs and1/2/4 bars remain.
ROLL now overrides final-beat density even at zero,and its acceleration varies.
New Fill opens at1 bar on first use;Rise at2. Explicit previous bar choices stay.

Rise develops density,pitch,filter brightness and a reverse spatial layer toward
the downbeat. Fill edits the final bar with source attacks and purposeful rests;
earlier bars retain the source. Endings include reverse pickups and tape stops.
Every grain fades within the actual available source as well as its destination.
An8-phase full-band peak meter includes raw samples and a-1.5dB ceiling,checked
independently at8x. No upward normalization substitutes for recipe differences.

Only explicit transition generation uses the new choice. Existing old render API
defaults to Classic;old normal playback,225 programs,state IDs and automation are
unchanged. No new audio-thread processing/allocations. Existing load,capture,
preview,WAV export and drag are retained;bar re-render preserves the selected
recipe. Reopening the editor restores the correct Rise/Fill/recipe/knob display.
Shared PresetScreen unchanged. Render remains the historical offline control-
thread operation;do not claim a new background worker or instantaneous rendering.

## Evidence

| Check | Final result |
|---|---|
| Classic corpus compatibility |96/96 exact PCM |
| New source corpus |96/96 finite |
| Rise energy growth |48/48 >=6dB;minimum10.35dB |
| Rise brightness |12/12 >=15% centroid increase |
| Independent8x demo peaks |24/24 <=-1dBTP;worst-3.14dBTP |
| Structural diversity |Auto16/16;each fixed family8/8 |
| Normal program/state compatibility |225/225 loads and225/225 reloads exact |
| Native suite |4769passed/0failed |
| Offline corpus render time |median113.7ms;max315.3ms under concurrent checks |
| Validators |pluginval5-10;Steinberg537/537 |

Immutable TransitionLab SHA256:
bc31e4416511131098e451be213fefd494455cb740c4bc7ca8a90e05c7dacbdb.
96 Classic corpus renders match the pre-change hashes exactly.96 modern renders
cover three inputs,both kinds,one/two bars,eight seeds. All finite;Rise growth
and independent peak/brightness results are in analysis_final.json.225 normal
program loads and225 old-state reloads match R10 independently. The17 historical
within-version load-versus-restore differences from R10 are not redefined away.

Structural tests exclude gain,pan and pitch-only changes:Auto gives16 distinct
timing/source/length/direction plans in16 seeds for both kinds;each fixed family
gives8 in8. On these three inputs,old Rise gave2 unique PCM clips per8 seeds,
new Rise gives8. Envelope comparisons generally vary more,though not every
input/length has a lower similarity:the120BPM two-bar Fill can have higher
edited-envelope correlation than Classic. This metric alone is not musical
quality. Full-clip Fill similarity also includes deliberately unchanged bars.

Final native4769/0 covers exact lengths at44.1/48/96kHz,tempos70/95/120/140/175,
bars1/2/4,finite/silent/mono sources,soft endpoints,repeat determinism,control
ranges,unchanged earlier Fill bars,near-Nyquist/impulse headroom,UI recipe wiring,
reopened-editor state and immutable old exported files. Existing audio allocation,
block-size and normal-playback tests pass. Pluginval5-10 and Steinberg537/537
pass on the exact delivered DLL. Final snapshots reviewed at opening/minimum size.
Minimum-size text remains small;opening size is recommended.

Twelve private A/B comparisons use BEFORE,0.4s gap,AFTER at the same RMS with
independent8x peak control. Uniform gains are recorded;no timing/timbre edits
were added to auditions. Original unscaled renders remain in out/r12.
These measurements establish function,development and variety,not a100/100
subjective score. Owner listening and actual cross-app FL/Explorer drag remain
unverified. The plugin contains no reference-library audio.

## Intermediate corrections and scope

Initial control fixture used default STOP.3 and expected an effect on one seed;
it failed because no stop was selected. Corrected the fixture to0 vs1. Initial
binary/log retained. No target weakened. Initial diversity analysis measured
the whole Fill including unchanged earlier bars;retained it and added edited-
region/Classic comparisons. Release review corrected metering from the grain
reader's.94 cutoff/four phases to full-band eight phases plus raw peaks,with hot
input regressions,and bounded grains before zero-padding. Original passing
prepared-material results are retained as intermediates.

Work is isolated in E:/HamloRevolutionTransitions,feature/transition-v2. Managed
worktree creation rejected this chat's workspace registration,so a clean shared
local clone was used. The original E:/HamloRevolution parked R11 work was not
mixed into this release. No HamloVerb,HPEQ,installer,installed replacement or push.
Replay Tools/verify_r12.py. acceptance.json pins sources,methods,inputs,reports,
tested binaries,package and demonstrations. Prior R9/R10 receipts belong to
their frozen source snapshots,not this modified source.
