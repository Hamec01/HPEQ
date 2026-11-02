"""Generate the user-facing catalog from the exact calibrated recipes and checked bank."""
import json
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
bank=json.loads((ROOT/'Tools/genre_recipes.json').read_text(encoding='utf-8'))
check=json.loads((ROOT/'docs/reports/presets/check_all.json').read_text(encoding='utf-8'))
assert len(bank)==200 and check['passed']==200 and check['failed']==0
assert Counter(p['genre'] for p in bank)==dict.fromkeys(['Boom Bap','Trap','Techno','DnB'],50)
out=['# HamloVerb genre presets','',
     '**200/200 independently checked**, after complete calibration on the final DSP. Default and the seven original Factory programs remain unchanged (208 programs total).',
     '', 'Each genre has Vocal10, Drums12, Samples/Keys10, Synth8, Atmosphere/FX10.',
     'The genre voicing and tempo derive from the owner\'s musical workflow; recipes are authored in Tools/genre_presets.py, not randomized.',
     '', 'Depth means complete-render wet/dry energy: Gentle -24..-16 dB, Medium -16..-8 dB, Heavy -8..+6 dB.',
     'Calibration uses normalized owner clips, includes tails, and verifies actual Insert and Send renders, 8x true peak, state recall, finite output and unique sound settings.',
     'MIX and Output alone do not count toward uniqueness. Cached calibration is rejected if source/DSP/material/recipe identity changes.',
     '', '[Full independent check](reports/presets/check_all.md) · [Calibration](reports/presets/calibrate_all.md)',
     '', 'Material buttons filter only. LOAD START loads a recipe. Mix Lock preserves Insert MIX; routing survives preset loads; Send remains wet only.',
     '', '| Program | ID | Genre | Group | Name | BPM | Depth | Insert MIX | Purpose |',
     '|---:|---|---|---|---|---:|---|---:|---|']
for i,p in enumerate(bank,8):
    out.append(f"| {i} | {p['id']} | {p['genre']} | {p['group']} | {p['name']} | {p['tempo']} | {p['depth']} | {p['mix']*100:.2f}% | {p['note']} |")
(ROOT/'docs/GENRE_PRESETS.md').write_text('\n'.join(out)+'\n',encoding='utf-8')
print('Catalog: 200 recipes, programs 8..207')
