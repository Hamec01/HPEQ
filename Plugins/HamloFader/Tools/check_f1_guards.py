"""Exercise rejected method/contract/raw/state references without altering originals."""
import hashlib
import json
import os
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];ref=ROOT/'out/f1/reference';folder=ROOT/'out/f1/guards';assert not folder.exists();folder.mkdir()
before=json.loads((ref/'capture.json').read_text());first=before['cases'][0]['case'];rows=[]
for name in ['method','contract','raw','state']:
    target=folder/name;target.mkdir();meta=json.loads((ref/'capture.json').read_text())
    if name=='method':meta['method_id']='0'*64
    if name=='contract':meta['contract']['programs'][1]['name']='Wrong program'
    (target/'capture.json').write_text(json.dumps(meta),encoding='utf-8')
    if name in ['raw','state']:
        corrupt=first+('_0.f32' if name=='raw' else '.state')
        for p in ref.iterdir():
            if p.name=='capture.json':continue
            if p.name==corrupt:
                b=bytearray(p.read_bytes());b[min(100,len(b)-1)]^=1;(target/p.name).write_bytes(b)
            else:os.link(p,target/p.name)
    result=subprocess.run([str(ROOT/'build/Release/HamloFader_F1Lab.exe'),'verify',str(target),str(target/'result.json')],capture_output=True,text=True)
    (target/'log.txt').write_text(result.stdout+result.stderr,encoding='utf-8')
    if name in ['method','contract']:
        reason='Changed method' if name=='method' else 'Changed host contract';assert result.returncode==2 and reason in result.stderr
    else:
        report=json.loads((target/'result.json').read_text());assert result.returncode==1 and report['failures']>=1;reason=f"{report['failures']} measured failing case(s)"
    rows.append(dict(guard=name,rejected=True,exit=result.returncode,reason=reason,log_sha256=hashlib.sha256((target/'log.txt').read_bytes()).hexdigest()))
(ROOT/'docs/reports/f1/guards.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8',newline='\n');print('Four corrupted/stale reference guards rejected; original untouched')
