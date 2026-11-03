"""Owner-derived, explicitly annotated fixtures plus separate natural excerpts."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT=Path(__file__).resolve().parents[1];FS=48000
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def peak(a):return float(np.max(np.abs(resample_poly(a,4,1,axis=0))))
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def load(path):
    rate,a=wavfile.read(path);a=a[:12*rate]
    if a.ndim==1:a=np.repeat(a[:,None],2,axis=1)
    assert a.shape[1]==2
    if a.dtype.kind=='i':a=a.astype(float)/2**(a.dtype.itemsize*8-1)
    elif a.dtype.kind=='u':a=(a.astype(float)-128)/128
    else:a=a.astype(float)
    assert np.isfinite(a).all()
    if rate!=FS:
        d=int(np.gcd(rate,FS));a=resample_poly(a,FS//d,rate//d,axis=0)
    scale=10**(-6/20)/max(peak(a),1e-20);return (a*scale).astype('<f4'),rate,float(scale)
def snippet(a,seconds,transient=False):
    length=round(seconds*FS)
    if transient:at=max(0,int(np.argmax(np.max(np.abs(a),axis=1)))-96)
    else:
        width=min(len(a),length);e=np.sum(a.astype(float)**2,axis=1);cs=np.r_[0,np.cumsum(e)];candidates=np.arange(0,len(a)-width+1,960);at=int(candidates[np.argmax(cs[candidates+width]-cs[candidates])])
    return a[np.arange(at,at+length)%len(a)].copy()
def repeated(a,frames):return a[np.arange(frames)%len(a)].copy()


def main():
    p=argparse.ArgumentParser();p.add_argument('material',type=Path);p.add_argument('output',type=Path);args=p.parse_args();assert not args.output.exists(),'Refusing to replace fixtures';args.output.mkdir(parents=True)
    recipes=json.loads((ROOT/'Source/Presets/ZastavaRecipes.json').read_text());names=sorted(set(r['material'] for r in recipes)|{'kick.wav'});audio={};sources={}
    for name in names:
        path=args.material/name;a,rate,scale=load(path);audio[name]=a;sources[name]=dict(path=str(path),sha256=sha(path),rate=rate,scale=scale)
    result=dict(rate=FS,method_sha256=sha(Path(__file__)),sources=sources,recipes={});saved={}
    def store(name,a):
        if name not in saved:
            path=args.output/(name+'.f32');a.astype('<f4').tofile(path);saved[name]=dict(path=str(path.resolve()),sha256=sha(path),frames=len(a))
        return saved[name]
    for r in recipes:
        role=r['role'];name=r['material'];a=audio[name];beat=60/r['bpm'];key=None
        rng=np.random.default_rng(int(hashlib.sha256((name+role+str(r['bpm'])).encode()).hexdigest()[:8],16))
        if role=='Clean':
            frames=6*FS;x=rng.normal(0,10**(-58/20),(frames,2));clip=snippet(a,.35);events=[]
            for n in range(4):
                base=round(n*1.5*FS);start=base+2400;x[start:start+len(clip)]+=clip;quiet=clip[:12000]*.22;x[base+21600:base+33600]+=quiet;events.append(base/FS)
        elif role=='Drum':
            spacing=beat*.5;length=min(.14,.72*spacing);clip=snippet(a,length,True);clip[:15]*=np.linspace(0,1,15)[:,None];clip[-96:]*=np.linspace(1,0,96)[:,None]
            events=[.1+n*spacing for n in range(8)];frames=round((events[-1]+spacing+.25)*FS);x=rng.normal(0,10**(-62/20),(frames,2))
            for n,t in enumerate(events):at=round(t*FS);x[at:at+len(clip)]+=clip*(1 if n%2==0 else .28)
        elif role=='Duck':
            events=[.1+n*beat for n in range(8)];frames=round((events[-1]+beat+.25)*FS);x=repeated(snippet(a,.7),frames);x+=rng.normal(0,10**(-65/20),x.shape);key=np.zeros_like(x);kick=snippet(audio['kick.wav'],.09,True);kick[:15]*=np.linspace(0,1,15)[:,None];kick[-96:]*=np.linspace(1,0,96)[:,None]
            for t in events:at=round(t*FS);key[at:at+len(kick)]+=kick
        else:
            values=r['values'];step=(beat*values['v2_beats']) if values['v2_sync'] else values['v2_ms']/1000;duration=step*values['v2_steps'];frames=round((duration+.25)*FS);x=repeated(snippet(a,.5),frames);x+=rng.normal(0,10**(-65/20),x.shape);events=[]
        # A single scale preserves all annotated strong/quiet ratios and noise.
        factor=min(1,10**(-6/20)/max(peak(x),1e-20));x=(x*factor).astype('<f4');x=np.pad(x,((0,12000),(0,0)))
        fixture=store('fixture_'+r['id'],x);key_info=None
        if key is not None:key_info=store('key_'+r['id'],np.pad(key,((0,12000),(0,0))).astype('<f4'))
        natural=store('natural_'+name.removesuffix('.wav'),np.pad(repeated(a,6*FS),((0,12000),(0,0))).astype('<f4'))
        natural_key=None
        if role=='Duck':
            k=np.zeros((6*FS+12000,2),dtype='<f4');kick=snippet(audio['kick.wav'],.09,True)
            for t in np.arange(.1,6-.1,beat):at=round(t*FS);k[at:at+len(kick)]+=kick
            natural_key=store('natural_key_'+str(r['bpm']),k)
        result['recipes'][r['id']]=dict(fixture=fixture,key=key_info,natural=natural,natural_key=natural_key,events=events,scale=factor)
    write(args.output/'manifest.json',result);print('Prepared200owner-derived fixtures; natural excerpts separate')


if __name__=='__main__':main()
