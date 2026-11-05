"""Freeze R14 evidence and the validated portable0.4.0VST3. Private audio excluded."""
import json,pathlib,re,shutil,sys,zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import verify_r14 as verify
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
    report=ROOT/'docs/reports/r14';package=ROOT/'dist/HamloRevolution-0.4.0-r14';accepted=ROOT/'out/r14/accepted';archive=ROOT/'dist/HamloRevolution-0.4.0-r14-win64-vst3.zip'
    for p in (package,accepted,archive,report/'acceptance.json'):assert not p.exists(),p
    identity=verify.read(report/'validators_final/identity.json');build=ROOT/'build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3';dll=pathlib.Path('Contents/x86_64-win/HamloRevolution.vst3')
    assert verify.digest(build/dll)==identity['dll_sha256']
    for s in verify.read(report/'materials.json')['sources']:
        assert verify.digest(pathlib.Path(s['original']))==s['original_sha256']
        assert verify.digest(pathlib.Path(s['path']))==s['prepared_sha256']
    package.mkdir(parents=True);shutil.copytree(build,package/'HamloRevolution.vst3');shutil.copytree(ROOT/'out/r14/demos_final',package/'Audio')
    (package/'README.txt').write_text(
        'HamloRevolution0.4.0 / R14 / Windows x64 VST3\n\n'
        'Copy the complete HamloRevolution.vst3 bundle to your VST3 location and rescan.\n'
        'SIMPLE -> DRUM FILL -> AUTO COMPOSE. Match host BPM to your source loop.\n'
        'The composer extracts filtered kick/snare/hat slices and builds new parts.\n'
        'Eight formula families,independent kit switches,coloured rhythm rows.\n'
        'CHOP changes density,ROLL builds snare bursts,REVERSE adds pickups,STOP leaves a final gap.\n'
        'BARS selects output length;EDIT LAST selects the final1/2/4beats to rewrite.\n'
        'NEW VARIATION keeps capture and advances seed;GENERATE captures again.\n'
        'The formula uses aggregate statistics from597owner-reference fills,not their audio/patterns.\n'
        'Signal-based band/time extraction cannot perfectly isolate overlapping drums.\n'
        'R13 SLICER and CLASSIC / R12 RECIPES preserve old renderers. RISE/projects unchanged.\n'
        'Audio folder:24private equal-RMS comparisons:source,0.3sgap,R13,0.3sgap,R14.\n'
        'Private audio is outside the portable ZIP and plugin.\n'
        '444old exact,225program/state comparisons,native suite,pluginval5-10,Steinberg537pass.\n'
        'Independent audio-onset similarity measured separately from plan novelty;see report.\n'
        'Offline rendering can briefly occupy the UI. No tempo detection/time stretching.\n'
        'Owner FL Studio listening remains unverified. No installer.\n',encoding='utf-8')
    (package/'HamloRevolution.vst3'/dll).with_name('HamloRevolution.vst3.sha256').write_text(identity['dll_sha256']+'  HamloRevolution.vst3\n')
    with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(package.rglob('*')):
            if p.is_file() and 'Audio' not in p.relative_to(package).parts:z.write(p,p.relative_to(package).as_posix())
    archive.with_name(archive.name+'.sha256').write_text(verify.digest(archive)+'  '+archive.name+'\n')
    accepted.mkdir(parents=True)
    for name in ('Source','Tests','Tools','Assets','cmake'):shutil.copytree(ROOT/name,accepted/name,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copy2(ROOT/'CMakeLists.txt',accepted/'CMakeLists.txt');shutil.copytree(build,accepted/'HamloRevolution.vst3')
    for name in ('HamloRevolution_Tests.exe','HamloRevolution_DrumComposeLab.exe','HamloRevolution_SpatialLab.exe','HamloRevolution_Snapshot.exe'):shutil.copy2(ROOT/'build/Release'/name,accepted/name)
    paths=set()
    for folder in ('Source','Tests','Assets','cmake','docs/reports/r14','out/r14/accepted','out/r13/materials_v2','out/r14/reference','dist/HamloRevolution-0.4.0-r14'):
        paths.update(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for name in ('.gitattributes','AGENTS.md','README.md','CMakeLists.txt','docs/DRUM_FILL_R14.md','docs/R14_HANDOFF.md','docs/NEXT_STAGE.md','Tools/verify_r14.py','Tools/finalize_r14.py','Tools/analyze_r14.py','Tools/r14_reference.py','Tools/validate_r14.ps1','out/r14/DrumComposeLab-before.cpp','out/r14/DrumComposeLab-before.exe','out/r13/DrumFillLab-before.cpp'):
        paths.add(ROOT/name)
    paths.add(archive);paths.add(archive.with_name(archive.name+'.sha256'));files={}
    for p in sorted(paths):
        name=p.relative_to(ROOT).as_posix();normal=not name.startswith('docs/reports/r14/') and (p.name=='.gitattributes' or p.suffix in ('.cpp','.h','.inc','.md','.py','.ps1','.txt','.cmake'))
        files[name]={'sha256':verify.digest(p,normal),'normalized':normal}
    match=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, 0 failed',(report/'native_final.txt').read_text(errors='replace'));assert match
    receipt={'version':'0.4.0','parent':'fe46e5cd0b90a7cbbfbbeca58d85f6b5e43ec5d8','native_expectations':int(match[1]),'files':files,'build_dll':(build/dll).relative_to(ROOT).as_posix(),'delivery_dll':(package/'HamloRevolution.vst3'/dll).relative_to(ROOT).as_posix(),'package':package.relative_to(ROOT).as_posix(),'archive':archive.relative_to(ROOT).as_posix(),'manual_FL_listening':'unverified','installer':'not requested','push':'not requested'}
    (report/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n');verify.check();print('DLL',identity['dll_sha256']);print('ZIP',verify.digest(archive))
if __name__=='__main__':main()
