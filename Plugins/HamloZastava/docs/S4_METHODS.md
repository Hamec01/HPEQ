# S4: modern editor and detector audition

2026-11-03. Parent1341976. Goals recorded before DSP/editor changes. No
subagents, other product changes, installer, installed-plugin update or push.

QUICK exposes the four roles, Soft/Hard, power, the six essential controls and
context. ADVANCED has Envelope, Key/Mix and Rhythm pages with exact numeric
controls and all32pattern levels. Legacy remains separately accessible. Preserve
Black Steel and the shared unmodified PresetScreen at56,20,336x116 on1180x780.
Persist modern editor view/tab, show modified recipe state and neutral startup
honestly. Navigation/metadata must not silently alter another engine's controls.

Append v2_listen after existing77parameters. Listen monitors the selected signed
HP/LP-filtered key before envelope detection, with fixed5msPDC and a20ms
transition when toggled. It works with Modern Processing off. Global/host bypass
and missing requested external key override it. Listen does not apply gate gain,
Mix or Output. Initialize its ramp at the requested state on first processing.
Allocate all history in prepare. With Listen off, existing arithmetic/output
must remain exact. Missing new fields/old projects default Listen off.

| ID | Acceptance |
|---|---|
| Q01 | Original16contract/seven programs and229M0primary+458restore observations unchanged; first77host IDs/order retained |
| Q02 | Listen-off same66kernel fixtures bit-exact to accepted S2/S3, all275numerical gates and283S3host checks remain passed |
| Q03 | Unfiltered listen equals5msdelayed selected key byte-for-byte, main/external mono/stereo,8/44.1/48/96/192k; HP/LP discrimination>=30dB; envelope/Mix/Output cannot change audition |
| Q04 | Listen startup/bypass/key-loss,20ms transition and restoration; zero C++ audio allocations, finite output and same-position events exact at1/64/127/512/4096 |
| Q05 | Editor view/tab and Listen restore on fresh/edited instances; old states clear them; factory loading clears modified flag; a modern edit marks it; undoing that edit clears it |
| Q06 | UI controls target correct stable IDs with gestures/numeric entry; visible modern power matches enabled+bypass;32levels editable; English contextual help; no visible-control overlap/preset obstruction at supported sizes |
| Q07 | Inspect actual screenshots:all4Quick roles,Advanced Envelope/Key/Rhythm and Legacy, including smaller/larger sizes and Listen/missing-key status |
| Q08 | Fresh production VST pluginval5..10/Steinberg pass and DLL/source stamps match measured build |

Before:archive S3source,bundle and method tools; run the immutable accepted
S2kernel executable on a new directory. Its identical current driver is reused
after, and every66PCM hash is compared to the measured before (not just pass
counts). S3primary/M0methods stay frozen and may reuse their matching original
references; record fresh current reports. Additional S4audition/UI unit tests
record new functionality; baseline lacks it and is n/a. Never relabel earlier
vendor results. Any changed test/target after measurement must be disclosed.
200genre recipes and final owner FL listening belong to the next stage.

*First S4suite:202checks passed; its numeric-control check called Slider.setValue.
The final suite commits text through the actual Slider value label (including
percent steps), checking parsing and gestures. No threshold/target is weakened.
Initial screenshots exposed clipped header/pattern text, corrected before final
visual acceptance. These are additional UI checks, not a changed frozen S2/S3
numerical method; their compiled drivers and original before fixtures stay.*
Zero-valued text boxes initially retained JUCE's default formatter. Refresh after
installing the custom formatter; actual OFF/zero-with-unit labels are checked.
The final companion suite also renders Listen through the actual processor and
prepared state load, then host bypass, on all six main/key layouts. This closes
the gap between separate Gate tests and the earlier parameter-only UI checks;
the original Q03/Q04 exactness requirements remain unchanged.
Final screenshot review found common Release still enabled in Drum and Stereo
Link enabled in Rhythm although neither changes the role's gain. Disable these
controls and add two role-mask assertions;465native expectations are required.
Keep the earlier463pass logs/screenshots/validators as intermediate evidence.
