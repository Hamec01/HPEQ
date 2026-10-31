"""Exploratory analysis of existing M0 renders; no S1 acceptance threshold.

Introduced after capture. Does not render audio, change the lab or replace any
reference. Requires numpy; results are specific to the archived synthetic input.
"""
import json
from pathlib import Path
import numpy as np

root = Path(__file__).resolve().parents[1]
rows = []
for name in ['program_0_48000_ch2_b127', 'boundary_width_max', 'boundary_safe_min']:
    a = np.fromfile(root / 'out/s1/reference' / (name + '.f32'), dtype='<f4').reshape(2, -1).astype(np.float64)[:, 4800:]
    mid, side = (a[0] + a[1]) * .5, (a[0] - a[1]) * .5
    rows.append(dict(case=name, side_to_mid_db=float(10 * np.log10(np.sum(side * side) / np.sum(mid * mid))), correlation=float(np.corrcoef(a)[0, 1])))
result = dict(method='Post-capture diagnostic only; no acceptance threshold. Original mixed-sine/noise input,48kHz stereo, discard first100ms; total channel M=(L+R)/2,S=(L-R)/2 energy ratio and Pearson correlation.', cases=rows)
(root / 'docs/reports/s1/width_diagnostic.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(json.dumps(rows, indent=2))
