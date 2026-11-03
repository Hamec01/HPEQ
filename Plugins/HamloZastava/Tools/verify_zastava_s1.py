"""Freeze original M0 artifacts, then audit the unmodified-method isolation pair."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
REPORT=ROOT/'docs/reports/s1'
WORK=ROOT/'out/s1'
BUNDLE=ROOT/'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3'


def sha(path,normal=False):
    value=path.read_bytes();return hashlib.sha256(value.replace(b'\r\n',b'\n') if normal else value).hexdigest()


def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')


def identities(base=ROOT):
    names=sorted(str(p.relative_to(base)).replace('\\','/') for p in (base/'Source').rglob('*') if p.is_file())
    files={name:sha(base/name,True) for name in names}
    methods={name:sha(ROOT/name,True) for name in ['Tests/ZastavaLabMain.cpp','Tools/verify_zastava_s1.py','docs/S1_METHODS.md']}
    return dict(source_id=hashlib.sha256(('ZastavaS1SourceV1'+''.join(files.values())).encode()).hexdigest(),
                method_id=hashlib.sha256(('ZastavaS1MethodV3'+''.join(methods.values())).encode()).hexdigest(),files=files,methods=methods)


def freeze():
    target=WORK/'original';assert not target.exists(),'Refusing to overwrite original checkpoint'
    original=read(WORK/'reference/capture.json');identity=identities()
    assert original['source_id']==identity['source_id'] and original['method_id']==identity['method_id'],'Stale capture binary'
    assert len(original['cases'])==229 and original['failures']==0
    target.mkdir(parents=True);shutil.copytree(ROOT/'Source',target/'Source');shutil.copy2(ROOT/'CMakeLists.txt',target/'CMakeLists.txt')
    shutil.copytree(BUNDLE,target/'HamloZastava.vst3')
    for name in ['HamloZastava_Tests.exe','HamloZastava_CompatLab.exe']:
        shutil.copy2(ROOT/'build/Release'/name,target/name)
    files={str(p.relative_to(WORK/'reference')).replace('\\','/'):sha(p) for p in sorted((WORK/'reference').rglob('*')) if p.is_file()}
    dll=BUNDLE/'Contents/x86_64-win/HamloZastava.vst3'
    module=(BUNDLE/'Contents/Resources/moduleinfo.json').read_text()
    write(REPORT/'before.json',dict(identity=identity,cases=len(original['cases']),processor_bytes=original['processor_bytes'],
                                    dll_sha256=sha(dll),class_ids=re.findall(r'"CID": "([A-F0-9]+)"',module),reference_files=files,
                                    laboratory_sha256=sha(ROOT/'build/Release/HamloZastava_CompatLab.exe')))
    print('Frozen M0:',len(original['cases']),'cases; source',identity['source_id'])



def freeze_v3():
    old=read(REPORT/'before.json');reference=WORK/'reference_v3';capture=read(reference/'capture.json');identity=identities(WORK/'original')
    assert identity['source_id']==old['identity']['source_id']==capture['source_id']
    assert identity['method_id']==capture['method_id'] and len(capture['cases'])==229 and capture['failures']==0
    original=read(WORK/'reference/capture.json')
    for a,b in zip(capture['cases'],original['cases']):
        assert all(a[k]==b[k] for k in ['case','render_sha256','state_sha256','latency_events']),'Changed primary fixture'
    write(REPORT/'before_v3.json',dict(identity=identity,cases=229,processor_bytes=capture['processor_bytes'],dll_sha256=old['dll_sha256'],class_ids=old['class_ids'],
         reference_files={str(p.relative_to(reference)).replace('\\','/'):sha(p) for p in sorted(reference.rglob('*')) if p.is_file()},
         laboratory_sha256=sha(ROOT/'build/Release/HamloZastava_M0CompatLab.exe')))
    print('Frozen v3 actual M0 restores; all229primary fixtures equal original v1')


def audit():
    identity=identities();before=read(REPORT/'before_v3.json');after=read(REPORT/'after_v3.json')
    assert after['source_id']==identity['source_id'] and after['method_id']==before['identity']['method_id']==identity['method_id']
    assert len(after['cases'])==before['cases']==229 and after['failures']==0
    for name,value in before['reference_files'].items():assert sha(WORK/'reference_v3'/name)==value,'Original fixture changed'
    for row in after['cases']:
        assert row['bit_exact'] and row['state_exact'] and row['latency_exact'] and row['finite'],row['case']
    source=(ROOT/'CMakeLists.txt').read_text().split('add_library(HamloZastava_Shared STATIC',1)[1].split(')',1)[0]
    forbidden=['LayerEngine','FxNodeProcessor','HamloComp','HamloEcho','HamloVerb','HamloMag','HamloRedWall','HamloUdarnik','HamloShirota','HamloRevolution','ProcessorsM6','PreEqProcessor','CrossoverTree']
    assert not any(name in source for name in forbidden),'Foreign production translation unit'
    units=re.findall(r'Source/[\w/]+\.cpp',source)
    visited=set();pending=[ROOT/name for name in units]
    while pending:
        path=pending.pop()
        if path in visited:continue
        visited.add(path)
        for include in re.findall(r'#include\s+"([^"]+)"',path.read_text()):
            assert not any(name in include for name in forbidden+['LayerSnapshot','FxProjectState']),'Foreign include: '+include
            candidate=ROOT/'Source'/include
            if not candidate.exists():candidate=path.parent/include
            if candidate.exists():pending.append(candidate)
    native=(REPORT/'native.txt').read_text(encoding='utf-8-sig');match=re.search(r'HamloZastava_Tests: (\d+) expectations passed, 0 failed',native);assert match and int(match[1])>=71
    validators=read(REPORT/'validators_final/identity.json');dll=BUNDLE/'Contents/x86_64-win/HamloZastava.vst3'
    assert sha(dll)==validators['dll_sha256']
    assert validators['results']==[f'pluginval {i} exit=0' for i in range(5,11)]+['Steinberg exit=0']
    assert 'Result: 537 tests passed, 0 tests failed' in (REPORT/'validators_final/steinberg.txt').read_text()
    assert sorted(re.findall(r'"CID": "([A-F0-9]+)"',(BUNDLE/'Contents/Resources/moduleinfo.json').read_text()))==sorted(before['class_ids'])
    assert (REPORT/'panel.png').stat().st_size>10000
    archive=WORK/'original'
    assert sha(archive/'HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3')==before['dll_sha256']
    for name,value in before['identity']['files'].items():assert sha(archive/name,True)==value,'Original source changed'
    groups={}
    for row in read(WORK/'reference_v3/capture.json')['cases']:
        if row['case'].startswith('events_'):
            group=row['case'].split('_b')[0];groups.setdefault(group,set()).add(row['render_sha256'])
    result=dict(stage='S1 isolation',identity=identity,cases=229,native_expectations=int(match[1]),
                historical_nonidempotent_states=sum(not row['historical_roundtrip_idempotent'] for row in after['cases']),old_processor_bytes=before['processor_bytes'],new_processor_bytes=after['processor_bytes'],
                production_units=units,include_closure=sorted(str(p.relative_to(ROOT)).replace('\\','/') for p in visited),
                original_dll_sha256=before['dll_sha256'],validated_dll_sha256=sha(dll),class_ids=before['class_ids'],
                legacy_automation_distinct_renders={k:len(v) for k,v in groups.items()},
                artifacts={str(p.relative_to(REPORT)).replace('\\','/'):sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']},
                new_V2_features='not implemented in isolation stage',owner_scope='all four roles; separate soft/hard profiles',installer_built=False)
    write(REPORT/'acceptance.json',result)
    print('S1 audit passed:229bit-exact cases, native',match[1],'no foreign production/include dependencies')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['freeze','freeze-v3','audit']);a=p.parse_args()
    freeze() if a.action=='freeze' else freeze_v3() if a.action=='freeze-v3' else audit()
