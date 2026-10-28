"""HamloUdarnik genre presets (docs/GENRE_PRESETS.md): the recipes, written out to Source/Plugin/GenrePresets.inc.

python Tools/genre_presets.py
The V2 engine delivers about SNAP / BODY in dB on every hit whatever the level (docs/UDARNIK_V2.md), so nothing
is calibrated: HamloUdarnik_PresetLab check plays each preset on its material and checks it stays finite, under
0 dBFS (PROTECT) and does what its name says (attack / body contrast in the right direction).
"""
from pathlib import Path

GENRES = ["Boom Bap", "Trap", "Techno", "DnB"]  # HPDG's genre engines; the names come from their substyles
# SPEED per genre: the faster the tempo (90 / 140 / 132 / 174 BPM), the shorter the window that counts as attack
GENRE_SPEED = {"Boom Bap": -0.04, "Trap": 0.0, "Techno": 0.03, "DnB": 0.06}
GROUPS = {  # group -> the material HamloUdarnik_PresetLab plays it on
    "Kick": "kick", "Snare / Clap": "snare", "Hats / Cymbals": "hats", "Percussion": "perc", "Drum Bus": "drums",
    "808 / Bass": "bass", "Vocals": "vocal", "Keys / Piano": "keys", "Samples / Melody": "samples", "Master": "mix",
}
STYLE = {"P": 0, "U": 1, "R": 2}  # PRAVDA classic, UDARNIK punch without louder peaks, PERESTROIKA (RISE / DECAY)
STYLE_NAME = {"P": "PRAVDA", "U": "UDARNIK", "R": "PERESTROIKA"}

