# Hamlo plug-ins in HPEQ

Published source snapshots, synchronized on 2026-11-04 at the owner's request.
Each `Plugins/<name>` is the exact tracked tree of its standalone repository,
with its Git history retained by subtree imports. HPEQ's main DSP/build is not
rewired by these source imports. The EQ remains part of HPEQ, not a separate
standalone plug-in repository. HamloProd is a website folder, not an audio plug-in.

`feature/hamlo-plugins` contains all ten current snapshots. Each dedicated
`plugins/<lowercase-name>` branch updates just that plug-in from the common
pre-publication base and records its source in `Plugins/sync/<name>.json`.
The latest synchronization updates Verb to 0.2.1 and Revolution to 0.3.0.
The other eight source snapshots are unchanged from the previous publication.

| Plug-in | Version | Purpose | Snapshot status | Source commit | Dedicated HPEQ branch |
|---|---|---|---|---|---|
| [HamloComp](HamloComp/) | 0.2.0 | Compressor: VCA/FET/OPTO/TUBE | 0.2.0 source snapshot | `c7d344bffc59` | [plugins/hamlocomp](https://github.com/Hamec01/HPEQ/tree/plugins/hamlocomp) |
| [HamloUdarnik](HamloUdarnik/) | 0.2.0 | Transient shaper | 0.2.0 source snapshot | `11d37dbff5fe` | [plugins/hamloudarnik](https://github.com/Hamec01/HPEQ/tree/plugins/hamloudarnik) |
| [HamloRedWall](HamloRedWall/) | 0.3.0 | Limiter | 0.3.0 source snapshot | `d3c9f9a47e94` | [plugins/hamloredwall](https://github.com/Hamec01/HPEQ/tree/plugins/hamloredwall) |
| [HamloMag](HamloMag/) | 0.2.1 | Saturator and sampler colour | 0.2.1; aliasing/Match fixes included | `c5dcd10dcfdd` | [plugins/hamlomag](https://github.com/Hamec01/HPEQ/tree/plugins/hamlomag) |
| [HamloEcho](HamloEcho/) | 0.2.2 | Delay, Quick Note Score and detailed score | 0.2.2 source snapshot | `00dedb88cc3d` | [plugins/hamloecho](https://github.com/Hamec01/HPEQ/tree/plugins/hamloecho) |
| [HamloVerb](HamloVerb/) | 0.2.1 | Reverb, six characters and effects | Keep Dry insert option; old sounds preserved | `115db1322907` | [plugins/hamloverb](https://github.com/Hamec01/HPEQ/tree/plugins/hamloverb) |
| [HamloZastava](HamloZastava/) | 0.2.0 | Gate/expander: Clean/Drum/Duck/Rhythm | 0.2.0 S5 release snapshot | `ccc1de21b095` | [plugins/hamlozastava](https://github.com/Hamec01/HPEQ/tree/plugins/hamlozastava) |
| [HamloShirota](HamloShirota/) | 0.2.0 | Stereo width and spatial motion | 0.2.0 VST-only source snapshot | `ea316b0ea177` | [plugins/hamloshirota](https://github.com/Hamec01/HPEQ/tree/plugins/hamloshirota) |
| [HamloFader](HamloFader/) | 0.1.0 | Trim/Match/Ride/Motion | 0.1.0 F3; modern UI and library pending | `3223d0d5761d` | [plugins/hamlofader](https://github.com/Hamec01/HPEQ/tree/plugins/hamlofader) |
| [HamloRevolution](HamloRevolution/) | 0.3.0 | Rhythmic multi-effect and sample workbench | R12 source-aware Rise/Drum Fill; R10 spatial generation included | `8470eeb16d6c` | [plugins/hamlorevolution](https://github.com/Hamec01/HPEQ/tree/plugins/hamlorevolution) |

Fader is explicitly an unfinished development checkpoint; do not call it a
completed0.2.0 release. A published source snapshot does not imply that every
manual host/listening check or historical numerical target has passed. See the
individual README, handoff and reports for the exact acceptance/limitations.

Build a plug-in from its own folder using its CMake instructions and a local
JUCE8.0.12 checkout with the renderer patch. Generated build/dist/out files and
installed VST copies are not added to Git by this synchronization. Existing
tracked fixtures/assets remain exactly as committed by each source project.

Provenance index: [sync/2026-11-04.json](sync/2026-11-04.json).
The uncommitted Revolution R11 investigation is parked locally and excluded.
Verified R12 comes from `E:/HamloRevolutionTransitions`, not the original dirty
Revolution master. VST packages and private listening examples remain local.
Working handoff: [NEXT_STAGE.md](NEXT_STAGE.md). Keep source trees unmodified
here when verifying exact subtree equality; place registry metadata outside them.
