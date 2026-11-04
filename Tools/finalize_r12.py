"""Freeze R12 evidence and package the exact validated transition build."""
import json
import pathlib
import re
import shutil
import zipfile
import verify_r12
ROOT=pathlib.Path(__file__).resolve().parents[1]


def main():
    report=ROOT/'docs/reports/r12';package=ROOT/'dist/HamloRevolution-0.3.0-r12';accepted=ROOT/'out/r12/accepted'
    archive=ROOT/'dist/HamloRevolution-0.3.0-r12-win64-vst3.zip'
    assert not package.exists() and not accepted.exists() and not archive.exists() and not (report/'acceptance.json').exists()
    identity=json.loads((report/'validators_final/identity.json').read_text(encoding='utf-8-sig'))
    dll=pathlib.Path('HamloRevolution.vst3/Contents/x86_64-win/HamloRevolution.vst3');build=ROOT/'build/HamloRevolution_artefacts/Release/VST3'
    assert verify_r12.digest(build/dll)==identity['dll_sha256']
    assert (ROOT/'Tests/TransitionLab.cpp').read_bytes()==(ROOT/'out/r12/baseline/TransitionLab.cpp').read_bytes()
    materials=json.loads((report/'materials.json').read_text())
    for ref in materials['references']:assert verify_r12.digest(pathlib.Path(ref['path']))==ref['sha256'],ref['path']
    for source in materials['sources']:
        assert verify_r12.digest(pathlib.Path(source['original']))==source['original_sha256']
        assert verify_r12.digest(pathlib.Path(source['path']))==source['prepared_sha256']
    package.mkdir(parents=True);shutil.copytree(build/'HamloRevolution.vst3',package/'HamloRevolution.vst3')
    shutil.copytree(ROOT/'out/r12/demos_final',package/'Audio')
    (package/'README.txt').write_text(
        'HamloRevolution0.3.0 - R12 - Windows x64 VST3\n\n'
        'Copy the complete HamloRevolution.vst3 bundle to your VST3 folder,rescan in FL.\n'
        'SIMPLE -> RISE or FILL -> STYLE. AUTO varies one of six musical families.\n'
        'RISE:LIFT,PULSE,SWELL,RUSH,ARC,PRESSURE. Try2/4bars.\n'
        'DRUM FILL:POCKET,ROLL,BREAK,TRIPLETS,SCATTER,TURNAROUND. Try1bar.\n'
        'LOAD SAMPLE or capture your track,adjust four knobs,GENERATE,PLAY,EXPORT/DRAG WAV.\n'
        'CLASSIC retains the previous transition renderer. Existing sequencer/programs unchanged.\n'
        'Local Audio folder:private owner-material BEFORE,0.4s gap,AFTER at equal RMS.\n'
        'Only the VST3 and this README are in the portable ZIP;Audio is separate.\n'
        '4769 native,96 exact Classic,225 exact program/state renders,pluginval5-10/Steinberg537.\n'
        'No new installer or installed-copy replacement. No reference-library sounds inside the plugin.\n',encoding='utf-8')
    (package/dll).with_name('HamloRevolution.vst3.sha256').write_text(identity['dll_sha256']+'  HamloRevolution.vst3\n',encoding='ascii')
    entries=[p for p in sorted(package.rglob('*')) if p.is_file() and 'Audio' not in p.relative_to(package).parts]
    with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for path in entries:z.write(path,path.relative_to(package).as_posix())
    archive.with_name(archive.name+'.sha256').write_text(verify_r12.digest(archive)+'  '+archive.name+'\n',encoding='ascii')
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for name in z.namelist():assert z.read(name)==(package/name).read_bytes()
    accepted.mkdir(parents=True)
    for name in ('Source','Tests','Tools','Assets','cmake'):
        shutil.copytree(ROOT/name,accepted/name,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copy2(ROOT/'CMakeLists.txt',accepted/'CMakeLists.txt');shutil.copytree(package/'HamloRevolution.vst3',accepted/'HamloRevolution.vst3')
    for name in ('HamloRevolution_Tests.exe','HamloRevolution_TransitionLab.exe','HamloRevolution_SpatialLab.exe','HamloRevolution_Snapshot.exe'):
        shutil.copy2(ROOT/'build/Release'/name,accepted/name)
    folders=('Source','Tests','Assets','cmake','docs/reports/r12','out/r12/baseline','out/r12/materials','out/r12/accepted','dist/HamloRevolution-0.3.0-r12')
    paths=set()
    for folder in folders:paths.update(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for name in ('.gitattributes','AGENTS.md','README.md','CMakeLists.txt','docs/TRANSITIONS_V2.md','docs/R12_HANDOFF.md','docs/NEXT_STAGE.md',
                 'Tools/verify_r12.py','Tools/finalize_r12.py','Tools/validate_r12.ps1','Tools/transition_materials.py','Tools/analyze_transitions.py','Tools/transition_demos.py'):
        paths.add(ROOT/name)
    paths.add(archive);paths.add(archive.with_name(archive.name+'.sha256'))
    files={}
    for path in sorted(paths):
        name=path.relative_to(ROOT).as_posix();normal=not name.startswith('docs/reports/r12/') and (path.name=='.gitattributes' or path.suffix in ('.cpp','.h','.inc','.md','.py','.ps1','.txt','.cmake','.html'))
        files[name]={'sha256':verify_r12.digest(path,normal),'normalized':normal}
    native=(report/'native_final.txt').read_text();count=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, 0 failed',native)
    receipt={'version':'0.3.0','parent':'5e89bae','native_expectations':int(count[1]),'files':files,
        'build_dll':(build/dll).relative_to(ROOT).as_posix(),'delivery_dll':(package/dll).relative_to(ROOT).as_posix(),
        'manual_FL_listening':'unverified','installer':'not requested','push':'not requested'}
    (report/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    verify_r12.check();print('DLL',identity['dll_sha256']);print('ZIP',verify_r12.digest(archive))


if __name__=='__main__':main()
