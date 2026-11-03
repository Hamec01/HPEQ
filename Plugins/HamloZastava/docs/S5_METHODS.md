# S5: 200 genre recipes and guarded calibration

2026-11-03; parentb1f4f73. Targets recorded before new catalog/calibration.
Keep kernel9877b219 and all78parameter definitions unchanged. Mono-main stereo
key retains its documented first-channel mapping; stereo main uses both.
No subagents/other products/installer/push. Version promotion/distribution follows
final source-bound verification; owner FL listening remains an owner action.

## Library and compatibility

Four genres at50each: Boom Bap88BPM,Trap144,Techno132,DnB174. Each has16Clean,
14Drum,8Duck,12Rhythm recipes. Every recipe has permanent ID,name,genre,role,
purpose,material,tempo and depth. Original Default+six programs stay0..6;
new programs7..206. Preserve all78IDs/order/ranges. Listen off,Mix1,Output0 for
genre recipes. Distinct processing values/patterns beyond Mix/Output, globally
unique IDs/names and full processing signatures. No competing product names.

Selected recipe ID persists in schema1 with numeric-program fallback for older
states. Recognized ID wins over stale index; unknown ID retains supplied settings
and falls back to a valid index. Original compact Legacy encoding stays exact.
Loading a recipe applies modern values and clears Listen; keep host bypass's
historical independent behavior. Library/filter selection and modified marker
must work with207programs; shared PresetScreen source is unchanged.

## Fixtures and metrics

48kHz stereo preparation uses owner WAVs, normalized to -6dBTP with4x polyphase
peak estimation. Annotated fixtures are explicitly constructed from owner
snippets plus deterministic low noise; they are not claimed to be annotated
natural performances. A separate natural excerpt tests every actual recipe.
PDC is accounted for by shifting input/masks240samples. Drain250ms.

| Role | Defined fixture and acceptance |
|---|---|
| Clean | Four1.5speriods: wanted snippet50..400ms, quieter copy450..700ms at22% amplitude; deterministic -58dBFS noise bed. Measure pause1120..1450ms. Noise rejection Gentle4..12,Medium12..24,Strong24..48dB; wanted loss<=1dB,quiet-detail loss<=2dB,onset first20ms loss<=3dB |
| Drum | Eight owner-derived onsets alternating full/28% strength, half-beat spacing. Clip length=min140ms,72%spacing; onset lead2ms with0.3msfade. Exactly8triggers; first20ms attack loss<=1dB; post-tail window10..40ms after declared attack/body/tail end must meet Gentle4..12,Medium12..24,Strong24..48dB rejection. Envelopes must finish before next hit; use finite S2timing model |
| Duck | Eight quarter-note owner kick keys, first90ms per key, repeated owner main. Key-active20..65ms attenuation Gentle1..3,Medium3..6,Strong6..12dB; recovery loss<=1.5dB in final35..5ms before next key. Missing key separately passes delayed dry |
| Rhythm | Material-derived repeating main; tempo/free clock,16/32editable levels. At least2full and2floor steps. Center-window full-step loss<=0.2dB; full/floor contrast Gentle4..12,Medium12..24,Strong24..48dB. Intermediate levels/pattern timing contribute to recipe uniqueness |

Natural excerpts: finite output, peak<=-1dBTP. Annotated outputs also<=-1dBTP.
All200actual compiled recipes must restore settings/selected ID and render
bit-identically on fresh and already-prepared/edited instances. Zero measured
C++ audio allocations. Check at256samples; representative/all-program event
and oversize schedules covered by native/final compatibility suites.

Calibration may adjust Clean threshold, Drum threshold/onset/retrigger and
role Range inside the published depth bands. Do not move goal windows/bands or
use Mix/Output to force a pass. Retain unsuccessful trials. Candidate overrides
are proposals, never final catalog evidence: regenerate once, rebuild, validate
all200without overrides. No dependent results from another DSP/method/material.

## Cache and final checks

Native worker stamps DSP/mapping, method and catalog. Python records owner/
prepared-input hashes and recipe control hash. Resume individual jobs only when
all keys and output hash match; corrupt/stale jobs are rejected. Negative guards
must demonstrate rejection, including changed DSP/method/input/recipe/raw output.
Do not upgrade environments or restart valid long jobs unnecessarily.

Clone frozen S3drivers for S5:contract/cases enumerate only original7 and old
automation modulo7, preserving exact229fixtures. Original M0/current use the
identical clone; baseline must match existing S1v3primary/restored hashes. The
host clone tests the retained prefix rather than requiring total count7; a
separate library test requires207. Never edit frozen S2/S3/S4methods.

Before new presets:archive S4sources/bundle and run matching M0control. New
library/calibration functions are n/a in baseline. After:465or more native,
kernel275/66PCM unchanged,229M0+458restore matches,200/200library acceptance,
UIgenre/group selection screenshots,pluginval5..10/Steinberg,0.2.0VST/ZIP/SHA256.
No installer. Disclose every changed method/goal after its first measurement.

### First-run disclosure

The initial catalog passed195/200. Four Hat Blips recipes and DB Kick Staccato
detected exactly8hits but attenuated the first20ms by1.095..3.148dB. Their Body
was only2.75..5.5ms, so the tail cut began inside the protected attack window.
Manual recipe revision: Hat Blips Body20ms, DB Kick Staccato Body18ms. This is
an authored envelope correction, outside automated threshold/onset/gap search.
No metric, threshold, fixture or DSP change. Retain calibration_initial.json
and out/s5/initial_recipes.json. The disclosure changes the method stamp;
rebuild workers and remeasure under the final stamp rather than reuse old jobs.
