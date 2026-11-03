"""Audit the paired S2 kernel stage without rebuilding the accepted S1 VST."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
REPORT=ROOT/'docs/reports/s2'
WORK=ROOT/'out/s2'


def sha(path,normal=False):
    value=path.read_bytes()
    return hashlib.sha256(value.replace(b'\r\n',b'\n') if normal else value).hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
def source_id(root):
    return hashlib.sha256(('ZastavaS2SourceV1'+''.join(sha(root/name,True) for name in ['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h'])).encode()).hexdigest()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('label');args=parser.parse_args()
    assert re.fullmatch(r'[a-zA-Z0-9_-]+',args.label)
    audit=WORK/args.label;assert not audit.exists(),'Refusing to overwrite audit';audit.mkdir()
    before=read(REPORT/'before_v3.json');after=read(REPORT/'after_v3.json')
    assert before['failed']==after['failed']==0 and after['unavailable']==0
    assert before['cases']==after['cases']==66 and before['passed']==218 and after['passed']==275
    assert before['identity']['method_id']==after['identity']['method_id']
    assert after['source_id']==source_id(ROOT) and before['source_id']==source_id(WORK/'before_sources')
    baseline=read(WORK/'before_sources.json')
    for name,value in baseline['files'].items():assert sha(WORK/'before_sources'/name,True)==value,'Baseline source changed'
    # Re-use the accepted same driver/analyzer, with cache pins intact.
    for name,report,extra in [('before_v3','before_v3.json',['--baseline']),('after_v3','after_v3.json',[])]:
        run=subprocess.run([sys.executable,'-I','-X','utf8',str(ROOT/'Tools/analyze_zastava_s2.py'),str(WORK/name),str(audit/report),*extra],capture_output=True,text=True)
        assert run.returncode==0,run.stdout+run.stderr
        assert sha(audit/report)==sha(REPORT/report),'Acceptance replay changed'
    original=read(WORK/'before/identity.json');current=read(WORK/'before_v3/identity.json')
    assert len(original['raw'])==49
    for name,value in original['raw'].items():assert current['raw'][name]==value,'Old control PCM changed'
    for old,new in [('before_final','before_v3'),('candidate_02','after_v3')]:
        assert read(WORK/old/'identity.json')['raw']==read(WORK/new/'identity.json')['raw'],'Comment-only revision changed PCM'
    regression=read(WORK/'regression_v3/manifest.json')
    assert regression['method_id']==after['identity']['method_id'] and regression['source_id']==source_id(WORK/'candidate_01_sources')
    regression_checks=[]
    for mode in range(2):
        for profile in range(2):
            def pcm(look):return np.fromfile(WORK/f'regression_v3/window_{mode}_{profile}_{look}.f32',dtype='<f4').reshape(-1,2)[10560:24000]
            a,b=pcm(0),pcm(1);assert not np.array_equal(a,b),'Regression no longer reproduces'
            regression_checks.append(dict(mode=mode,profile=profile,maximum_difference=float(np.max(np.abs(a-b)))))
    write(REPORT/'regression_v3.json',dict(source_id=regression['source_id'],method_id=regression['method_id'],checks=regression_checks,artifacts={p.name:sha(p) for p in (WORK/'regression_v3').glob('*') if p.is_file()}))
    # Rejected stale/corrupt experiments never replace accepted measurements.
    guards=[];accepted=WORK/'after_v3';manifest=read(accepted/'manifest.json')
    for name,field,value,extra,expected in [
        ('role','implemented',False,[],'Wrong experiment role'),
        ('source','source_id','0'*64,[],'Stale compiled source'),
        ('method','method_id','0'*64,[],'Stale compiled method'),
        ('baseline_source','source_id','0'*64,['--baseline'],'Wrong baseline source'),
        ('corrupt_raw',None,None,[],'Corrupt cached render')]:
        folder=audit/name;folder.mkdir();mutated=dict(manifest)
        if name=='baseline_source':mutated['implemented']=False
        if field:mutated[field]=value
        write(folder/'manifest.json',mutated)
        if name=='corrupt_raw':
            shutil.copy2(accepted/'identity.json',folder/'identity.json')
            for path in accepted.glob('*.f32'):
                target=folder/path.name
                if path.name=='static_0.f32':
                    data=bytearray(path.read_bytes());data[100]^=1;target.write_bytes(data)
                else:os.link(path,target)
        run=subprocess.run([sys.executable,'-I','-X','utf8',str(ROOT/'Tools/analyze_zastava_s2.py'),str(folder),str(folder/'report.json'),*extra],capture_output=True,text=True)
        assert run.returncode!=0 and expected in run.stderr,name+': '+run.stdout+run.stderr
        (folder/'rejection.txt').write_text(run.stdout+run.stderr,encoding='utf-8')
        guards.append(dict(name=name,rejected=True,reason=expected))
    s1=read(ROOT/'docs/reports/s1/acceptance.json');retained={}
    for name,value in s1['identity']['files'].items():
        if name=='CMakeLists.txt':continue  # Only offline targets were appended in S2.
        assert sha(ROOT/name,True)==value,'Accepted S1 source changed: '+name;retained[name]=value
    for name,value in s1['identity']['methods'].items():assert sha(ROOT/name,True)==value,'S1 method changed'
    cmake=(ROOT/'CMakeLists.txt').read_text().split('add_library(HamloZastava_Shared STATIC',1)[1].split(')',1)[0]
    assert re.findall(r'Source/[\w/]+\.cpp',cmake)==s1['production_units'],'Shipping source closure changed'
    dll=ROOT/'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3/Contents/x86_64-win/HamloZastava.vst3'
    assert sha(dll)==s1['validated_dll_sha256'],'Accepted VST changed'
    audition=read(REPORT/'auditions.json');assert audition['source_id']==after['source_id'] and len(audition['rows'])==8
    assert audition['driver_sha256']==sha(ROOT/'Tests/V2RenderMain.cpp',True) and audition['script_sha256']==sha(ROOT/'Tools/render_zastava_s2.py',True)
    assert audition['renderer_sha256']==sha(ROOT/'build/Release/HamloZastava_V2Render.exe')
    for row in audition['rows']:
        assert sha(Path(row['wav']))==row['wav_sha256'] and sha(Path(row['wav']).parent/row['name']/'output.f32')==row['raw_sha256']
        assert row['manifest']['source_id']==after['source_id'] and row['manifest']['render_id']==audition['driver_sha256']
        assert row['manifest']['finite'] and row['manifest']['allocations']==0 and row['true_peak_dbtp']<=-1
        assert sha(Path(row['input']['path']))==row['input']['sha256'],'Owner input changed'
        assert sha(Path(row['wav']).parent/(row['recipe']+'_input.f32'))==row['input']['prepared_sha256']
        if row['key']:assert sha(Path(row['key']['path']))==row['key']['sha256']
    files=['CMakeLists.txt','Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h','Tests/V2LabMain.cpp','Tests/V2RenderMain.cpp','Tools/analyze_zastava_s2.py','Tools/render_zastava_s2.py','Tools/verify_zastava_s2.py','cmake/WriteS2Identity.cmake','docs/S2_METHODS.md']
    result=dict(stage='S2 offline four-role kernel',source_id=after['source_id'],method_id=after['identity']['method_id'],before=dict(cases=66,passed=218,failed=0,unavailable=9),after=dict(cases=66,passed=275,failed=0,unavailable=0),original_control_PCM_unchanged=49,comment_revision_PCM_unchanged=132,regression_detected=regression_checks,negative_guards=guards,retained_S1_files=retained,production_dll_sha256=sha(dll),audition_count=8,audition_true_peak_range_dbtp=[min(r['true_peak_dbtp'] for r in audition['rows']),max(r['true_peak_dbtp'] for r in audition['rows'])],source_files={name:sha(ROOT/name,True) for name in files},artifacts={str(path.relative_to(REPORT)).replace('\\','/'):sha(path) for path in sorted(REPORT.glob('*.json')) if path.name!='acceptance.json'},executables={name:sha(WORK/name) for name in ['before.exe','candidate_01.exe','before_final.exe','candidate_02.exe','regression_01_final_method.exe','before_v3.exe','after_v3.exe','regression_v3.exe']},audit_folder=str(audit),host_integration=False,ear_audition_verified=False,installer_built=False)
    write(REPORT/'acceptance.json',result)
    print('S2 accepted:275checks,5negative guards,4regression detections,8owner renders; S1VST unchanged')


if __name__=='__main__':main()
