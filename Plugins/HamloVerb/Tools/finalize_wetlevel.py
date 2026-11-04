"""Freeze accepted Wet Level sources and make the VST-only portable package."""
import json
import re
import shutil
import zipfile
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import verify_wetlevel as verify

ROOT=verify.ROOT
report=ROOT/'docs/reports/wetlevel'
package=ROOT/'dist/WetLevel0.2.2'
accepted=ROOT/'build/wetlevel_accepted'
receipt_path=report/'acceptance.json'
assert not package.exists() and not accepted.exists() and not receipt_path.exists()
identity=json.loads((report/'validators/identity.json').read_text(encoding='utf-8-sig'))
bundle=ROOT/'build/HamloVerb_artefacts/Release/VST3/HamloVerb.vst3'
relative_dll='HamloVerb.vst3/Contents/x86_64-win/HamloVerb.vst3'
assert verify.digest(bundle/'Contents/x86_64-win/HamloVerb.vst3')==identity['dll_sha256']
package.mkdir(parents=True)
shutil.copytree(bundle,package/'HamloVerb.vst3')
(package/'README.txt').write_text(
    'HamloVerb 0.2.2 - Windows x64 VST3\n\n'
    'Copy the whole HamloVerb.vst3 bundle to your VST3 folder and rescan.\n'
    'WET LEVEL on Main controls only the reverb return, -24..+24 dB.\n'
    'For a quiet tail, try +6 or +12 dB, then adjust MIX.\n'
    'KEEP DRY on / INSERT / OUTPUT 0 dB keeps the direct source at unity.\n'
    'SHIMMER remains in Advanced > Effects. Legacy ignores WET LEVEL.\n'
    'Factory presets reset WET LEVEL to 0 dB; save your edits in the project.\n'
    '0 dB keeps all previous sounds unchanged. WET meters the trimmed return.\n'
    'Boost can increase peaks; no automatic limiting or gain compensation.\n'
    'No installer or installed-copy replacement; manual FL listening pending.\n',encoding='utf-8')
(package/relative_dll).with_suffix('.vst3.sha256').write_text(identity['dll_sha256']+'  HamloVerb.vst3\n',encoding='ascii')
archive=package/'HamloVerb-0.2.2-win64-vst3.zip'
files=sorted(p for p in package.rglob('*') if p.is_file())
with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for path in files:z.write(path,path.relative_to(package).as_posix())
archive.with_name(archive.name+'.sha256').write_text(verify.digest(archive)+'  '+archive.name+'\n',encoding='ascii')
accepted.mkdir()
for name in ('Source','Tests','Tools','Assets','cmake'):
    shutil.copytree(ROOT/name,accepted/name,ignore=shutil.ignore_patterns('__pycache__'))
shutil.copy2(ROOT/'CMakeLists.txt',accepted/'CMakeLists.txt')
shutil.copytree(bundle,accepted/'HamloVerb.vst3')
for name in ('HamloVerb_Tests.exe','HamloVerb_Reference.exe','HamloVerb_Snapshot.exe'):
    shutil.copy2(ROOT/'build/Release'/name,accepted/name)
paths=set()
for folder in ('Source','Tests','Assets','cmake','docs/reports/wetlevel','build/wetlevel_baseline','build/wetlevel_accepted','dist/WetLevel0.2.2','out/wetlevel_demo_final'):
    paths.update(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
for name in ('.gitattributes','CMakeLists.txt','README.md','AGENTS.md','docs/NEXT_STAGE.md','docs/WET_LEVEL.md','docs/WET_LEVEL_HANDOFF.md',
             'Tools/verify_wetlevel.py','Tools/finalize_wetlevel.py','Tools/Validate-WetLevel.ps1','Tools/analyze_wetlevel.py','docs/reports/keepdry/native_final.txt'):
    paths.add(ROOT/name)
pins={}
for path in sorted(paths):
    name=path.relative_to(ROOT).as_posix()
    normalized=not name.startswith('docs/reports/') and (path.name=='.gitattributes' or path.suffix in ('.cpp','.h','.inc','.md','.py','.ps1','.txt','.cmake','.html'))
    pins[name]=dict(sha256=verify.digest(path,normalized),normalized=normalized)
text=(report/'native_final.txt').read_text(errors='replace')
count=re.search(r'HamloVerb_Tests: (\d+) expectations passed, 0 failed',text)
assert count
receipt=dict(version='0.2.2',native_expectations=int(count[1]),files=pins,
    build_dll=(bundle/'Contents/x86_64-win/HamloVerb.vst3').relative_to(ROOT).as_posix(),
    delivery_dll=(package/relative_dll).relative_to(ROOT).as_posix(),
    archive=archive.relative_to(ROOT).as_posix(),package=package.relative_to(ROOT).as_posix(),
    manual_FL_listening='unverified',installer='not produced',push='not requested for this change')
receipt_path.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
verify.check()
print('DLL',identity['dll_sha256'])
print('ZIP',verify.digest(archive))
