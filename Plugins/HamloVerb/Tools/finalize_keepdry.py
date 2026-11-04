"""Freeze the tested Keep Dry patch and create its separate VST3 package."""
import json
import pathlib
import re
import shutil
import zipfile
import verify_keepdry

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    report = ROOT / 'docs/reports/keepdry'
    receipt_path = report / 'acceptance.json'
    package = ROOT / 'dist/KeepDry0.2.1'
    accepted = ROOT / 'build/keepdry_accepted'
    assert not receipt_path.exists() and not package.exists() and not accepted.exists()
    before = (report / 'before.txt').read_text()
    after = (report / 'native_final.txt').read_text()
    pattern = r'^PROGRAM (\d+) ([a-f0-9]{64})$'
    assert re.findall(pattern, before, re.M) == re.findall(pattern, after, re.M)
    assert len(re.findall(pattern, before, re.M)) == 208
    method = ROOT / 'Tests/KeepDryTests.cpp'
    assert method.read_bytes() == (ROOT / 'build/keepdry_baseline/KeepDryTests.cpp').read_bytes()
    identity = json.loads((report / 'validators/identity.json').read_text(encoding='utf-8-sig'))
    dll = pathlib.Path('HamloVerb.vst3/Contents/x86_64-win/HamloVerb.vst3')
    build = ROOT / 'build/HamloVerb_artefacts/Release/VST3'
    assert verify_keepdry.digest(build / dll) == identity['dll_sha256']
    package.mkdir(parents=True)
    shutil.copytree(build / 'HamloVerb.vst3', package / 'HamloVerb.vst3')
    (package / 'README.txt').write_text(
        'HamloVerb0.2.1 - Windows x64 VST3\n\n'
        'Copy the entire HamloVerb.vst3 bundle to your VST3 folder and rescan in FL.\n'
        'For a source that stays at unity: modern character,INSERT,KEEP DRY on,OUTPUT0dB.\n'
        'MIX now adds reverb. The choice persists across preset loads.\n'
        'Old projects start with KEEP DRY off to preserve their original sound.\n'
        'SEND is wet only. LEGACY retains its old additive law.\n'
        'Wet can change total level/peaks;this is not automatic loudness matching.\n'
        '1439 native,208 unchanged programs,390 M0,pluginval5-10/Steinberg537.\n'
        'No new installer or automatic installed-copy replacement.\n', encoding='utf-8')
    (package / dll).with_name('HamloVerb.vst3.sha256').write_text(
        identity['dll_sha256'] + '  HamloVerb.vst3\n', encoding='ascii')
    archive = package / 'HamloVerb-0.2.1-win64-vst3.zip'
    entries = sorted(p for p in package.rglob('*') if p.is_file())
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in entries:
            z.write(path, path.relative_to(package).as_posix())
    archive.with_name(archive.name + '.sha256').write_text(
        verify_keepdry.digest(archive) + '  ' + archive.name + '\n', encoding='ascii')
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for name in z.namelist():
            assert z.read(name) == (package / name).read_bytes(), name
    accepted.mkdir(parents=True)
    for name in ('Source', 'Tests', 'Tools', 'Assets', 'cmake'):
        shutil.copytree(ROOT / name, accepted / name, ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copy2(ROOT / 'CMakeLists.txt', accepted / 'CMakeLists.txt')
    shutil.copytree(package / 'HamloVerb.vst3', accepted / 'HamloVerb.vst3')
    for name in ('HamloVerb_Tests.exe', 'HamloVerb_Reference.exe', 'HamloVerb_Snapshot.exe'):
        shutil.copy2(ROOT / 'build/Release' / name, accepted / name)
    paths = set()
    for folder in ('Source', 'Tests', 'Assets', 'cmake', 'docs/reports/keepdry',
                   'build/keepdry_baseline', 'build/keepdry_accepted', 'dist/KeepDry0.2.1'):
        paths.update(p for p in (ROOT / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for name in ('.gitattributes', 'CMakeLists.txt', 'README.md', 'AGENTS.md', 'docs/NEXT_STAGE.md',
                 'docs/KEEP_DRY_HANDOFF.md', 'docs/OUTPUT_LEVEL_AUDIT.md', 'Tools/verify_keepdry.py',
                 'Tools/finalize_keepdry.py', 'Tools/Validate-KeepDry.ps1'):
        paths.add(ROOT / name)
    files = {}
    for path in sorted(paths):
        name = path.relative_to(ROOT).as_posix()
        normal = not name.startswith('docs/reports/keepdry/') and (path.name == '.gitattributes' or path.suffix in (
            '.cpp', '.h', '.inc', '.md', '.py', '.ps1', '.txt', '.cmake'))
        files[name] = {'sha256': verify_keepdry.digest(path, normal), 'normalized': normal}
    count = re.search(r'HamloVerb_Tests: (\d+) expectations passed, 0 failed', after)
    receipt = {'version': '0.2.1', 'native_expectations': int(count[1]), 'files': files,
               'build_dll': (build / dll).relative_to(ROOT).as_posix(),
               'delivery_dll': (package / dll).relative_to(ROOT).as_posix(),
               'owner_FL_listening': 'unverified', 'installer': 'not produced', 'push': 'not requested'}
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    verify_keepdry.check()
    print('DLL', identity['dll_sha256'])
    print('ZIP', verify_keepdry.digest(archive))


if __name__ == '__main__':
    main()
