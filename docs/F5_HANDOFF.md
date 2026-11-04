# HamloFader0.2.0 complete / 2026-11-04

Parent6455e9f(F4UI),completioncommit follows (git log-1). Owner requested full
completion and subsequently push. No subagents/model switch. Work on Fader
only;HPEQ subtree publication is separately authorized by the owner.

UI QUICK/ADVANCED/LEGACY;Trim/Match/Ride/Motion. PresetScreen/BlackSteel/old
panel andDSPkernel unchanged. Alignment andphase readouts0.001unit precision.
200genre recipes(4x50),208programs;old0..7Legacy remain. New factory metadata
has stableID,genre/group,note,material,depth,BPM,goal. Match loads held gain0,
Learn/Stop/Apply separate. Native added-controls load only for programs>=8.
Schema3/APVTS77/VST3identity/0samplePDC retained. Editor filtering is read-only;
page/state/modified status true. Current version0.2.0,Windowsresourcefixedtoo.

Accepted:17432finalnative/0,224unchangedF3Host,678exactF5Legacyrenders,
200calibrated+200compiled-check+200independent8xpeakchecks. Calibration/check
PCM200/200exact,workdelta0,worst8xpeak-3.906150dBTP. State/127block restore
exact onall200. HostmeanCPUmax1.721298%block. pluginval5..10/Steinberg537pass.
10deliverysnapshots reviewed;minimumsmalltext,prefer1180x700.12privateA/Bs
dry,0.4sgap,processed,commongain;exactpreparedinputproof;8xpeak<=-6.532854.

Calibration rawv1=198/200,v2=199,v3=199,v4=200;no threshold relaxation. BBHeavy
Keys strengthened Natural response;DnBHeavyKeys neededvisibleInput+12/Gain-12
andfloor+12 toextenddetectorrange on sparsekeys2. Netfixedgainunity. Full
disclosures/actualvalues:docs/reports/f5/REPORT.md. Reused onlyexactunchanged
perIDrecipe+passedresult withsameDSP/method/input;all200compiledrerendered anew.

ImmutableF5LibraryTests sourceSHA5c6efe4915d05ad932513409c3de6cb81ebf1c82d541badd26eaaa12d3070515.
BaselineactualF4binary+source:out/f5/Tests-before.exe,F5LibraryTests-before.cpp;
libraryn/a in docs/reports/f5/before.txt. F5Legacy ownprojected8program method
captured BEFOREF5:out/f5/legacy_reference/legacy_before.json;currentmethodsame.
Do not mutateF1..F4acceptance guards;theycertifyfrozencopies. F5guard iscurrent.

Reports:docs/reports/f5;authoritativenative_delivery,host_delivery,legacy_delivery,
check_final,analysis_final,calibration_v4,validators_delivery,screens_delivery,
demos_final. Source/tools/bundlefrozenout/f5/accepted. VST/ZIP/SHA256 indist/
HamloFader-0.2.0. Privateaudioout/f5/check_final/audio anddemos_finalignored.
No installer,installed replacement,manualFLlistening,fullEBUcertification or
universalpeaklimiterclaim. Ownerlisteningnext;normalbuildsneednoowneraudio.

Commands:
python -B -I -X utf8 Tools/verify_f5.py
cmake --build build --config Release --target HamloFader_Tests HamloFader_VST3 HamloFader_Snapshot HamloFader_F3Host HamloFader_F5Legacy -- /m:2 /v:minimal
build/Release/HamloFader_Tests.exe
build/Release/HamloFader_F3Host.exe <NEW.json>
build/Release/HamloFader_F5Legacy.exe verify E:/HamloFader/out/f5/legacy_reference <NEW.json>
Tools/validate_f5.ps1 -Label <UNUSED>

Materialheavycheck usesprocess-scopedenvironment(seeF5_PROGRESS historical
notes) andTestfilter'F5 preset library'. Freshoridentity-matchedresumeoutputonly.
Generatorapply:python -B -I Tools/fader_presets.py --calibration out/f5/calibration_v4/result.json
ItverifiesDSP/method/exactuncalibratedbank/sourceSHA;do notbypassfailure.
Privateauditionenvrequiresfreshfolder;don'toverwriteexportedfiles.

No active jobs after release. Publishverifiedcommit through existing HPEQ
plugins/hamlofader andfeature/hamlo-plugins;newownerpushrequest authorizesit.
Verifyexactsubtreetreeandremoteheads;keepmainHPEQworktreeunchanged. Medium
forroutinegit/build,xhighforDSP/metricinvestigation;no actualmodelchange.
