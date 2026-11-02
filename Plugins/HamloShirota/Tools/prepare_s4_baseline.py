"""Materialize only the frozen pre-integration facade; common Legacy is unchanged."""
import hashlib
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
folder=ROOT/'out/s4/frozen'
files={}
for stem in ['ModulePluginProcessor','ModulePluginEditor','ModuleVariant']:
    for suffix in ['h','cpp']:
        name='Source/Standalone/'+stem+'.'+suffix
        data=subprocess.run(['git','show','d1ef423:'+name],cwd=ROOT,check=True,capture_output=True).stdout
        path=folder/name;path.parent.mkdir(parents=True,exist_ok=True)
        if path.exists():assert path.read_bytes()==data
        else:path.write_bytes(data)
        files[name]=hashlib.sha256(data).hexdigest()
(folder/'source.json').write_text(json.dumps(dict(commit='d1ef423',files=files),indent=2)+'\n',encoding='utf-8',newline='\n')
print('Frozen original facade verified:',len(files),'files')
