"""HamloRevolution genre presets (docs/REVOLUTION_V2.md section 11): SIMPLE recipes, written to
Source/Core/RevolutionPresets.inc.

python Tools/revolution_presets.py

A preset is a recipe for the SIMPLE generator (genre, INTENT, AMOUNT, VARIATION, GROOVE, FILL, TARGET, MIX, seed),
not a stored pattern: loading it runs the generator, on the genre's template or, linked, on HPDG's beat.
"""
import zlib
from pathlib import Path

GENRES = {"Boom Bap": 1, "Trap": 2, "Techno": 3, "DnB": 4}
INTENT = {"subtle": 0.1, "groove": 0.3, "fills": 0.5, "breakdown": 0.7, "chaos": 0.9}
FILL = {0: 0, 1: 1, 2: 2, 4: 3, 8: 4}           # bars -> the simpleFill choice
TARGET = {"hats": 0, "snare": 1, "tops": 2, "full": 3}

# group -> the recipes in order: (intent, amount, variation, fill bars, target, mix); the names below go with them
GROUPS = {
    "Drum Bus": [("groove", 0.5, 0.25, 4, "tops", 1.0), ("groove", 0.35, 0.0, 4, "full", 0.85), ("fills", 0.6, 0.25, 4, "tops", 1.0),
                 ("groove", 0.45, 0.5, 8, "snare", 0.8), ("subtle", 0.4, 0.0, 0, "tops", 0.7), ("fills", 0.7, 0.35, 4, "full", 1.0),
                 ("groove", 0.6, 0.25, 2, "tops", 0.9), ("chaos", 0.55, 0.6, 4, "tops", 1.0), ("groove", 0.3, 0.15, 8, "full", 0.8),
                 ("breakdown", 0.6, 0.4, 8, "tops", 1.0)],
    "Hats / Percussion": [("groove", 0.6, 0.25, 4, "hats", 1.0), ("chaos", 0.5, 0.5, 4, "hats", 1.0), ("subtle", 0.5, 0.0, 0, "hats", 0.9),
                          ("fills", 0.7, 0.3, 4, "hats", 1.0), ("groove", 0.85, 0.6, 2, "hats", 1.0), ("groove", 0.4, 0.15, 8, "tops", 0.85),
                          ("chaos", 0.8, 0.8, 4, "hats", 0.9), ("subtle", 0.3, 0.25, 4, "hats", 0.8), ("fills", 0.5, 0.45, 8, "tops", 1.0)],
    "Loops / Samples": [("groove", 0.5, 0.35, 4, "full", 1.0), ("breakdown", 0.6, 0.4, 8, "full", 1.0), ("fills", 0.55, 0.5, 4, "full", 1.0),
                        ("groove", 0.7, 0.7, 2, "full", 0.9), ("subtle", 0.4, 0.3, 8, "full", 0.8), ("chaos", 0.6, 0.8, 4, "full", 1.0),
                        ("groove", 0.35, 0.25, 4, "tops", 1.0), ("breakdown", 0.8, 0.5, 4, "full", 1.0), ("fills", 0.4, 0.6, 8, "full", 0.9)],
    "Vocals": [("subtle", 0.35, 0.25, 4, "full", 0.5), ("groove", 0.3, 0.25, 8, "full", 0.6), ("subtle", 0.5, 0.0, 4, "full", 0.4),
               ("groove", 0.45, 0.4, 4, "full", 0.7), ("fills", 0.4, 0.25, 8, "full", 0.6), ("subtle", 0.25, 0.5, 2, "full", 0.5),
               ("breakdown", 0.4, 0.3, 8, "full", 0.6)],
    "Transitions / Fills": [("fills", 0.7, 0.25, 4, "full", 1.0), ("breakdown", 0.8, 0.25, 8, "full", 1.0), ("fills", 0.9, 0.5, 8, "tops", 1.0),
                            ("breakdown", 0.6, 0.0, 4, "full", 1.0), ("chaos", 0.7, 0.6, 8, "full", 1.0), ("fills", 0.5, 0.35, 2, "tops", 1.0),
                            ("breakdown", 0.9, 0.5, 8, "full", 1.0), ("fills", 0.6, 0.25, 1, "snare", 1.0), ("chaos", 0.9, 0.8, 4, "full", 1.0)],
    "Master FX": [("subtle", 0.3, 0.0, 8, "full", 0.35), ("subtle", 0.4, 0.25, 4, "full", 0.5), ("groove", 0.25, 0.25, 8, "full", 0.3),
                  ("subtle", 0.2, 0.15, 0, "full", 0.25), ("fills", 0.3, 0.25, 8, "full", 0.4), ("breakdown", 0.35, 0.3, 8, "full", 0.45)],
}

