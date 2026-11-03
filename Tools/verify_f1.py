"""Audit source-bound paired Legacy isolation, raw files, build closure and vendors."""
import hashlib
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1];REPORT=ROOT/'docs/reports/f1';BASE=ROOT/'out/audit/original'
def sha(p,normal=False):
    b=p.read_bytes();return hashlib.sha256(b.replace(b'\r\n',b'\n') if normal else b).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def identity(root,prefix):return hashlib.sha256((prefix+''.join(sha(p,True) for p in sorted((root/'Source').rglob('*')) if p.is_file())).encode()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')

before=read(REPORT/'before.json');after=read(REPORT/'after_initial.json');original=read(ROOT/'docs/reports/audit/original.json')
for n,h in original['files'].items():assert sha(BASE/n)==h,'Original archive changed: '+n
assert before['source_id']==identity(BASE,'FaderF1SourceV1') and after['source_id']==identity(ROOT,'FaderF1SourceV1')
methods=['Tests/F1LabMain.cpp','Tests/AllocationAudit.cpp','Tests/AllocationAudit.h','docs/F1_METHODS.md']
method=hashlib.sha256(('FaderF1MethodV1'+''.join(sha(ROOT/n,True) for n in methods)).encode()).hexdigest()
assert before['method_id']==after['method_id']==method and before['contract']==after['contract']
assert len(after['contract']['programs'])==len(after['contract']['parameters'])==8
assert before['failures']==after['failures']==0 and len(before['cases'])==len(after['cases'])==226
reference=ROOT/'out/f1/reference';raw={};states={};historical_changes=0
for b,a in zip(before['cases'],after['cases']):
    assert b==a and a['passed'];name=a['case'];assert sha(reference/(name+'.state'))==a['initial_state_sha256'];states[name+'.state']=a['initial_state_sha256']
    for o in a['observations']:
        prefix=name+'_'+str(o['restore']);assert sha(reference/(prefix+'.f32'))==o['render_sha256'] and sha(reference/(prefix+'.state'))==o['state_sha256']
        raw[prefix+'.f32']=o['render_sha256'];states[prefix+'.state']=o['state_sha256'];assert o['finite'] and o['allocations']==0
        assert all(m['latency']==0 for m in o['meters'])
    historical_changes+=sum(o['parameters']!=a['observations'][0]['parameters'] for o in a['observations'][1:])
assert len(raw)==678
for n in ['UI/BlackSteel.cpp','UI/BlackSteel.h','UI/PresetScreen.cpp','UI/PresetScreen.h','UI/HamloFaderPanel.cpp','UI/HamloFaderPanel.h','DSP/DspConfig.h','DSP/Ramp.h','DSP/DelayLine.h']:
    assert sha(ROOT/'Source'/n,True)==sha(BASE/'Source'/n,True),'Retained file changed: '+n
old=(BASE/'Source/DSP/Processors.cpp').read_text();new=(ROOT/'Source/DSP/FaderGain.cpp').read_text();body=old[old.index('void GainProcessor::prepare'):old.index('void CompressorProcessor::prepare')];assert body in new
cmake=(ROOT/'CMakeLists.txt').read_text();block=cmake.split('add_library(HamloFader_Shared STATIC',1)[1].split(')',1)[0];units=re.findall(r'Source/[^\s)]+\.cpp',block);assert len(units)==9
closure=set()
def visit(p):
    if p in closure:return
    closure.add(p)
    for inc in re.findall(r'^#include "([^"]+)"',p.read_text(encoding='utf-8'),re.M):
        child=ROOT/'Source'/inc
        if child.exists():visit(child)
for name in units:visit(ROOT/name)
forbidden=['LayerEngine','FxNodeProcessor','ProcessorsM6','FxProjectState','HamloComp','HamloVerb','HamloEcho','HamloMag','HamloShirota','HamloZastava','HamloUdarnik','HamloRedWall','Crossover','PreEq']
assert not any(any(term in p.as_posix() for term in forbidden) for p in closure)
members=(REPORT/'library_members.txt').read_text(encoding='utf-8-sig').splitlines();assert len(members)==9 and all(Path(n).stem+'.obj' in '\n'.join(members) for n in units)
native=(REPORT/'native.txt').read_text(encoding='utf-8-sig');assert '49 expectations passed, 0 failed' in native
# The initial isolated image differed at688needle pixels: unchanged BlackSteel
# adds a wall-clock tremor. Keep it. Final original/isolated repeat captures
# agree exactly; compare those, not two different animation instants.
assert sha(REPORT/'panel_isolated_repeat.png')==sha(REPORT/'panel_original_repeat.png'),'Legacy UI pixels changed'
dll=ROOT/'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3';vendor=read(REPORT/'validators_initial/identity.json');assert sha(dll)==vendor['dll_sha256']
assert vendor['results']==[f'pluginval {i} exit=0' for i in range(5,11)]+['Steinberg exit=0']
assert 'Result: 537 tests passed, 0 tests failed' in (REPORT/'validators_initial/steinberg.txt').read_text(encoding='utf-8-sig')
cid=lambda p:sorted(re.findall(r'"CID": "([A-F0-9]+)"',p.read_text()))
assert cid(dll.parents[1]/'Resources/moduleinfo.json')==cid(BASE/'HamloFader.vst3/Contents/Resources/moduleinfo.json')
guards=read(REPORT/'guards.json');assert [r['guard'] for r in guards]==['method','contract','raw','state'] and all(r['rejected'] for r in guards)
for r in guards:assert sha(ROOT/'out/f1/guards'/r['guard']/'log.txt')==r['log_sha256']
result=dict(stage='F1 Legacy isolation',source_id=after['source_id'],method_id=method,original_source_id=before['source_id'],parameters=8,programs=8,primary_cases=226,actual_restorations=452,exact_raw_outputs=678,zero_callback_allocations=True,native_passed=49,production_units=units,include_closure=sorted(p.relative_to(ROOT).as_posix() for p in closure),library_members=members,processor_object_bytes_before=before['processor_bytes'],processor_object_bytes_after=after['processor_bytes'],historical_parameter_roundtrip_changes=historical_changes,panel_exact=True,dll_sha256=sha(dll),dll_bytes=dll.stat().st_size,original_dll_bytes=(BASE/'HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3').stat().st_size,new_features='n/a; Trim/Match/Ride/Motion next',owner_FL_listening='pending',installer=False,method_files={n:sha(ROOT/n,True) for n in methods},raw_hashes=raw,state_hashes=states,executables={n:sha(ROOT/n) for n in ['out/f1/reference_lab.exe','build/Release/HamloFader_F1Lab.exe','build/Release/HamloFader_Tests.exe','build/Release/HamloFader_Snapshot.exe']},artifact_hashes={p.relative_to(REPORT).as_posix():sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']})
result['negative_guards']=4
result['additional_methods']={n:sha(ROOT/n,True) for n in ['Tools/verify_f1.py','Tools/check_f1_guards.py','Tools/isolate_f1.py','Tools/validate_f1.ps1','cmake/WriteF1Identity.cmake']}
write(REPORT/'acceptance.json',result);print('F1accepted:226primary+452actual restores;678exact PCM;49native;9production units;vendors pass')
