# HPEQ Factory Library (presets pass 3)

224 presets in three packs, embedded in the plug-in (the Presets button before Overlay):

- **Drums & Bass**: Kick, Snare, Hi Hat, Percussion, Drum Bus, Bass.
- **Vocals, Mastering & More**: Lead Vocal, Backing Vocal, Guitar, Keys, Synth, Mix Bus, Mastering (Pop, Rap,
  Trap, Techno, House / EDM, Rock / Indie: 7 masters each; Lo-Fi: 2).
- **Hamlo Showcase**: what HPEQ can do that a channel strip cannot: Layers (parallel and serial), HamloRevolution,
  Precision (regions and points with their own modules), Tubes (BRING LENIN / STALIN BACK).

**How the library is made.** `Tests/Tools/LabPresets.cpp` holds every design; `HPEQ_Lab presets --material <dir>
--out Assets/Presets/Factory` builds each preset, runs it on real material (48 kHz WAV: vocals, drum and bass
stems, unmastered mixdowns) and calibrates it, then writes the `.hpeq` files, the manifests and
`CALIBRATION.md` (every preset's measured loudness in / out, Output gain and true peak).

- **Track presets are level-matched**: the Output gain makes the integrated loudness (BS.1770) out equal to in
  on the category's material, so switching a preset on does not make the part quieter or louder.
- **Mastering presets** drive HamloRedWall (true peak, ceiling -1 dBTP) to a loudness target per genre and
  style (-14 LUFS streaming masters, -9.5 .. -7 LUFS loud masters), measured on unmastered mixdowns.
- **Vocal presets** carry the DE-ESS point (DETECT VOICE).

Dynamics thresholds still depend on the source level: trim the input if a compressor works too much or too
little. The material is the owner's and is not in the repository; CALIBRATION.md lists which clips were used.

**Two material sets.** The 197 presets of pass 2 were calibrated on the owner's first material, which is lost.
Pass 3 rebuilt a second set (`Tests/Tools/make_material_set2.py`: the owner's vocal takes and mixes plus stems
and one-shots from the owner's sample folders, each gained to the first set's loudness) and added 27 presets with
`--keep Assets/Presets/Factory`: presets already in the library keep their files byte for byte, only the new ones
are calibrated; CALIBRATION.md marks them (set 2).
