"""Hand-authored musical recipes, expanded by tempo and genre voicing.

Calibration only changes MIX and Output. A recipe's character, timing and EQ
remain an explicit musical decision. Run with an optional calibration JSON.
"""
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CHARACTERS = ("Legacy", "Clean", "Warm", "Velvet", "Crisp", "Deep", "Bloom")
# name, group, material, engine, decay in beats, predelay in beats, size,
# distance, width, shape, depth, optional shimmer semitones
RECIPES = [
    ("Close Booth", "Vocal", "vocal_lead1", 1, .8, .03125, .55, .12, .85, 0, "Gentle", None),
    ("Warm Lead", "Vocal", "vocal_lead2", 2, 2.3, .0625, .85, .28, 1, 0, "Medium", None),
    ("Velvet Phrase", "Vocal", "vocal_lead3", 3, 3.1, .125, 1, .40, 1.1, 0, "Medium", None),
    ("Bright Plate", "Vocal", "vocal_lead1", 4, 1.9, .0625, .8, .25, 1, 0, "Medium", None),
    ("Distant Reply", "Vocal", "vocal_back1", 5, 4.2, .125, 1.2, .88, 1.2, 0, "Heavy", None),
    ("Breath Halo", "Vocal", "vocal_back2", 6, 5, .0625, 1.15, .7, 1.25, 0, "Medium", 12),
    ("Hook Chamber", "Vocal", "vocal_lead2", 1, 1.5, .125, .78, .32, .95, 0, "Gentle", None),
    ("Backing Silk", "Vocal", "vocal_back3", 3, 3.5, .03125, 1.1, .65, 1.3, 0, "Medium", None),
    ("Cut Phrase", "Vocal", "vocal_lead3", 4, 2, .0625, .7, .3, 1.05, 1, "Medium", None),
    ("Reverse Adlib", "Vocal", "vocal_back1", 5, 2.7, 0, 1, .75, 1.2, 3, "Heavy", None),
    ("Snare Snap", "Drums", "snare", 4, .7, .015625, .45, .12, 1, 0, "Medium", None),
    ("Snare Steel", "Drums", "snare", 4, 1.3, .03125, .75, .25, 1.15, 0, "Medium", None),
    ("Snare Velvet", "Drums", "snare", 3, 1.7, .0625, .8, .35, 1.1, 0, "Medium", None),
    ("Gated Backbeat", "Drums", "snare", 4, 2.1, .03125, .8, .32, 1.2, 1, "Heavy", None),
    ("Snare Uplift", "Drums", "snare", 4, 1.8, 0, .9, .5, 1.15, 2, "Heavy", None),
    ("Kick Room", "Drums", "kick", 1, .45, 0, .35, .1, .5, 0, "Gentle", None),
    ("Hat Air", "Drums", "hihat", 1, .65, .015625, .5, .2, 1.2, 0, "Gentle", None),
    ("Metal Wash", "Drums", "hihat", 3, 2.4, .03125, .95, .6, 1.3, 0, "Medium", None),
    ("Percussion Yard", "Drums", "perc", 2, 1.1, .0625, .7, .4, 1.05, 0, "Medium", None),
    ("Break Room", "Drums", "drumbus1", 1, .85, .03125, .65, .2, 1, 0, "Gentle", None),
    ("Drum Warehouse", "Drums", "drumbus1", 5, 2.8, .0625, 1.2, .8, 1.2, 0, "Medium", None),
    ("Reverse Break", "Drums", "drumbus1", 4, 1.5, 0, .8, .45, 1.1, 3, "Heavy", None),
    ("Piano Wood", "Samples / Keys", "keys", 2, 2.3, .03125, .85, .35, 1, 0, "Medium", None),
    ("Piano Glass", "Samples / Keys", "keys2", 1, 2.8, .0625, 1.05, .4, 1.1, 0, "Medium", None),
    ("Rhodes Silk", "Samples / Keys", "keys", 3, 3.4, .0625, 1, .5, 1.15, 0, "Medium", None),
    ("Chopped Room", "Samples / Keys", "keys2", 1, .9, .015625, .55, .2, .9, 0, "Gentle", None),
    ("Dust Chamber", "Samples / Keys", "guitar", 2, 1.7, .03125, .7, .4, 1, 0, "Gentle", None),
    ("Guitar Plate", "Samples / Keys", "guitar", 4, 2.4, .0625, .85, .35, 1.1, 0, "Medium", None),
    ("Keys Horizon", "Samples / Keys", "keys2", 5, 4.8, .125, 1.3, .85, 1.2, 0, "Heavy", None),
    ("Melody Bloom", "Samples / Keys", "keys", 6, 5.5, .0625, 1.2, .65, 1.25, 0, "Heavy", None),
    ("Reverse Chops", "Samples / Keys", "guitar", 2, 2.5, 0, .9, .5, 1.1, 3, "Heavy", None),
    ("Piano Octave", "Samples / Keys", "keys2", 3, 4.2, .125, 1.1, .65, 1.2, 0, "Medium", 12),
    ("Pluck Space", "Synth", "synth", 1, 1.2, .03125, .6, .2, 1, 0, "Gentle", None),
    ("Lead Plate", "Synth", "synth", 4, 2.1, .0625, .85, .4, 1.1, 0, "Medium", None),
    ("Warm Arp", "Synth", "synth", 2, 2.7, .125, .9, .5, 1.1, 0, "Medium", None),
    ("Pad Velvet", "Synth", "synth", 3, 5.2, .0625, 1.2, .65, 1.3, 0, "Heavy", None),
    ("Deep Chord", "Synth", "synth", 5, 6.5, .125, 1.35, .9, 1.25, 0, "Heavy", None),
    ("Bloom Fifth", "Synth", "synth", 6, 6, .0625, 1.25, .7, 1.3, 0, "Heavy", 7),
    ("Pulse Gate", "Synth", "synth", 4, 2, .03125, .75, .35, 1.15, 1, "Medium", None),
    ("Reverse Sequence", "Synth", "synth", 5, 3.3, 0, 1.05, .7, 1.25, 3, "Heavy", None),
    ("Endless Sky", "Atmosphere / FX", "keys2", 6, 12, .125, 1.5, .9, 1.3, 0, "Heavy", 12),
    ("Low Orbit", "Atmosphere / FX", "synth", 5, 8, .0625, 1.4, .95, 1.1, 0, "Heavy", -12),
    ("Silver Cloud", "Atmosphere / FX", "vocal_back2", 3, 9, .125, 1.35, .85, 1.3, 0, "Heavy", 24),
    ("Warm Fog", "Atmosphere / FX", "guitar", 2, 7, .0625, 1.3, .8, 1.15, 0, "Heavy", None),
    ("Distant Station", "Atmosphere / FX", "keys", 5, 10, .25, 1.5, 1, 1.25, 0, "Heavy", None),
    ("Velvet Lift", "Atmosphere / FX", "vocal_back3", 3, 6, 0, 1.3, .8, 1.25, 2, "Heavy", None),
    ("Reverse Horizon", "Atmosphere / FX", "keys2", 6, 8.5, 0, 1.4, .85, 1.3, 3, "Heavy", None),
    ("Fractured Room", "Atmosphere / FX", "perc", 1, 3.8, .125, 1.1, .6, 1.2, 1, "Medium", None),
    ("Octave Rain", "Atmosphere / FX", "hihat", 4, 5, .0625, 1.1, .7, 1.3, 0, "Medium", 12),
    ("Afterglow", "Atmosphere / FX", "synth", 6, 14, .125, 1.45, .92, 1.25, 0, "Heavy", None),
]
GENRES = [
    ("Boom Bap", "bb", 90, .42, 1.3, 8500, ["Classic", "Dusty", "Jazzy", "Gold", "Underground", "Lofi"]),
    ("Trap", "trap", 140, .62, .85, 14000, ["ATL", "Dark", "Cloud", "Rage", "Memphis", "Luxury"]),
    ("Techno", "techno", 132, .52, 1.05, 11000, ["Peak", "Hypnotic", "Minimal", "Detroit", "Dub", "Acid", "Hard"]),
    ("DnB", "dnb", 174, .72, .75, 16000, ["Modern", "Roller", "Liquid", "Neuro", "Jump-Up", "Break"]),
]

