"""Resumable actual-processor calibration/check; provenance precedes cache reuse."""
import argparse
import copy
import hashlib
import json
import subprocess
import time
from pathlib import Path
import numpy as np
from scipy.signal import resample_poly

ROOT=Path(__file__).resolve().parents[1];WORK=ROOT/'out/s5';REPORT=ROOT/'docs/reports/s5';FS=48000
DSP_FILES=['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h','Source/DSP/LegacyGateProcessor.cpp','Source/DSP/LegacyGateProcessor.h','Source/DSP/LegacyZastavaCore.cpp','Source/DSP/LegacyZastavaCore.h','Source/DSP/HamloZastava.cpp','Source/DSP/HamloZastava.h','Source/Standalone/ModulePluginProcessor.cpp','Source/Standalone/ModulePluginProcessor.h','Source/Standalone/ModernParameters.cpp','Source/Standalone/ModernParameters.h','Source/Standalone/ModuleVariant.cpp','Source/Standalone/ModuleVariant.h','Source/Core/ModulePresets.cpp','Source/Core/ModulePresets.h','Source/Core/GenrePresets.cpp']
METHOD_FILES=['Tests/PresetLabMain.cpp','Tools/prepare_zastava_s5.py','Tools/calibrate_zastava_s5.py','docs/S5_METHODS.md']
CATALOG_FILES=['Source/Presets/ZastavaRecipes.json','Source/Presets/GeneratedZastavaPresets.h']
def sha(p,normal=False):
    b=p.read_bytes();return hashlib.sha256(b.replace(b'\r\n',b'\n') if normal else b).hexdigest()
