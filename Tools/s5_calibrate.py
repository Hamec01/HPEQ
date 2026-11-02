"""S5 source/material/recipe-pinned calibration and actual program verification."""
import argparse
import hashlib
import importlib.util
import json
import math
import subprocess
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly, correlate

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'out/s5'
REPORT=ROOT/'docs/reports/s5'
EXE=ROOT/'build/Release/HamloShirota_PresetLab.exe'
MATERIAL=Path('C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav')
spec=importlib.util.spec_from_file_location('authored_recipes',ROOT/'Tools/genre_presets.py')
author=importlib.util.module_from_spec(spec);spec.loader.exec_module(author)


def sha(p,normalized=False):
    data=p.read_bytes();return hashlib.sha256(data.replace(b'\r\n',b'\n') if normalized else data).hexdigest()


def identifier(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,value):
    p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
def db(value):return 20*math.log10(max(float(value),1e-30))
def peak(x):return db(np.max(np.abs(resample_poly(x.astype(float),8,1,axis=0))))


def identities():
    names=sorted([str(p.relative_to(ROOT)).replace('\\','/') for p in (ROOT/'Source/DSP').rglob('*') if p.is_file()]+[
        'CMakeLists.txt','Source/Core/ModulePresets.cpp','Source/Core/ModulePresets.h',
        'Source/Standalone/ModulePluginProcessor.cpp','Source/Standalone/ModulePluginProcessor.h',
        'Source/Standalone/ModuleVariant.cpp','Source/Standalone/ModuleVariant.h',
        'Source/Standalone/V2Parameters.cpp','Source/Standalone/V2Parameters.h',
        'Assets/Hrtf/production.bin','Assets/Hrtf/production.json'])
    files={n:sha(ROOT/n,not n.endswith('.bin')) for n in names}
    methods={n:sha(ROOT/n,True) for n in ['Tests/PresetLabMain.cpp','Tools/s5_calibrate.py','docs/S5_METHODS.md']}
    return dict(sound_id=hashlib.sha256(('ShirotaS5SoundV1'+''.join(files.values())).encode()).hexdigest(),
                method_id=hashlib.sha256(('ShirotaS5MethodV1'+''.join(methods.values())).encode()).hexdigest(),files=files,methods=methods)


def native(folder,jobs=(),catalog=False):
    folder.mkdir(parents=True,exist_ok=True)
    run=0
    while (folder/f'run_{run}').exists():run+=1
    target=folder/f'run_{run}';request=folder/f'request_{run}.json'
    write(request,dict(jobs=list(jobs),catalog=catalog))
    completed=subprocess.run([str(EXE),str(request),str(target)],cwd=ROOT,capture_output=True,text=True)
    (folder/f'run_{run}.log').write_text(completed.stdout+completed.stderr,encoding='utf-8',newline='\n')
    if completed.returncode:raise RuntimeError(completed.stderr+completed.stdout)
    result=read(target/'result.json');identity=identities()
    assert result['sound_id']==identity['sound_id'] and result['method_id']==identity['method_id'],'Stale compiled native renderer'
    for row in result['renders']:
        assert row['finite'] and row['allocations']==0 and row['roundtrip'],row
        assert sha(target/(row['id']+'.f32'))==row['sha256'],'Changed raw output'
    return target,result


