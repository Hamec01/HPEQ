# R14 / HamloRevolution0.4.0 / drum-kit composer

2026-11-05,parentfe46e5c/R13,feature/transition-v2. Owner still found R13 weak
and requested independently sliced kick/snare/hat and a formula informed by
E:/DRUMS/DRUMFILL,producing rhythms unlike the input. Work stays in the clean
E:/HamloRevolutionTransitions checkout;original E:/HamloRevolution R11 is parked.

## What changed

R13 grains retained65%full-band source,so many rearrangements carried other
drums with the selected hit. New recipe21AUTO COMPOSE and22..29families use
separate filtered/time-sliced streams:30..260Hzkick,260..5000Hzsnare body,
5500Hzuphat,with fourth-order high/low-pass sections. Each has independent
onset detection and source-hit pools. This is band/time extraction,not perfect
semantic stem separation. Overlapping instruments can remain in the same band.
No external samples or model weights are added.

Kick,snare and hat receive independent new event schedules. Eight families:
SNARE TORRENT,KICK / SNARE,HAT WEAVE,ROLL / ANSWER,TRIPLET BREAK,HALF-TIME CUT,
CROSS RHYTHM,FINAL BURST. Auto advances through all8families with consecutive
NEW VARIATION seeds. Variants within a family redraw counts,placement,source,
velocity,reverse pickups and selected pitch falls. Snare bursts reuse a strong
body hit with a rising velocity ramp. Roll anchors must have>=20%of the maximum
body-hit energy,so quiet hat leakage cannot anchor the whole roll. Ghost
one-shots can still choose weaker hits. No full-band source is mixed into grains.

KICK/SNARE/HATswitches mute parts of the same chosen composition;soloing does not
redraw the rhythm. Slice counts show available source hits. Three coloured rows
show generated event positions. EDIT LAST1/2/4beats,BARS1/2/4,CHOP/ROLL/REVERSE/
STOP,FILTER + ROOM,capture/load,preview/export/drag remain. Controls are scratch
state restored on editor reopen and sample-toolbar regeneration,not new host
preset parameters. New Fill UI defaults toAUTO COMPOSE. Old R13 recipes8..20,
Classic/R12recipes0..7and Rise remain selectable and bit-exact. Normal225host
programs,state schema,parameter IDs and latency remain unchanged.

## Reference statistics and formula

790WAVs inventoried;597tempo-labelled references analyzed. Exclusions:109without
an unambiguous BPM label,41outside60..200BPM,43outside0.7..16bars. Analysis uses
the final bar and independent broad-band novelty detection;it is not labelled
kick/snare/hat ground truth. Median band-onset counts13/12/15;late-half fractions
0.4860/0.4554/0.4650. These numbers include mixed-instrument energy. The aggregate
phase probabilities are close to uniform,with strongest first-position accents;
references do not all use a uniform final-beat acceleration.

Tools/r14_reference.py records every used path/hash and exclusion reason in
reference_analysis.json. Generated DSP/DrumFillGrammarData.h contains only file
count,aggregate16phase probabilities,median counts and late fractions. No audio
or per-file rhythm template is embedded in the plugin. Reference analysis uses
third-order broad-band filters;the rendered kit uses stronger fourth-order
sections to reduce leakage. These related but distinct methods are disclosed.

For role r,phrase progress x,and tick t:

`N_r=clamp(round(median_r*phraseBeats/4*density_r*familyWeight_r*seedVariation),bounds_r)`

`w_r(t)=phase_r(t)*(.35+1.8*x^(.55+2*(1-lateFraction_r)))*family_r(t)*spacing_r(t)*(1-.97*inputAttack_r(t))`

The final anti-copy weight is floored at.025. Draw positions without replacement,
insert family-specific coherent rolls/rests,then evaluate32seeded candidates.
Score rewards role-aware dissimilarity,shared-transient dissimilarity and build
contrast,and penalizes excessive three-role collisions. Input/event masks use
continuous96cell Gaussian pulses with20mstolerance;small timing jitter is not
accepted as a new rhythm. Input similarity<=.65 receives priority. Source choice
and velocities do not contribute to rhythm-mask uniqueness.

## Fixed acceptance results

Targets preceded baseline in docs/DRUM_FILL_R14.md. Same unchanged
Tests/DrumComposeLab.cpp before/after;new composition reports n/a before.
Before444rows,after540rows. Unchanged6-source preparations from R13:four ordinary
90/120/140/170BPMdrum loops,90BPMtoms/percussion,and a170BPMready-fill stress input.
Source identities in materials.json. Raw source/reference audio stays local.

