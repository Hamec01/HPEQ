"""HamloMag genre presets (docs/GENRE_PRESETS.md): the recipes, written out to Source/Plugin/GenrePresets.inc.

python Tools/genre_presets.py [calibration.tsv]
calibration.tsv (from `HamloMag_PresetLab calibrate`): "CAL<TAB>genre/group/name<TAB>drive dB<TAB>depth" lines;
without it DRIVE is the depth's first guess. Every other value is designed here.
"""
import sys
from pathlib import Path

GENRES = {"Boom Bap": 90.0, "Trap": 140.0, "Techno": 132.0, "DnB": 174.0}  # HPDG's engines; names from their substyles
GROUPS = {
    "Kick": "kick", "Snare / Clap": "snare", "Hats / Cymbals": "hats", "Percussion": "perc", "Drum Bus": "drums",
    "808 / Bass": "bass", "Vocals": "vocal", "Keys / Piano": "keys", "Samples / Melody": "samples", "Master": "mix",
}
MODEL = {"CLASSIC": 0, "TAPE": 1, "TUBE": 2, "IRON": 3, "GERMANIUM": 4, "VINYL": 5}
SAMPLER = {"": 0, "SP": 1, "MPC": 2, "S950": 3}
SAMPLER_NAME = {1: "SP-1200", 2: "MPC60", 3: "S950"}
DEPTH = {"g": "Gentle", "m": "Medium", "h": "Heavy"}
GUESS = {"g": 4.0, "m": 10.0, "h": 20.0}
# extras keys: tone, lc (low cut Hz), hc (high cut Hz), harm, eo (even / odd), bias, speed, wow, hiss, crackle, hum,
# phone, sub (Hz), dyn, punish, thump, steep, os, mix, match, auto, output (dB)
KEYS = {"tone": "tone", "lc": "satLowCutHz", "hc": "satHighCutHz", "harm": "harmonics", "eo": "evenOdd", "bias": "tapeBias",
        "speed": "speed", "wow": "wow", "hiss": "hiss", "crackle": "crackle", "hum": "hum", "phone": "phoneBass",
        "sub": "subProtectHz", "dyn": "dynamics", "punish": "punish", "thump": "thump", "steep": "steep", "os": "oversampling",
        "match": "match", "output": "trimDb", "auto": "autoGain"}
# Every preset starts from no noises / wow, 2x, auto gain on, harmonics / bias 50 %; recipes can override gain.
BASE = {"wow": 0.0, "hiss": 0.0, "crackle": 0.0, "hum": 0.0}

