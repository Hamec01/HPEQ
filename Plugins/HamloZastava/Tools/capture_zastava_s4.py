"""Capture actual editor pixels with source/binary identity; refuse stale tools."""
import hashlib
import argparse
import importlib.util
import json
import struct
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
REPORT=ROOT/'docs/reports/s4'


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--label',default='');args=parser.parse_args();assert args.label in ['', '_final']
    folder=REPORT/('images'+args.label);assert not folder.exists(),'Refusing to overwrite screenshots';folder.mkdir()
    spec=importlib.util.spec_from_file_location('s3identity',ROOT/'Tools/verify_zastava_s3.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    identity=module.identity(ROOT);production=hashlib.sha256(('ZastavaS1SourceV1'+''.join(identity['files'].values())).encode()).hexdigest()
    exe=ROOT/'build/Release/HamloZastava_Snapshot.exe';binary=sha(exe);assert production.encode() in exe.read_bytes(),'Stale snapshot executable'
    # name, program, view, engine, tab, scale, listen, external, profile, enabled
    cases=[
        ('quick_clean',0,0,1,0,1,0,0,0,0),('quick_drum',0,0,2,0,1,0,0,0,0),
        ('quick_duck',0,0,3,0,1,0,0,0,0),('quick_rhythm',0,0,4,2,1,0,0,0,0),
        ('advanced_envelope',0,1,1,0,1,0,0,0,0),('advanced_key',0,1,1,1,1,0,0,0,0),
        ('advanced_rhythm',0,1,4,2,1,0,0,0,0),('legacy',1,2,0,0,1,0,0,0,0),
        ('small_quick',0,0,1,0,.5,0,0,0,0),('large_advanced',0,1,1,0,1.5,0,0,0,0),
        ('listen',0,1,1,1,1,1,0,0,0),('missing_key',0,1,3,1,1,1,1,0,1),
        ('hard_drum',0,1,2,0,1,0,0,1,1),('small_rhythm',0,1,4,2,.5,0,0,0,0),
        ('stopped_rhythm',0,0,4,2,1,0,0,0,1)]
    rows=[]
    for case in cases:
        name,*values=case;path=folder/(name+'.png');subprocess.run([str(exe),str(path),*[str(v) for v in values]],check=True,capture_output=True)
        width,height=struct.unpack('>II',path.read_bytes()[16:24]);scale=values[4];assert (width,height)==(round(1180*scale),round(780*scale))
        rows.append(dict(name=name,args=values,width=width,height=height,sha256=sha(path)))
    assert sha(exe)==binary,'Tool changed during capture'
    (REPORT/('screenshots'+args.label+'.json')).write_text(json.dumps(dict(source_id=identity['source_id'],compiled_source_id=production,executable_sha256=binary,driver_sha256=module.sha(ROOT/'Tests/SnapshotMain.cpp',True),images=rows),indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Captured',len(rows),'source-bound editor screenshots')


if __name__=='__main__':main()
