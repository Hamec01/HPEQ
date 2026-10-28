# The Hamlo plug-ins

HPEQ's modules as products of their own (sold alone and as the HPEQ combo). Each folder is a complete repository of
its own (build, tests, installer, docs), brought here with `git subtree` so its history stays; HPEQ's build does not
include them.

| Plug-in | What | Version | Notes |
|---|---|---|---|
| HamloComp | compressor: VCA / FET / OPTO / TUBE, DRUMS / SAMPLES / VOC | 0.2.0 | docs/COMPRESSOR_MODELS.md |
| HamloUdarnik | transient shaper, V2 engine | 0.2.0 | docs/UDARNIK_V2.md |
| HamloRedWall | limiter, V2 engine, DRIVE / TUBE colour | 0.3.0 | docs/REDWALL_V2.md |
| HamloMag | saturator (M0 here; v2 in progress) | 0.1.0 | docs/MAG_V2.md |

All four: 200 genre presets (Boom Bap, Trap, Techno, DnB x 50, ten groups; HamloMag with v2) and the preset screen top
left on the page (docs/GENRE_PRESETS.md in each). Keeping a module in step with HPEQ: docs/SYNC_WITH_HPEQ.md in each.
Build one: `cmake -S Plugins/<name> -B Plugins/<name>/build -G "Visual Studio 18 2026" -A x64 -D<NAME>_JUCE_DIR=<a JUCE 8.0.12 checkout with the renderer patch>`.
