# R9: audible control audit and main sample workbench

2026-11-03, parent94051dd. Before production changes.

Diagnose rather than amplify the output blindly. Use one fixed ImpactLab before
and after: seeded synthetic drums and bass, constructor/GENERATE, TOPS/FULL,
default and all-bar/full-amount Chaos recipes. Report RMS(y-dry)/RMS(dry), latency
aligned, plus exact PCM hashes. Do not silently alter existing DSP or recipes.

Acceptance:
- A1 all baseline renders and original state/parameter/program contracts remain
  unchanged. The explicit BOLD action selects FULL, AMOUNT100%, INTENT Chaos,
  FILL everybar, MIX100%, VARIATION65%, unlocks and generates. It preserves genre.
  On the 16s owned drums its difference ratio must exceed -18dB. BOLD is an
  opt-in recipe, not a louder output stage or a rewrite of old projects.
- A2 main SIMPLE/ADVANCED pages offer LOAD SAMPLE, play/stop/loop, USE INPUT,
  render/save WAV and outgoing file drag. Incoming drop works on both pages;
  RISE/FILL retains its separate transition workflow, with explicit load/export.
- A3 mono/stereo sources up to60s and8..192kHz; matching-rate PCM is preserved,
  other rates resampled on a worker, finite data checked. Invalid/too-long files
  report an error and preserve the last good source. No file IO/allocation/waiting
  in audio. Changing host rate stops playback and prepares a new rate off audio.
- A4 sample playback feeds the actual sequencer, with a local clock while the
  host is stopped; loop position does not restart the musical pattern. Block
  schedules1/64/127/512/4096afterprepare256 match. Off/input leaves old audio exact.
- A5 export snapshots parameters/scenes and the source at request time. An
  independent offline processor renders one sample pass plus selected0/2/4/8s
  tail; fixed latency is trimmed. 32-bit float WAV preserves headroom, no automatic
  normalizing/limiting. File is only marked ready after a successful complete write.
  Failures preserve existing destination files; WAV decoder agrees with float PCM.
  Source/settings changes invalidate ready status; already dragged files stay.
- A6 async load/render keeps the editor responsive; bounded atomic audio handoff,
  commands never dereference editor objects. Cancellation/closing and repeated
  loads/exports are tested. Sample audition is a temporary workspace, not embedded
  project audio; project restore stops it. Show this in help.
- A7 full native tests, before/after audio identity, pluginval5..10/Steinberg,
  visual main/advanced/transition and minimum-size snapshots. FL/Explorer drag
  must not be claimed manually tested unless actually performed.

Baseline absent controls are n/a. No HPDG/HPEQ edits, installer or push.

First baseline disclosure: delta alone showed large bass differences despite
TOPS preserving bass magnitude; cascaded band recombination changes phase.
Before production edits the fixed lab gained windowed output/input RMS gain
and p95-p05 modulation spread (200ms windows, skip first1s). Recapture both sides
with this same final method. The original before.json is retained. Difference
ratio is not a psychoacoustic strength score; report all three measurements.

Additional import probe (after initial implementation, before its first run):
96kHz source with32kHz tone ->48kHz output, discard boundary1000samples;
RMS below -70dBFS to catch missing downsampling anti-alias filtering. Also inspect
IEEE-float metadata and test preserved headroom at Mix0. No old criterion changes.

Final review added a mono-host render probe before its first measurement. Export
now snapshots main-bus layout as well as parameters/scenes; mono remains mono.
The first stereo-only export candidate and validator run are retained as history.
