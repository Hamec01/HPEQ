# HamloShirota V2: owner requirements and initial audit

Status: owner approved implementation2026-10-31. S1 isolation is complete; see
reports/s1/REPORT.md. S2 offline width/early-space prototypes pass181checks;
see reports/s2/REPORT.md. Production integration and motion/binaural remain planned.
The audit notes below
describe the pre-isolation M0 and are retained as the starting point.
Work only in E:/HamloShirota; no Verb/Echo/HPEQ edits, subtree, push or subagents.

## Owner requirements

- Stronger audible width, several spatial characters, and precise numerical controls.
- Retain normal stereo-expander controls as a complete, usable section.
- User-programmable left/right movement, including hold times and smooth transitions.
- Spatial movement should include a path around/behind the listener's head.
- Primary owner listening reference: AKG K712 headphones (owner confirmed).
- Own implementation and presentation; use published methods as references.

## Proposed sections

STEREO: Mid/Side width and levels, true source panning and separate stereo balance,
frequency-dependent width, mono bass, mono-to-stereo generation, mono audition,
correlation and measured mono loss. Existing SAFE must be assessed independently
from artistic width so its intervention is visible and intentional.

SPACE: close/wide/diffuse/distant characters, independent direct/early spatial
energy, diffusion and depth. Preserve source stereo spread rather than silently
collapsing every stereo input to one point. A point-source option may be explicit.

MOTION: editable position nodes and drawn paths; per-node hold and transition,
free milliseconds or tempo divisions, direction, phase/retrigger and smoothing.
Example: left -> hold -> arc behind the head -> right -> hold -> repeat.
Controls need typed values and host automation, not only broad macro knobs.

Separate Stereo and Binaural headphone rendering. Provide selectable spatial
profiles with front/side/back comparison sounds for owner audition on K712.
The headphone model is a listening reference, not a measurement of the owner's
head/ears or proof that one HRTF is individually calibrated. Headphone correction
EQ has not been requested or selected. QUICK and full controls remain distinct.

## Code audit findings

Baseline commit: 93d965f; repository initially clean. Version0.1.0/M0.
The extraction report records59/59 native expectations and prior validators;
these results have not been rerun in this scope stage.

ModuleVariant.cpp currently exposes Width0..2, Stereoize, Space, Mono Bass,
Rotation-45..45, EDINSTVO/PROSTOR/KOSMOS, Balance, Mix, Output, SAFE and Mono Check.
HamloShirota.cpp applies Mid/Side gain, side high-pass, an eight-all-pass mono
stereoizer, KOSMOS frequency shaping, SAFE side reduction and a static channel
rotation/balance. SAFE is enabled by default. Its effect on strong width is a
hypothesis to measure, not an established diagnosis of the owner's listening case.
No HRTF renderer or user-programmable motion path was found in this core.
The present core passes a one-channel input unchanged; mono-to-stereo bus support
must be explicitly designed. The build still includes the large module stack.
The report records fixed120samples at48kHz; verify the current contract before
choosing latency for new modes. Do not promise zero PDC without measurement.

## Ordered work

1. Preserve M0 binary, full parameter/program/state contract, renders and event
   schedules before changing audio. Record numeric goals and a single lab method.
2. Isolate the Shirota processor and prove Legacy bit identity to the captured M0.
3. Prototype widening and SAFE interaction; measure mono sum, response, stereo
   correlation, gain, automation and CPU. Choose character algorithms from evidence.
4. Prototype binaural profiles and interpolated movement. Check channel direction,
   timing, source spread, trajectory events, interpolation and audible transitions.
   Owner verifies front/back perception on K712; objective filter tests alone do
   not certify perceived location. Do not silently weaken stereo/mono constraints
   to accommodate binaural artistic processing; define their acceptance separately.
5. Implement STEREO / SPACE / MOTION with precise values, then QUICK. Preserve the
   shared top-left PresetScreen, IDs/state compatibility and a neutral Default.
6. Complete tests (block1/64/127/512/4096, zero audio allocations, old states,
   routing, latency and stress),200genre recipes with resume/source guards,
   screenshots, pluginval5..10, Steinberg, owner FL audition, report and packages.

## Primary research references

- AKG K712 manufacturer sheet: https://se.akg.com/on/demandware.static/-/Sites-masterCatalog_Harman/default/dwa1c95a86/pdfs/AKG_K712Pro_Cutsheet.pdf
- OpenAL Soft HRTF documentation: https://github.com/kcat/openal-soft/blob/master/docs/hrtf.txt
- HRTF individualisation/localisation literature: https://acta-acustica.edpsciences.org/articles/aacus/pdf/2023/01/aacus230021.pdf
- University of York SADIE II database and Apache2 licensing: https://www.york.ac.uk/sadie-project/database.html
- Pinned SADIE II publication/download record: https://zenodo.org/records/10886409

SADIE II is a candidate measurement source, not yet bundled or selected. Inspect
exact archive licensing, attribution, processing and file hashes before reuse.
No listening test, headphone EQ, HRTF calibration or model/effort switch has been
performed by the agent.