| Check | Final result |
|---|---|
| Old Rise0..7,Fill0..20and previous Auto corpus |444/444PCMexact,including348fixed compatibility cases |
| New Auto corpus |96/96finite;prefix exact on48two-bar prepared renders |
| Auto64seeds/source |64/64ordered plans,64/64role-aware32cell masks,64/64source orders on all6sources |
| Each fixed family32seeds/source |32/32plans,masks and source orders;target24 |
| Input role-pattern cosine<=.65 |64/64on each of6sources;ordinary-loop target>=58/64 |
| Independent rendered original-similarity median |0.408505R13 ->0.257857R14;36.8778%reduction,target>=20% |
| Prepared new audio,true peak8x |48/48pass;worst-1.799107dBTP,target<=-1 |
| Native suite |5336expectations,0failures |
| Normal program loads/old state reloads/latency |225/225exact vsR13 |
| Normal blocks/allocation |1/64/127/512/4096exact,mono/stereo;zero measured C++audio allocation |
| VST3 validators |pluginval5..10exit0;Steinberg537passed,0failed |
| UI review |Full/minimum composer and Classic images;preset position unchanged,no overlap |

Native tests cover representative65/1200/10500Hzband rejection>=18dB,three
detected roles,actual roll anchors,all controls/role solos,stable soloed rhythm,
64/32seed novelty,determinism,prefix/continuous partial boundary/zero endpoint,
mono/silence/nonfinite/hot input,rates44.1/48/96kHz,70/175BPM,1/2/4bars,real
capture reuse from previous R13 coverage,scratch recall and file immutability.
All-zero macros retain source except5msending/peak protection. A2msboundary
taper continues the last prefix value,not the original full-band slice. Hot
original input may require common attenuation;prepared prefixes remain exact.

Normal225program comparisons use the same separate load and post-playback state
restore procedures as R13. The17historical within-version load/restore differences
remain unchanged;see R12/R13reports. Existing live engine source is untouched.

## Independent audio comparison

Use independent SciPy band-onset detection,128cells and20msGaussian tolerance,
then cosine similarity to the original source. Overall gain is ignored. Eight
seeds per source,not selected successful examples. Lower means a more different
onset envelope;it is not a musical-quality or listening score.

| Source | R13 mean similarity | R14 mean similarity |
|---|---:|---:|
| break170,ready-fill stress input |0.66850|0.66891 |
| drums120 |0.32035|0.19508 |
| drums140 |0.48846|0.30635 |
| drums170 |0.44659|0.33602 |
| drums90 |0.37725|0.21655 |
| toms90,percussion |0.15253|0.08561 |

All4ordinary drum-loop sources improve. The ready-made fill stress input is
effectively unchanged and very slightly worse(0.000415);this is an open limitation,
not hidden by the aggregate median. There is no claim every possible input or
every individual seed sounds better. Role extraction remains heuristic.

24private A/B files use seeds1..4on all6sources. Order:source,300msgap,R13,
300msgap,R14,all at identical RMS with a common peak-safe gain. Worst demo peak
-1.200000dBTP. Private audio is excluded from Git,VST and portable ZIP. The owner
still needs to audition the release in FL Studio;manual listening is unverified.

Observed new1/2bar offline render time mean149.41ms,max379.64ms,under concurrent
verification load. This is not an isolated real-time CPU benchmark. Rendering
allocates offline and can briefly occupy the UI;the new composer is never called
in processBlock. Host BPM must match the source;sample loading repeats audio
without tempo detection/time stretching. Room and STOP end at the chosen bar line.

## Retained attempts and delivery

First independent measurement only12.52%similarity reduction,below20%goal,
despite passing plan uniqueness. Hard32cell internal comparison misread tiny
cross-boundary timing shifts as novelty. Replaced by continuous20mstolerance
and shared-transient penalty. Independent analyzer,paired lab and thresholds
were not changed. First analysis retained as analysis_first.json and local audio.
Second run reached36.26%. Final roll-anchor review excludes weak mid-band hat
residue;final rerender reaches36.88%. Solo switches were reviewed to mute the
same composition rather than redraw it. Intermediate analyses/raw renders and
pre-anchor native logs remain local. No acceptance threshold was relaxed.

Delivery:dist/HamloRevolution-0.4.0-r14/HamloRevolution.vst3 and portable ZIP/SHA256.
PEFileVersion/ProductVersion0.4.0verified. acceptance.jsonpins the validated DLL,
delivery,ZIP,frozen accepted source/executables/methods and private materials.
Replay:`python -B -I -X utf8 Tools/verify_r14.py`. Handoff:docs/R14_HANDOFF.md.
No installer,installed-copy replacement,HPEQ/subtree,push,subagents or model switch.