NAMES = {
    "Boom Bap": {
        "Drum Bus": ["Dusty Chop Bus", "SP Knock Glue", "Crate Digger Fills", "Snare Repeat Bus", "Soft Vinyl Touch", "Brake Bus",
                     "Two-Bar Juggle", "Chopped Chaos Bus", "Golden Era Ride", "Breakdown Filter Bus"],
        "Hats / Percussion": ["Swung Hat Repeats", "Scratchy Hats", "Tape Hat Glide", "Hat Fill Turns", "Busy Shaker Chops", "Tops Delay Taps",
                              "Hat Scratch Storm", "Quiet Hat Jumps", "Perc Phrase Ends"],
        "Loops / Samples": ["Sample Juggle", "Filtered Loop Drop", "Loop Brake Ends", "Restless Chops", "Low-Pass Loop Swell", "Chop Shop Chaos",
                            "Tops Only Chops", "Half-Time Sample", "Varied Loop Flips"],
        "Vocals": ["Vocal Delay Throws", "Hook Echo Tail", "Adlib Vinyl", "Verse Chop Glue", "Phrase End Throw", "Dub Vox Hint",
                   "Vocal Slowdown"],
        "Transitions / Fills": ["Vinyl Brake Fill", "Tape Stop Drop", "Tops Stutter Turn", "Half-Time Switch", "Scratch Out", "Two-Bar Turnaround",
                                "Eight-Bar Slowdown", "Snare Repeat Fill", "Chaos Ending"],
        "Master FX": ["Mix Vinyl Breath", "Gentle Filter Swells", "Low Throw Glue", "Barely There", "Phrase End Brake", "Master Low-Pass Drop"],
    },
    "Trap": {
        "Drum Bus": ["Roll Bus", "Clean 808 Bus", "Stutter Snare Bus", "Snare Ratchets", "Light Hat Motion", "Drop Prep Bus",
                     "Two-Bar Rolls", "Glitch Bus", "Smooth Pan Bus", "Half-Time Bus"],
        "Hats / Percussion": ["Hat Rolls 1/32", "Chaos Rolls", "Gentle Rolls", "Roll Fills", "Triplet Storm", "Tops Pan Rolls",
                              "Machine Gun Hats", "Lazy Hat Rolls", "Perc Ratchets"],
        "Loops / Samples": ["Melody Rolls", "Half-Time Melody", "Loop Stop Drop", "Restless Loop", "Dark Loop Pan", "Glitched Melody",
                            "Tops Melody Rolls", "Breakdown Loop", "Varied Loop Rolls"],
        "Vocals": ["Adlib Delay", "Hook Reverb Throw", "Ad-Lib Pan", "Vocal Roll Echo", "Phrase End Reverb", "Mumble Throws",
                   "Vocal Half-Time"],
        "Transitions / Fills": ["Pre-Drop Tape Stop", "Half-Time Drop", "Roll Into Drop", "Bar Stop", "Glitch Transition", "Two-Bar Riser",
                                "Eight-Bar Drag", "Snare Stutter Fill", "Chaos Switch"],
        "Master FX": ["Master Pan Air", "Soft Rolls Master", "Reverb Tail Glue", "Master Whisper", "Drop Tail", "Master Half-Time Hint"],
    },
    "Techno": {
        "Drum Bus": ["Pump Bus", "Warehouse Glue", "Sweep Bus", "Clap Delay Bus", "Subtle Pump", "Peak Time Bus",
                     "Two-Bar Pump", "Gate Chaos Bus", "Rolling Width", "Breakdown Sweep Bus"],
        "Hats / Percussion": ["Hat Gate", "Hat Stutter Bursts", "Soft Hat Pump", "Hat Repeat Fills", "Busy Hat Gate", "Tops Delay Grid",
                              "Glitch Hats", "Quiet Hat Width", "Perc Sweep"],
        "Loops / Samples": ["Loop Pump", "Four-Bar Sweep", "Loop Freeze Fill", "Moving Loop", "Gentle Loop Width", "Stutter Loop",
                            "Tops Loop Pump", "Long Sweep Breakdown", "Varied Loop Gate"],
        "Vocals": ["Vocal Off-Beat Delay", "Vocal Reverb Wash", "Vox Pump", "Vocal Gate", "Phrase Reverb Throw", "Spoken Word Echo",
                   "Vocal Filter Fall"],
        "Transitions / Fills": ["Reverb Throw Fill", "Freeze Break", "Stutter Into Drop", "Filter Down Bar", "Burst Transition",
                                "Two-Bar Build", "Long Freeze", "Clap Repeat Fill", "Chaos Build"],
        "Master FX": ["Master Pump Breath", "Master Sweep", "Master Delay Glue", "Master Air", "Master Reverb Ends", "Master Filter Dive"],
    },
    "DnB": {
        "Drum Bus": ["Break Bus", "Amen Glue", "Reverse Turn Bus", "Snare Ratchet Bus", "Light Drive Bus", "Roller Bus",
                     "Two-Bar Break", "Chaos Break Bus", "Liquid Bus", "Half-Speed Bus"],
        "Hats / Percussion": ["Shuffle Hats", "Hat Chaos", "Soft Hat Drive", "Hat Reverse Fills", "Busy Breaks", "Tops Jumps",
                              "Hat Glitch Storm", "Liquid Hats", "Perc Reverse"],
        "Loops / Samples": ["Break Jumps", "Half-Speed Break", "Reverse Ends", "Restless Amen", "Liquid Loop", "Break Chaos",
                            "Tops Break Jumps", "Breakdown Break", "Varied Breaks"],
        "Vocals": ["MC Delay", "Vocal Reverb Throws", "Liquid Vocal Drive", "Vocal Jumps", "Phrase End Reverse", "Dub Vox",
                   "Vocal Half-Speed"],
        "Transitions / Fills": ["Tape Stop Ending", "Half-Speed Drop", "Reverse Into Drop", "Break Stop", "Glitch Out", "Two-Bar Roll",
                                "Eight-Bar Half-Speed", "Snare Ratchet Fill", "Chaos Switch-Up"],
        "Master FX": ["Master Drive Hint", "Master Reverse Ends", "Master Delay", "Master Lift", "Master Space Ends", "Master Half-Speed"],
    },
}

