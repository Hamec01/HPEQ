"""Final F2 audit; F1 shipping files stay frozen while the offline kernel evolves."""
import hashlib
import importlib.util
import json
import re
import subprocess
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1];REPORT=ROOT/'docs/reports/f2';WORK=ROOT/'out/f2'
spec=importlib.util.spec_from_file_location('oracle',ROOT/'Tools/analyze_f2.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
sha=oracle.sha;write=oracle.write
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
base=read(REPORT/'baseline.json');assert base['parent']=='68f98ed'
for name,h in base['files'].items():assert sha(WORK/'baseline'/name)==h,'Baseline changed: '+name
for p in (WORK/'baseline/Source').rglob('*'):
 if p.is_file():assert sha(ROOT/'Source'/p.relative_to(WORK/'baseline/Source'),True)==sha(p,True),'F1 shipping source changed'
f1=read(ROOT/'docs/reports/f1/acceptance.json')
for name,h in f1['method_files'].items():assert sha(ROOT/name,True)==h,'Frozen F1method changed'
for name,h in f1['raw_hashes'].items():assert sha(ROOT/'out/f1/reference'/name)==h
for name,h in f1['state_hashes'].items():assert sha(ROOT/'out/f1/reference'/name)==h
dll=ROOT/'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3';assert sha(dll)==f1['dll_sha256']
assert sha(ROOT/'build/Release/HamloFader_Tests.exe')==f1['executables']['build/Release/HamloFader_Tests.exe']
cmake=(ROOT/'CMakeLists.txt').read_text();production=lambda t:re.findall(r'Source/[^\s)]+\.cpp',t.split('add_library(HamloFader_Shared STATIC',1)[1].split(')',1)[0]);assert production(cmake)==production((WORK/'baseline/CMakeLists.txt').read_text())==f1['production_units']
draft=(WORK/'baseline/HAMLOFADER_V2.md').read_text()
# Two signal-contract clarifications were made before the first measurement.
# Recover that exact version and verify it against the first compiled method.
first_targets=draft.replace('Output trim. Input/output meters','Output trim -> enable/bypass blend. Input/output meters').replace('Neutral and settled bypass/Mix0 exact dry, latency0;','Neutral and settled bypass exact dry; Mix0 exact dry with neutral Output trim; latency0;')
assert (ROOT/'docs/HAMLOFADER_V2.md').read_text().startswith(first_targets),'Measured goals changed'
first_method=hashlib.sha256(('FaderF2MethodV1'+''.join(sha(ROOT/n,True) for n in oracle.METHOD[:-1])+hashlib.sha256(first_targets.encode()).hexdigest()).encode()).hexdigest()
assert first_method==read(REPORT/'before.json')['method_id'],'Premeasurement contract does not match first compiled method'
before=read(REPORT/'before_v4.json');after=read(REPORT/'after_v4.json');bm=read(WORK/'before_v4/manifest.json');am=read(WORK/'after_v4/manifest.json')
assert before['method_id']==after['method_id']==oracle.identity('FaderF2MethodV1',[ROOT/n for n in oracle.METHOD])
source=oracle.identity('FaderF2KernelV1',[ROOT/n for n in ['Source/DSP/FaderKernel.h','Source/DSP/FaderMeter.h','Source/DSP/FaderKernel.cpp','Source/DSP/FaderMeter.cpp']]);assert after['source_id']==am['source_id']==source
assert before['passed']==30 and before['failed']==0 and before['unavailable']==13
assert after['passed']==281 and after['failed']==after['unavailable']==0 and all(c['passed'] is True for c in after['checks'])
for name,r in [('before_v4',before),('after_v4',after)]:
 folder=WORK/name;pins=read(folder/'identity.json');assert sha(folder/'identity.json')==r['raw_identity_sha256'];assert sha(folder/'manifest.json')==pins['manifest']
 for n,h in pins['raw'].items():assert sha(folder/n)==h,'Cached output changed'
for row in bm['cases']:
 if row['available']:
  for suffix in ['.f32','_input.f32']:assert sha(WORK/'before'/ (row['name']+suffix))==sha(WORK/'before_v4'/(row['name']+suffix))==sha(WORK/'after_v4'/(row['name']+suffix))
unit=(REPORT/'unit_v4.txt').read_text(encoding='utf-8-sig');assert 'F2Unit: 59 passed, 0 failed' in unit and source in unit
guards=read(REPORT/'guards_v4.json');assert len(guards)==4 and all(g['rejected'] for g in guards)
for g in guards:assert sha(WORK/'guards_v4'/g['name']/'rejection.txt')==g['log_sha256']
regressions=[]
for name in ['motion_custom','motion_sine','motion_triangle']:
 old=np.fromfile(WORK/'after_initial'/(name+'.f32'),dtype='<f4').reshape(-1,2);new=np.fromfile(WORK/'after_v4'/(name+'.f32'),dtype='<f4').reshape(-1,2);assert not np.array_equal(old[48000:72000],old[72000:96000]) and np.array_equal(new[48000:72000],new[72000:96000]);regressions.append(name+'_repeat')
old_source=oracle.identity('FaderF2KernelV1',[WORK/'candidate_v2'/n for n in ['FaderKernel.h','FaderMeter.h','FaderKernel.cpp','FaderMeter.cpp']]);assert old_source==read(REPORT/'after_v2.json')['source_id']
probe=subprocess.run([str(WORK/'candidate_v2/unit_transition_probe.exe')],capture_output=True,text=True);assert probe.returncode==1 and '44 passed, 1 failed' in probe.stdout and "mode fade preserves each channel's applied gain" in probe.stdout and old_source in probe.stdout;regressions.append('stereo_mode_transition')
probe=subprocess.run([str(WORK/'accepted/unit_cancel_probe.exe')],capture_output=True,text=True);assert probe.returncode==1 and '58 passed, 1 failed' in probe.stdout and 'cancelled learn cannot propose stale gain' in probe.stdout;regressions.append('learn_cancel')
executables={}
for name,stamp in [('build/Release/HamloFader_F2Current.exe',source),('build/Release/HamloFader_F2Reference.exe',bm['source_id']),('build/Release/HamloFader_F2Tests.exe',source)]:
 path=ROOT/name;assert stamp.encode() in path.read_bytes(),'Stale executable';executables[name]=sha(path)
for name,h in read(REPORT/'frozen_tools_v4.json').items():assert sha(WORK/'accepted_v4'/name)==h,'Frozen tool/source changed'
assert sha(WORK/'replay_v4.json')==sha(REPORT/'after_v4.json'),'Valid cached analysis changed'
result=dict(stage='F2 offline four-mode kernel and metering',source_id=source,method_id=after['method_id'],baseline=dict(passed=30,failed=0,unavailable=13),current=dict(passed=281,failed=0,unavailable=0,cases=len(am['cases'])),unit_passes=59,negative_guards=4,regression_detections=regressions,legacy_shipping_files_unchanged=True,legacy_PCM_retained=678,shipping_dll_sha256=sha(dll),shipping_version='0.1.0 F1; new kernel not hosted',worst_mean_fraction=max(after['performance'].values()),hardware='Intel Core i5-10300H, 4 cores/8 threads',executables=executables,additional_methods={n:sha(ROOT/n,True) for n in ['Tests/F2UnitMain.cpp','Tools/verify_f2.py','Tools/check_f2_guards.py','cmake/WriteF2Identity.cmake']},artifacts={p.relative_to(REPORT).as_posix():sha(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']},new_host_UI_presets='not implemented; F3/F4/F5 next',owner_FL_listening='pending',installer=False)
write(REPORT/'acceptance.json',result);print('F2accepted:281lab/59unit;30control+13n-a;4cache guards,5regressions;F1shipping preserved')