def prepare(name):
    original=MATERIAL/(name+'.wav');original_sha=sha(original)
    folder=WORK/'material'/name/original_sha[:16];record=folder/'identity.json'
    if record.exists():
        saved=read(record);assert saved['method_id']==identities()['method_id'],'Material method changed: use new run namespace'
        assert sha(folder/'input.f32')==saved['input_sha256'];return folder/'input.f32',saved
    rate,x=wavfile.read(original)
    if np.issubdtype(x.dtype,np.integer):x=x.astype(float)/(np.iinfo(x.dtype).max+1)
    else:x=x.astype(float)
    if x.ndim==1:x=np.column_stack([x,x])
    x=x[:,:2];divisor=math.gcd(rate,48000);x=resample_poly(x,48000//divisor,rate//divisor,axis=0)
    n=480000
    if len(x)<n:x=np.pad(x,((0,n-len(x)),(0,0)))
    candidates=np.arange(0,len(x)-n+1,4800);energy=np.concatenate([[0.],np.cumsum(np.sum(x*x,axis=1))])
    start=int(candidates[np.argmax(energy[candidates+n]-energy[candidates])]);x=x[start:start+n]
    rms=float(np.sqrt(np.mean(x*x)));assert rms>1e-8
    target=-14 if name.startswith('mixdown') else -18
    gain=min(10**(target/20)/rms,10**((-6-peak(x))/20));x*=gain
    x=np.pad(x,((0,48000),(0,0))).astype('<f4');folder.mkdir(parents=True,exist_ok=True);x.tofile(folder/'input.f32')
    saved=dict(original=str(original),original_sha256=original_sha,input_sha256=sha(folder/'input.f32'),source_start_samples=start,
               rate=48000,frames=len(x),gain=gain,prepared_peak_dbtp=peak(x),method_id=identities()['method_id'])
    write(record,saved);return folder/'input.f32',saved


def aligned(path):
    x=np.fromfile(path,dtype='<f4').reshape(-1,2).astype(float);return np.pad(x,((120,0),(0,0)))[:len(x)]


def strength(y,x,output=0):return db(np.sqrt(np.sum((y/10**(output/20)-x)**2)/np.sum(x*x)))


def calibrate():
    identity=identities();records={};failures=[]
    for index,recipe in enumerate(author.recipes()):
        source,material=prepare(recipe['material'])
        key=dict(sound_id=identity['sound_id'],method_id=identity['method_id'],recipe=recipe,input_sha256=material['input_sha256'])
        folder=WORK/'calibration'/recipe['id']/identifier(key)[:20];record=folder/'complete.json'
        if record.exists():
            result=read(record);assert result['identity']==key
            assert all(sha(Path(p))==h for p,h in result['files'].items()),'Corrupt calibration cache'
            records[recipe['id']]=result
            print(f'{index+1}/200 cached {recipe["name"]}',flush=True);continue
        try:
            run,result=native(folder,[dict(id='full',input=str(source),rate=48000,bpm=recipe['bpm'],program=0,values=recipe['values'])])
            y=np.fromfile(run/'full.f32',dtype='<f4').reshape(-1,2).astype(float);x=aligned(source)
            full=strength(y,x);target={'Gentle':-20,'Medium':-12,'Heavy':-4}[recipe['depth']]
            mix=10**((target-full)/20)
            assert 0<mix<=1,f'Unreachable {recipe["depth"]}: full residual {full:.3f}dB'
            predicted=x+mix*(y-x);output=min(0.,-1.05-peak(predicted))
            assert output>=-24,'Required output trim outside range'
            result=dict(identity=key,mix=mix,output_db=output,full_residual_db=full,predicted_residual_db=strength(predicted,x),
                        predicted_peak_dbtp=peak(predicted)+output,renderer_sha256=sha(EXE),files={str(run/'full.f32'):sha(run/'full.f32'),str(run/'result.json'):sha(run/'result.json')})
            write(record,result);records[recipe['id']]=result
            print(f'{index+1}/200 calibrated {recipe["name"]} mix={mix:.5f}',flush=True)
        except Exception as error:
            failures.append(dict(id=recipe['id'],error=str(error)));write(folder/'failure.json',failures[-1]);print(f'FAIL {recipe["name"]}: {error}',flush=True)
    write(REPORT/'calibration.json',dict(identity=identity,presets=records,failures=failures))
    assert len(records)==200 and not failures,'Calibration incomplete; failed recipes retained'


def catalog(label):
    folder,result=native(WORK/label,catalog=True);write(REPORT/(label+'.json'),dict(**result,renderer_sha256=sha(EXE),raw_manifest=str(folder/'result.json')))
    return result


def verify():
    current=catalog('catalog_final');before=read(REPORT/'catalog_before.json');cal=read(REPORT/'calibration.json')
    assert current['program_count']==207 and current['genre_count']==200
    assert current['programs'][:7]==before['programs'],'Original seven programs changed'
    records={};identity=identities();seen=set();names=set()
    bounds={p['id']:p for p in current['parameters']}
    for index,(recipe,compiled) in enumerate(zip(author.recipes(),current['programs'][7:])):
        assert compiled['id']==recipe['id'] and compiled['name']==recipe['name'] and compiled['program']==index+7
        assert compiled['id'] not in seen and compiled['name'] not in names;seen.add(compiled['id']);names.add(compiled['name'])
        for field in ['genre','group','note','material','depth','bpm']:assert compiled[field]==recipe[field],field
        expected=dict(recipe['values'],v2Mix=cal['presets'][recipe['id']]['mix'],v2Output=cal['presets'][recipe['id']]['output_db'])
        for key,value in expected.items():assert abs(compiled['values'][key]-value)<max(1e-4,abs(value)*2e-6),(key,value,compiled['values'][key])
        for key,p in bounds.items():assert p['minimum']<=compiled['values'][key]<=p['maximum'],key
        source,material=prepare(recipe['material']);key=dict(sound_id=identity['sound_id'],method_id=identity['method_id'],recipe=compiled,input_sha256=material['input_sha256'])
        folder=WORK/'verified'/recipe['id']/identifier(key)[:20];record=folder/'complete.json'
        if record.exists():
            result=read(record);assert result['identity']==key and all(sha(Path(p))==h for p,h in result['files'].items()),'Changed verified cache'
        else:
            run,report=native(folder,[dict(id='actual',input=str(source),rate=48000,bpm=recipe['bpm'],program=index+7,roundtrip=True)])
            y=np.fromfile(run/'actual.f32',dtype='<f4').reshape(-1,2).astype(float);x=aligned(source)
            depth=strength(y,x,compiled['values']['v2Output']);actual_peak=peak(y);lo,hi={'Gentle':(-24,-16),'Medium':(-16,-8),'Heavy':(-8,0)}[recipe['depth']]
            passed=lo<=depth<=hi and actual_peak<=-1 and report['renders'][0]['roundtrip']
            result=dict(identity=key,passed=passed,residual_db=depth,peak_dbtp=actual_peak,renderer_sha256=sha(EXE),roundtrip=True,
                        files={str(run/'actual.f32'):sha(run/'actual.f32'),str(run/'result.json'):sha(run/'result.json')})
            write(record,result)
        records[recipe['id']]=result;print(f'{index+1}/200 verified {recipe["name"]} depth={result["residual_db"]:.3f} peak={result["peak_dbtp"]:.3f} pass={result["passed"]}',flush=True)
    signatures=[]
    for row in current['programs'][7:]:
        signatures.append(identifier({k:v for k,v in row['values'].items() if k not in ['v2Mix','v2Output','v2Page','v2EditNode','v2Restart']}))
    assert len(set(signatures))==200,'Duplicate sound recipe beyond Mix/Output'
    write(REPORT/'verified.json',dict(identity=identity,presets=records,unique_recipes=len(set(signatures)),passed=sum(r['passed'] for r in records.values())))
    assert all(r['passed'] for r in records.values()) and len(records)==200


def filters():
    identity=identities();rows=[];bank=np.fromfile(ROOT/'Assets/Hrtf/production.bin',dtype='<f4').reshape(3,72,2,512)
    for rate in [8000,32000,44100,48000,88200,96000,176400,192000]:
        folder=WORK/'filters'/str(rate)/identifier(identity)[:16];source=folder/'impulse.f32';folder.mkdir(parents=True,exist_ok=True)
        frames=int(rate*.2);at=int(rate*.05);x=np.zeros((frames,2),dtype='<f4');x[at]=.5;x.tofile(source)
        jobs=[dict(id=f'p{p}_a{a}',input=str(source),rate=rate,bpm=120,program=0,values=dict(v2Mode=2,v2PositionOn=1,v2Source=1,v2Profile=p,v2Position=a if a<=180 else a-360)) for p in range(3) for a in [0,90,180,270]]
        run,report=native(folder,jobs);nfft=2**int(math.ceil(math.log2(rate*2)));all_hz=np.fft.rfftfreq(nfft,1/rate)
        selected=np.flatnonzero((all_hz>=100)&(all_hz<=min(16000,.4*rate)));selected=selected[::max(1,len(selected)//4000)];hz=all_hz[selected]
        for job,row in zip(jobs,report['renders']):
            profile=job['values']['v2Profile'];angle=job['values']['v2Position']%360
            y=np.fromfile(run/(job['id']+'.f32'),dtype='<f4').reshape(-1,2)[at:]*2
            h=bank[profile,angle//5].astype(float)
            # Independently evaluate the original native FIR at the host frequencies.
            reference=np.exp(-2j*np.pi*hz[:,None]/48000*np.arange(512))@h.T
            actual=np.fft.rfft(y,nfft,axis=0)[selected]
            magnitude=np.abs(reference);valid=(hz[:,None]>=100)&(hz[:,None]<=min(16000,.4*rate))&(magnitude>=np.max(magnitude,axis=0)*.01)
            error=float(np.max(np.abs(20*np.log10(np.maximum(np.abs(actual),1e-30)/np.maximum(magnitude,1e-30)))[valid]))
            lag=lambda a:int(np.argmax(correlate(a[:,0],a[:,1],mode='full'))-(len(a)-1))
            observed=lag(y);divisor=math.gcd(rate,48000);independent=resample_poly(h.T,rate//divisor,48000//divisor,axis=0);itd=abs(observed-lag(independent))
            rows.append(dict(rate=rate,profile=profile,angle=angle,magnitude_error_db=error,itd_error_samples=itd,passed=error<=.5 and itd<=1,raw=str(run/(job['id']+'.f32')),sha256=row['sha256']))
        print('Measured integrated filters at',rate,'Hz',flush=True)
    write(REPORT/'filters.json',dict(identity=identity,cases=rows,passed=sum(r['passed'] for r in rows)))
    assert all(r['passed'] for r in rows),'Resampled filter goal failed; see retained metrics'


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['before','calibrate','verify','filters']);a=p.parse_args();WORK.mkdir(parents=True,exist_ok=True)
    if a.action=='before':
        result=catalog('catalog_before');assert result['program_count']==7 and result['genre_count']==0
    elif a.action=='calibrate':calibrate()
    elif a.action=='verify':verify()
    else:filters()
