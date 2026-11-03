"""Release evidence, negative cache guards, editor captures and owner audition files."""
import argparse
import copy
import importlib.util
import json
import re
import struct
import subprocess
import zipfile
from pathlib import Path
import numpy as np
from scipy.io import wavfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cal',ROOT/'Tools/calibrate_zastava_s5.py')
cal=importlib.util.module_from_spec(spec);spec.loader.exec_module(cal)
REPORT=cal.REPORT;WORK=cal.WORK;sha=cal.sha;write=cal.write
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def source(prefix,root=ROOT):return cal.identity(prefix,[p.relative_to(ROOT).as_posix() for p in sorted((root/'Source').rglob('*')) if p.is_file()]) if root==ROOT else __import__('hashlib').sha256((prefix+''.join(sha(p,True) for p in sorted((root/'Source').rglob('*')) if p.is_file())).encode()).hexdigest()

def assets():
    result=read(REPORT/'library_check.json');assert result['passed']==200 and not result['failed']
    fixtures=read(WORK/'fixtures/manifest.json');recipes={r['id']:r for r in read(ROOT/'Source/Presets/ZastavaRecipes.json')}
    folder=REPORT/'images';assert not folder.exists();folder.mkdir()
    exe=ROOT/'build/Release/HamloZastava_Snapshot.exe';stamp=source('ZastavaS1SourceV1');assert stamp.encode() in exe.read_bytes()
    images=[]
    for name,program,view,tab,scale in [('bb_clean',7,0,0,1),('tr_drum',73,0,0,1),('tc_duck',137,1,1,1),('db_rhythm',195,1,2,1),('small_rhythm',206,0,2,.5),('legacy',1,2,0,1)]:
        path=folder/(name+'.png');args=[str(path),str(program),'preset',str(view),str(tab),str(scale)];subprocess.run([str(exe),*args],check=True,capture_output=True)
        width,height=struct.unpack('>II',path.read_bytes()[16:24]);assert (width,height)==(round(1180*scale),round(780*scale))
        images.append(dict(name=name,args=args,sha256=sha(path)))
    write(REPORT/'screenshots.json',dict(source_id=stamp,exe_sha256=sha(exe),driver_sha256=sha(ROOT/'Tests/SnapshotMain.cpp',True),images=images))
    folder=WORK/'auditions';assert not folder.exists();folder.mkdir();rows=[]
    for genre in ['Boom Bap','Trap','Techno','DnB']:
        for role in ['Clean','Drum','Duck','Rhythm']:
            row=next(r for r in result['rows'] if r['genre']==genre and r['role']==role);recipe=recipes[row['id']];fixture=fixtures['recipes'][row['id']]
            record=read(WORK/'cache'/row['natural_key']/'result.json');cal.validate_cache(record,row['natural_key'],result['identity'])
            x=np.fromfile(fixture['natural']['path'],dtype='<f4').reshape(-1,2);y=np.fromfile(record['output'],dtype='<f4').reshape(-1,2);dry=np.zeros_like(x);dry[240:]=x[:-240]
            name=row['id'];paths=[]
            for suffix,audio in [('dry',dry),('processed',y)]:
                path=folder/(name+'_'+suffix+'.wav');wavfile.write(path,48000,audio);paths.append(dict(path=str(path),sha256=sha(path)))
            if fixture['natural_key']:
                key=np.fromfile(fixture['natural_key']['path'],dtype='<f4').reshape(-1,2);path=folder/(name+'_external_key.wav');wavfile.write(path,48000,key);paths.append(dict(path=str(path),sha256=sha(path)))
            rows.append(dict(id=name,program=recipe['program'],name=recipe['name'],bpm=recipe['bpm'],peak_dbtp=row['natural']['peak_dbtp'],files=paths))
    write(REPORT/'auditions.json',dict(identity=result['identity'],rate=48000,rows=rows,owner_listening='pending'))
    print('Captured6editor pages;16A/B pairs with Duck key WAVs')

