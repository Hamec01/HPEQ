"""Report stereo behavior of real owner-material A/B renders, without new pass thresholds."""
from pathlib import Path
import numpy as np
from scipy.io import wavfile

ROOT=Path(__file__).resolve().parents[1]
rows=[]
for file in sorted((ROOT/'build/demos').glob('*/send.wav')):
    rate,audio=wavfile.read(file)
    audio=audio.astype(np.float64)
    audio-=audio.mean(axis=0)
    e=np.sum(audio*audio,axis=0)
    corr=float(np.sum(audio[:,0]*audio[:,1])/max(1e-30,np.sqrt(e[0]*e[1])))
    mono=10*np.log10(max(1e-300,np.sum(audio.mean(axis=1)**2))/max(1e-300,e.mean()))
    rows.append((file.parent.name,rate,corr,mono))
assert len(rows)==20, f'Expected 20 completed demo sets, found {len(rows)}'
out=['# Owner-material stereo diagnostics','',
     'Final preset-check Send renders, complete tails included. Mono fold uses (L+R)/2 relative to average channel energy.',
     'Informational measurements; no threshold is introduced after the baseline.', '',
     '| Demo | Rate | Correlation | Mono-fold dB |','|---|---:|---:|---:|']
out += [f'| {name} | {rate} | {corr:.4f} | {mono:.3f} |' for name,rate,corr,mono in rows]
(ROOT/'docs/reports/v2/material_stereo.md').write_text('\n'.join(out)+'\n',encoding='utf-8')
print(f'{len(rows)} stereo material diagnostics')