# (group, name, SNAP dB, BODY dB, SPEED, mode, extras: rise / decay (PERESTROIKA), sensitivity, mix)
# Faster genres get a faster SPEED (only the click counts as attack) and shorter tails; boom bap keeps the body.
K, S_, H, P, B, BA, V, KE, SA, M = GROUPS.keys()
RECIPES = {
    "Boom Bap": [
        (K, "Classic Knock", 6, -2, 0.6, "U", {}), (K, "Dusty Long Boom", 2, 5, 0.5, "P", {}),
        (K, "Jazzy Soft Kick", -3, 2, 0.5, "P", {}), (K, "Gold Punch", 8, -3, 0.65, "U", {}),
        (K, "Underground Thud", 4, 3, 0.4, "P", {}), (K, "Lofi Dull Thump", -8, 4, 0.7, "P", {}),
        (S_, "Classic Crack", 7, -2, 0.65, "U", {}), (S_, "Dusty Fat Snare", 2, 6, 0.5, "P", {}),
        (S_, "Jazzy Brush Ghosts", 3, 0, 0.5, "P", {"sensitivity": 0.8}), (S_, "Gold Snap", 9, -4, 0.7, "U", {}),
        (S_, "Underground Room Snare", 3, 5, 0.45, "P", {}), (S_, "Lofi Soft Crack", -6, 3, 0.6, "P", {}),
        (H, "Classic Hat Tight", 2, -6, 0.5, "R", {"decay": 0.6}), (H, "Dusty Hat Soft", -4, 1, 0.5, "P", {}),
        (H, "Jazzy Ride Wash", 0, 4, 0.4, "P", {}), (H, "Gold Hat Tick", 5, -3, 0.7, "U", {}),
        (H, "Lofi Hat Short", -2, -8, 0.5, "R", {"decay": 0.5}),
        (P, "Classic Perc Snap", 5, -2, 0.6, "U", {}), (P, "Dusty Bongo Body", 2, 4, 0.5, "P", {}),
        (P, "Jazzy Shaker Soft", -3, 0, 0.5, "P", {}), (P, "Underground Perc Short", 4, -6, 0.5, "R", {"decay": 0.6}),
        (B, "Classic Bus Punch", 4, 0, 0.55, "U", {"mix": 0.7}), (B, "Dusty Bus Bloom", 1, 3, 0.45, "P", {}),
        (B, "Jazzy Bus Room", 0, 4, 0.4, "P", {"mix": 0.6}), (B, "Gold Bus Knock", 6, -2, 0.6, "U", {}),
        (B, "Underground Bus Tight", 3, -6, 0.5, "P", {}), (B, "Lofi Bus Soften", -5, 2, 0.6, "P", {}),
        (B, "Break Bus Snap", 5, -3, 0.6, "R", {"rise": 0.7}),
        (BA, "Classic Bass Pluck", 4, -1, 0.6, "U", {}), (BA, "Dusty Bass Bloom", 0, 3, 0.4, "P", {}),
        (BA, "Jazzy Upright Attack", 5, 0, 0.6, "P", {}), (BA, "Gold Bass Tight", 2, -5, 0.5, "R", {"decay": 0.7}),
        (BA, "Underground Sub Swell", -3, 4, 0.4, "P", {}),
        (V, "Classic Rap Articulate", 3, -2, 0.6, "U", {}), (V, "Dusty Vocal Dry", 0, -6, 0.5, "P", {}),
        (V, "Jazzy Vocal Soften", -4, 1, 0.6, "P", {}), (V, "Gold Adlib Punch", 5, -3, 0.6, "U", {"material": "backing"}),
        (V, "Underground Room Kill", 0, -10, 0.4, "P", {}),
        (KE, "Classic Piano Attack", 4, 0, 0.6, "P", {}), (KE, "Dusty Rhodes Bloom", -2, 4, 0.4, "P", {}),
        (KE, "Jazzy Keys Sustain", 0, 5, 0.4, "R", {"decay": 1.4}), (KE, "Gold Stab Tight", 3, -6, 0.5, "R", {"decay": 0.6}),
        (SA, "Classic Chop Clarity", 4, -3, 0.6, "U", {}), (SA, "Dusty Sample Soften", -4, 2, 0.5, "P", {}),
        (SA, "Jazzy Loop Bloom", 0, 4, 0.4, "P", {}), (SA, "Lofi Chop Tight", 2, -8, 0.5, "R", {"decay": 0.5}),
        (M, "Classic Master Punch", 2, 0, 0.6, "U", {"mix": 0.5}), (M, "Dusty Master Bloom", 0, 2, 0.4, "P", {"mix": 0.5}),
        (M, "Gold Master Knock", 3, -1, 0.6, "U", {"mix": 0.6}), (M, "Lofi Master Soft", -3, 1, 0.6, "P", {"mix": 0.5}),
    ],
    "Trap": [
        (K, "ATL Click", 8, -3, 0.75, "U", {}), (K, "Dark Kick Long", 2, 4, 0.5, "P", {}),
        (K, "Cloud Soft Kick", -4, 2, 0.5, "P", {}), (K, "Rage Slam", 10, -4, 0.8, "U", {}),
        (K, "Memphis Knock", 6, 0, 0.6, "R", {"rise": 0.7}), (K, "Luxury Tight Kick", 4, -6, 0.6, "R", {"decay": 0.6}),
        (S_, "ATL Clap Snap", 7, -2, 0.75, "U", {}), (S_, "Dark Snare Body", 2, 4, 0.5, "P", {}),
        (S_, "Cloud Clap Soft", -4, 3, 0.5, "P", {}), (S_, "Rage Crack", 10, -4, 0.8, "U", {}),
        (S_, "Memphis Rim Tight", 5, -6, 0.65, "R", {"decay": 0.6}), (S_, "Luxury Clap Air", 3, 3, 0.6, "P", {}),
        (H, "ATL Roll Crisp", 5, -4, 0.8, "U", {}), (H, "Dark Hat Short", 2, -8, 0.7, "R", {"decay": 0.5}),
        (H, "Cloud Hat Soft", -4, 0, 0.6, "P", {}), (H, "Rage Hat Bite", 8, -3, 0.85, "U", {}),
        (H, "Luxury Open Hat Tame", 0, -6, 0.5, "R", {"decay": 0.7}),
        (P, "ATL Perc Pop", 6, -2, 0.7, "U", {}), (P, "Dark Perc Short", 2, -8, 0.6, "R", {"decay": 0.5}),
        (P, "Rage Perc Smack", 9, -3, 0.8, "U", {}), (P, "Luxury Perc Air", 3, 2, 0.6, "P", {}),
        (B, "ATL Bus Punch", 4, -1, 0.65, "U", {"mix": 0.7}), (B, "Dark Bus Weight", 1, 3, 0.5, "P", {}),
        (B, "Cloud Bus Soft", -3, 2, 0.5, "P", {}), (B, "Rage Bus Slam", 7, -3, 0.75, "U", {}),
        (B, "Memphis Bus Tight", 3, -6, 0.6, "P", {}), (B, "Luxury Bus Snap", 5, -1, 0.7, "R", {"rise": 0.7}),
        (B, "Dark Bus Room Kill", 0, -10, 0.5, "P", {}),
        (BA, "ATL 808 Knock", 6, 0, 0.7, "U", {}), (BA, "Dark 808 Long", 0, 4, 0.4, "R", {"decay": 1.4}),
        (BA, "Rage 808 Punch", 9, -2, 0.75, "U", {}), (BA, "Memphis 808 Tight", 3, -6, 0.6, "R", {"decay": 0.6}),
        (BA, "Luxury 808 Soft Start", -4, 2, 0.5, "P", {}),
        (V, "ATL Vocal Articulate", 4, -2, 0.7, "U", {}), (V, "Dark Vocal Dry", 0, -8, 0.5, "P", {}),
        (V, "Cloud Vocal Soft", -4, 2, 0.6, "P", {}), (V, "Rage Vocal Punch", 6, -3, 0.75, "U", {}),
        (V, "Luxury Adlib Snap", 5, -4, 0.7, "U", {"material": "backing"}),
        (KE, "ATL Piano Attack", 4, -1, 0.65, "P", {}), (KE, "Dark Keys Bloom", -2, 4, 0.4, "P", {}),
        (KE, "Cloud Pad Swell", -6, 3, 0.4, "P", {}), (KE, "Luxury Bell Tight", 3, -6, 0.6, "R", {"decay": 0.6}),
        (SA, "ATL Loop Clarity", 4, -3, 0.65, "U", {}), (SA, "Dark Melody Bloom", 0, 4, 0.4, "P", {}),
        (SA, "Cloud Guitar Soft", -4, 2, 0.5, "P", {}), (SA, "Rage Synth Snap", 6, -4, 0.75, "U", {}),
        (M, "ATL Master Punch", 2, -1, 0.65, "U", {"mix": 0.5}), (M, "Dark Master Weight", 0, 2, 0.45, "P", {"mix": 0.5}),
        (M, "Rage Master Slam", 4, -2, 0.7, "U", {"mix": 0.6}), (M, "Luxury Master Clarity", 2, -2, 0.6, "U", {"mix": 0.4}),
    ],
    "Techno": [
        (K, "Peak Kick Punch", 6, -2, 0.65, "U", {}), (K, "Hypnotic Kick Tight", 3, -6, 0.6, "R", {"decay": 0.7}),
        (K, "Minimal Kick Click", 8, -4, 0.8, "U", {}), (K, "Detroit Kick Round", 2, 3, 0.5, "P", {}),
        (K, "Hard Kick Slam", 10, -3, 0.8, "U", {}), (K, "Dub Kick Long", 0, 5, 0.4, "R", {"decay": 1.4}),
        (S_, "Peak Clap Snap", 6, -2, 0.7, "U", {}), (S_, "Minimal Clap Tight", 4, -8, 0.7, "R", {"decay": 0.5}),
        (S_, "Detroit Clap Room", 2, 5, 0.5, "P", {}), (S_, "Acid Clap Bite", 8, -3, 0.75, "U", {}),
        (S_, "Hard Clap Slam", 10, -4, 0.8, "U", {}), (S_, "Dub Snare Tail", 0, 6, 0.4, "R", {"decay": 1.5}),
        (H, "Peak Hat Drive", 5, -3, 0.75, "U", {}), (H, "Hypnotic Hat Groove", 3, -4, 0.65, "P", {"sensitivity": 0.8}),
        (H, "Minimal Hat Tick", 6, -8, 0.85, "R", {"decay": 0.5}), (H, "Acid Hat Short", 3, -10, 0.7, "R", {"decay": 0.5}),
        (H, "Hard Ride Tame", -2, -4, 0.5, "P", {}),
        (P, "Peak Perc Pop", 6, -2, 0.7, "U", {}), (P, "Hypnotic Perc Loop", 3, -3, 0.6, "P", {}),
        (P, "Minimal Click Snap", 8, -6, 0.85, "R", {"rise": 0.6}), (P, "Detroit Conga Body", 2, 4, 0.5, "P", {}),
        (B, "Peak Bus Punch", 4, -1, 0.65, "U", {"mix": 0.7}), (B, "Hypnotic Bus Tight", 2, -6, 0.6, "P", {}),
        (B, "Minimal Bus Snap", 5, -3, 0.75, "U", {}), (B, "Detroit Bus Warm", 1, 3, 0.5, "P", {}),
        (B, "Hard Bus Slam", 7, -3, 0.75, "U", {}), (B, "Dub Bus Space", 0, 5, 0.4, "P", {"mix": 0.6}),
        (B, "Hard Bus Short", 3, -10, 0.6, "R", {"decay": 0.5}),
        (BA, "Peak Bass Pluck", 5, -2, 0.7, "U", {}), (BA, "Hypnotic Rumble Tight", 0, -6, 0.4, "R", {"decay": 0.7}),
        (BA, "Acid 303 Snap", 6, -2, 0.75, "U", {}), (BA, "Detroit Bass Bloom", 0, 3, 0.45, "P", {}),
        (BA, "Minimal Sub Soft", -4, 2, 0.5, "P", {}),
        (V, "Peak Vocal Chop Snap", 5, -3, 0.7, "U", {}), (V, "Hypnotic Whisper Dry", 0, -8, 0.5, "P", {}),
        (V, "Detroit Soul Vocal Soft", -3, 2, 0.6, "P", {}), (V, "Dub Vocal Tail", 0, 5, 0.4, "R", {"decay": 1.4}),
        (V, "Hard Shout Punch", 6, -2, 0.75, "U", {}),
        (KE, "Detroit Chord Stab", 5, -4, 0.7, "U", {}), (KE, "Dub Chord Tail", 0, 6, 0.4, "R", {"decay": 1.5}),
        (KE, "Peak Pad Swell", -6, 3, 0.4, "P", {}), (KE, "Minimal Keys Pluck", 4, -6, 0.7, "R", {"decay": 0.6}),
        (SA, "Acid Lead Snap", 6, -3, 0.75, "U", {}), (SA, "Hypnotic Loop Tight", 2, -6, 0.6, "P", {}),
        (SA, "Peak Synth Punch", 4, -1, 0.65, "U", {}), (SA, "Hard Noise Short", 3, -10, 0.6, "R", {"decay": 0.5}),
        (M, "Peak Club Punch", 2, -1, 0.65, "U", {"mix": 0.5}), (M, "Hypnotic Master Tight", 1, -2, 0.6, "P", {"mix": 0.5}),
        (M, "Detroit Master Warm", 0, 2, 0.45, "P", {"mix": 0.5}), (M, "Hard Master Slam", 4, -2, 0.7, "U", {"mix": 0.6}),
    ],
    "DnB": [
        (K, "Modern Kick Punch", 7, -3, 0.75, "U", {}), (K, "Roller Kick Tight", 4, -6, 0.7, "R", {"decay": 0.6}),
        (K, "Liquid Kick Soft", -3, 2, 0.55, "P", {}), (K, "Neuro Kick Slam", 10, -4, 0.85, "U", {}),
        (K, "Jump-Up Kick Knock", 8, -2, 0.75, "R", {"rise": 0.6}), (K, "Break Kick Body", 2, 3, 0.5, "P", {}),
        (S_, "Modern Snare Crack", 8, -3, 0.8, "U", {}), (S_, "Roller Snare Tight", 5, -6, 0.75, "R", {"decay": 0.6}),
        (S_, "Liquid Snare Air", 2, 3, 0.55, "P", {}), (S_, "Neuro Snare Slam", 10, -4, 0.85, "U", {}),
        (S_, "Jump-Up Snare Smack", 8, -1, 0.75, "U", {}), (S_, "Break Ghost Lift", 3, 0, 0.6, "P", {"sensitivity": 0.9}),
        (H, "Modern Shuffle Crisp", 5, -4, 0.85, "U", {}), (H, "Roller Hat Short", 3, -8, 0.8, "R", {"decay": 0.5}),
        (H, "Liquid Ride Wash", 0, 3, 0.5, "P", {}), (H, "Neuro Hat Bite", 8, -4, 0.9, "U", {}),
        (H, "Break Cymbal Tame", -3, -4, 0.6, "P", {}),
        (P, "Modern Perc Pop", 6, -3, 0.8, "U", {}), (P, "Liquid Shaker Soft", -3, 1, 0.6, "P", {}),
        (P, "Neuro Perc Smack", 9, -4, 0.85, "U", {}), (P, "Break Perc Short", 3, -8, 0.7, "R", {"decay": 0.5}),
        (B, "Modern Bus Punch", 5, -2, 0.75, "U", {"mix": 0.7}), (B, "Roller Bus Tight", 3, -6, 0.7, "P", {}),
        (B, "Liquid Bus Soft", -2, 2, 0.55, "P", {}), (B, "Neuro Bus Slam", 7, -3, 0.8, "U", {}),
        (B, "Amen Break Snap", 6, -2, 0.75, "R", {"rise": 0.6}), (B, "Break Room Kill", 2, -10, 0.6, "P", {}),
        (B, "Jump-Up Bus Knock", 6, -1, 0.75, "U", {}),
        (BA, "Modern Reese Bite", 4, -2, 0.7, "U", {}), (BA, "Roller Sub Tight", 0, -6, 0.5, "R", {"decay": 0.7}),
        (BA, "Liquid Bass Bloom", -2, 3, 0.5, "P", {}), (BA, "Neuro Bass Snap", 6, -3, 0.8, "U", {}),
        (BA, "Jump-Up Wobble Punch", 7, -2, 0.75, "U", {}),
        (V, "Modern MC Articulate", 5, -3, 0.75, "U", {}), (V, "Liquid Vocal Soft", -3, 2, 0.6, "P", {}),
        (V, "Roller Vocal Dry", 0, -8, 0.55, "P", {}), (V, "Jump-Up MC Punch", 7, -3, 0.8, "U", {}),
        (V, "Liquid Backing Air", -2, 3, 0.5, "P", {"material": "backing"}),
        (KE, "Liquid Rhodes Bloom", -2, 4, 0.5, "P", {}), (KE, "Liquid Piano Attack", 4, 0, 0.65, "P", {}),
        (KE, "Modern Stab Tight", 4, -6, 0.75, "R", {"decay": 0.6}), (KE, "Roller Pad Swell", -6, 3, 0.45, "P", {}),
        (SA, "Liquid Strings Swell", -6, 4, 0.45, "P", {}), (SA, "Neuro Synth Snap", 6, -4, 0.8, "U", {}),
        (SA, "Break Loop Clarity", 4, -3, 0.7, "U", {}), (SA, "Jump-Up Hoover Punch", 5, -2, 0.75, "U", {}),
        (M, "Modern Master Punch", 2, -1, 0.7, "U", {"mix": 0.5}), (M, "Liquid Master Warm", 0, 2, 0.5, "P", {"mix": 0.5}),
        (M, "Neuro Master Slam", 4, -2, 0.75, "U", {"mix": 0.6}), (M, "Roller Master Tight", 1, -2, 0.65, "P", {"mix": 0.5}),
    ],
}