def recipes():
    out = []
    for genre, slug, bpm, hf, bass, cut, styles in GENRES:
        for i, (name, group, material, engine, beats, prebeats, size, distance, width, shape, depth, pitch) in enumerate(RECIPES):
            beat = 60 / bpm
            params = dict(engine=engine, predelayMs=1000 * beat * prebeats,
                decaySeconds=min(30, max(.1, beat * beats)), size=size, distance=distance,
                width=width, bassMult=bass, hfDecayRatio=hf, highCutHz=cut,
                lowCutHz=180 if group == "Vocal" else (300 if material == "kick" else 100),
                attack=.55 if engine == 6 else (.35 if engine in (3, 5) else .05),
                earlyDiffusion=.8 if engine in (3, 4) else .6,
                lateDiffusion=.9 if engine in (3, 4, 6) else .72,
                modDepth=.03 if engine == 4 else (.35 if engine in (3, 6) else .12),
                modRate=1 / (beat * (8 if engine in (3, 6) else 4)),
                earlyDb=-9 if group == "Atmosphere / FX" else (-3 if engine == 5 else 0),
                lateDb=0, timeShape=shape, shapeTimeMs=min(2000, 1000 * beat * (2 if group == "Atmosphere / FX" else .5)),
                gateThresholdDb=-36, gateReleaseMs=beat * 40,
                duck=.42 if group == "Vocal" else (.15 if group == "Drums" else .08),
                duckAttackMs=3 if group == "Drums" else 8, duckReleaseMs=beat * 500,
                shimmerAmount=0 if pitch is None else (.3 if group == "Atmosphere / FX" else .15),
                shimmerPitch=12 if pitch is None else pitch, shimmerFeedback=.35,
                decayEq2On=1, decayEq2Hz=350, decayEq2Value=-.3 if genre in ("Trap", "DnB") else -.15,
                decayEq2Q=.7, postEq1On=1, postEq1Type=3,
                postEq1Hz=250 if group == "Vocal" else (350 if material == "kick" else 90),
                postEq1Q=.707, outputDb=0, freeze=0)
            if shape in (1, 2):
                params["shapeSync"] = 10 if group == "Atmosphere / FX" else 7
            if group == "Vocal":
                params.update(postEq5On=1, postEq5Hz=6500, postEq5Value=-3, postEq5Q=1)
            ident = f"{slug}-{i+1:02d}"
            note = f"{CHARACTERS[engine]} {beats:g}-beat tail, {bpm} BPM; {material.replace('_', ' ')}"
            if shape: note += "; " + ("", "gated", "rising swell", "reverse wet fragments")[shape]
            if pitch is not None: note += f"; shimmer {pitch:+d} st"
            out.append(dict(id=ident, name=f"{slug.upper()} {styles[i % len(styles)]} {name}", genre=genre,
                group=group, material=material, depth=depth, tempo=bpm, note=note,
                mix={"Gentle": .12, "Medium": .25, "Heavy": .45}[depth], params=params))
    return out

