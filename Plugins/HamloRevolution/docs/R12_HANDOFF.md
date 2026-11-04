# R12 completed / Revolution0.3.0 transitions

2026-11-04. Parent5e89bae. Completion commit:git log-1. Owner requested stronger,
less repetitive RISE/DRUM FILL using examples inE:/DRUMS. No subagents.

Worktree tool rejected sourceRoot registration;used a clean shared clone at
E:/HamloRevolutionTransitions,branchfeature/transition-v2. This is a feature
checkout of the same plugin,not an extra plugin. OriginalE:/HamloRevolution
retains parked uncommitted R11 work. New branch can be fetched locally into that
repo without touching its current branch/files. Do not blend R11 into this release.

Implemented source-onset bank,windowed-sinc grains,accent/pause/velocity/source
variation and6 families per kind inDSP/TransitionRecipes.cpp. HeaderSettings
appends recipe0Classic,1Auto,2..7fixed. Classic renderer/math untouched aside
from dispatch when recipe>0. Direct API defaultClassic;UI new generationAuto.
STYLE menu is below the title without moving PresetScreen. UI initialFill1bar,
Rise2bars;reopen restores previouskind/bars/knobs/recipe. SampleBar rerender
preserves recipe. Four old knobs remain. Offline rendering stays on control
thread (observed corpus median113.7ms/max315.3ms during concurrent checks).
8-phase full-band+raw peak meter,-1.5dB ceiling;source-end grains fade before
zero padding. Existing capture,preview,24-bit export,file immutability retained.

Corpus:28 references,0rejected;three prepared owner inputs (DECAPdrums120,
BoomBapjazz toms90,MusicRadaracoustic fill170). Source paths/SHA/features in
docs/reports/r12/materials.json;prepared audio inout/r12/materials. Originals
read only;not embedded in plugin. Do not commit raw user/reference audio.

Final native4769/0,96Classic baseline PCM matches,96modern finite corpus renders,
48/48Risegrowth>=6dB(min10.35dB),12/12Risebrightness>=1.15,24demo raw8xTP<=-1
(worst-3.14dBTP). Auto16/16unique timing/source/direction/length plans;fixed8/8.
OldRise2unique clips per8seeds on eachinput/length,now8. Envelope diversity
usually improves,but120BPM2barFill edited-envelope cosine is.754 vsClassic.720;
don't claim every statistical similarity measure is better or a100/100 rating.
225normalprogram loads+225old-state reloads match R10 independently. Legacy17
within-version load/restore differences remain disclosed,not a new regression.
pluginval5-10/Steinberg537 pass on delivery DLL. Snapshotsfinal inspected.

12matched A/B comparisons:before,0.4sgap,after,sameRMS,independent8x peak<=-1.1.
Gain factors/identities inAudio/comparisons.json. Private user-material examples,
not factory samples. No manual FL/OS drag claim. No installer,installed-copy
replacement,HPEQ/subtree/push or model switch. DeliveryVST/ZIP/SHA256 in dist.
ReplayTools/verify_r12.py against acceptance.json;requires local ignored out/dist.
Normal builds/native tests don't require the user's E:/DRUMS folder.

Authoritative:before.json,after_final.json,analysis_final.json,native_final.txt,
playback_final.json,validators_final,screens/*_final.png. Others are intermediates.
Immutable TransitionLabhashbc31e4416511131098e451be213fefd494455cb740c4bc7ca8a90e05c7dacbdb.
First STOP fixture incorrectly used.3probability and expected an event;fixed
test0vs1,original binary/log retained. First analysis included unchanged Fill
prefix;kept it and added edited-region/Classic comparisons. Peak/source-end
review corrections disclosed;no threshold loosened. Baselineout/r12/baseline,
finalout/r12/accepted. Reference-analysis Python3.11.9,numpy2.4.6,scipy1.17.1.

Commands:
cmake --build build --config Release --target HamloRevolution_Tests HamloRevolution_VST3 HamloRevolution_Snapshot HamloRevolution_TransitionLab HamloRevolution_SpatialLab -- /m:2 /v:minimal
build/Release/HamloRevolution_Tests.exe
build/Release/HamloRevolution_TransitionLab.exe <materials.json> <NEW-FOLDER>
E:/HamloShirota/build/s3_venv/Scripts/python.exe -B -I -X utf8 Tools/analyze_transitions.py out/r12/before <AFTER-FOLDER> <NEW-REPORT.json>
Tools/validate_r12.ps1 -Label <UNUSED-LABEL>
python -I -X utf8 Tools/verify_r12.py

Next:owner listens to A/B and loads new0.3.0 inFL;only then change requested
musical behavior. No active jobs after completion. Preferred Astraxhigh for
algorithm/listening diagnosis;medium for known checks. No switch performed.
