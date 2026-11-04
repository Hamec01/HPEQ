"""Local reference analysis and reproducible transition fixtures; never embeds the library."""
import hashlib
import json
import math
import pathlib
import re
import sys
import warnings
import numpy as np
from scipy.io import wavfile
from scipy import signal


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        fs, x = wavfile.read(path)
    if x.dtype.kind in 'iu':
        if x.dtype.kind == 'u':
            x = (x.astype(np.float64)-128)/128
        else:
            x = x.astype(np.float64)/float(2**(8*x.dtype.itemsize-1))
    x = np.asarray(x, dtype=np.float64)
    if x.ndim == 1:
        x = np.column_stack((x,x))
    x = x[:,:2]
    if not np.isfinite(x).all() or not len(x):
        raise ValueError('empty/nonfinite source')
    return fs,x


def describe(x,fs):
    hop=max(1,round(.004*fs));pad=(-len(x))%hop
    frames=np.pad(x,((0,pad),(0,0))).reshape(-1,hop,2)
    e=np.sqrt(np.mean(frames*frames,axis=(1,2))+1e-20)
    novelty=np.maximum(0,e-np.roll(e,1));novelty[0]=e[0]
    peaks,_=signal.find_peaks(novelty,height=max(float(np.max(novelty))*.055,float(np.median(novelty))*3),distance=3)
    if novelty[0]>np.max(novelty)*.055:peaks=np.r_[0,peaks]
    slots=np.minimum(31,(peaks/max(1,len(e))*32).astype(int))
    hist=np.bincount(slots,minlength=32)
    slices=np.array_split(x,8)
    rms=[float(np.sqrt(np.mean(a*a)+1e-30)) for a in slices]
    centroid=[]
    for a in slices:
        _,_,s=signal.stft(a.mean(axis=1),fs,nperseg=min(1024,len(a)),noverlap=min(512,max(0,len(a)//2-1)))
        mag=np.abs(s).sum(axis=1);freq=np.linspace(0,fs/2,len(mag))
        centroid.append(float(np.sum(freq*mag)/max(1e-30,np.sum(mag))))
    return dict(duration=len(x)/fs,peak=float(np.max(np.abs(x))),rms=float(np.sqrt(np.mean(x*x))),
                onset_count=int(len(peaks)),onset_bins32=hist.tolist(),rms8=rms,centroid8=centroid,
                growth_db=20*math.log10(max(1e-20,np.mean(rms[-2:]))/max(1e-20,np.mean(rms[:2]))))


def main():
    root=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);assert not out.exists()
    out.mkdir(parents=True)
    paths=sorted(root.rglob('*.wav'))
    selected=[]
    for bpm in (95,120,140,170):
        for family in ('SnareRoll','KikSnrFillA','AcouGlitchChop','AcouFill_'):
            candidates=[p for p in paths if 'Musicradar Drum Fills' in str(p) and f'{bpm}bpm' in str(p) and family in p.name]
            selected.extend(candidates[:1])
    for name in ('Turn Up Riser.wav','Turn Up Sidechain Riser.wav','Build Up Riser.wav','Drop Down Up Riser.wav',
                 'Merkaba Rollercoaster Riser.wav','[A] Rise (1).wav','[A] Rise (5).wav','[A] Rise (10).wav',
                 'Diamond Wav - Hyper Riser.wav','Diamond Wav - Think Riser.wav',
                 'EKALI_fx_fill_forever_riser.wav','DECAP_texture_start_your_engines_riser.wav'):
        selected.extend([p for p in paths if p.name==name][:1])
    references=[];rejected=[]
    for path in selected:
        try:
            fs,x=load(path)
            references.append(dict(path=str(path),sha256=sha(path),sample_rate=fs,**describe(x,fs)))
        except Exception as e:rejected.append(dict(path=str(path),error=str(e)))
    source_specs=[('drums120','DECAP_drum_loop_lit_120.wav',120),
                  ('toms90','90_JazzToms_01_SP.wav',90),
                  ('break170','IF_AcouFill_170-01.wav',170)]
    sources=[]
    for name,filename,bpm in source_specs:
        matches=[p for p in paths if p.name==filename];assert matches,filename
        path=matches[0];fs,x=load(path);original=sha(path)
        divisor=math.gcd(int(fs),48000);x=signal.resample_poly(x,48000//divisor,fs//divisor,axis=0)
        length=round(4*4*60/bpm*48000);x=np.tile(x,(math.ceil(length/len(x)),1))[:length]
        scale=min(10**(-18/20)/max(1e-20,float(np.sqrt(np.mean(x*x)))),10**(-3/20)/max(1e-20,float(np.max(np.abs(x)))))
        x=(x*scale).astype(np.float32);dest=out/(name+'.wav');wavfile.write(dest,48000,x)
        sources.append(dict(name=name,path=str(dest.resolve()),bpm=bpm,original=str(path),original_sha256=original,prepared_sha256=sha(dest),**describe(x,48000)))
    result=dict(method_sha256=sha(pathlib.Path(__file__)),root=str(root),references=references,rejected=rejected,sources=sources)
    (out/'materials.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'{len(references)} references,{len(rejected)} rejected,{len(sources)} prepared sources')
    for r in references:print(pathlib.Path(r['path']).name,round(r['duration'],2),'sec',r['onset_count'],'attacks','growth',round(r['growth_db'],1),'dB')


if __name__=='__main__':main()