def digest(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def identity(prefix,files):return hashlib.sha256((prefix+''.join(sha(ROOT/n,True) for n in files)).encode()).hexdigest()
def write(p,value):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
def db(x):return float(20*np.log10(max(float(x),1e-30)))
def validate_cache(meta,key,hello):
    assert meta['key']==key,'Changed recipe/input/request'
    for field in ['dsp_id','method_id','catalog_id']:assert meta['reply'][field]==hello[field],'Changed '+field
    assert sha(Path(meta['output']))==meta['reply']['output_sha256'],'Corrupt cached output'

class Worker:
    def __init__(self):
        self.p=subprocess.Popen([str(ROOT/'build/Release/HamloZastava_PresetLab.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,encoding='utf-8')
        self.hello=self.ask(dict(cmd='hello'))
        assert self.hello['dsp_id']==identity('ZastavaPresetDSPV1',DSP_FILES),'Stale DSP worker'
        assert self.hello['method_id']==identity('ZastavaPresetMethodV1',METHOD_FILES),'Stale method worker'
        assert self.hello['catalog_id']==identity('ZastavaPresetCatalogV1',CATALOG_FILES),'Stale catalog worker'
    def ask(self,request):
        self.p.stdin.write(json.dumps(request)+'\n');self.p.stdin.flush();line=self.p.stdout.readline()
        if not line:raise RuntimeError('Worker stopped: '+self.p.stderr.read()[-1000:])
        result=json.loads(line);assert 'error' not in result,result;return result
    def close(self):self.p.stdin.close();self.p.wait(timeout=30)
    def render(self,r,fixture,overrides,state=False,natural=False):
        main=fixture['natural' if natural else 'fixture'];key=fixture['natural_key' if natural else 'key'];assert sha(Path(main['path']))==main['sha256']
        if key:assert sha(Path(key['path']))==key['sha256']
        request=dict(cmd='render',program=r['program'],main=main['path'],key=key['path'] if key else '',bpm=r['bpm'],overrides=overrides,state=state)
        binding=dict(request=request,recipe=r,main_sha256=main['sha256'],key_sha256=key['sha256'] if key else None,identity={k:self.hello[k] for k in ['dsp_id','method_id','catalog_id']})
        cachekey=digest(binding);folder=WORK/'cache'/cachekey;meta_path=folder/'result.json'
        if meta_path.exists():
            result=json.loads(meta_path.read_text());validate_cache(result,cachekey,self.hello);return result
        folder.mkdir(parents=True,exist_ok=True);attempt=1
        while (folder/f'output_{attempt}.f32').exists():attempt+=1
        output=folder/f'output_{attempt}.f32';request['output']=str(output.resolve());reply=self.ask(request)
        result=dict(key=cachekey,binding=binding,output=str(output.resolve()),reply=reply);validate_cache(result,cachekey,self.hello);write(meta_path,result);return result

def metrics(r,fixture,record,natural=False):
    xinfo=fixture['natural' if natural else 'fixture'];x=np.fromfile(xinfo['path'],dtype='<f4').reshape(-1,2).astype(float);y=np.fromfile(record['output'],dtype='<f4').reshape(-1,2).astype(float)
    reply=record['reply'];truepeak=float(np.max(np.abs(resample_poly(y,4,1,axis=0))));result=dict(peak_dbtp=db(truepeak),finite=bool(np.isfinite(y).all()),allocations=reply['allocations'],state_exact=reply['state_exact'],audio_exact=reply['audio_exact'],parameters_exact=reply['parameters_exact'])
    result['base_pass']=result['finite'] and result['peak_dbtp']<=-1 and reply['allocations']==0 and reply['latency']==240 and not reply['missing'] and all(result[k] for k in ['state_exact','audio_exact','parameters_exact'])
    if natural:result['passed']=result['base_pass'];return result
    d=240;x=x[:-d];y=y[d:];t=np.arange(len(x))/FS;v=reply['values']
    def mask(windows):
        m=np.zeros(len(t),dtype=bool)
        for a,b in windows:m|=(t>=a)&(t<b)
        return m
    def gain(m):
        assert np.any(m) and np.sum(x[m]**2)>1e-20,'Empty measurement window'
        return 10*float(np.log10(max(np.sum(y[m]**2),1e-40)/np.sum(x[m]**2)))
    depth={'Gentle':(4,12),'Medium':(12,24),'Strong':(24,48)}[r['depth']]
    if r['role']=='Clean':
        events=fixture['events'];wanted=gain(mask([(b+.05,b+.4) for b in events]));quiet=gain(mask([(b+.45,b+.7) for b in events]));onset=gain(mask([(b+.05,b+.07) for b in events]));rejection=-gain(mask([(b+1.12,b+1.45) for b in events]));result.update(rejection_db=rejection,wanted_loss_db=-wanted,quiet_loss_db=-quiet,onset_loss_db=-onset);quality=depth[0]<=rejection<=depth[1] and -wanted<=1 and -quiet<=2 and -onset<=3
    elif r['role']=='Drum':
        events=fixture['events'];end=(v['v2_attack']+v['v2_body']+v['v2_tail'])/1000;windows=[(b+end+.01,min(b+end+.04,(events[n+1] if n<7 else b+60/r['bpm']*.5)-.015)) for n,b in enumerate(events)];valid=all(b>a for a,b in windows)
        rejection=-gain(mask(windows)) if valid else -999;attack=-gain(mask([(b,b+.02) for b in events]));result.update(rejection_db=rejection,attack_loss_db=attack,hits=reply['hits'],tail_window_valid=valid);quality=valid and depth[0]<=rejection<=depth[1] and attack<=1 and reply['hits']==8
    elif r['role']=='Duck':
        beat=60/r['bpm'];events=fixture['events'];atten=-gain(mask([(b+.02,b+.065) for b in events]));recover=-gain(mask([(b+beat-.035,b+beat-.005) for b in events]));lo,hi={'Gentle':(1,3),'Medium':(3,6),'Strong':(6,12)}[r['depth']];result.update(duck_db=atten,recovery_loss_db=recover);quality=lo<=atten<=hi and recover<=1.5
    else:
        step=60/r['bpm']*v['v2_beats'] if v['v2_sync'] else v['v2_ms']/1000;levels=[v[f'v2_step{n+1}'] for n in range(int(v['v2_steps']))];full=[(n*step+.45*step,n*step+.55*step) for n,value in enumerate(levels) if value==1];floor=[(n*step+.45*step,n*step+.55*step) for n,value in enumerate(levels) if value==0];open_gain=gain(mask(full));closed_gain=gain(mask(floor));contrast=open_gain-closed_gain;result.update(contrast_db=contrast,open_loss_db=-open_gain,full_steps=len(full),floor_steps=len(floor));quality=len(full)>=2 and len(floor)>=2 and abs(open_gain)<=.2 and depth[0]<=contrast<=depth[1]
    result['quality_pass']=bool(quality);result['passed']=bool(result['base_pass'] and quality);return result

def active_signature(row):
    v=row['values'];mode=int(v['v2_engine']);common=['v2_engine','v2_profile','v2_range'];keys=['v2_external','v2_hp','v2_lp','v2_link','v2_lookahead','v2_threshold']
    if mode==1:ids=common+keys+['v2_expand','v2_rms','v2_attack','v2_hold','v2_release','v2_detectorMs']+(['v2_ratio','v2_knee'] if v['v2_expand'] else ['v2_hysteresis'])
    elif mode==2:ids=common+keys+['v2_attack','v2_body','v2_tail','v2_onset','v2_retrigger']
    elif mode==3:ids=common+keys+['v2_rms','v2_attack','v2_hold','v2_release','v2_hysteresis','v2_detectorMs']
    else:ids=common+['v2_sync','v2_steps','v2_attack','v2_release', 'v2_beats' if v['v2_sync'] else 'v2_ms']+[f'v2_step{i+1}' for i in range(int(v['v2_steps']))]
    return digest({k:round(v[k],6) for k in ids})

def calibrate(worker,recipes,fixtures):
    proposals=[];failures=[]
    for index,r in enumerate(recipes):
        fixture=fixtures['recipes'][r['id']];base=dict(r['values']);candidates=[{}]
        if r['role']=='Clean':candidates=[{'v2_threshold':t} for t in [-46,-42,-38,-34,-50,-30]]
        elif r['role']=='Drum':candidates=[dict(v2_threshold=t,v2_onset=o,v2_retrigger=g) for o,g in [(8,25),(12,40),(16,60),(20,90),(24,120)] for t in [-38,-32,-44]]
        selected=None;trials=[]
        for override in candidates:
            record=worker.render(r,fixture,override);m=metrics(r,fixture,record);trials.append(dict(overrides=override,key=record['key'],metrics=m))
            if m['passed']:selected=override;break
        if selected is None:failures.append(r['id'])
        proposals.append(dict(id=r['id'],base_control_hash=digest(base),overrides=selected,trials=trials))
        write(WORK/'calibration_progress.json',dict(completed=index+1,total=200,failed=failures,identity=worker.hello,rows=proposals))
        if (index+1)%10==0:print('Calibration',index+1,'/200; failed',len(failures),flush=True)
    result=dict(identity=worker.hello,recipes_sha256=sha(ROOT/'Source/Presets/ZastavaRecipes.json'),fixtures_sha256=sha(WORK/'fixtures/manifest.json'),rows=proposals,failed=failures)
    write(REPORT/'calibration.json',result);return not failures

def check(worker,recipes,fixtures):
    catalog=worker.ask(dict(cmd='catalog'));write(REPORT/'catalog.json',catalog);assert len(catalog['programs'])==207
    actual={r['id']:r for r in catalog['programs'][7:]};assert len(actual)==200 and len(set(r['name'] for r in actual.values()))==200
    signatures=[active_signature(r) for r in actual.values()];assert len(set(signatures))==200,'Duplicate active recipes'
    rows=[];failed=[]
    for index,r in enumerate(recipes):
        a=actual[r['id']];assert a['program']==r['program'] and not a['modified'];assert a['values']['v2_listen']==0 and a['values']['v2_mix']==1 and a['values']['v2_output']==0
        fixture=fixtures['recipes'][r['id']];record=worker.render(r,fixture,{},True);m=metrics(r,fixture,record);natural=worker.render(r,fixture,{},False,True);n=metrics(r,fixture,natural,True)
        passed=m['passed'] and n['passed'];rows.append(dict(id=r['id'],program=r['program'],genre=r['genre'],role=r['role'],depth=r['depth'],key=record['key'],natural_key=natural['key'],metrics=m,natural=n,active_signature=active_signature(a),passed=passed))
        if not passed:failed.append(r['id'])
        write(WORK/'check_progress.json',dict(completed=index+1,total=200,failed=failed,rows=rows,identity=worker.hello))
        if (index+1)%10==0:print('Library',index+1,'/200; failed',len(failed),flush=True)
    result=dict(identity=worker.hello,fixtures_sha256=sha(WORK/'fixtures/manifest.json'),recipes_sha256=sha(ROOT/'Source/Presets/ZastavaRecipes.json'),rows=rows,passed=200-len(failed),failed=failed);write(REPORT/'library_check.json',result);return not failed

def apply():
    report=json.loads((REPORT/'calibration.json').read_text());assert not report['failed'];path=ROOT/'Source/Presets/ZastavaRecipes.json';assert sha(path)==report['recipes_sha256'],'Seeds changed';recipes=json.loads(path.read_text());rows={r['id']:r for r in report['rows']}
    for r in recipes:assert digest(r['values'])==rows[r['id']]['base_control_hash'];r['values'].update(rows[r['id']]['overrides'])
    write(WORK/'seed_recipes.json',json.loads(path.read_text()));write(path,recipes);subprocess.run([str(Path(__import__('sys').executable)),'-I','-X','utf8',str(ROOT/'Tools/generate_zastava_presets.py'),'export'],check=True)

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['calibrate','apply','check']);args=p.parse_args()
    if args.action=='apply':apply();return 0
    recipes=json.loads((ROOT/'Source/Presets/ZastavaRecipes.json').read_text());fixtures=json.loads((WORK/'fixtures/manifest.json').read_text());worker=Worker()
    try:ok=calibrate(worker,recipes,fixtures) if args.action=='calibrate' else check(worker,recipes,fixtures)
    finally:worker.close()
    return 0 if ok else 1


if __name__=='__main__':raise SystemExit(main())
