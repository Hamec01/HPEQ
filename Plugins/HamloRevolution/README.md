# HamloRevolution 0.3.0

RISE / DRUM FILL now offers source-aware **STYLE** recipes. RISE:LIFT,PULSE,
SWELL,RUSH,ARC,PRESSURE. DRUM FILL:POCKET,ROLL,BREAK,TRIPLETS,SCATTER,TURNAROUND.
**AUTO** selects a family;**GENERATE** varies actual source attacks,timing,
accents and pickups. Four knobs and1/2/4 bars remain. CLASSIC retains the old
renderer. The sample toolbar keeps load,capture,preview,WAV export and drag.

For a quick fill,open FILL,choose1 bar and try BREAK or TRIPLETS. For a riser,
try RUSH or PRESSURE over2/4 bars. Adjust the four knobs,then GENERATE again.
Normal sequencer playback,225 programs and existing projects are unchanged.

4769 native checks,96 exact Classic corpus renders,96 new renders,225 exact
normal program/state renders,pluginval5-10 and Steinberg537 pass.28 local audio
references were analyzed;12 level-matched owner-material A/B files are prepared.
No reference audio is embedded in the plugin. [Report](docs/reports/r12/REPORT.md),
[handoff](docs/R12_HANDOFF.md). Delivery:dist/HamloRevolution-0.3.0-r12 and ZIP.
This is the clean feature/transition-v2 checkout of HamloRevolution,not a new
plugin. Original checkout:E:/HamloRevolution;its parked R11 draft is separate.

Windows x64 VST3 rhythmic multi-effect sequencer. SIMPLE generates genre-aware
patterns; ADVANCED edits TIME, GLITCH, MOTION, SHAPE and THROW lanes. RISE/FILL
creates transitions from captured audio or a dropped sample. Existing225programs
(Default,24Factory,200genre presets) remain in place.

**0.2.2: spatial generation.** Explicit SIMPLE **GENERATE** now creates a delay
and reverb pair. On ADVANCED, each SHAPE/THROW **GEN** builds a recipe for its
selected effect, clearing old condition/chance restrictions. Delay/reverb capture
strong attacks; repeated presses vary their capture rhythm. SHAPE creates musical
envelopes for the chosen filter, gate, pan, width, drive or crush target.

Right-click a THROW target to choose **INPUT / CHAIN**. INPUT captures the original
audio in the selected frequency band, so upstream gates/filters cannot starve
the echo. CHAIN takes the preceding effects' output. Time, note and band remain
yours when generating one lane. GEN changes that lane's visible mix/depth/tail/
tone settings. Existing project audio and all225 program loads retain their exact
old sound; press GENERATE/row GEN to opt into the new recipes.

For an obvious full-band effect, press **BOLD** on SIMPLE. It generates a strong
Chaos recipe, AMOUNT/MIX100%, FULL, fills everybar, and unlocks editing. Genre
is preserved. TOPS protects bass; fills every4bars and LOCK can make changes
less obvious. Existing projects/presets are not rewritten by this update.

The bottom toolbar is a direct sample workspace:

1. **LOAD SAMPLE** or drop mono/stereo WAV, AIFF, FLAC or supported MP3/OGG.
2. **PLAY** through current settings; **LOOP** keeps the sample running even with
   the host transport stopped. Adjust SIMPLE or ADVANCED while listening.
3. **RENDER WAV**, wait for ready, then drag **DRAG WAV** into the DAW.
   **EXPORT WAV** renders to a chosen filename instead.
4. **USE INPUT** stops auditioning and returns to processing the track.

Main samples are limited to60seconds at8..192kHz. Loading/rate conversion/rendering
run off audio. Exports keep the mono/stereo host layout, one source pass plus
0/2/4/8seconds tail,32-bit float, without automatic normalization/limiting.
The source waveform is shown. Changed settings/source/tail require a new render.
Workspace audio is temporary, not embedded in projects; reload it after reopening.

On RISE/FILL the toolbar loads, previews, saves and drags the generated transition;
it retains its24-bit transition format. Rerendering never overwrites a WAV already
dragged into the DAW. Main and transition sources remain separate.

4365native checks pass; all225 R9 program renders and old-state reloads remain
exact,16/16 new spatial probes pass. pluginval5–10 and Steinberg537/537pass.
[R10 report](docs/reports/r10/REPORT.md), [spatial contracts](docs/THROW_GENERATION.md),
[R9 workspace report](docs/reports/r9/REPORT.md),
[sample contracts](docs/SAMPLE_WORKBENCH.md), [handoff](docs/NEXT_STAGE.md).
Manual FL/Explorer cross-application drag/listening remains unclaimed.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_REVOLUTION_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloRevolution_VST3 HamloRevolution_Tests -- /m:2 /v:minimal
build/Release/HamloRevolution_Tests.exe
```

VST: `build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3`.
Delivery: `dist/HamloRevolution-0.3.0-r12-win64-vst3.zip`, SHA256 beside it.
The installed copy is untouched. No installer, push or subtree was produced.