def main():
    bank = recipes()
    calibration = json.loads(pathlib.Path(sys.argv[1]).read_text()) if len(sys.argv) > 1 else {}
    entries = calibration.get("presets", {})
    for preset in bank:
        if preset["id"] in entries:
            row = entries[preset["id"]]
            preset["mix"] = row["mix"]
            preset["params"]["outputDb"] = row["outputDb"]
    (ROOT / "Tools" / "genre_recipes.json").write_text(json.dumps(bank, indent=2) + "\n", encoding="utf-8")
    text = ["// Generated by Tools/genre_presets.py. Musical recipes remain in that source."]
    quote = lambda x: json.dumps(x, ensure_ascii=True)
    for p in bank:
        values = ", ".join("{ " + quote(k) + ", " + format(v, ".10g") + " }" for k, v in p["params"].items())
        metadata = ", ".join(quote(p[k]) for k in ("id", "genre", "group", "note", "material", "depth"))
        text.append("{ " + quote(p["name"]) + ", " + format(p["mix"], ".10f") + "f, { " + values + " }, " + metadata + ", " + str(p["tempo"]) + " },")
    (ROOT / "Source" / "Core" / "GenrePresets.inc").write_text("\n".join(text) + "\n", encoding="utf-8")
    print(f"Wrote {len(bank)} recipes; {len(entries)} calibration entries applied.")

if __name__ == "__main__":
    main()