def fmt(v):
    return ("%.2f" % v).rstrip("0").rstrip(".")


def main():
    out = ["// Generated by Tools/genre_presets.py - edit the recipes there. docs/GENRE_PRESETS.md",
           "// name, mix, values, genre, group, note, material"]
    names = set()
    for genre in GENRES:
        recipes = RECIPES[genre]
        assert len(recipes) == 50, (genre, len(recipes))
        for group, name, snap, body, speed, style, extra in recipes:
            key = f"{genre}/{group}/{name}"
            assert key not in names, key
            names.add(key)
            speed = round(min(0.95, max(0.05, speed + GENRE_SPEED[genre])), 2)
            values = {"attackDb": float(snap), "sustainDb": float(body), "speed": float(speed), "style": STYLE[style]}
            note = f"{STYLE_NAME[style]}, snap {snap:+d} dB, body {body:+d} dB, speed {round(speed * 100)} %"
            for k in ("rise", "decay"):
                if k in extra:
                    values[k] = float(extra[k])
                    note += f", {k} x{fmt(extra[k])}"
            if "sensitivity" in extra:
                values["sensitivity"] = float(extra["sensitivity"])
                note += f", sens {round(extra['sensitivity'] * 100)} %"
            mix = extra.get("mix", 1.0)
            if mix < 1.0:
                note += f", mix {round(mix * 100)} %"
            vals = ", ".join(f'{{ "{k}", {fmt(v) if isinstance(v, float) else v} }}' for k, v in values.items())
            material = extra.get("material", GROUPS[group])
            out.append(f'{{ "{name}", {float(mix)}f, {{ {vals} }}, "{genre}", "{group}", "{note}", "{material}" }},')
    Path(__file__).resolve().parent.parent.joinpath("Source/Plugin/GenrePresets.inc").write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(names)} genre presets")


if __name__ == "__main__":
    main()
