# HamloRack beta

The rack includes the full pinned native processing wrappers for HamloComp,
Udarnik, RedWall, Mag, Echo, Verb, Zastava, Shirota, Fader and Revolution.
Newly added modules use the native revision and its real public parameters.
Existing HPEQ projects keep their legacy path. The plugin ID, automation IDs,
state root, `.hpeq` format family and user-preset folder remain compatible.

The new browser contains 250 authored Boom Bap, Trap, Techno, Drum and Bass and
Studio recipes. The old 224 factory programs remain available. Native panels
provide searchable controls, typed values, module programs, live meters, real
Fader Learn/Apply and editable eight-scene Revolution patterns. Dual Mono uses
independent instances. Wet-only returns subtract one aligned dry reference;
master recipes keep their final RedWall after the other layers and output gain.

HamloMag 0.2.2 fixes a mono oversampler layout assertion in its shared standalone
and rack source. Echo/Verb node bypass preserves native tails, including gaps
before the first repeat. Runtime handover does not allocate/free on audio.

The numerical bank measurements and individual exceptions are in
`docs/hamlorack/RACK_BANK_REPORT.md`; the complete source/test receipt is
`docs/hamlorack/RACK_COMPLETE.md`. Engineering calibration does not certify
musical intent. Owner listening, VST host validators and DAW load/save are the
next checks on the owner's build. No installed plugin was replaced by this work.

Revolution's offline sample/chop/drum workbench is a separate asset/rendering
workflow; the rack integration exposes its live insert parameters and scenes.
