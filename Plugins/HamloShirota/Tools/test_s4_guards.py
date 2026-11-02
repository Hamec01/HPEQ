"""Negative tests for S4 identities/raw caches and the actual embedded bank."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'out/s4'
final=json.loads((ROOT/'docs/reports/s4/stage_final.json').read_text())
run=WORK/'stage_final'
original=json.loads((run/'manifest.json').read_text())
results=[]


def reject(name,mutate,message,corrupt=False):
    with tempfile.TemporaryDirectory(prefix='guard-',dir=WORK) as temp:
        folder=Path(temp);assert folder.resolve().is_relative_to(WORK.resolve())
        manifest=dict(original);mutate(manifest)
        (folder/'manifest.json').write_bytes((run/'manifest.json').read_bytes() if corrupt else json.dumps(manifest).encode())
        if corrupt:
            (folder/'identity.json').write_bytes((run/'identity.json').read_bytes())
            (folder/'wrong.f32').write_bytes(b'changed PCM')
        process=subprocess.run([sys.executable,'-I','-X','utf8','Tools/analyze_s4.py',str(folder),str(folder/'report.json')],cwd=ROOT,capture_output=True,text=True)
        passed=process.returncode!=0 and message in process.stderr and not (folder/'report.json').exists()
        results.append(dict(name=name,passed=passed,rejection=message,exit_code=process.returncode));assert passed,process.stderr


reject('wrong_method',lambda m:m.update(method_id='old method'),'Stale compiled laboratory method')
reject('wrong_production',lambda m:m.update(source_id='old object'),'Stale compiled production identity')
reject('unimplemented_candidate',lambda m:m.update(implemented=False),'Wrong experiment role')
reject('changed_raw_cache',lambda m:None,'Corrupt or stale cached render',True)
process=subprocess.run([str(ROOT/'build/Release/HamloShirota_EmbeddedBankFault.exe')],cwd=ROOT,capture_output=True,text=True)
passed=process.returncode==0 and 'Stale embedded HRIR bank' in process.stdout
results.append(dict(name='actual_embedded_bank_fault',passed=passed,exit_code=process.returncode,rejection=process.stdout.strip(),scope='one byte in this disposable process memory only; no file/DLL changed'));assert passed,process.stderr+process.stdout
(ROOT/'docs/reports/s4/guard_tests.json').write_text(json.dumps(dict(source_id=final['source_id'],method_id=final['method_id'],checks=results),indent=2)+'\n',encoding='utf-8',newline='\n')
print('5/5 S4 corruption/identity rejection tests passed')
