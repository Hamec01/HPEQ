# HPEQ Factory Presets — Vocals, Mastering & More

85 full-state HPEQ schema-v1 presets: 15 lead vocals, 10 backing vocals, 30 mastering presets
(five each for Pop, Rap, Trap, Techno, House/EDM and Rock/Indie), plus Guitar, Synth, Keys and Mix Bus.

Each `.hpeq` is a JUCE `copyXmlToBinary` state block ready for `FxPluginProcessor::setStateInformation`.
`manifest.json` contains stable IDs, categories, tags, descriptions and relative paths.

These are musically targeted starting points, not automatic analysis. Vocal de-essing bands must be tuned to the
singer, and dynamics thresholds must be matched to the input level. Mastering presets intentionally make no fixed
LUFS promise; compare level-matched, watch gain reduction and preserve the mix's transient intent.