# Revolution R10 complete / FL listening next

2026-11-04. Parent71545ff; completion commit:git log-1. No active jobs after
completion. Work only E:/HamloRevolution; efficient-staged-work,no subagents.
Owner asked for stronger SHAPE/THROW generation and an audible delay.

Delivered0.2.2 at dist/HamloRevolution-0.2.2-r10/HamloRevolution.vst3 plus ZIP,
SHA256 and controlled synthetic A/B clips. See docs/reports/r10/REPORT.md and
acceptance.json; replay Tools/verify_r10.py. Current source pins include all
Source/Tests and applicable tools/CMake; original lab source before/after exact.

Implementation:Core/RevolutionRecipes builds target-aware level-lane gestures.
Advanced GEN resets selected lane scheduling/chance and applies visible controls,
preserving target/time/note/band/other lanes/seed. Repeated presses vary structure;
sends retain strongest hit. Sustained material uses longer captures. Explicit
SIMPLE generation adds delay+reverb. The pure R6 simple::generate stays frozen;
setCurrentProgram calls generateSimple(false,false), preserving225 old programs.
Other generateSimple callers (including linked HPDG updates) use the new recipes.

New choice IDs throwASource/throwBSource are appended after old host bypass.
Slot229/230:0 CHAIN exact old arithmetic,1 INPUT original signal capture in its
selected band; return joins existing chain without filtering its dry. Insert
targets ignore this selector. Missing IDs reset explicitly on reused instances.
No new audio buffers,latency,locks,allocations or output gain stage. Existing
DELAY/REVERB algorithms otherwise unchanged. Right-click target exposes source;
caption appears below band. Shared PresetScreen unchanged. Sample R9 kept.

Verification:4365 native,225/225 exact program+old-state reload hashes,16/16 new
spatial probes (four genres,normal/stale EightOfEight,delay/reverb),new routing
mono/stereo Full/Tops block1/64/127/512/4096 exact,zero measured audio allocation.
Band rejection-88.48/-90.41dB; all return/dry>=-24dB,peak<1 and finite tails.
pluginval5-10 and Steinberg537 pass on delivery DLL; snapshots inspected.
Authoritative native_delivery,validators_delivery,after_release.json and screen
*_delivery.png. Other initial/release logs are retained intermediates.

Failed work disclosed:initial T3 fixture fed SHAPE's10ms startup ramp before
expecting zero delayed tail; corrected to500ms input silence,threshold unchanged.
Frozen source/exe/log:out/r10/initial-tests. First T4 Techno delay-25.38dB failed
the fixed-24dB target; recipe now always captures strongest attack. Original
after_initial retained. Early StringRef compile error fixed. Combined band/source
tag initially truncated; source now its own line. Minimum-size text remains small.
First receipt verifier also wrongly cross-compared program-load and post-playback
state-restore within a version. R9 already differs in17 cases. Corrected to compare
each procedure R9 vs R10 (225+225 exact), retaining failed receipt/tools/ZIP.
Legacy17 within-version load/restore hash differences remain undiagnosed; do not
claim exact within-version roundtrips for them. Laboratory/method unchanged.

Commands:
cmake --build build --config Release --target HamloRevolution_VST3 HamloRevolution_Tests HamloRevolution_Snapshot HamloRevolution_SpatialLab -- /m:2 /v:minimal
build/Release/HamloRevolution_Tests.exe
build/Release/HamloRevolution_SpatialLab.exe <NEW-OUTPUT-FOLDER> after E:/HamloRevolution/out/r10/before
Tools/validate_r10.ps1 -Label <UNUSED-LABEL>
python -I -X utf8 Tools/verify_r10.py

Build VS18x64/JUCE8.0.12 read-only E:/HPEQ/build/_deps/juce-src. No dependency
changes. Frozen sources/states/binary before:out/r10/baseline and before. Final:
out/r10/accepted. Lab/validators refuse existing output folders. Receipt needs
local out/dist artifacts; regular build/native tests do not. R9 receipt remains
valid only against its frozen R9 source; never update it to bless this new DSP.

Next action:owner loads new bundle in FL,presses SIMPLE GENERATE/BOLD or row GEN,
checks the echo on their actual music. If useful,compare source INPUT and CHAIN
while gating/filtering upstream. A/B fixture demonstrates stale-condition bug;
it is not a claim all old recipes were silent. No manual FL/audio listening/OS
file-drag certification; no installer,installed replacement,HPEQ edits,subtree
or push. Any new synchronization requires owner's instruction. Medium for known
checks; preferred Astra xhigh for DSP/measurement diagnosis. No switch performed.
