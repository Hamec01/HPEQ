# HamloFader0.2.0 / F5 release

2026-11-04. Parent6455e9f(F4 interface); earlier accepted host3223d0d.
Completed QUICK/ADVANCED/LEGACY and200genre recipes/208host programs. No audio
kernel changes. Original0..7programs/IDs/ranges/order/state root/VST3identity and
neutral modern construction retained. Shared BlackSteel/PresetScreen/Legacy
panel unchanged. Genre/group metadata is in the existing preset screen.

## Delivered behavior

Trim:input/manual/output gain,channel routing/polarity/trim and alignment.
Match:RMS/LUFS learning,explicit Stop/proposal and Apply,visible limits and held
gain. Ride:Natural/Fast,strength/range/activity/timing. Motion:custom2..32points,
Sine/Triangle,depth,Sync/Free,phase/right offset and exact numeric point editing.
Channel alignment/phase now display0.001unit precision. Original Legacy UI is
available separately. Opening/filtering the editor does not edit the project.

RMS/LUFS M/S/I readiness,SP/TP and integrating-full state are explicit.72000
integration windows cap storage;Reset clears it. Peak Guard limits a Match
proposal,not arbitrary output. Meter is not a certified complete EBU Mode.

## Library acceptance

Four genres x50:Boom Bap90,Trap140,Techno132,DnB174BPM. Per genre11Match,
19Ride,20Motion. Old seven utility Trim programs and Default retain positions.
Stable IDs,descriptions,source,tempo,group and depth provided. Effective recipe
signatures exclude changes only in MIX/Output.208program count and metadata/
group distributions/uniqueness/ranges/neutral held Match are tested natively.

Owner material:strongest6s windows,resampled48k,-18dBFS RMS subject to-9dBFS
peak. Common9dB headroom accommodates Ride's up-to6dB boost;no output pad or
limiter disguises work. Work is mean absolute20ms window input/output energy
level difference after0.5s for inputRMS>-60dBFS. Fixed bands:Gentle1..3dB,
Medium3..6dB,Heavy6..12dB. Match is actually learned/stopped/applied before
measurement;load-time matchedDb stays0. Ride target and Motion depth calibrated.

| Check | Result |
|---|---:|
| Calibrated recipes | 200/200 |
| Independently rerendered compiled bank | 200/200 |
| Calibration versus compiled PCM | 200/200 exact |
| Maximum calibration/check work difference | 0dB |
| Measured work range | 1.999920..9.424999dB |
| Finite/state restore/block127 equivalence/zero latency/workflow | 200/200 |
| Independent exported-WAV8x TP<=-1dBTP | 200/200 |
| Worst independent8x peak | -3.906150dBTP |

Some bounded recipes reach their assigned band without reaching its preferred
midpoint. For example BBHeavyKeys finishes6.012125dB against desired9.025dB,
inside the fixed6..12band. Do not call every desired midpoint exact or promise
the same depth/peak on another recording. [Measured index](../../GENRE_PRESETS.md).

## Failed runs and recipe corrections

Calibrationv1:198/200. BBJazzyKeysLevel andDNBLiquidKeysLevel Heavy recipes
reached best work just below6dB(5.999980/5.999999998),not in the Heavy band.
The sparse keys2 preparation is-30.191460dBFS RMS due its crest factor. Neither
an epsilon nor band change was used. v2 strengthened Natural strength/timing:
BBpassed6.012125dB;DnBstillfailed. v3Fast timing/detector stillfailed.

Final DnB recipe stages Input+12/Gain-12,with activity floor also raised12dB,
extending the useful detector target range while leaving net fixed gain unity.
The visible controls and preset description explain this. Same goal9.325dB,
same6..12band,maxBoost6/maxCut24. Final work9.324918dB,peak4x-26.171967dBTP.
No DSP change or hidden Output/MIX boost. All four runs/raw logs are retained.

Resuming v2/v3/v4 copied only198/199/199 passed records whose complete Python
recipe dictionaries matched exactly. Changed/failed recipes were measured anew.
DSP/method/input identities match across runs;only recipe skeleton changes.
Migration records and uncalibrated banks retained. Generator rejects another
DSP/method/material/recipe bank. Final compiled check rerendered all200 without
using migrated audio results;all PCM matches calibration exactly.

## Compatibility and interface

Final native17432expectations/0fail. Prior full native17492/0 included60additional
private-audition checks before the final readout-precision refinement;private
auditions stay valid because the audio kernel/recipes did not change.
Unchanged F3Host224/0,including exact callback/event/oracle/allocation checks.
F5Legacy uses the original226cases x3restore procedures:678exact render/state/
meter comparisons. Its contract explicitly projects original0..7programs while
allowing200additional programs. Captured before library implementation with
the same F5method. Original F3method/receipts were not rewritten.

F5LibraryTests.cpp before/after SHA256:
5c6efe4915d05ad932513409c3de6cb81ebf1c82d541badd26eaaa12d3070515.
F4 baseline reports library n/a;actual F4binary and immutable source retained.
DSP files match frozen F4/F3;older281DSP/59kernel acoustic/unit proofs remain
historical,not claimed as new runs. Final wholeprocessor mean CPU1.721298% of
48k/256block at worst in host lab,below25%;not a manual FL CPU certification.

Final pluginval5..10 exit0;Steinberg537passed/0failed on deliveryDLL. Ten delivery
snapshots reviewed:fourQuick,threeAdvanced,channeltools,Legacy,minimum590x350.
No overlaps found;minimum text is small,1180x700recommended. Full descriptions
are also in the JSON bank;the shared preset screen keeps its compact note line.

Incremental version bump initially left Windows RCmetadata at0.1.0 although
VST metadata was0.2.0. JUCE's RCcustomrule omitted Info.txt as a dependency.
Local CMake rule now appends it;rebuilt PEFile/ProductVersion both0.2.0 before
final validation. No JUCE source edit or audio change. Earlier validated image
retained as validators_final;authoritative delivery is validators_delivery.
Final numeric precision change also triggered fresh native/host/Legacy checks.

## Delivery and remaining manual work

Portable VST3/ZIP/SHA256:dist/HamloFader-0.2.0. Package DLL equals validated
build DLL;ZIP decoded contents/CRC verified. Documentation and JUCE license
included. No installer or installed-copy overwrite.12private owner-material
auditions cover4genres x3roles,exact reconstructed prepared input,one common
gain per dry/processed pair. All independent8x peaks<=-6.532854dBTP. Audio is
local inout/f5/demos_final,not factory content,not in Git or the portable ZIP.

Owner FLStudio loading/listening remains unverified. No subjective listening
success,universal peak bound or new full acoustic/CPU certification claimed.
Owner authorized GitHub publication after verification;subtree/push evidence
belongs to the HPEQ integration handoff. No main-HPEQ DSP rewiring.

Authoritative:check_final.json,analysis_final.json,native_delivery.txt,
host_delivery.json,legacy_delivery.json,calibration_v4,validators_delivery,
screens_delivery,demos_final.json andacceptance.json. Frozen source/bundle/tools:
out/f5/accepted. Earlier failed/intermediate runs remain. ReplayTools/verify_f5.py.
