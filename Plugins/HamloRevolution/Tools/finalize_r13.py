"""Freeze the validated R13 source and exact VST3; private A/B audio stays outside ZIP."""
import json
import pathlib
import re
import shutil
import sys
import zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import verify_r13 as verify
ROOT=pathlib.Path(__file__).resolve().parents[1]


def main():
    report=ROOT/'docs/reports/r13';package=ROOT/'dist/HamloRevolution-0.3.1-r13'
    accepted=ROOT/'out/r13/accepted';archive=ROOT/'dist/HamloRevolution-0.3.1-r13-win64-vst3.zip'
    for p in (package,accepted,archive,report/'acceptance.json'):assert not p.exists(),p
    identity=verify.read(report/'validators_final/identity.json')
    build=ROOT/'build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3'
    dll=pathlib.Path('Contents/x86_64-win/HamloRevolution.vst3')
    assert verify.digest(build/dll)==identity['dll_sha256']
    for s in verify.read(report/'materials.json')['sources']:
        assert verify.digest(pathlib.Path(s['original']))==s['original_sha256']
        assert verify.digest(pathlib.Path(s['path']))==s['prepared_sha256']
    package.mkdir(parents=True);shutil.copytree(build,package/'HamloRevolution.vst3')
    shutil.copytree(ROOT/'out/r13/demos_final',package/'Audio')
    (package/'README.txt').write_text(
        'HamloRevolution 0.3.1 / R13 / Windows x64 VST3\n\n'
        'Copy the complete HamloRevolution.vst3 bundle to your VST3 location and rescan.\n'
        'SIMPLE -> DRUM FILL -> AUTO SLICER. Load a drum loop or capture your track.\n'
        'Set host BPM to match the source; sample loading does not detect/stretch tempo.\n'
        'BARS is output length; EDIT LAST selects the final 1/2/4 beats to rearrange.\n'
        'Try the twelve STYLE families, CHOP/ROLL/REVERSE/STOP, FILTER + ROOM.\n'
        'NEW VARIATION reuses the capture; GENERATE captures again. PLAY, EXPORT/DRAG WAV.\n'
        'Earlier recipes are in CLASSIC / R12 RECIPES. RISE and old projects unchanged.\n'
        'Audio folder: 24 private pairs, R12 then 0.3s gap then R13, equal RMS.\n'
        'Private audio is excluded from the portable ZIP and factory plugin.\n'
        '4897 native,288 old exact,225 programs/states,pluginval5-10,Steinberg537.\n'
        '48 prepared renders <= -1 dBTP. Twelve families broaden available gestures;\n'
        'one dense 170 BPM source has lower envelope diversity than R12 Auto.\n'
        'Role analysis is heuristic, not stem separation. Rendering is offline.\n'
        'Manual FL Studio listening is still for the owner. No installer.\n',encoding='utf-8')
    (package/'HamloRevolution.vst3'/dll).with_name('HamloRevolution.vst3.sha256').write_text(identity['dll_sha256']+'  HamloRevolution.vst3\n')
    with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(package.rglob('*')):
            if p.is_file() and 'Audio' not in p.relative_to(package).parts:z.write(p,p.relative_to(package).as_posix())
    archive.with_name(archive.name+'.sha256').write_text(verify.digest(archive)+'  '+archive.name+'\n')
    accepted.mkdir(parents=True)
    for name in ('Source','Tests','Tools','Assets','cmake'):
        shutil.copytree(ROOT/name,accepted/name,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copy2(ROOT/'CMakeLists.txt',accepted/'CMakeLists.txt');shutil.copytree(build,accepted/'HamloRevolution.vst3')
    for name in ('HamloRevolution_Tests.exe','HamloRevolution_DrumFillLab.exe','HamloRevolution_SpatialLab.exe','HamloRevolution_Snapshot.exe'):
        shutil.copy2(ROOT/'build/Release'/name,accepted/name)
    paths=set()
    for folder in ('Source','Tests','Assets','cmake','docs/reports/r13','out/r13/accepted','out/r13/materials_v2','dist/HamloRevolution-0.3.1-r13'):
        paths.update(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for name in ('.gitattributes','AGENTS.md','README.md','CMakeLists.txt','docs/DRUM_FILL_R13.md','docs/R13_HANDOFF.md','docs/NEXT_STAGE.md','Tools/verify_r13.py','Tools/finalize_r13.py','Tools/analyze_r13.py','Tools/r13_materials.py','Tools/validate_r13.ps1','out/r13/DrumFillLab-before.cpp','out/r13/DrumFillLab-before.exe'):
        paths.add(ROOT/name)
    paths.add(archive);paths.add(archive.with_name(archive.name+'.sha256'))
    files={}
    for p in sorted(paths):
        name=p.relative_to(ROOT).as_posix();normal=not name.startswith('docs/reports/r13/') and (p.name=='.gitattributes' or p.suffix in ('.cpp','.h','.inc','.md','.py','.ps1','.txt','.cmake'))
        files[name]={'sha256':verify.digest(p,normal),'normalized':normal}
    native=(report/'native_final.txt').read_text(errors='replace');count=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, 0 failed',native)
    assert count
    receipt={'version':'0.3.1','parent':'8470eeb16d6c28bda17dc4acfd9d084a9f44df75','native_expectations':int(count[1]),'files':files,
             'build_dll':(build/dll).relative_to(ROOT).as_posix(),'delivery_dll':(package/'HamloRevolution.vst3'/dll).relative_to(ROOT).as_posix(),
             'package':package.relative_to(ROOT).as_posix(),'archive':archive.relative_to(ROOT).as_posix(),
             'manual_FL_listening':'unverified','installer':'not requested','push':'not requested'}
    (report/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n')
    verify.check();print('DLL',identity['dll_sha256']);print('ZIP',verify.digest(archive))


if __name__=='__main__':main()
