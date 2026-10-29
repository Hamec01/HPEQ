# HamloMag: genre presets and the preset screen

Owner (2026-10-28): presets per HPDG genre - Boom Bap, Trap, Techno, DnB - 50 each, "for kicks, snares, hats, drum
bus mastering, vocals, samples, everything", sorted into groups; a small screen like the CRT with the presets and
left / right arrows, at the same place on the left of every Hamlo plug-in; "the presets must be really good, not just
different values".

## 1. The preset screen

`Source/UI/PresetScreen.h` (the same file in HamloComp, HamloUdarnik and HamloRedWall): a green CRT at the design
rectangle 56, 20, 336 x 116 of the 1180 x 940 stage - top left on every page (the LCDs that sat there on HamloUdarnik
and HamloRedWall moved right of the nameplate). Lines: GENRE v and GROUP v (click: choose), the preset name between
the arrows (previous / next inside the genre and group, also the mouse wheel; click the name: a menu of every preset
of the filter, in genre / group sub-menus), the preset's note and its place (12 / 50). A `*` after the name: a knob
moved since the preset was loaded. The host's program list carries the same presets as "Genre / Group / Name"; the
loaded program is saved with the project. HPEQ's presets and the earlier ones are the genre "Factory".

## 2. Genres and groups

| Genre | Tempo (HPDG default) | Names come from the HPDG substyles |
|---|---|---|
| Boom Bap | 90 BPM | Classic, Dusty, Jazzy, (Boom Bap) Gold, (Russian) Underground, Lofi |
| Trap | 140 BPM | ATL, Dark, Cloud, Rage, Memphis, Luxury |
| Techno | 132 BPM | Peak (Time), Hypnotic, Minimal, Detroit, Dub, Acid, Hard |
| DnB | 174 BPM | Modern, Roller, Liquid, Neuro(funk), Jump-Up, Break(beat) |

Per genre: Kick 6, Snare / Clap 6, Hats / Cymbals 5, Percussion 4, Drum Bus 7, 808 / Bass 5, Vocals 5, Keys / Piano 4,
Samples / Melody 4, Master 4 = 50.

Rules used for the saturation recipes (these are design choices; measured depth is checked below):
- **Kick / bass:** SUB reserves a clean low band; PHONE adds harmonics for small-speaker translation. DYNAMICS
  weights the saturated share toward attacks (+) or bodies (-); its envelope timing is fixed in the DSP.
- **Parallel colour:** MIX below 100 % preserves dry signal alongside heavy distortion or sampler colour.
- **Genre character:** Boom Bap uses tape / vinyl and SP-1200 / S950 colour; Trap adds 808 harmonics and MPC60
  colour for Memphis recipes; Techno uses tape / iron for buses and rumble; DnB combines protected subs, attack
  emphasis and sampler colour on breaks. Tape SPEED changes model voicing, not tempo-synced release.
- Every recipe has its own purpose, written in its note on the screen; two presets of one group never share all
  values (checked).

## 3. HamloMag

Each recipe sets MODEL (TAPE warm and glued, TUBE even and round, IRON transformer weight and edge, GERMANIUM fuzz,
VINYL record colour, CLASSIC the plain curve), the SAMPLER where the genre lives on one (Boom Bap: SP-1200 knock,
S950 dusty loops; Trap / Memphis: the MPC60; DnB: the SP-1200 for the Amen), SUB on everything with a kick or a bass
in it (the sub stays clean while its top saturates), PHONE on the 808s and basses (so they read on phones),
DYNAMICS where the hits should bite (+) and BUMP / OVERLOAD / filters / wow / crackle as the style needs; the
masters run MATCH. **DRIVE is calibrated** on the material: `HamloMag_PresetLab calibrate` finds, per preset, the
DRIVE that gives the middle of its depth's band of the saturation's own share N = y(x) - 2 y(x / 2) (the linear
parts - the models' filters, wow, the samplers' rates - cancel), as dB re the output: GENTLE -36 .. -24, MEDIUM
-24 .. -14, HEAVY -14 .. -4; on its group's clips (the loudest 10 s, -18 dBFS RMS held to a -3 dBFS peak, mixes -14).
`check` judges the band, the peak (<= +3 dBFS), the level change (+-6 dB) and uniqueness per group.

Final release check (2026-10-29): **200 / 200 pass**, including depth, finite output, peak <= +3 dBFS,
RMS change within +/-6 dB and unique parameter values within each group across genres. Full table:
[genre_presets_check.md](reports/presets/genre_presets_check.md).

The inherited first 48 DRIVE records assumed monotonically increasing saturation. HamloLaw's bounded response
can turn back at high drive; calibration now scans for a target crossing and bisects in either direction,
preferring candidates within the existing output-level / peak constraints. The independent check still renders
the complete preset with its noises. Calibration material, depth bands and acceptance limits did not change.

The first full check passed 138 / 200, the first recipe repair 195 / 200, and the final bank 200 / 200.
63 recipes changed: Gentle tape recipes gained dry blend where DRIVE 0 was already too coloured; selected heavy
recipes gained more wet signal / lower SUB crossovers or manual gain; excessive output loss was compensated;
duplicate recipes gained distinct active tone, dynamics or model settings. Jump-Up Snare Bite lost OVERLOAD to
remain a Medium recipe at a usable level. Factory presets are unchanged. Exact changes and all intermediate
tables: [V2_REPORT.md](reports/v2/V2_REPORT.md), [recipe_changes.md](reports/presets/recipe_changes.md).

Reproduce (after building): `python -I Tools/run_genre_calibration.py <material-dir>`;
`python -I Tools/genre_presets.py Tools/genre_presets_cal.tsv`; rebuild;
`python -I Tools/run_genre_check.py <material-dir> docs/reports/presets/genre_presets_check.md`.
The check runs genres in parallel, while retaining full-bank duplicate detection.

## Sources

1. [Mastering The Mix: compression settings for a punchy mix](https://www.masteringthemix.com/blogs/learn/best-compression-settings-for-a-punchy-mix), [ProSoundWeb: five steps to punchy drum compression](https://www.prosoundweb.com/five-steps-to-punchy-drum-compression/), [Sonarworks: drum compression tips](https://www.sonarworks.com/blog/learn/killer-drum-tracks-compression-tips), [EDMProd: three ways to compress drums](https://www.edmprod.com/three-different-ways-to-compress-drums/)
2. [Sage Audio: how to mix trap](https://www.sageaudio.com/articles/how-to-mix-trap), [Violet Recording: mixing trap drums](https://violetrecording.com/how-to-mix-trap-drums/), [Rys Up Audio: trap vocals](https://rysupaudio.com/blogs/news/how-to-mix-trap-vocals), [Antares: vocals in dense trap beats](https://www.antarestech.com/blog/how-to-make-your-vocals-sit-in-dense-hip-hop-trap-beats)
3. [Attack Magazine: dark techno rumble](https://www.attackmagazine.com/technique/beat-dissected/dark-techno-rumble/), [MusicRadar: rumbling techno kick](https://musicradar.com/how-to/rumbling-techno-kick)
4. [MusicRadar: clean up and enhance a sampled breakbeat](https://musicradar.com/tuition/tech/how-to-clean-up-and-enhance-a-sampled-breakbeat-621106), [Gearspace: cracking those transients](https://gearspace.com/threads/cracking-those-transients.544384/)
