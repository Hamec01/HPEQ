# The Hamlo plug-ins

HPEQ's modules as products of their own (sold alone and as the HPEQ combo). Each folder is a complete repository of
its own (build, tests, installer, docs), brought here with `git subtree` so its history stays; HPEQ's build does not
include them.

| Plug-in | What | Version | Notes |
|---|---|---|---|
| HamloComp | compressor: VCA / FET / OPTO / TUBE, DRUMS / SAMPLES / VOC | 0.2.0 | docs/COMPRESSOR_MODELS.md |
| HamloUdarnik | transient shaper, V2 engine | 0.2.0 | docs/UDARNIK_V2.md |
| HamloRedWall | limiter, V2 engine, DRIVE / TUBE colour | 0.3.0 | docs/REDWALL_V2.md |
| HamloMag | saturator, sampler colour, PHONE / SUB / DYNAMICS / MATCH / DELTA | 0.2.1 | docs/reports/v2/TARGETS_FIX_REPORT.md |
| HamloEcho | delay, v2 (Quick Note Score) | 0.2.2 | docs/reports/v2 |
| HamloVerb | reverb, v2 | 0.2.0 | docs/reports/v2 |
| HamloZastava | gate / expander (M0, kind 6) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloShirota | stereo processor, 200 genre presets | 0.2.0 | docs/reports/s5 |
| HamloFader | gain utility: gain, delay, polarity, MATCH (M0, new kind 8) | 0.1.0 | docs/reports/M0_EXTRACTION_REPORT.md |
| HamloRevolution | rhythmic multi-effect sequencer: SIMPLE (genre generator, CONNECT TO HPDG), ADVANCED (7 lanes, GLITCH 2), RISE / FILL | 0.2.0 | docs/REVOLUTION_V2.md, docs/reports/R6_FINAL_REPORT.md |

HamloZastava and HamloFader are still being worked on in their own repositories (this folder holds their M0).
The EQ is not a plug-in of its own yet (it stays in HPEQ).
The rules and the hand-over for the next agent: AGENTS.md (the same file in every plug-in).

The first four: 200 genre presets (Boom Bap, Trap, Techno, DnB x 50, ten groups) and the preset screen top
left on the page (docs/GENRE_PRESETS.md in each). Keeping a module in step with HPEQ: docs/SYNC_WITH_HPEQ.md in each.
Build one: `cmake -S Plugins/<name> -B Plugins/<name>/build -G "Visual Studio 18 2026" -A x64 -D<NAME>_JUCE_DIR=<a JUCE 8.0.12 checkout with the renderer patch>`.