K, S_, H, P, B, BA, V, KE, SA, M = GROUPS.keys()
R = lambda group, name, depth, model, sampler="", **x: (group, name, depth, model, sampler, x)
RECIPES = {
    "Boom Bap": [
        R(K, "Classic SP Knock", "m", "TAPE", "SP", sub=50, thump=1, dyn=0.4),
        R(K, "Dusty Tape Thump", "m", "TAPE", speed=1, thump=1, tone=-0.2, bias=0.4),
        R(K, "Jazzy Warm Kick", "g", "TUBE", eo=0.5, tone=-0.1),
        R(K, "Gold Iron Punch", "m", "IRON", dyn=0.6, sub=60),
        R(K, "Underground Grit Kick", "h", "GERMANIUM", "SP", sub=70, dyn=0.5),
        R(K, "Lofi Vinyl Kick", "m", "VINYL", "S950", crackle=0.1, tone=-0.3),
        R(S_, "Classic SP Crack", "m", "TAPE", "SP", dyn=0.5),
        R(S_, "Dusty Fat Snare", "m", "TAPE", speed=1, bias=0.35, tone=-0.15),
        R(S_, "Jazzy Brush Warmth", "g", "TUBE", eo=0.6, tone=-0.2),
        R(S_, "Gold Iron Snap", "m", "IRON", dyn=0.7, tone=0.1),
        R(S_, "Underground Dirt Snare", "h", "GERMANIUM", "SP", punish=1, mix=0.7),
        R(S_, "Lofi S950 Snare", "m", "TAPE", "S950", speed=2, tone=-0.3),
        R(H, "Classic Hat Shine", "g", "TAPE", tone=0.1, lc=200, mix=0.35),
        R(H, "Dusty Hat Dull", "m", "TAPE", speed=2, bias=0.3, hc=9000),
        R(H, "Jazzy Ride Air", "g", "TUBE", eo=0.4, lc=300),
        R(H, "Gold Hat Grit", "m", "GERMANIUM", lc=400, mix=0.6),
        R(H, "Lofi SP Hats", "h", "CLASSIC", "SP", lc=300),
        R(P, "Classic Perc Tape", "g", "TAPE", lc=100, mix=0.3),
        R(P, "Dusty Bongo Warm", "m", "TUBE", eo=0.5, tone=-0.2),
        R(P, "Jazzy Shaker Soft", "g", "TAPE", speed=1, hc=10000, mix=0.28),
        R(P, "Underground Perc Crunch", "h", "GERMANIUM", "SP", mix=0.7, output=3),
        R(B, "Classic SP Bus", "m", "TAPE", "SP", sub=60, mix=0.8),
        R(B, "Dusty Tape Bus", "m", "TAPE", speed=1, thump=1, wow=0.05),
        R(B, "Jazzy Bus Glow", "g", "TUBE", eo=0.4),
        R(B, "Gold Iron Bus", "m", "IRON", sub=45, dyn=0.25),
        R(B, "Underground Crush Bus", "h", "GERMANIUM", "SP", sub=35, mix=0.8, auto=0, output=-6),
        R(B, "Lofi Vinyl Bus", "m", "VINYL", "S950", crackle=0.15, tone=-0.25),
        R(B, "Knock MPC Bus", "m", "IRON", "MPC", dyn=0.5),
        R(BA, "Classic Bass Tape", "m", "TAPE", sub=60, phone=0.3),
        R(BA, "Dusty Upright Tube", "g", "TUBE", eo=0.5, phone=0.2),
        R(BA, "Jazzy Bass Warm", "g", "TAPE", speed=1, bias=0.45),
        R(BA, "Gold Iron Bass", "m", "IRON", sub=70, phone=0.4),
        R(BA, "Underground Sub Grit", "h", "GERMANIUM", sub=40, phone=0.5),
        R(V, "Classic Rap Tape", "g", "TAPE", lc=90, mix=0.5),
        R(V, "Dusty Vocal Tube", "m", "TUBE", eo=0.5, lc=100),
        R(V, "Jazzy Vocal Iron", "g", "IRON", lc=90, tone=-0.1),
        R(V, "Gold Adlib Grit", "h", "GERMANIUM", lc=150, mix=0.6, material="backing"),
        R(V, "Lofi Phone Vocal", "h", "CLASSIC", "S950", lc=400, hc=4000),
        R(KE, "Classic Rhodes Tape", "g", "TAPE", wow=0.08),
        R(KE, "Dusty Piano Vinyl", "m", "VINYL", crackle=0.12, tone=-0.2),
        R(KE, "Jazzy Keys Tube", "g", "TUBE", eo=0.6),
        R(KE, "Gold Keys S950", "m", "TAPE", "S950", wow=0.1),
        R(SA, "Classic Soul Chop SP", "m", "TAPE", "SP", wow=0.1, lc=60),
        R(SA, "Dusty Vinyl Sample", "m", "VINYL", "S950", crackle=0.2, wow=0.12, hiss=0.05),
        R(SA, "Jazzy Loop Warmth", "g", "TUBE", eo=0.5, wow=0.05),
        R(SA, "Lofi Cassette Loop", "h", "TAPE", "MPC", speed=2, wow=0.35, hiss=0.12, hc=9000),
        R(M, "Classic Boom Bap Master", "g", "TAPE", sub=40, match=1),
        R(M, "Dusty Tape Master", "g", "TAPE", speed=1, wow=0.04, match=1),
        R(M, "Gold Iron Master", "m", "IRON", sub=50, match=1),
        R(M, "Lofi Vinyl Master", "m", "VINYL", crackle=0.1, tone=-0.15, match=1),
    ],
    "Trap": [
        R(K, "ATL Kick Click", "m", "IRON", sub=60, dyn=0.7, tone=0.15),
        R(K, "Dark Kick Weight", "m", "TUBE", sub=50, eo=0.4),
        R(K, "Cloud Kick Soft", "g", "TAPE", tone=-0.1),
        R(K, "Rage Kick Distort", "h", "GERMANIUM", sub=70, punish=1),
        R(K, "Memphis MPC Kick", "m", "TAPE", "MPC", sub=60, thump=1),
        R(K, "Luxury Kick Polish", "g", "TUBE", sub=50, eo=0.3, tone=0.1),
        R(S_, "ATL Clap Bite", "m", "IRON", dyn=0.6, tone=0.15),
        R(S_, "Dark Snare Body", "m", "TUBE", eo=0.5, tone=-0.1),
        R(S_, "Cloud Clap Air", "g", "TAPE", lc=150, mix=0.35),
        R(S_, "Rage Snare Smash", "h", "GERMANIUM", punish=1, mix=0.8, output=4),
        R(S_, "Memphis Rim Dirt", "m", "CLASSIC", "MPC", dyn=0.5),
        R(S_, "Luxury Clap Shine", "g", "TUBE", eo=0.3, tone=0.2, lc=200),
        R(H, "ATL Hat Crisp", "g", "IRON", lc=500, tone=0.1),
        R(H, "Dark Hat Tape", "m", "TAPE", speed=1, hc=12000, lc=300),
        R(H, "Rage Hat Fizz", "h", "GERMANIUM", lc=600, mix=0.6),
        R(H, "Memphis Hat MPC", "m", "TAPE", "MPC", lc=300),
        R(H, "Luxury Hat Silk", "g", "TUBE", eo=0.4, lc=500),
        R(P, "ATL Perc Pop", "m", "IRON", dyn=0.5, lc=150),
        R(P, "Dark Perc Tape", "g", "TAPE", speed=1, mix=0.3),
        R(P, "Rage Perc Crunch", "h", "GERMANIUM", mix=0.7, lc=200),
        R(P, "Luxury Perc Glow", "g", "TUBE", eo=0.4),
        R(B, "ATL Drum Glue", "g", "TAPE", sub=60),
        R(B, "Dark Bus Weight", "m", "TUBE", sub=60, eo=0.4),
        R(B, "Cloud Bus Haze", "g", "TAPE", speed=1, wow=0.05, tone=-0.1),
        R(B, "Rage Bus Distort", "h", "GERMANIUM", sub=40, punish=1, mix=0.85, auto=0, output=-12),
        R(B, "Memphis Bus Dirt", "m", "CLASSIC", "MPC", sub=70),
        R(B, "Luxury Bus Polish", "g", "IRON", sub=50, tone=0.1),
        R(B, "Dark Parallel Smash", "h", "IRON", sub=45, punish=1, mix=0.7, auto=0, output=-10),
        R(BA, "ATL 808 Phone", "m", "TUBE", sub=55, phone=0.8),
        R(BA, "Dark 808 Weight", "m", "TAPE", sub=50, phone=0.6, thump=1),
        R(BA, "Rage 808 Distort", "h", "GERMANIUM", sub=45, phone=0.5, punish=1),
        R(BA, "Memphis 808 Grit", "m", "IRON", sub=65, phone=0.7),
        R(BA, "Luxury Sub Clean", "g", "TUBE", sub=80, phone=0.4, eo=0.3),
        R(V, "ATL Vocal Edge", "m", "IRON", lc=120, tone=0.1),
        R(V, "Dark Vocal Tape", "m", "TAPE", lc=100, speed=1),
        R(V, "Cloud Vocal Air", "g", "TUBE", eo=0.4, lc=120, tone=0.1),
        R(V, "Rage Vocal Distort", "h", "GERMANIUM", lc=200, punish=1, mix=0.7, output=2),
        R(V, "Luxury Adlib Shine", "g", "TUBE", eo=0.3, lc=150, material="backing"),
        R(KE, "ATL Piano Warm", "g", "TAPE", tone=-0.05),
        R(KE, "Dark Keys Tube", "m", "TUBE", eo=0.5, tone=-0.15),
        R(KE, "Cloud Pad Tape", "g", "TAPE", speed=1, wow=0.12),
        R(KE, "Luxury Bell Iron", "g", "IRON", tone=0.1),
        R(SA, "ATL Loop Glue", "g", "TAPE", wow=0.04),
        R(SA, "Dark Melody Vinyl", "m", "VINYL", crackle=0.08, tone=-0.2),
        R(SA, "Cloud Guitar Tape", "g", "TAPE", speed=1, wow=0.15),
        R(SA, "Rage Synth Fuzz", "h", "GERMANIUM", punish=1, mix=0.7),
        R(M, "ATL Trap Master", "g", "TUBE", sub=50, match=1),
        R(M, "Dark Master Weight", "m", "TAPE", sub=40, thump=1, match=1),
        R(M, "Rage Loud Master", "h", "IRON", sub=45, match=1),
        R(M, "Luxury Clean Master", "g", "TUBE", sub=45, eo=0.3, match=1),
    ],
    "Techno": [
        R(K, "Peak Kick Drive", "m", "IRON", sub=55, dyn=0.65),
        R(K, "Hypnotic Kick Tape", "g", "TAPE", sub=50, speed=0),
        R(K, "Minimal Kick Click", "g", "IRON", sub=70, dyn=0.8, tone=0.15),
        R(K, "Detroit Kick Warm", "m", "TUBE", sub=55, eo=0.5),
        R(K, "Hard Kick Distort", "h", "GERMANIUM", sub=60, punish=1, os=3, output=3),
        R(K, "Dub Kick Round", "g", "TAPE", sub=45, thump=1, tone=-0.15),
        R(S_, "Peak Clap Drive", "m", "IRON", dyn=0.5),
        R(S_, "Minimal Clap Tick", "g", "TUBE", lc=250, tone=0.1),
        R(S_, "Detroit Clap Tape", "m", "TAPE", speed=1),
        R(S_, "Acid Clap Fuzz", "h", "GERMANIUM", mix=0.7, os=3),
        R(S_, "Hard Clap Smash", "h", "IRON", punish=1, os=3, output=3),
        R(S_, "Dub Snare Space", "g", "TAPE", speed=1, wow=0.08, tone=-0.2, mix=0.38),
        R(H, "Peak Hat Drive", "m", "IRON", lc=400),
        R(H, "Hypnotic Hat Tape", "g", "TAPE", lc=300, speed=0, mix=0.36),
        R(H, "Minimal Hat Air", "g", "TUBE", lc=600, tone=0.1),
        R(H, "Acid Hat Fizz", "h", "GERMANIUM", lc=500, mix=0.7, os=3),
        R(H, "Hard Ride Grit", "m", "IRON", lc=400, dyn=0.4),
        R(P, "Peak Perc Iron", "m", "IRON", lc=150),
        R(P, "Hypnotic Perc Loop", "g", "TAPE", speed=2, mix=0.32),
        R(P, "Minimal Click Shine", "g", "TUBE", lc=300, tone=0.15),
        R(P, "Detroit Conga Warm", "g", "TUBE", eo=0.5, tone=-0.1),
        R(B, "Peak Drum Drive", "m", "IRON", sub=45, dyn=0.3),
        R(B, "Hypnotic Bus Tape", "g", "TAPE", sub=60, speed=1),
        R(B, "Minimal Bus Clean", "g", "TUBE", sub=60, eo=0.3),
        R(B, "Detroit Bus Tape", "m", "TAPE", sub=60, speed=1, thump=1),
        R(B, "Hard Bus Distort", "h", "GERMANIUM", sub=35, punish=1, mix=0.8, os=3, auto=0, output=-12),
        R(B, "Dub Rumble Group", "m", "TAPE", sub=40, speed=1, tone=-0.2),
        R(B, "Hard Parallel Iron", "h", "IRON", sub=40, punish=1, mix=0.65, tone=0.1, auto=0, output=-12),
        R(BA, "Peak Bass Drive", "m", "IRON", sub=60, phone=0.5),
        R(BA, "Hypnotic Rumble Tape", "g", "TAPE", sub=45, speed=1),
        R(BA, "Acid 303 Fuzz", "h", "GERMANIUM", sub=40, dyn=0.2, os=3),
        R(BA, "Detroit Bass Warm", "m", "TUBE", sub=55, phone=0.4, eo=0.5),
        R(BA, "Minimal Sub Phone", "g", "TUBE", sub=90, phone=0.7),
        R(V, "Peak Vocal Chop Iron", "m", "IRON", lc=150),
        R(V, "Hypnotic Whisper Tape", "g", "TAPE", lc=150, speed=1, wow=0.05, mix=0.5),
        R(V, "Detroit Soul Tube", "m", "TUBE", eo=0.6, lc=100),
        R(V, "Dub Vocal Tape", "g", "TAPE", lc=120, speed=1, wow=0.1, mix=0.55),
        R(V, "Hard Shout Distort", "h", "GERMANIUM", lc=250, punish=1, mix=0.7, output=3),
        R(KE, "Detroit Chord Tape", "m", "TAPE", speed=1),
        R(KE, "Dub Chord Warm", "g", "TUBE", eo=0.5, wow=0.06),
        R(KE, "Peak Stab Iron", "m", "IRON", tone=0.15),
        R(KE, "Minimal Keys Clean", "g", "TUBE", eo=0.3),
        R(SA, "Acid Lead Fuzz", "h", "GERMANIUM", dyn=0.3, os=3),
        R(SA, "Hypnotic Loop Tape", "g", "TAPE", speed=1, wow=0.06),
        R(SA, "Peak Synth Iron", "m", "IRON", tone=0.05),
        R(SA, "Hard Noise Crush", "h", "CLASSIC", "SP", mix=0.6),
        R(M, "Peak Club Master", "m", "IRON", sub=45, match=1),
        R(M, "Hypnotic Tape Master", "g", "TAPE", sub=40, match=1, speed=1),
        R(M, "Detroit Warm Master", "g", "TUBE", sub=45, eo=0.4, match=1),
        R(M, "Hard Loud Master", "h", "IRON", sub=40, punish=1, match=1),
    ],
    "DnB": [
        R(K, "Modern Kick Drive", "m", "IRON", sub=60, dyn=0.8),
        R(K, "Roller Kick Tape", "g", "TAPE", sub=55),
        R(K, "Liquid Kick Warm", "g", "TUBE", sub=50, eo=0.4, tone=-0.1),
        R(K, "Neuro Kick Distort", "h", "GERMANIUM", sub=70, punish=1, os=3),
        R(K, "Jump-Up Kick Iron", "m", "IRON", sub=65, dyn=0.7, tone=0.1),
        R(K, "Break SP Kick", "m", "TAPE", "SP", sub=60),
        R(S_, "Modern Snare Crack", "m", "IRON", dyn=0.85, tone=0.1),
        R(S_, "Roller Snare Tape", "g", "TAPE", speed=0, mix=0.4),
        R(S_, "Liquid Snare Silk", "g", "TUBE", eo=0.4, tone=0.05),
        R(S_, "Neuro Snare Smash", "h", "GERMANIUM", punish=1, os=3, output=3),
        R(S_, "Jump-Up Snare Bite", "m", "IRON", dyn=0.5, punish=0, output=0),
        R(S_, "Amen SP Snare", "m", "TAPE", "SP", dyn=0.4),
        R(H, "Modern Shuffle Iron", "g", "IRON", lc=500),
        R(H, "Roller Hat Tape", "g", "TAPE", lc=300, speed=0, mix=0.45),
        R(H, "Liquid Ride Silk", "g", "TUBE", eo=0.4, lc=400),
        R(H, "Neuro Hat Fizz", "h", "GERMANIUM", lc=600, mix=0.75, os=3),
        R(H, "Break SP Cymbals", "m", "CLASSIC", "SP", lc=300),
        R(P, "Modern Perc Iron", "m", "IRON", lc=220),
        R(P, "Liquid Shaker Tape", "g", "TAPE", hc=12000, mix=0.4),
        R(P, "Neuro Perc Crunch", "h", "GERMANIUM", mix=0.6, os=3),
        R(P, "Break Perc S950", "m", "TAPE", "S950"),
        R(B, "Modern Bus Drive", "m", "IRON", sub=40, dyn=0.45),
        R(B, "Roller Bus Tape", "g", "TAPE", sub=60, speed=2),
        R(B, "Liquid Bus Warm", "g", "TUBE", sub=55, eo=0.4),
        R(B, "Neuro Bus Distort", "h", "GERMANIUM", sub=32, punish=1, mix=0.85, os=3, auto=0, output=-12),
        R(B, "Amen Break SP-1200", "h", "TAPE", "SP", sub=35, dyn=0.2, auto=0, output=-12),
        R(B, "Break Tape Squash", "h", "TAPE", sub=40, speed=1, punish=1, mix=0.75, output=-12, auto=0),
        R(B, "Jump-Up Bus Iron", "m", "IRON", sub=40, dyn=0.35),
        R(BA, "Modern Reese Drive", "m", "IRON", sub=90, phone=0.4),
        R(BA, "Roller Sub Phone", "g", "TUBE", sub=80, phone=0.8),
        R(BA, "Liquid Bass Warm", "g", "TAPE", sub=60, phone=0.3, speed=1),
        R(BA, "Neuro Bass Distort", "h", "GERMANIUM", sub=40, phone=0.4, os=3),
        R(BA, "Jump-Up Wobble Grit", "h", "IRON", sub=45, punish=1, phone=0.5),
        R(V, "Modern MC Edge", "m", "IRON", lc=150, tone=0.1),
        R(V, "Liquid Vocal Silk", "g", "TUBE", eo=0.5, lc=120, tone=0.05),
        R(V, "Roller Vocal Tape", "g", "TAPE", lc=120, mix=0.55),
        R(V, "Jump-Up MC Distort", "h", "GERMANIUM", lc=180, punish=1, mix=0.7, tone=0.1, output=2),
        R(V, "Liquid Backing Air", "g", "TAPE", lc=150, speed=1, wow=0.05, material="backing", mix=0.5),
        R(KE, "Liquid Rhodes Tube", "g", "TUBE", eo=0.5, wow=0.05),
        R(KE, "Liquid Piano Tape", "m", "TAPE", speed=2),
        R(KE, "Modern Stab Iron", "m", "IRON", tone=0.25),
        R(KE, "Roller Pad Warm", "g", "TAPE", speed=1, wow=0.1),
        R(SA, "Liquid Strings Tape", "g", "TAPE", wow=0.08, tone=-0.05),
        R(SA, "Neuro Synth Fuzz", "h", "GERMANIUM", dyn=0.6, os=3),
        R(SA, "Break Loop SP-1200", "m", "TAPE", "SP", wow=0.05),
        R(SA, "Jump-Up Hoover Grit", "h", "IRON", punish=1, mix=0.8, output=2),
        R(M, "Modern DnB Master", "m", "IRON", sub=40, match=1),
        R(M, "Liquid Warm Master", "g", "TUBE", sub=45, eo=0.55, match=1),
        R(M, "Neuro Loud Master", "h", "GERMANIUM", sub=45, match=1),
        R(M, "Roller Tape Master", "g", "TAPE", sub=45, speed=1, match=1),
    ],
}


