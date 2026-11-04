"""Write the measured preset index; no audio or recipe mutation."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
bank=json.loads((root/'Tools/fader_presets.json').read_text())
checked={r['id']:r for r in json.loads((root/'out/f5/check_final/result.json').read_text())['recipes']}
independent={r['id']:r for r in json.loads((root/'docs/reports/f5/analysis_final.json').read_text())['recipes']}
assert len(bank)==len(checked)==len(independent)==200
lines=['# HamloFader0.2.0 genre presets','',
       '200/200 compiled recipes independently checked on owner material. Original',
       'programs0..7 remain in place; the new library occupies8..207. Each genre has',
       '50recipes:11Match,19Ride,20Motion. Match loads with no held gain; Learn,Stop',
       'and Apply are deliberate. Presets contain no audio. See [F5 report](reports/f5/REPORT.md).','',
       'Work:mean absolute20ms input/output energy-level difference on active material',
       'after0.5s. Gentle1..3dB,Medium3..6dB,Heavy6..12dB. Sources are normalized to',
       '-18dBFS RMS subject to-9dBFS peak for Ride boost headroom. Peak measurements',
       'describe prepared demonstrations; this plugin is not an output limiter.','',
       'The sparse DNB Liquid Keys Level recipe uses Input+12/Gain-12 and a shifted',
       'activity floor to extend detector range, with neutral net fixed gain. These',
       'values are visible on QUICK/ADVANCED and explained in its preset note.','',
       '| Program | ID | Name | Genre / Group | Mode | BPM | Source | Depth | Work dB | Peak8x dBTP |',
       '|---:|---|---|---|---|---:|---|---|---:|---:|']
for program,recipe in enumerate(bank,8):
    row=checked[recipe['id']];peak=independent[recipe['id']]['true_peak_8x_dbtp']
    assert row['passed']
    role={1:'Match',2:'Ride',3:'Motion'}[recipe['values']['mode']]
    lines.append(f"| {program} | {recipe['id']} | {recipe['name']} | {recipe['genre']} / {recipe['group']} | {role} | {recipe['bpm']} | {recipe['material']} | {recipe['depth']} | {row['work_db']:.3f} | {peak:.3f} |")
(root/'docs/GENRE_PRESETS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('Wrote200 measured preset entries')
