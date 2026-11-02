import hashlib,json,runpy,zipfile,re
from pathlib import Path
r=Path(__file__).resolve().parents[1]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
p=load(r/'dist/release-manifest.json');v=load(r/'docs/reports/release_020_final/validated_payload.json')
source=runpy.run_path(str(r/'Tools/calibrate_score.py'))['current_dsp']()
assert source==p['dsp_source']==v['dsp_source']
lib=load(r/'docs/reports/presets/verification.json');assert lib['dsp_source']==source and lib['passed']==200 and lib['failed']==0
bundle=r/'build/HamloEcho_artefacts/Release/VST3/HamloEcho.vst3'
for f in v['files']:assert sha(bundle/f['path'])==f['sha256']
for a in p['artifacts']:
    path=r/'dist'/a['name'];assert sha(path)==a['sha256'];assert Path(str(path)+'.sha256').read_text().split()[0]==a['sha256']
with zipfile.ZipFile(r/'dist/HamloEcho-0.2.0-win64-vst3.zip') as z:
    entries={n.replace('\\','/'):n for n in z.namelist()}
    for f in v['files']:assert hashlib.sha256(z.read(entries['HamloEcho.vst3/'+f['path']])).hexdigest()==f['sha256']
old=r/'out/m0/HamloEcho.vst3/Contents/Resources/moduleinfo.json'
metadata=lambda path:json.loads(re.sub(r',(?=\s*[}\]])','',path.read_text(encoding='utf-8-sig')))
current=metadata(bundle/'Contents/Resources/moduleinfo.json')
cids=lambda x:sorted(c['CID'] for c in x['Classes'])
old_cids=cids(metadata(old)) if old.exists() else []
assert old_cids and old_cids==cids(current),'VST3 class identity changed or missing reference'
result=dict(dsp_source=source,plugin_sha256=p['plugin_sha256'],artifacts_verified=3,companion_sha256_verified=3,zip_core_hashes_match=True,original_vst3_class_ids_preserved=True,class_ids=old_cids,library_passed=200,owner_fl_audition='pending',installer_executed=False)
(r/'docs/reports/release_020_final/final_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
