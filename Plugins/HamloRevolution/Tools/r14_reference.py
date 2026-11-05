"""Aggregate band-onset grammar from the owner's tempo-labelled drum fills."""
import hashlib,json,math,pathlib,re,sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from transition_materials import load

def band_events(x,fs,bpm):
    if x.ndim==1:x=x[:,None]
    bands=[]
    for low,high,space in [(30,240,.060),(220,7500,.040),(5500,min(18000,fs*.45),.022)]:
        sos=signal.butter(3,[low,high],btype='bandpass',fs=fs,output='sos')
        y=signal.sosfilt(sos,x,axis=0);hop=max(1,round(fs*.004));n=len(y)//hop
        e=np.sqrt(np.mean(y[:n*hop].reshape(n,hop,-1)**2,axis=(1,2)))
        prior=np.convolve(e,np.ones(6)/6,'full')[:n];prior=np.r_[0,prior[:-1]]
        novelty=np.maximum(0,e-prior);threshold=max(np.max(novelty)*.13,np.median(novelty)+3*np.median(np.abs(novelty-np.median(novelty))))
        peaks,_=signal.find_peaks(np.r_[0,novelty,0],height=max(threshold,1e-9),distance=max(1,round(space*fs/hop)))
        times=np.maximum(0,(peaks-1)*hop-fs*.004)/fs
        bands.append(times)
    return bands

def main():
    output=ROOT/'out/r14/reference';assert not output.exists();output.mkdir(parents=True)
    files=sorted(pathlib.Path('E:/DRUMS/DRUMFILL').rglob('*.wav'));rows=[];skips=[]
    for p in files:
        text=p.stem+' '+str(p.parent)
        matches=re.findall(r'(\d{2,3})\s*bpm',text,re.I)
        if not matches:matches=re.findall(r'(?:^|[ _-])(\d{2,3})$',p.stem)
        if not matches:skips.append({'path':str(p),'reason':'no unambiguous tempo label'});continue
        bpm=int(matches[0])
        if not 60<=bpm<=200:skips.append({'path':str(p),'reason':'tempo outside60..200'});continue
        try:
            fs,x=load(p);duration=len(x)/fs;bar=240/bpm
            if not .7*bar<=duration<=16*bar:raise ValueError('duration outside0.7..16bars')
            # Analyze the final musical bar; shorter clips are padded,not repeated.
            x=x[-round(bar*fs):];events=band_events(x,fs,bpm)
            hist=np.zeros((3,16));counts=[]
            for role,times in enumerate(events):
                counts.append(len(times))
                for t in times:hist[role,min(15,int(t/bar*16))]+=1
            if sum(counts)<3:raise ValueError('fewer than3band onsets')
            rows.append({'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bpm':bpm,'duration':duration,'counts':counts,'phase16':hist.tolist()})
        except Exception as exc:skips.append({'path':str(p),'reason':str(exc)})
    counts=np.array([r['counts'] for r in rows]);hist=np.array([r['phase16'] for r in rows])
    # Equal file weighting prevents loud/dense recordings dominating the grammar.
    probabilities=(hist+.5)/(hist.sum(axis=2,keepdims=True)+8)
    phase=probabilities.mean(axis=0);phase/=phase.mean(axis=1,keepdims=True)
    median=np.median(counts,axis=0);build=np.mean(hist[:,:,8:].sum(axis=2)/(hist.sum(axis=2)+1e-9),axis=0)
    data={'method_sha256':hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),'inventory':len(files),'analyzed':len(rows),'skipped':skips,'references':rows,'median_counts':median.tolist(),'phase16':phase.tolist(),'late_fraction':build.tolist(),'note':'Band-onset statistics,not labelled kit stems. No waveform or per-file pattern in generated plugin data.'}
    (output/'analysis.json').write_text(json.dumps(data,indent=2)+'\n')
    lines=['#pragma once','#include <array>','namespace hpeq::dsp::transition::grammar {',f'inline constexpr int referenceCount={len(rows)};', 'inline constexpr std::array<double,3> medianCounts{'+','.join(f'{v:.8f}' for v in median)+'};','inline constexpr std::array<std::array<double,16>,3> phase{{']
    lines += ['    {{'+','.join(f'{v:.8f}' for v in values)+'}},' for values in phase]
    lines+=['}};','inline constexpr std::array<double,3> lateFraction{'+','.join(f'{v:.8f}' for v in build)+'};','}']
    header=ROOT/'Source/DSP/DrumFillGrammarData.h';header.write_text('\n'.join(lines)+'\n')
    print(json.dumps({'inventory':len(files),'analyzed':len(rows),'skipped':len(skips),'median_counts':median.tolist(),'late_fraction':build.tolist()},indent=2))

if __name__=='__main__':main()
