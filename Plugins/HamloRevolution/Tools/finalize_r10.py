"""Freeze and verify R10 evidence. Refuse to overwrite the acceptance receipt."""
import json
import pathlib
import re
import shutil
import zipfile

import verify_r10

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPORT = ROOT / 'docs/reports/r10'
PACKAGE = ROOT / 'dist/HamloRevolution-0.2.2-r10'


def main():
    receipt_path = REPORT / 'acceptance.json'
    accepted = ROOT / 'out/r10/accepted'
    archive = ROOT / 'dist/HamloRevolution-0.2.2-r10-win64-vst3.zip'
    assert not receipt_path.exists() and not accepted.exists() and not archive.exists()
    lab = ROOT / 'out/r10/after_delivery/results.json'
    # UI caption fix must not change even one result in the preceding sound audit.
    assert lab.read_bytes() == (ROOT / 'out/r10/after_release/results.json').read_bytes()
    shutil.copy2(lab, REPORT / 'after_release.json')
    assert verify_r10.digest(ROOT / 'Tests/SpatialLab.cpp') == '5664ba5805351e6e52fab204241776e2d8b981821f7533e588d8909899201ddb'
    assert (ROOT / 'out/r10/baseline/SpatialLab.cpp').read_bytes() == (ROOT / 'Tests/SpatialLab.cpp').read_bytes()
    identity = json.loads((REPORT / 'validators_delivery/identity.json').read_text(encoding='utf-8-sig'))
    dll = pathlib.Path('HamloRevolution.vst3/Contents/x86_64-win/HamloRevolution.vst3')
    assert verify_r10.digest(PACKAGE / dll) == identity['dll_sha256']
    native = (REPORT / 'native_delivery.txt').read_text(encoding='utf-8', errors='replace')
    match = re.search(r'HamloRevolution_Tests: (\d+) expectations passed, (\d+) failed', native)
    assert match and int(match[2]) == 0
    accepted.mkdir(parents=True)
    for name in ('Source', 'Tests', 'Tools', 'Assets', 'cmake'):
        shutil.copytree(ROOT / name, accepted / name, ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copy2(ROOT / 'CMakeLists.txt', accepted / 'CMakeLists.txt')
    shutil.copytree(PACKAGE / 'HamloRevolution.vst3', accepted / 'HamloRevolution.vst3')
    for name in ('HamloRevolution_Tests.exe', 'HamloRevolution_SpatialLab.exe', 'HamloRevolution_Snapshot.exe'):
        shutil.copy2(ROOT / 'build/Release' / name, accepted / name)
    (PACKAGE / dll).with_name(dll.name + '.sha256').write_text(
        verify_r10.digest(PACKAGE / dll) + '  ' + dll.name + '\n', encoding='ascii')
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in sorted(PACKAGE.rglob('*')):
            if path.is_file():
                z.write(path, path.relative_to(PACKAGE.parent).as_posix())
    for path in (PACKAGE / dll, archive):
        path.with_name(path.name + '.sha256').write_text(
            verify_r10.digest(path) + '  ' + path.name + '\n', encoding='ascii')
    paths = set()
    for folder in ('Source', 'Tests', 'Assets', 'cmake', 'docs/reports/r10', 'out/r10/baseline', 'out/r10/before', 'out/r10/accepted',
                   'dist/HamloRevolution-0.2.2-r10'):
        paths.update(p for p in (ROOT / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for name in ('.gitattributes', 'AGENTS.md', 'README.md', 'CMakeLists.txt', 'docs/NEXT_STAGE.md',
                 'docs/THROW_GENERATION.md', 'Tools/finalize_r10.py', 'Tools/verify_r10.py', 'Tools/validate_r10.ps1'):
        paths.add(ROOT / name)
    paths.add(archive)
    paths.add(archive.with_name(archive.name + '.sha256'))
    files = {}
    for path in sorted(paths):
        name = path.relative_to(ROOT).as_posix()
        normalized = not name.startswith('docs/reports/r10/') and (path.name == '.gitattributes' or path.suffix.lower() in (
            '.cpp', '.h', '.inc', '.md', '.py', '.ps1', '.txt', '.cmake'))
        files[name] = {'sha256': verify_r10.digest(path, normalized), 'normalized': normalized}
    receipt = {
        'stage': 'R10', 'version': '0.2.2', 'parent': '71545ffa60e2805cc5fa2f6c3927b8d2be3a33ac',
        'native_expectations': int(match[1]), 'files': files,
        'build_dll': 'build/HamloRevolution_artefacts/Release/VST3/' + dll.as_posix(),
        'delivery_dll': 'dist/HamloRevolution-0.2.2-r10/' + dll.as_posix(),
        'manual_FL_listening': 'not claimed', 'installer': 'not requested', 'push': 'not requested',
    }
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    verify_r10.check()
    print('DLL', identity['dll_sha256'])
    print('ZIP', verify_r10.digest(archive))


if __name__ == '__main__':
    main()
