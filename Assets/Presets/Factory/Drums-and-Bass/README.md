# HPEQ Factory Presets — Drums & Bass

100 full-state factory presets for HPEQ schema v1. Each `.hpeq` file is the exact binary block produced by
JUCE `AudioProcessor::copyXmlToBinary`; its bytes can be passed directly to
`FxPluginProcessor::setStateInformation`.

The pack is organized as Kick (18), Snare (17), HiHat (15), Percussion (15), DrumBus (20), and Bass (15).
`manifest.json` is the browser catalog and contains stable IDs, display names, tags, intent, relative paths,
and output trim. Presets are starting points: compressor and gate thresholds necessarily depend on source level.

Signal order inside every preset is PRE-EQ followed by the layer MIX chain. The designs favor complementary EQ,
envelope-aware attack/release, moderate saturation, mono-safe low end, and explicit headroom. Presets with strong
colour are named/tagged accordingly.

Design references: Sound On Sound's drum-compression and kick/bass technique articles, its Dylan “3-D”
Dresdow interview, and iZotope's drum-compression and drum-bus guides. Their advice was translated to HPEQ's
actual DSP ranges rather than copied as brand-specific settings.