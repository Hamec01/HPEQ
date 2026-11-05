"""Stratified reference analysis and unchanged owner-loop corpus for drum fills."""
import collections,hashlib,json,math,pathlib,re,sys
import numpy as np
from scipy.io import wavfile
from scipy import signal
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from transition_materials import load,describe
root=pathlib.Path(__file__).resolve().parents[1]
source=pathlib.Path('E:/DRUMS/DRUMFILL');out=root/'out/r13/materials_v2'
assert not out.exists();out.mkdir(parents=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
files=sorted(source.rglob('*.wav'));selected=[]
for bpm in (95,110,120,128,140,170):
    for family in ('AcouFill_','AcouGlitchChop','KikSnrFillA','SnareRoll'):
        matches=[p for p in files if p.parent.name==f'{bpm}bpm' and family in p.name]
        selected+=matches[:1]
axel=[p for p in files if "Axel's" in str(p)]
for token in ('Tom','Triplets','Flam','Chopped','Swing','Bounce','Perc','Jungle'):
    selected+=[p for p in axel if token.lower() in p.name.lower()][:1]
selected=list(dict.fromkeys(selected));references=[];rejected=[]
for p in selected:
    try:
        fs,x=load(p);references.append(dict(path=str(p),sha256=sha(p),sample_rate=fs,**describe(x,fs)))
    except Exception as e:rejected.append(dict(path=str(p),error=str(e)))
old=json.loads((root/'out/r12/materials/materials.json').read_text())
sources=[]
for item in old['sources']:
    p=pathlib.Path(item['path']);dest=out/p.name;dest.write_bytes(p.read_bytes())
    fs,x=load(dest);sources.append(dict(item,path=str(dest.resolve()),prepared_sha256=sha(dest),source_kind='fill stress input' if item['name']=='break170' else 'drum/percussion loop'))
# Additional actual owner loops,selected explicitly rather than guessing genre
# from unlabelled audio. No reference fill is mistaken for an ordinary source loop.
paths=sorted(pathlib.Path('E:/DRUMS').rglob('*.wav'))
for name,filename,bpm in [('drums140','Cymatics - Cobra Full Drum Loop 5 - 140 BPM.wav',140),('drums90','Cymatics - Eternity Drum Loop 11 - 90 BPM.wav',90),('drums170','Cymatics - Cobra Full Drum Loop 16 - 170 BPM.wav',170)]:
    candidates=[p for p in paths if p.name==filename]
    assert candidates,filename
    p=candidates[0];fs,x=load(p);factor=math.gcd(int(fs),48000);x=signal.resample_poly(x,48000//factor,fs//factor,axis=0)
    n=round(16*60/bpm*48000);x=np.tile(x,(math.ceil(n/len(x)),1))[:n]
    gain=min(10**(-18/20)/max(1e-20,float(np.sqrt(np.mean(x*x)))),10**(-3/20)/max(1e-20,float(np.max(np.abs(x)))))
    dest=out/(name+'.wav');wavfile.write(dest,48000,(x*gain).astype(np.float32))
    sources.append(dict(name=name,path=str(dest.resolve()),bpm=bpm,original=str(p),original_sha256=sha(p),prepared_sha256=sha(dest),source_kind='full drum loop',**describe(x*gain,48000)))
data=dict(method_sha256=sha(pathlib.Path(__file__)),wav_inventory=len(files),references=references,rejected=rejected,sources=sources)
(out/'materials.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
print(f'{len(files)} reference WAVs inventoried;{len(references)} analyzed,{len(rejected)} rejected;{len(sources)} prepared sources')
for item in sources:print(item['name'],item['bpm'],item['original'])
