# HPEQ Factory Library

The factory library contains 185 JUCE binary state presets for HPEQ schema v1.

- `Drums-and-Bass`: 100 presets for kick, snare, hi-hat, percussion, drum bus and bass.
- `Vocals-Mastering-and-More`: 85 presets for lead and backing vocals, genre-oriented mastering,
  guitar, synth, keys and mix bus.

Every `.hpeq` file is a complete `AudioProcessor::copyXmlToBinary` state block and can be passed
directly to `FxPluginProcessor::setStateInformation`. Pack manifests provide stable IDs, names,
categories, tags, descriptions and relative paths. The root `manifest.json` indexes both packs.

Dynamics thresholds are starting points and depend on source level. Vocal dynamic-EQ bands must be
tuned to the singer. Mastering presets deliberately make no fixed loudness claim: level-match comparisons,
watch gain reduction, and preserve transient intent.
