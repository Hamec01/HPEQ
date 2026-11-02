"""Resumable source/material/recipe-pinned factory calibration and compiled-bank QA."""
import argparse,hashlib,json,subprocess,sys,time,re
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT=Path(__file__).resolve().parents[1]
PYTHON=sys.executable
EXE=ROOT/'build/Release/HamloEcho_ScorePresetLab.exe'
OUT=ROOT/'out/presets'
METHOD='echo-energy-both-routes-v1'
RANGES={'Gentle':(-24,-16),'Medium':(-16,-8),'Heavy':(-8,6)}
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for chunk in iter(lambda:f.read(1048576),b''):h.update(chunk)
    return h.hexdigest()
def save(p,x):p.parent.mkdir(parents=True,exist_ok=True);tmp=p.with_suffix(p.suffix+'.tmp');tmp.write_text(json.dumps(x,indent=2,default=lambda v:v.item() if isinstance(v,np.generic) else str(v))+'\n',encoding='utf-8');tmp.replace(p)
def load(p):return json.loads(p.read_text(encoding='utf-8'))
def run(jobs,path):
    if not jobs:return
    save(path,jobs);subprocess.run([str(EXE),'render',str(path)],cwd=ROOT,check=True)
def prepare(directory,rows):
    result={};target=OUT/'materials';target.mkdir(parents=True,exist_ok=True)
    for name in sorted({r['material'] for r in rows}):
        source=directory/(name+'.wav');fingerprint=sha(source);meta=target/(name+'.json');dest=target/(name+'.wav')
        if meta.exists() and dest.exists():
            old=load(meta)
            if old['source_sha256']==fingerprint and old['prepared_sha256']==sha(dest):result[name]=old;continue
        rate,x=wavfile.read(source)
        if np.issubdtype(x.dtype,np.integer):x=x.astype(float)/max(abs(np.iinfo(x.dtype).min),np.iinfo(x.dtype).max)
        else:x=x.astype(float)
        if x.ndim==1:x=np.repeat(x[:,None],2,axis=1)
        x=x[:,:2];length=min(len(x),int(rate*10));hop=max(1,int(rate*.25))
        energy=np.r_[0.,np.cumsum(np.mean(x*x,axis=1))];starts=np.arange(0,len(x)-length+1,hop);start=int(starts[np.argmax(energy[starts+length]-energy[starts])]);x=x[start:start+length]
        from math import gcd
        g=gcd(int(rate),48000);x=resample_poly(x,48000//g,int(rate)//g,axis=0)
        gain=min(10**(-18/20)/(np.sqrt(np.mean(x*x))+1e-30),10**(-3/20)/(np.max(abs(x))+1e-30));x=(gain*x).astype('<f4');wavfile.write(dest,48000,x)
        record=dict(path=str(dest),source=str(source),source_sha256=fingerprint,prepared_sha256=sha(dest),start_source_sample=start,source_rate=int(rate),samples=len(x),gain=gain,rms_db=20*np.log10(np.sqrt(np.mean(x.astype(float)**2))+1e-30),peak_db=20*np.log10(np.max(abs(x))+1e-30))
        save(meta,record);result[name]=record
    return result
def identity(row,material,manifest,tool):
    key=dict(method=METHOD,dsp_source=manifest['dsp_source'],tool_sha256=tool,recipe_sha256=row['recipe_sha256'],material_sha256=material['prepared_sha256'],bpm=row['bpm'])
    return hashlib.sha256(json.dumps(key,sort_keys=True).encode()).hexdigest(),key
def current_dsp():
    version=re.search(r'project\(HamloEcho VERSION ([\d.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
    paths=sorted([p for directory in ['DSP','Standalone'] for p in (ROOT/'Source'/directory).rglob('*') if p.is_file()],key=lambda p:str(p).replace('\\','/'))
    return hashlib.sha256((version+''.join(sha(p) for p in paths)).encode()).hexdigest()
def job(row,program,material,folder,routing,dsp,override=None):
    folder.mkdir(parents=True,exist_ok=True);j=dict(program=program,input=material['path'],bpm=row['bpm'],routing=routing,raw=str(folder/f'{routing}.f32'),report=str(folder/f'{routing}.json'),expected_dsp=dsp)
    if override is not None:j['output_db']=override
    return j
def truepeak(x):
    # Offline 8x polyphase estimate; process one complete render, including tail.
    return float(np.max(abs(resample_poly(x,8,1,axis=0,window=('kaiser',8.6)))))
def main():
    ap=argparse.ArgumentParser();ap.add_argument('mode',choices=['prepare','calibrate','check']);ap.add_argument('--material-dir',type=Path,default=Path('C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav'));args=ap.parse_args()
    rows=load(ROOT/'Tools/score_recipes.json');materials=prepare(args.material_dir,rows);save(OUT/'materials.json',materials)
    if args.mode=='prepare':print(f'Prepared {len(materials)} owner materials',flush=True);return 0
    subprocess.run([str(EXE),'manifest',str(OUT/'compiled_bank.json')],cwd=ROOT,check=True);manifest=load(OUT/'compiled_bank.json');tool=sha(EXE)
    assert manifest['dsp_source']==current_dsp(),'Laboratory executable was built from a different DSP source'
    assert manifest['program_count']==208 and len(manifest['presets'])==200
    bank={r['id']:r for r in manifest['presets']};assert set(bank)=={r['id'] for r in rows}
    if args.mode=='calibrate':
        jobs=[];records={}
        for row in rows:
            material=materials[row['material']];key,pin=identity(row,material,manifest,tool);folder=OUT/'calibration'/row['id']/key
            j=job(row,bank[row['id']]['program'],material,folder,1,manifest['dsp_source'],0);records[row['id']]=(j,pin,key)
            if not Path(j['report']).exists():jobs.append(j)
        run(jobs,OUT/'calibrate_jobs.json');cal={};failed=[]
        for row in rows:
            j,pin,key=records[row['id']];r=load(Path(j['report']));depth=10*np.log10(r['wet_energy']/r['dry_energy']);lo,hi=RANGES[row['depth']]
            target_insert=(lo+hi-20*np.log10(2))/2;output_db=target_insert+20*np.log10(2)-depth
            good=r['finite'] and r['state_exact'] and -24<=output_db<=12 and r['wet_energy']>0
            if not good:failed.append(dict(id=row['id'],output_db=output_db,result=r))
            cal[row['id']]=dict(**pin,resume_key=key,output_db=float(output_db),uncalibrated_send_db=float(depth),target_insert_db=target_insert,passed=bool(good))
        save(OUT/'calibration_results.json',cal);save(ROOT/'docs/reports/presets/calibration.json',dict(method=METHOD,dsp_source=manifest['dsp_source'],completed=len(cal),failed=failed,materials=materials,results=cal))
        print(f'Calibration: {len(cal)} measured, {len(failed)} failures',flush=True);return int(bool(failed))
    cal=load(OUT/'calibration_results.json');jobs=[];records={};fingerprints=set()
    for row in rows:
        b=bank[row['id']];c=cal[row['id']];assert c['passed'] and c['recipe_sha256']==row['recipe_sha256'] and c['dsp_source']==manifest['dsp_source'],'Stale calibration'
        assert abs(b['values']['scoreOutput']-c['output_db'])<1e-3,'Compiled gain differs from calibration'
        for field in ['name','genre','group','material','depth','bpm','note']:
            assert b[field]==row[field],f'Compiled metadata mismatch: {row["id"]} {field}'
        for field,value in row['values'].items():
            if field!='scoreOutput':assert abs(b['values'][field]-value)<=max(1e-4,abs(value)*1e-6),f'Compiled recipe mismatch: {row["id"]} {field}'
        # Compare compiled values rather than trusting the authored sparse dictionary.
        fingerprint=hashlib.sha256(json.dumps({k:round(v,5) for k,v in b['values'].items() if k not in ['scoreMix','scoreOutput','scoreRouting','scoreMixLock','scoreOctaves']},sort_keys=True).encode()).hexdigest()
        assert fingerprint not in fingerprints,'Duplicate compiled recipe';fingerprints.add(fingerprint)
        key,pin=identity(row,materials[row['material']],manifest,tool);folder=OUT/'verification'/row['id']/key
        pair=[job(row,b['program'],materials[row['material']],folder,routing,manifest['dsp_source']) for routing in [0,1]];records[row['id']]=(pair,key)
        for j in pair:
            if not Path(j['report']).exists():jobs.append(j)
    save(OUT/'verification_progress.json',dict(dsp_source=manifest['dsp_source'],phase='rendering',total_renders=400,missing_renders=len(jobs),completed=0,failed=0))
    run(jobs,OUT/'verify_jobs.json');results=[];failures=[];demo_ids={'echo.bb.drums.06','echo.trap.vocal.09','echo.techno.synth.06','echo.dnb.fx.03'}
    for row in rows:
        pair,key=records[row['id']];a,b=[load(Path(j['report'])) for j in pair]
        insert,send=[np.fromfile(j['raw'],dtype='<f4').reshape(-1,2).astype(float) for j in pair]
        _,dry=wavfile.read(materials[row['material']]['path']);dry=dry.astype(float);padded=np.zeros_like(insert);padded[:len(dry)]=dry
        wet=insert-(1-a['mix'])*padded;null=float(np.max(abs(wet-a['mix']*send)));di=10*np.log10(np.sum(wet*wet)/np.sum(dry*dry));ds=10*np.log10(np.sum(send*send)/np.sum(dry*dry));lo,hi=RANGES[row['depth']]
        peak_i=truepeak(insert);peak_s=truepeak(send);peak_d=truepeak(dry);scale=min(1.,10**(-1.1/20)/max(peak_i,peak_s,peak_d));corr=float(np.corrcoef(send.T)[0,1]);mono=np.mean(send,axis=1);loss=float(10*np.log10(2*np.sum(mono*mono)/(np.sum(send*send)+1e-30)+1e-30))
        checks=dict(depth_insert=lo<=di<hi,depth_send=lo<=ds<hi,finite=a['finite'] and b['finite'] and np.isfinite(insert).all() and np.isfinite(send).all(),state=a['state_exact'] and b['state_exact'],send_null=null<=2e-6,tail=a['last_second_energy']/(a['energy']+1e-30)<1e-8 and b['last_second_energy']/(b['energy']+1e-30)<1e-8,demo_peak=max(peak_i,peak_s,peak_d)*scale<=10**(-1/20))
        record=dict(id=row['id'],program=bank[row['id']]['program'],resume_key=key,depth=row['depth'],insert_wet_db=float(di),send_db=float(ds),insert_true_peak_db=float(20*np.log10(peak_i+1e-30)),send_true_peak_db=float(20*np.log10(peak_s+1e-30)),send_reconstruction_error=null,correlation=corr,mono_loss_db=loss,demo_scale=scale,checks={k:bool(v) for k,v in checks.items()},passed=bool(all(checks.values())))
        results.append(record)
        if not record['passed']:failures.append(record)
        if row['id'] in demo_ids:
            dest=OUT/'demos'/row['id'];dest.mkdir(parents=True,exist_ok=True)
            for name,x in [('dry',padded),('insert',insert),('send',send)]:wavfile.write(dest/(name+'.wav'),48000,(scale*x).astype('<f4'))
            save(dest/'identity.json',dict(recipe=row,verification=record,dsp_source=manifest['dsp_source'],tool_sha256=tool))
        save(OUT/'verification_progress.json',dict(dsp_source=manifest['dsp_source'],phase='analysis',completed=len(results),failed=len(failures)))
    report=dict(method=METHOD,dsp_source=manifest['dsp_source'],tool_sha256=tool,programs=208,recipes=200,passed=sum(r['passed'] for r in results),failed=len(failures),unique=len(fingerprints),materials=materials,results=results)
    save(ROOT/'docs/reports/presets/verification.json',report);save(OUT/'verification_progress.json',dict(dsp_source=manifest['dsp_source'],phase='complete',completed=200,failed=len(failures)));print(f"Compiled bank: {report['passed']}/200, {len(failures)} failures",flush=True)
    for r in failures:print(r['id'],r['checks'],flush=True)
    return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