def audit():
    library=read(REPORT/'library_check.json');assert library['passed']==200 and library['failed']==[]
    hello=library['identity'];assert hello['dsp_id']==cal.identity('ZastavaPresetDSPV1',cal.DSP_FILES);assert hello['method_id']==cal.identity('ZastavaPresetMethodV1',cal.METHOD_FILES);assert hello['catalog_id']==cal.identity('ZastavaPresetCatalogV1',cal.CATALOG_FILES)
    assert library['fixtures_sha256']==sha(WORK/'fixtures/manifest.json') and library['recipes_sha256']==sha(ROOT/'Source/Presets/ZastavaRecipes.json')
    catalog=read(REPORT/'catalog.json')['programs'];recipes=read(ROOT/'Source/Presets/ZastavaRecipes.json');assert len(catalog)==207 and len(recipes)==200
    fixtures=read(WORK/'fixtures/manifest.json');signatures=set();worst_time=0
    calibration=read(REPORT/'calibration.json');seeds=read(REPORT/'calibration_seeds.json');initial=read(REPORT/'initial_recipes.json')
    assert calibration['failed']==[] and calibration['recipes_sha256']==sha(REPORT/'calibration_seeds.json')
    assert calibration['fixtures_sha256']==library['fixtures_sha256']
    assert all(calibration['identity'][k]==hello[k] for k in ['dsp_id','method_id'])
    for original,seed,recipe,proposal in zip(initial,seeds,recipes,calibration['rows']):
        expected=copy.deepcopy(seed);expected['values'].update(proposal['overrides']);assert expected==recipe
        assert proposal['base_control_hash']==cal.digest(seed['values']) and proposal['id']==recipe['id']
        differences={k for k in set(original['values'])|set(seed['values']) if original['values'].get(k)!=seed['values'].get(k)}
        assert differences in [set(),{'v2_body'}]
        assert set(proposal['overrides'])<=set(['v2_threshold','v2_onset','v2_retrigger','v2_range'])
    for recipe,row,actual in zip(recipes,library['rows'],catalog[7:]):
        assert recipe['id']==row['id']==actual['id'] and recipe['program']==actual['program'] and not actual['modified']
        for key,value in recipe['values'].items():assert abs(actual['values'][key]-value)<=.011,(recipe['id'],key)
        for key in ['name','genre','note','description','material','depth','bpm']:assert actual[key]==recipe[key]
        signatures.add(cal.active_signature(actual));fixture=fixtures['recipes'][row['id']]
        for natural,field in [(False,'key'),(True,'natural_key')]:
            record=read(WORK/'cache'/row[field]/'result.json');cal.validate_cache(record,row[field],hello);metrics=cal.metrics(recipe,fixture,record,natural);assert metrics==row['natural' if natural else 'metrics'] and metrics['passed'];worst_time=max(worst_time,record['reply']['mean_fraction'])
        for field in ['fixture','key','natural','natural_key']:
            item=fixture[field]
            if item:assert sha(Path(item['path']))==item['sha256']
    assert len(signatures)==200
    guards=[];sample=read(WORK/'cache'/library['rows'][0]['key']/'result.json')
    def reject(name,meta,key,identity):
        try:cal.validate_cache(meta,key,identity)
        except AssertionError as e:guards.append(dict(name=name,rejected=True,reason=str(e)));return
        raise AssertionError('Guard accepted '+name)
    for field in ['dsp_id','method_id','catalog_id']:
        changed=copy.deepcopy(hello);changed[field]='0'*64;reject(field,sample,sample['key'],changed)
    for name,field in [('input','main_sha256'),('recipe','recipe'),('request','request')]:
        binding=copy.deepcopy(sample['binding']);binding[field]='changed';reject(name,sample,cal.digest(binding),hello)
    corrupt=WORK/'guard_corrupt.f32';raw=bytearray(Path(sample['output']).read_bytes());raw[100]^=1
    if corrupt.exists():assert corrupt.read_bytes()==raw
    else:corrupt.write_bytes(raw)
    bad=copy.deepcopy(sample);bad['output']=str(corrupt);reject('raw_output',bad,sample['key'],hello)
    worker=cal.Worker()
    try:
        recipe=recipes[0];fixture=fixtures['recipes'][recipe['id']];cached=worker.render(recipe,fixture,{},True);before=Path(cached['output']).stat().st_mtime_ns;again=worker.render(recipe,fixture,{},True);assert again==cached and Path(again['output']).stat().st_mtime_ns==before
    finally:worker.close()
    write(REPORT/'cache_guards.json',dict(guards=guards,resume_without_rewrite=True))
    host=read(REPORT/'host_final.json');legacy=read(REPORT/'legacy_final.json');baseline=read(REPORT/'before_legacy_final.json')
    methods=cal.identity('ZastavaS5MethodV1',['Tests/S5LegacyLabMain.cpp','Tests/S5HostLabMain.cpp','docs/S5_METHODS.md'])
    assert host['source_id']==legacy['source_id']==source('ZastavaS5SourceV1');assert host['method_id']==legacy['method_id']==baseline['method_id']==methods
    assert host['passed']==283 and host['failed']==host['unavailable']==0
    assert len(legacy['cases'])==229 and legacy['failures']==0
    old=read(ROOT/'out/s1/reference_v3/capture.json')
    for current,control,original in zip(legacy['cases'],baseline['cases'],old['cases']):
        assert all(current[k] for k in ['bit_exact','state_exact','latency_exact','finite'])
        for field in ['case','render_sha256','state_sha256','latency_events','restorations']:assert current[field]==control[field]==original[field]
    for name in ['UI/PresetScreen.cpp','UI/PresetScreen.h','UI/BlackSteel.cpp','UI/BlackSteel.h','UI/HamloZastavaPanel.cpp','UI/HamloZastavaPanel.h','Standalone/ModuleVariant.cpp','Standalone/ModuleVariant.h','Standalone/ModernParameters.cpp','Standalone/ModernParameters.h','DSP/V2Gate.cpp','DSP/V2Gate.h']:
        assert sha(ROOT/'Source'/name,True)==sha(WORK/'baseline/Source'/name,True),'Retained source changed '+name
    parent=read(ROOT/'docs/reports/s4/acceptance.json')
    for name,value in parent['identity']['files'].items():assert sha(WORK/'baseline'/name,True)==value
    assert sha(WORK/'baseline/HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3')==parent['dll_sha256']
    assert (ROOT/'docs/S5_METHODS.md').read_text().startswith((WORK/'baseline/S5_METHODS.md').read_text()),'Recorded targets changed'
    kernel=read(REPORT/'kernel.json');assert kernel['passed']==275 and kernel['failed']==kernel['unavailable']==0
    assert kernel['source_id']==cal.identity('ZastavaS2SourceV1',['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h'])
    old_raw=read(ROOT/'out/s4/after_kernel/identity.json')['raw'];assert len(old_raw)==66
    for name,value in old_raw.items():assert sha(WORK/'kernel'/name)==value
    native=(REPORT/'native_final.txt').read_text(encoding='utf-8-sig');match=re.search(r'HamloZastava_Tests: (\d+) expectations passed, 0 failed',native);assert match and int(match[1])>=465
    dll=ROOT/'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3';vendor=read(REPORT/'validators_final/identity.json');stamp=source('ZastavaS1SourceV1')
    assert sha(dll)==vendor['dll_sha256'] and stamp.encode() in dll.read_bytes();assert vendor['results']==[f'pluginval {i} exit=0' for i in range(5,11)]+['Steinberg exit=0']
    assert 'Result: 537 tests passed, 0 tests failed' in (REPORT/'validators_final/steinberg.txt').read_text(encoding='utf-8-sig')
    # JUCE moduleinfo emits trailing commas, accepted by its parser.
    moduleinfo=lambda p:json.loads(re.sub(r',(\s*[}\]])',r'\1',p.read_text(encoding='utf-8-sig')))
    info=moduleinfo(dll.parents[1]/'Resources/moduleinfo.json');oldinfo=moduleinfo(WORK/'baseline/HamloZastava.vst3/Contents/Resources/moduleinfo.json');assert info['Version']=='0.2.0'
    ids=lambda v:sorted(re.findall(r'"CID": "([A-F0-9]+)"',json.dumps(v,indent=2)));assert ids(info)==ids(oldinfo)
    shots=read(REPORT/'screenshots.json');assert shots['source_id']==stamp and shots['exe_sha256']==sha(ROOT/'build/Release/HamloZastava_Snapshot.exe')
    for row in shots['images']:assert sha(REPORT/'images'/(row['name']+'.png'))==row['sha256']
    auditions=read(REPORT/'auditions.json');assert auditions['identity']==hello and len(auditions['rows'])==16
    for row in auditions['rows']:
        for f in row['files']:assert sha(Path(f['path']))==f['sha256']
    package=read(REPORT/'package.json') if (REPORT/'package.json').exists() else None
    if package:
        assert package['dll_sha256']==sha(dll) and package['zip_dll_exact'] and not package['installer'] and not package['installed_plugin_changed']
        assert sha(Path(package['zip']))==package['zip_sha256']
        with zipfile.ZipFile(package['zip']) as archive:
            assert __import__('hashlib').sha256(archive.read('HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3')).hexdigest()==sha(dll)
        for p in sorted(dll.parents[2].rglob('*')):
            if p.is_file():assert sha(Path(package['release'])/'HamloZastava.vst3'/p.relative_to(dll.parents[2]))==sha(p)
    result=dict(stage='S5 library and VST release',version='0.2.0',source_id=source('ZastavaS5SourceV1'),compiled_source_id=stamp,library_identity=hello,programs=207,library_passed=200,active_signatures=200,legacy_primary=229,legacy_restored=458,host_passes=283,kernel_passes=275,kernel_exact_PCM=66,native_passes=int(match[1]),cache_negative_guards=len(guards),cache_resume=True,screenshots=6,audition_pairs=16,dll_sha256=sha(dll),preset_lab_sha256=sha(ROOT/'build/Release/HamloZastava_PresetLab.exe'),worst_library_mean_fraction=worst_time,installer=False,owner_FL_listening='pending',artifact_hashes={p.relative_to(REPORT).as_posix():sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']})
    result['package_verified']=package is not None
    result['additional_methods']={name:sha(ROOT/name,True) for name in ['Tests/S5Tests.cpp','Tools/release_zastava_s5.py','Tools/package_s5.ps1','Tools/validate_s5.ps1','cmake/WritePresetIdentity.cmake','cmake/WriteS5CompatIdentity.cmake']}
    write(REPORT/'acceptance.json',result);print('S5accepted:200recipes,207programs,',match[1],'native;283host/229M0/275kernel;7cache guards')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('action',choices=['assets','audit']);args=parser.parse_args();assets() if args.action=='assets' else audit()
