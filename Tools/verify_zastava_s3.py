"""Freeze/audit the same-method M0/host integration pair; never relabel old runs."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'out/s3'
REPORT=ROOT/'docs/reports/s3'


def sha(path,normal=False):
    b=path.read_bytes();return hashlib.sha256(b.replace(b'\r\n',b'\n') if normal else b).hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
def identity(root):
    files={p.relative_to(root).as_posix():sha(p,True) for p in sorted((root/'Source').rglob('*')) if p.is_file()}
    methods={name:sha(ROOT/name,True) for name in ['Tests/S3LegacyLabMain.cpp','Tests/S3HostLabMain.cpp','docs/S3_METHODS.md']}
    return dict(source_id=hashlib.sha256(('ZastavaS3SourceV1'+''.join(files.values())).encode()).hexdigest(),method_id=hashlib.sha256(('ZastavaS3MethodV1'+''.join(methods.values())).encode()).hexdigest(),files=files,methods=methods)


def freeze():
    assert not (REPORT/'before_v3.json').exists(),'Refusing to overwrite frozen control'
    source=identity(ROOT/'out/s1/original');legacy=read(WORK/'before_legacy_v3.json');host=read(WORK/'before_host_v3.json')
    assert legacy['source_id']==host['source_id']==source['source_id'] and legacy['method_id']==host['method_id']==source['method_id']
    assert legacy['failures']==0 and len(legacy['cases'])==229 and not host['modern'] and host['failed']==0
    old=read(ROOT/'out/s1/reference_v3/capture.json')
    for a,b in zip(legacy['cases'],old['cases']):
        assert all(a[k]==b[k] for k in ['case','render_sha256','state_sha256','latency_events','restorations']),'Changed M0 fixture'
    reference={p.relative_to(WORK/'reference_v3').as_posix():sha(p) for p in sorted((WORK/'reference_v3').glob('*')) if p.is_file()}
    before=dict(identity=source,legacy_cases=229,old_primary_unchanged=229,host=host,reference_files=reference,executables={n:sha(ROOT/'build/Release'/n) for n in ['HamloZastava_S3HostReference.exe','HamloZastava_S3LegacyReference.exe']})
    for name in before['executables']:shutil.copy2(ROOT/'build/Release'/name,WORK/'baseline'/('v3_'+name))
    for name in source['methods']:
        target=WORK/'baseline/v3_method'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
    write(REPORT/'before_v3.json',before);print('Frozen v3:229 unchanged M0 primary fixtures,458 restored observations; host features n/a')


def audit():
    before=read(REPORT/'before_v3.json');current=identity(ROOT);legacy=read(REPORT/'after_legacy_v4.json');host=read(REPORT/'after_host_v4.json')
    assert current['method_id']==before['identity']['method_id']==legacy['method_id']==host['method_id'],'Changed method; recapture both sides'
    assert current['source_id']==legacy['source_id']==host['source_id'],'Stale current laboratory'
    assert not legacy['failures'] and len(legacy['cases'])==229 and host['modern'] and host['failed']==0 and host['unavailable']==0
    assert sum(c['passed'] is True for c in host['checks'])==host['passed'] and all(c['passed'] is True for c in host['checks'])
    for row in legacy['cases']:assert all(row[k] for k in ['bit_exact','state_exact','latency_exact','finite']),row['case']
    for name,value in before['reference_files'].items():assert sha(WORK/'reference_v3'/name)==value,'Changed M0 raw fixture'
    for name,value in before['identity']['files'].items():assert sha(ROOT/'out/s1/original'/name,True)==value,'Original source changed'
    s2=read(ROOT/'docs/reports/s2/acceptance.json')
    for name in ['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h']:assert sha(ROOT/name,True)==s2['source_files'][name],'S2 kernel changed'
    native=(REPORT/'native_v3.txt').read_text(encoding='utf-8-sig');match=re.search(r'HamloZastava_Tests: (\d+) expectations passed, 0 failed',native);assert match
    validators=read(REPORT/'validators_v3/identity.json');dll=ROOT/'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3'
    assert sha(dll)==validators['dll_sha256'] and validators['results']==[f'pluginval {n} exit=0' for n in range(5,11)]+['Steinberg exit=0']
    assert re.search(r'Result: \d+ tests passed, 0 tests failed',(REPORT/'validators_v3/steinberg.txt').read_text(encoding='utf-8-sig'))
    binary_id=hashlib.sha256(('ZastavaS1SourceV1'+''.join(current['files'].values())).encode()).hexdigest()
    assert binary_id.encode() in dll.read_bytes(),'DLL does not contain current compiled source identity'
    contents=dll.parents[1];original=WORK/'baseline/HamloZastava.vst3/Contents/Resources/moduleinfo.json'
    ids=lambda p:sorted(re.findall(r'"CID": "([A-F0-9]+)"',p.read_text()))
    assert ids(contents/'Resources/moduleinfo.json')==ids(original),'VST class identity changed'
    regression=(REPORT/'alias_regression_native.txt').read_text();assert '4 tests failed' in regression
    result=dict(stage='S3 host adapter',identity=current,legacy_primary=229,legacy_restored=458,host_passes=host['passed'],host_failures=host['failed'],native_passes=int(match[1]),dll_sha256=sha(dll),compiled_DLL_source_id=binary_id,S2_kernel_unchanged=True,alias_regressions_detected=4,additional_methods={p:sha(ROOT/p,True) for p in ['Tests/S3IntegrationTests.cpp','Tools/verify_zastava_s3.py','Tools/validate_s3.ps1','cmake/WriteS3Identity.cmake']},editor='Legacy editor retained; modern UI not implemented',presets='seven original programs; 200 recipes pending',installer=False,artifacts={p.relative_to(REPORT).as_posix():sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']})
    write(REPORT/'acceptance.json',result);print('S3 accepted:229 M0 matches,',host['passed'],'host checks, native',match[1],'; validators5..10 passed')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['freeze','audit']);args=p.parse_args();freeze() if args.action=='freeze' else audit()
