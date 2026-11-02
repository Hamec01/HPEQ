"""Production library A/Bs and three-profile headphone comparisons, common gain."""
import importlib.util
from pathlib import Path
import numpy as np
from scipy.io import wavfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('s5_lab',ROOT/'Tools/s5_calibrate.py')
lab=importlib.util.module_from_spec(spec);spec.loader.exec_module(lab)


def main():
    verified=lab.read(lab.REPORT/'verified.json');assert verified['passed']==200
    sound=lab.identities();assert verified['identity']==sound
    selections={'vocal_lead1':['Lead Frame'],'keys':['Felt Sides','Window Room'],
                'drumbus1':['Break Panorama','Moving Break'],'synth':['Dub Corner','Passing Texture']}
    groups=[]
    for material,titles in selections.items():
        source,input_identity=lab.prepare(material);dry=lab.aligned(source)
        selected=[r for r in lab.author.recipes() if r['material']==material and any(r['name'].endswith(t) for t in titles)]
        pin=dict(sound=sound['sound_id'],method=sound['method_id'],tool=lab.sha(Path(__file__),True),input=input_identity,
                 presets={r['id']:verified['presets'][r['id']]['files'] for r in selected})
        folder=lab.WORK/'listen'/material/lab.identifier(pin)[:20];record=folder/'complete.json'
        if record.exists():
            saved=lab.read(record);assert saved['identity']==pin and all(lab.sha(Path(p))==h for p,h in saved['files'].items())
            groups.append(saved);continue
        waves=[('00_Dry',dry)]
        for recipe in selected:
            native_file=next(Path(p) for p in verified['presets'][recipe['id']]['files'] if p.endswith('.f32'))
            waves.append((recipe['name'],np.fromfile(native_file,dtype='<f4').reshape(-1,2).astype(float)))
        if material=='vocal_lead1':
            jobs=[]
            for profile in range(3):
                for angle,name in [(0,'Front'),(90,'Left'),(180,'Back'),(-90,'Right')]:
                    jobs.append(dict(id=f'Profile_{profile+1}_{name}',input=str(source),rate=48000,bpm=90,program=0,
                                     values=dict(v2Mode=2,v2PositionOn=1,v2Source=1,v2Profile=profile,v2Position=angle)))
                values=dict(v2Mode=2,v2PositionOn=1,v2Source=1,v2Profile=profile,v2Motion=1,v2Nodes=4,v2Unit=0)
                for node,angle in enumerate([-90,180,90,0]):
                    values.update({f'v2Node{node}Angle':angle,f'v2Node{node}Hold':250,f'v2Node{node}Travel':750,f'v2Node{node}Arc':1})
                jobs.append(dict(id=f'Profile_{profile+1}_Orbit',input=str(source),rate=48000,bpm=90,program=0,values=values))
            run,result=lab.native(folder/'native',jobs)
            for row in result['renders']:waves.append((row['id'],np.fromfile(run/(row['id']+'.f32'),dtype='<f4').reshape(-1,2).astype(float)))
        worst=max(lab.peak(x) for _,x in waves);gain=min(1.,10**((-1.05-worst)/20));files={};clips=[]
        folder.mkdir(parents=True,exist_ok=True)
        for name,audio in waves:
            safe=''.join(c if c.isalnum() or c in ' _-' else '_' for c in name);path=folder/(safe+'.wav')
            y=(audio*gain).astype('<f4');actual=lab.peak(y);assert actual<=-1
            wavfile.write(path,48000,y);files[str(path)]=lab.sha(path);clips.append(dict(name=name,path=str(path),peak_dbtp=actual))
        saved=dict(identity=pin,common_gain=gain,files=files,clips=clips);lab.write(record,saved);groups.append(saved)
        print(material,len(clips),'production listening files',flush=True)
    lab.write(lab.REPORT/'demos.json',dict(sound_id=sound['sound_id'],groups=groups,count=sum(len(g['clips']) for g in groups),owner_listening='pending'))


if __name__=='__main__':main()
