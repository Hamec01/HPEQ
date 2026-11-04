import pathlib,shutil,json
import sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import verify_f4 as verify
root=verify.ROOT;report=root/'docs/reports/f4';accepted=root/'out/f4/accepted'
assert not accepted.exists() and not (report/'acceptance.json').exists()
for name in ('host_v1.json','legacy_v1.json'):shutil.copy2(root/'out/f4'/name,report/name)
accepted.mkdir()
for folder in ('Source','Tests','Tools','cmake'):
    shutil.copytree(root/folder,accepted/folder,ignore=shutil.ignore_patterns('__pycache__'))
shutil.copy2(root/'CMakeLists.txt',accepted/'CMakeLists.txt')
bundle=root/'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3'
shutil.copytree(bundle,accepted/'HamloFader.vst3')
for name in ('HamloFader_Tests.exe','HamloFader_Snapshot.exe','HamloFader_F3Host.exe','HamloFader_F3Legacy.exe'):
    shutil.copy2(root/'build/Release'/name,accepted/name)
paths=set()
for folder in ('Source','Tests','Tools','cmake','docs/reports/f4','out/f4/accepted'):
    paths.update(p for p in (root/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
for name in ('.gitattributes','CMakeLists.txt','AGENTS.md','docs/NEXT_STAGE.md','docs/F4_UI.md'):paths.add(root/name)
files={}
for p in sorted(paths):
    name=p.relative_to(root).as_posix();normal=not name.startswith('docs/reports/') and (p.suffix in ('.cpp','.h','.py','.ps1','.md','.txt','.cmake') or p.name=='.gitattributes')
    files[name]=dict(sha256=verify.sha(p,normal),normalized=normal)
receipt=dict(stage='F4',files=files,dll=(accepted/'HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3').relative_to(root).as_posix())
(report/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
verify.check()