NOTE_INTENT = {"subtle": "Light touches", "groove": "Groove moves", "fills": "Fills", "breakdown": "Breakdown moves", "chaos": "Chaos bursts"}
NOTE_TARGET = {"hats": "on the hats", "snare": "on the snare", "tops": "above the kick", "full": "on the whole mix"}


def note(intent, fill, target, mix):
    text = f"{NOTE_INTENT[intent]} {NOTE_TARGET[target]}"
    text += f", fills every {fill} bar{'s' if fill != 1 else ''}" if fill else ", no fills"
    if mix < 1.0:
        text += f", mix {round(mix * 100)} %"
    return text


def main():
    rows = []
    for genre, number in GENRES.items():
        count = 0
        for group, recipes in GROUPS.items():
            names = NAMES[genre][group]
            assert len(names) == len(recipes), (genre, group)
            for name, (intent, amount, variation, fill, target, mix) in zip(names, recipes):
                seed = zlib.crc32(f"{genre}/{name}".encode()) & 0x7fffffff
                rows.append(f'    {{ "{genre}", "{group}", "{name}", "{note(intent, fill, target, mix)}", {number}, {INTENT[intent]:.2f}f, '
                            f'{amount:.2f}f, {variation:.2f}f, 1.0f, {FILL[fill]}, {TARGET[target]}, {mix:.2f}f, {seed} }},')
                count += 1
        assert count == 50, (genre, count)
    names = [r.split('"')[5] for r in rows]
    assert len(names) == len(set(names)), "duplicate names (the host's program list)"
    out = Path(__file__).resolve().parent.parent / 'Source' / 'Core' / 'RevolutionPresets.inc'
    out.write_text('// Written by Tools/revolution_presets.py - edit the recipes there.\n' + '\n'.join(rows) + '\n', encoding='utf-8')
    print(f'{len(rows)} presets -> {out}')


if __name__ == '__main__':
    main()
