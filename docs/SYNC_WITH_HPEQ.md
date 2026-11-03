# HamloFader lineage and compatibility

M0copied HPEQ's Gain slot and module host atb277315. HPEQ had no standalone
Fader VST3: HamloFader's independent identity isBBLB/HmFd/
com.hamloprod.hamlofader, module kind8, state tagHamloFader. Original seven
recipes plus Default and seven parameter IDs plus bypass remain unchanged.

F1isolates only this product. FaderGain.cpp/h preserve the original Gain body
and members from Processors.cpp/h. LegacyFaderCore preserves the single Gain
slot fade, node delta and whole-layer sum; FaderSettings replaces the project
model with the required original tags/indices. ModuleVariant retains Fader's
original parameter layout; processor/editor no longer reference other engines
or panels. Static library membership and recursive include closure are audited.
Unused historical copies stay outside production. BlackSteel, PresetScreen,
Fader panel, Ramp, DelayLine and DspConfig remain byte-equivalent sources.

Actual original/current PCM/state/meters match under226schedules and452actual
restorations. See reports/f1/REPORT.md and acceptance.json. HPEQ itself was not
edited, tested as a paired reference or updated via subtree. Integration into
the common product is a separate future operation.
