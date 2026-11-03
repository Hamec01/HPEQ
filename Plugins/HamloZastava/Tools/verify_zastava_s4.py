"""Audit S4 extension against frozen numerical methods, raw PCM and UI evidence."""
import argparse
import hashlib
import importlib.util
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1];WORK=ROOT/'out/s4';REPORT=ROOT/'docs/reports/s4'


def sha(p,normal=False):
    b=p.read_bytes();return hashlib.sha256(b.replace(b'\r\n',b'\n') if normal else b).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def text(p):return p.read_text(encoding='utf-8-sig')
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def identities():
    spec=importlib.util.spec_from_file_location('s3identity',ROOT/'Tools/verify_zastava_s3.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module.identity(ROOT)


def freeze():
    assert not (REPORT/'before.json').exists(),'Refusing to overwrite baseline'
    parent=read(ROOT/'docs/reports/s3/acceptance.json');base=WORK/'baseline'
    for name,value in parent['identity']['files'].items():assert sha(base/name,True)==value,'S3 source snapshot changed'
    assert sha(base/'HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3')==parent['dll_sha256']
    manifest=read(WORK/'before_kernel/manifest.json');assert manifest['source_id']==read(ROOT/'docs/reports/s2/acceptance.json')['source_id'] and manifest['implemented']
    raw={p.name:sha(p) for p in sorted((WORK/'before_kernel').glob('*.f32'))};assert len(raw)==66
    original=read(ROOT/'out/s2/after_v3/identity.json')['raw'];assert raw==original,'Before render differs from accepted kernel'
    write(REPORT/'before.json',dict(parent_commit='1341976',parent_source_id=parent['identity']['source_id'],parent_dll_sha256=parent['dll_sha256'],kernel_source_id=manifest['source_id'],kernel_method_id=manifest['method_id'],kernel_manifest_sha256=sha(WORK/'before_kernel/manifest.json'),kernel_raw=raw,new_editor='n/a in S3',key_audition='n/a in S3',targets_sha256=sha(base/'S4_METHODS.md',True)))
    print('Frozen S4:66PCM outputs exactly equal accepted S2/S3; new features n/a')


def audit():
    before=read(REPORT/'before.json');identity=identities();kernel=read(REPORT/'kernel.json');host=read(REPORT/'host_complete.json');legacy=read(REPORT/'legacy_complete.json')
    assert host['source_id']==legacy['source_id']==identity['source_id'] and host['method_id']==legacy['method_id']==identity['method_id'],'Stale host method/source'
    assert host['failed']==host['unavailable']==0 and host['passed']==283 and all(c['passed'] is True for c in host['checks'])
    assert legacy['failures']==0 and len(legacy['cases'])==229
    for c in legacy['cases']:assert all(c[k] for k in ['bit_exact','state_exact','latency_exact','finite'])
    assert kernel['failed']==kernel['unavailable']==0 and kernel['passed']==275 and kernel['cases']==66
    assert kernel['identity']['method_id']==before['kernel_method_id']
    files={n:sha(ROOT/n,True) for n in ['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h']};source=hashlib.sha256(('ZastavaS2SourceV1'+''.join(files.values())).encode()).hexdigest();assert kernel['source_id']==source,'Stale kernel'
    methods={n:sha(ROOT/n,True) for n in ['Tests/V2LabMain.cpp','Tools/analyze_zastava_s2.py','docs/S2_METHODS.md']};method=hashlib.sha256(('ZastavaS2MethodV1'+''.join(methods.values())).encode()).hexdigest();assert method==before['kernel_method_id']
    for name,value in before['kernel_raw'].items():assert sha(WORK/'before_kernel'/name)==sha(WORK/'after_kernel'/name)==value,'Listen-off PCM changed: '+name
    assert sha(WORK/'before_kernel/manifest.json')==before['kernel_manifest_sha256']
    base=WORK/'baseline/Source'
    for name in ['UI/PresetScreen.h','UI/PresetScreen.cpp','UI/BlackSteel.h','UI/BlackSteel.cpp','UI/HamloZastavaPanel.h','UI/HamloZastavaPanel.cpp','Standalone/ModuleVariant.cpp','Standalone/ModuleVariant.h','Core/ModulePresets.cpp']:
        assert sha(base/name,True)==sha(ROOT/'Source'/name,True),'Retained shared/Legacy contract changed: '+name
    old=text(base/'Standalone/ModernParameters.cpp');new=text(ROOT/'Source/Standalone/ModernParameters.cpp')
    data=lambda s:s.split('std::vector<ParameterSpec> make()',1)[1].split('jassert(p.size()==Count)',1)[0]
    assert data(old)==data(new).replace('    p.push_back(choice("v2_listen","Listen Key",0,{"Off","On"}));\n',''),'Original61modern parameter definitions changed'
    append=lambda s:s.split('void append(',1)[1].split('int engine(',1)[0]
    assert append(old)==append(new),'Parameter construction/order changed'
    assert text(ROOT/'docs/S4_METHODS.md').startswith(text(WORK/'baseline/S4_METHODS.md')),'Recorded targets changed'
    native=text(REPORT/'native_roles.txt');match=re.search(r'HamloZastava_Tests: (\d+) expectations passed, 0 failed',native);assert match and int(match[1])>=465
    vendor=read(REPORT/'validators_final/identity.json');dll=ROOT/'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3'
    assert sha(dll)==vendor['dll_sha256'] and vendor['results']==[f'pluginval {n} exit=0' for n in range(5,11)]+['Steinberg exit=0']
    assert 'Result: 537 tests passed, 0 tests failed' in text(REPORT/'validators_final/steinberg.txt')
    production=hashlib.sha256(('ZastavaS1SourceV1'+''.join(identity['files'].values())).encode()).hexdigest();assert production.encode() in dll.read_bytes(),'Stale DLL'
    ids=lambda p:sorted(re.findall(r'"CID": "([A-F0-9]+)"',text(p)))
    assert ids(dll.parents[1]/'Resources/moduleinfo.json')==ids(WORK/'baseline/HamloZastava.vst3/Contents/Resources/moduleinfo.json'),'VST class IDs changed'
    shots=read(REPORT/'screenshots_final.json');assert shots['source_id']==identity['source_id'] and shots['compiled_source_id']==production and len(shots['images'])==15
    assert shots['executable_sha256']==sha(ROOT/'build/Release/HamloZastava_Snapshot.exe') and shots['driver_sha256']==sha(ROOT/'Tests/SnapshotMain.cpp',True)
    for row in shots['images']:assert sha(REPORT/'images_final'/(row['name']+'.png'))==row['sha256']
    result=dict(stage='S4 modern editor and audition',identity=identity,parameter_count=78,old_parameter_count_preserved=77,legacy_primary=229,legacy_restored=458,host_passes=283,kernel_passes=275,listen_off_PCM_exact=66,native_passes=int(match[1]),screenshots=15,dll_sha256=sha(dll),compiled_source_id=production,additional_methods={n:sha(ROOT/n,True) for n in ['docs/S4_METHODS.md','Tests/S4Tests.cpp','Tests/SnapshotMain.cpp','Tools/capture_zastava_s4.py','Tools/verify_zastava_s4.py','Tools/validate_s4.ps1']},presets='seven original;200new pending',installer=False,owner_FL_listening='pending',artifacts={p.relative_to(REPORT).as_posix():sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']})
    write(REPORT/'acceptance.json',result);print('S4 accepted:275kernel/283host/229M0/',match[1],'native;66exact PCM;15snapshots;validators passed')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['freeze','audit']);args=p.parse_args();freeze() if args.action=='freeze' else audit()
