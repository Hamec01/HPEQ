"""Freeze the verified Fader0.2.0 release and create its VST-only ZIP."""
import pathlib,shutil,json,re,zipfile,sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import verify_f5 as verify
root=verify.ROOT;report=verify.REPORT;accepted=root/'out/f5/accepted';package=root/'dist/HamloFader-0.2.0'
assert not accepted.exists() and not package.exists() and not (report/'acceptance.json').exists()
identity=verify.load(report/'validators_delivery/identity.json')
bundle=root/'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3';dll='HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3'
assert verify.sha(bundle/'Contents/x86_64-win/HamloFader.vst3')==identity['dll_sha256']
package.mkdir(parents=True);shutil.copytree(bundle,package/'HamloFader.vst3')
shutil.copy2(root/'docs/release/README.txt',package/'README.txt')
documentation=package/'Documentation';documentation.mkdir()
for source,name in ((root/'README.md','README.md'),(root/'docs/GENRE_PRESETS.md','GENRE_PRESETS.md'),(report/'REPORT.md','REPORT.md'),(root/'docs/release/THIRD-PARTY-NOTICES.txt','THIRD-PARTY-NOTICES.txt')):shutil.copy2(source,documentation/name)
license_folder=documentation/'ThirdParty/JUCE';license_folder.mkdir(parents=True);shutil.copy2(pathlib.Path('E:/HPEQ/build/_deps/juce-src/LICENSE.md'),license_folder/'LICENSE.md')
(package/dll).with_suffix('.vst3.sha256').write_text(identity['dll_sha256']+'  HamloFader.vst3\n',encoding='ascii')
archive=root/'dist/HamloFader-0.2.0-win64-vst3.zip'
assert not archive.exists()
with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for path in sorted(p for p in package.rglob('*') if p.is_file()):z.write(path,path.relative_to(package).as_posix())
archive.with_name(archive.name+'.sha256').write_text(verify.sha(archive)+'  '+archive.name+'\n',encoding='ascii')
accepted.mkdir()
for folder in ('Source','Tests','Tools','Assets','cmake'):shutil.copytree(root/folder,accepted/folder,ignore=shutil.ignore_patterns('__pycache__'))
shutil.copy2(root/'CMakeLists.txt',accepted/'CMakeLists.txt');shutil.copytree(bundle,accepted/'HamloFader.vst3')
for name in ('HamloFader_Tests.exe','HamloFader_Snapshot.exe','HamloFader_F3Host.exe','HamloFader_F5Legacy.exe'):shutil.copy2(root/'build/Release'/name,accepted/name)
paths=set()
for folder in ('Source','Tests','Tools','Assets','cmake','docs/reports/f5','out/f5/accepted','out/f5/check_final','out/f5/demos_final','out/f5/legacy_reference','dist/HamloFader-0.2.0','out/f4/accepted/Source/DSP'):
    paths.update(p for p in (root/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
for name in ('.gitattributes','.gitignore','CMakeLists.txt','README.md','AGENTS.md','docs/NEXT_STAGE.md','docs/F5_PRESETS.md','docs/F5_HANDOFF.md','docs/F5_PROGRESS.md','docs/GENRE_PRESETS.md','docs/release/README.txt','docs/release/THIRD-PARTY-NOTICES.txt','out/f5/F5LibraryTests-before.cpp','out/f5/Tests-before.exe','build/HamloFader_artefacts/JuceLibraryCode/HamloFader_resources.rc'):
    paths.add(root/name)
paths.update((archive,archive.with_name(archive.name+'.sha256')))
pins={}
for path in sorted(paths):
    name=path.relative_to(root).as_posix();normal=not name.startswith('docs/reports/') and (path.suffix in ('.cpp','.h','.inc','.md','.txt','.py','.ps1','.cmake','.html','.rc') or path.name in ('.gitattributes','.gitignore'))
    pins[name]=dict(sha256=verify.sha(path,normal),normalized=normal)
native=(report/'native_delivery.txt').read_text(errors='replace');count=re.search(r'HamloFader_Tests: (\d+) expectations passed, 0 failed',native);assert count
receipt=dict(stage='F5',version='0.2.0',native_expectations=int(count[1]),files=pins,
    build_dll=(bundle/'Contents/x86_64-win/HamloFader.vst3').relative_to(root).as_posix(),delivery_dll=(package/dll).relative_to(root).as_posix(),archive=archive.relative_to(root).as_posix(),package=package.relative_to(root).as_posix(),manual_FL='not certified',installer='not produced',publication='owner authorized;perform after local verified commit')
(report/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');verify.check()
print('DLL',identity['dll_sha256']);print('ZIP',verify.sha(archive))
