# Revolution R9 complete / owner check next

2026-11-03, parent94051dd, completion commit follows (git log-1). No active jobs.
Work only E:/HamloRevolution, efficient-staged-work, no subagents/other products,
HPEQ/HPDG/installed-copy/installer/push/subtree/model switch. Owner asked for
weak-effect audit, main sample load/export/drag. Those are implemented.

0.2.1 delivery in dist/HamloRevolution-0.2.1-r9/HamloRevolution.vst3 and ZIP/SHA.
Packaged DLL equals pluginval5..10/Steinberg537 tested DLL. Report/receipt:
docs/reports/r9/REPORT.md and acceptance.json; replay Tools/verify_r9.py.
1286native=1096old+190new. Before_v2/after_release8rows exact, Core/DSP/Link
sources/generator unchanged. BOLD is explicit: genre/seed preserved, FULL,
Chaos,Amount/Mix100%,Variation65%,FillEvery1,Unlock,generate. No hidden gainstage.
TOPS80Hz gain spread0.179dB vsFULL6.811; initialdelta-onlymeasure was phase-confounded,
corrected/repeated before production changes; original before.json retained.

Main toolbar:LOAD/DROP ->PLAY/LOOP actual chain withlocal clock ->RENDER WAV
->DRAG WAV orEXPORT WAV;USE INPUT returns track.32f WAV, mono/stereo bus layout,
0/2/4/8s tail, latency trimmed, no normalization. Stored-source waveform, not
processed-wave preview. Source/settings/rate/BPM/layout/tail invalidate readiness.
Sample audition is temporary, not project-embedded; state loading stops it.
Old parameter/program/state/IDs untouched. Existing225hostprograms retained.
RISE/FILL toolbar operates on its separate capture/sample, retains24-bit render;
rerender files unique,failedwrites invalid. That existing transition renderer
still runs on its historical control-thread path; don't claim worker conversion
there. Old transitioned clips/files are not modified by new generations.

New APIs:Source/Standalone/RevolutionSample.*(worker,3published slots/readers,
4queued jobs, bounded audio fill, off-audio resampler/export); sampleWorkspace(),
renderSample(),sampleRevision(),makeBold() inModulePluginProcessor. Processor's
optional offline constructor disables file worker/follower for bounce. Parameter
listeners andscene publications increment revision. Clone render warms wrapper
ramps on silence thenstarts local clock. Never move decode/IO/allocations/locks
into fill/process. Idle file thread starts lazily and waits for requests.
UI/RevolutionSampleBar.* owns chooser SafePointers/controls; editor closes bar
before callbacks/panels. Page coordinates preserved above a160design-pixel tray;
editor opens885x930. Minimum half-size is compact with small text/abbreviated
combo; defaultsize preferred. Shared PresetScreen not edited.

Test/visual evidence:Tests/SampleWorkspaceTests,ImpactLab,AllocationAudit,
SnapshotMain optional *_sample/_minimum.190checks cover playbackblockexact,
zeroaudioallocation,finite load/cancelstress,wrongfile/nonfinite/duration failure,
resampling pitch/anti-alias -112.887dBFS,rate cache,stale export,floatheadroom,
mono layout,exact decodedrender+tail,file/source protection,queuedclose,
transition immutability/UI routes and sizes. Actual OS cross-app FL/Explorer
file drag/file-dialog interaction and owner-material listening not manually
verified; don't claim that they were. No release installer created.

Commands:
cmake --build build --config Release --target HamloRevolution_VST3 HamloRevolution_Tests HamloRevolution_Snapshot HamloRevolution_ImpactLab -- /m:2 /v:minimal
build/Release/HamloRevolution_Tests.exe
E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8 Tools/verify_r9.py

CMake VS18x64/JUCE8.0.12 read-only E:/HPEQ/build/_deps/juce-src. Python venv from
E:/HamloShirota/build/s3_venv. No dependencies upgraded. Audits require local out
archives/binaries; normal builds/tests do not. ImpactLab refusesexistingreport;
validator refusesexistinglabel. Keep intermediate final/final_v2 reports; release
names are authoritative. Frozen out/r9/accepted includes sources/tools/bundle.
First new fixture's assertion-before-beginTest bug was corrected; stdout only
retained, first bad binary was not separately frozen. Stereo-only export gap
fixed before firstmono probe. No acceptance threshold relaxed.

Next action: address owner-specific FL/listening feedback if requested; no
remaining implementation promised in this stage. Medium for routine builds/UI;
Astra xhigh for a real DSP/measurement/compatibility issue. No switch performed.
