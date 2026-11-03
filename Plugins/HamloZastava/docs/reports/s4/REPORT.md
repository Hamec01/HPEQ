# S4: modern editor and signed detector audition

2026-11-03. Parent1341976. Modern controls are now available in the VST.
Version remains internal0.1.0; the 200-preset library and owner FL listening are
still pending. No installed plug-in, installer, ZIP, push or other product changed.

## Delivered

QUICK has four explicit roles, Soft/Hard, six essential knobs, neutral INIT,
Processing power, status, key routing and numerical readouts. Drum substitutes
Body/Tail for Hold/Release. Rhythm exposes its16–32editable level bars, numeric
percent entry, tempo/free clock and initial pattern buttons.

ADVANCED has Envelope, Key/Mix and Rhythm pages. Inactive role controls are
disabled and dimmed. English tooltips and footer help describe units and timing.
The envelope picture is a stage guide; TARGET GR is the internal envelope before
Mix. Missing external key and stopped synced transport show explicit dry status.
QUICK/ADVANCED view/tab restore in modern projects. Factory recipe changes are
marked, and reverting an edit clears the marker. Legacy keeps its separate page.
Shared PresetScreen, BlackSteel and Legacy panel source files remain unchanged;
the preset screen stays at its original top-left coordinates.

`v2_listen` is appended at host index77: **78parameters**, with the previous77
IDs/order/definitions retained. Its prepared ring monitors signed HP/LP key audio
at fixed5msPDC. The 20ms audition transition is independent of gate gain, Mix,
Output and Processing power. Global/direct host bypass and missing external key
override audition. New fields default off on old-state load. The normal path's
original arithmetic remains unchanged when audition is off.
Channel mapping follows the main layout: stereo main maps left/right keys (or
duplicates mono key); mono main uses the first key channel. The tests explicitly
validate that mapping, not a separate stereo-to-mono detector downmix policy.

## Accepted measurements

| Check | Result |
|---|---|
| S2 numerical method | 275passed/0failed,66cases; unchanged frozen driver/analyzer/method |
| Listen-off raw PCM | All66new raw renders exactly equal newly measured pre-S4 renders and accepted S2renders, including five-minute192kstress |
| Legacy M0 | 229primary PCM/state/latency observations plus458fresh/edited restored observations exact |
| S3 host method | 283passed/0failed, same frozen method |
| Native suite | 465passed/0failed, including old integration, audition, actual host output/restoration/bypass, UI layout and numeric text commit |
| Audition PDC/layouts | Exact delayed selected key in30rate/main/key cases at8/44.1/48/96/192k; actual processor routes/restores all6main/key layouts exactly |
| Audition HP/LP | 37.797122 /40.025014dB rejection of1kHz atHP8000/LP100; target>=30dB |
| Audition transition | First rise sample matches1/960blend at48k within1e-7; sample959 reaches exact key, release reaches exact dry at959 |
| Scheduling/allocation | Listen/filter/bypass/key-loss events exact at1/64/127/512/4096 after prepare256; zero C++ measured callback/setter allocations |
| UI | All4roles/Quick/Advanced tabs fit tested.5/1/1.5scales;15source-bound actual screenshots inspected |
| VST | pluginval5..10 all exit0; Steinberg537passed/0failed |

Kernel worst mean in the broader S2case set was1.5256125% of audio duration
(limit25%). This is a kernel laboratory observation, not a new whole-VST/FL CPU
claim. There is no measured owner listening judgment for this stage.

## Disclosed iterations

Targets were recorded in `docs/S4_METHODS.md` before changes. S3source/bundle and
the initial target document were archived. The accepted immutable S2executable
rendered a fresh66case before; all its PCM hashes also match the earlier accepted
S2outputs. Frozen S2/S3 numerical methods were not edited or loosened.

The first new UI/audition suite passed202checks but its numeric test used
Slider.setValue. It was strengthened to commit through the actual value label,
including percent pattern parsing and host gestures. Tests also cover the real
processor output, prepared restoration and direct host bypass, closing the gap
between separate kernel tests and UI parameter tests.

Initial images exposed clipped header/pattern text and zero-valued fields that
kept JUCE's default formatter. Labels/units were corrected and tested. Final
visual review found common Release enabled in Drum and Stereo Link enabled in
Rhythm despite not changing those roles' gain. They are now disabled, with two
explicit regression assertions; final native count465. Earlier463pass results,
images and vendor logs remain as intermediate evidence. The corrected DLL was
revalidated, with fresh current host/Legacy reports and15new captures.

Thirteen final images equal their already inspected previous PNGs byte-for-byte;
the changed Hard Drum and Legacy images were inspected again. Legacy's image
retains a32step modern setting, so its modified asterisk is intentional. Normal
factory loading clears modern edits and the marker, verified by the suite.
No numerical target was weakened. Routine compile issues were corrected before
runtime acceptance. Existing benign zero-fill C4244 is not suppressed.

## Evidence and remaining work

Authoritative: `acceptance.json`, `before.json`, `kernel.json`,
`host_complete.json`, `legacy_complete.json`, `native_roles.txt`,
`screenshots_final.json`, `images_final/`, `validators_final/identity.json`.
The raw pair is `out/s4/before_kernel` / `out/s4/after_kernel`. Audit checks raw
hashes, source/method stamps, initial targets, retained parameter definitions,
shared UI hashes, snapshot executable and final DLL/validator identity.

Final VST DLL SHA256:
`4634d22972057cf64460b46e63609aceca1cd702fd8ddc9cffe6b7438b134404`.
S4source ID:
`dda97ff70588c3f12ebeb682d81847f87901502599affa9ceb1b0d7131287eba`.
Extended kernel source ID:
`9877b2194c06629e6b0f93535f8beb528dc748a7d201f949176bda3829c206ac`.

Bundle: `E:/HamloZastava/build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3`.
Controls guide: `docs/ZASTAVA_CONTROLS.md`. Next:200calibrated genre recipes,
207host programs total, resumable source/method/material guards and final
release/owner FL checks. The existing seven Legacy recipes were not changed.