def fmt(v):
    return ("%.3f" % v).rstrip("0").rstrip(".")


def main():
    cal = {}
    if len(sys.argv) > 1:
        for line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if len(parts) >= 3 and parts[0] == "CAL":
                cal[parts[1]] = float(parts[2])
    out = ["// Generated by Tools/genre_presets.py - edit the recipes there. docs/GENRE_PRESETS.md",
           "// name, mix, values, genre, group, note, material, depth"]
    names = set()
    for genre in GENRES:
        recipes = RECIPES[genre]
        assert len(recipes) == 50, (genre, len(recipes))
        for group, name, depth, model, sampler, x in recipes:
            key = f"{genre}/{group}/{name}"
            assert key not in names, key
            names.add(key)
            x = dict(x)
            material = x.pop("material", GROUPS[group])
            mix = x.pop("mix", 1.0)
            values = dict(BASE)
            values["model"] = MODEL[model]
            values["sampler"] = SAMPLER[sampler]
            values["oversampling"] = 2 if model == "GERMANIUM" or "punish" in x else 1
            values["driveDb"] = cal.get(key, GUESS[depth])
            for k, v in x.items():
                values[KEYS[k]] = v
            parts = [model]
            if sampler:
                parts.append(SAMPLER_NAME[SAMPLER[sampler]])
            parts.append(f"drive {fmt(values['driveDb'])} dB")
            if x.get("sub"):
                parts.append(f"sub {x['sub']} Hz")
            if x.get("phone"):
                parts.append(f"phone {round(x['phone'] * 100)} %")
            if x.get("dyn"):
                parts.append(f"dynamics {round(x['dyn'] * 100):+d} %")
            if x.get("punish"):
                parts.append("overload")
            if mix < 1.0:
                parts.append(f"mix {round(mix * 100)} %")
            if x.get("output"):
                parts.append(f"output {fmt(x['output'])} dB")
            if x.get("auto") == 0:
                parts.append("manual gain")
            note = ", ".join(parts)
            vals = ", ".join(f'{{ "{k}", {fmt(float(v))} }}' for k, v in values.items())
            out.append(f'{{ "{name}", {float(mix)}f, {{ {vals} }}, "{genre}", "{group}", "{note}", "{material}", Preset::{DEPTH[depth]} }},')
    Path(__file__).resolve().parent.parent.joinpath("Source/Plugin/GenrePresets.inc").write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(names)} genre presets, {len(cal)} calibrated")


if __name__ == "__main__":
    main()
