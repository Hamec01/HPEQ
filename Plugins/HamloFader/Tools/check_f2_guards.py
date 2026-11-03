"""Negative provenance checks preserve original runs and invoke the real analyzer."""
import copy
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];original=ROOT/'out/f2/after_v4';target=ROOT/'out/f2/guards_v4';assert not target.exists();target.mkdir();rows=[]
for name,reason in [('method','Stale method'),('source','Stale kernel'),('raw','Corrupt cached output'),('manifest','Corrupt cached output')]:
 folder=target/name;folder.mkdir();manifest=json.loads((original/'manifest.json').read_text())
 if name=='method':manifest['method_id']='0'*64
 if name=='source':manifest['source_id']='0'*64
 if name=='manifest':manifest['unexpected']='changed'
 (folder/'manifest.json').write_text(json.dumps(manifest),encoding='utf-8')
 if name in ['raw','manifest']:
  # Copy original metadata bytes: the pinned manifest hash must remain binding.
  if name=='raw':(folder/'manifest.json').write_bytes((original/'manifest.json').read_bytes())
  (folder/'identity.json').write_bytes((original/'identity.json').read_bytes())
  for path in original.glob('*.f32'):
   if name=='raw' and path.name=='gain_6.f32':
    b=bytearray(path.read_bytes());b[100]^=1;(folder/path.name).write_bytes(b)
   else:os.link(path,folder/path.name)
 run=subprocess.run([sys.executable,'-I','-X','utf8',str(ROOT/'Tools/analyze_f2.py'),str(folder),str(folder/'result.json')],capture_output=True,text=True)
 assert run.returncode!=0 and reason in run.stderr,(name,run.stdout,run.stderr)
 text=run.stdout+run.stderr;(folder/'rejection.txt').write_text(text,encoding='utf-8');rows.append(dict(name=name,rejected=True,reason=reason,log_sha256=hashlib.sha256((folder/'rejection.txt').read_bytes()).hexdigest()))
(ROOT/'docs/reports/f2/guards_v4.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8',newline='\n');print('F2four negative guards rejected')
