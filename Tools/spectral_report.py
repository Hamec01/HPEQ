"""Independent octave-band analysis of diagnostic IR exports; does not alter lab goals."""
from pathlib import Path
import numpy as np
from scipy.signal import butter, sosfilt

ROOT=Path(__file__).resolve().parents[1]
FS=48000
def decay(signal):
    e=np.cumsum(np.sum(signal.astype(np.float64)**2,axis=0)[::-1])[::-1]
    db=10*np.log10(np.maximum(e,1e-300)/max(e[0],1e-300))
    selected=(db<=-5)&(db>=-35)
    x=np.arange(signal.shape[1])[selected]/FS;y=db[selected]
    slope,intercept=np.polyfit(x,y,1)
    residual=np.sum((y-(slope*x+intercept))**2)
    return -60/slope,1-residual/max(1e-30,np.sum((y-y.mean())**2))
rows=[]
ir={p.stem:np.fromfile(p,dtype='<f4').reshape(2,-1) for p in (ROOT/'build/spectral').glob('*.f32')}
for name,data in sorted(ir.items()):
    for centre in (125,250,500,1000,2000,4000,8000):
        sos=butter(4,[centre/2**.5,centre*2**.5],btype='bandpass',fs=FS,output='sos')
        t,r2=decay(sosfilt(sos,data,axis=1));rows.append((name,centre,t,r2))
out=['# Independent spectral diagnostics','',
     'Additional IR-export diagnostic added after the first V2 run. Existing acceptance methods/thresholds are unchanged.',
     '48 kHz, 12-second stereo impulses, fourth-order Butterworth octave bands, Schroeder T30 (-5 to -35 dB).',
     'Coloured settings: bass ratio 1.5, HF ratio 0.5; neutral ratios 1. Decay EQ: +1 octave bell at 1 kHz. Post EQ: +6 dB bell at 1 kHz.',
     '', '| Character/profile | Band Hz | T30 seconds | R-squared |','|---|---:|---:|---:|']
out += [f'| {n} | {f} | {t:.4f} | {r2:.5f} |' for n,f,t,r2 in rows]
ref=ir['Clean_neutral'];post=ir['Clean_post'];n=ref.shape[1]
osc=np.exp(-2j*np.pi*1000*np.arange(n)/FS)
gain=20*np.log10(np.linalg.norm(post@osc)/np.linalg.norm(ref@osc))
out+=['',f'Measured Post EQ gain at 1 kHz: **{gain:.4f} dB**.',
      'Decay curves shown in the UI are target models; these tables describe the actual rendered tails.']
(ROOT/'docs/reports/v2/spectral_diagnostics.md').write_text('\n'.join(out)+'\n',encoding='utf-8')
print(f'{len(rows)} band measurements; Post EQ {gain:.4f} dB')
