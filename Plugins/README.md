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
| HamloEcho | delay (M0: HPEQ's module-plug-in stack, kind 2) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloVerb | reverb (M0, kind 1) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloZastava | gate / expander (M0, kind 6) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloShirota | stereo processor (M0, kind 7) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloFader | gain utility: gain, delay, polarity, MATCH (M0, new kind 8) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |

HamloRevolution is not extracted yet (its page edits patterns kept as slot parameters; the plan is in AGENTS.md).
The rules and the hand-over for the next agent: AGENTS.md (the same file in every plug-in).

The first four: 200 genre presets (Boom Bap, Trap, Techno, DnB x 50, ten groups; HamloMag with v2) and the preset screen top
left on the page (docs/GENRE_PRESETS.md in each). Keeping a module in step with HPEQ: docs/SYNC_WITH_HPEQ.md in each.
Build one: `cmake -S Plugins/<name> -B Plugins/<name>/build -G "Visual Studio 18 2026" -A x64 -D<NAME>_JUCE_DIR=<a JUCE 8.0.12 checkout with the renderer patch>`.
