"""Verify the controller-only release, then package the already validated VST3.

Keeps the original calibration's source guard intact. Equivalence here applies
only to Quick-disabled factory recipes and byte-identical DSP source files.
"""
import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'docs/reports/quick_022'
WORK = ROOT / 'out/quick_022'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def read_log(path):
    data = path.read_bytes()
    return data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig')


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def main():
    version = re.search(r'project\(HamloEcho VERSION ([\d.]+)', (ROOT / 'CMakeLists.txt').read_text())[1]
    assert version == '0.2.2', 'This proof is scoped to the Quick 0.2.2 controller patch'
    tests = re.search(r'HamloEcho_Tests: (\d+) expectations passed, 0 failed', read_log(ROOT / 'build/quick_022_tests_final.log'))
    assert tests and int(tests[1]) >= 4197, 'Missing final tests, including monotonic TIME'
    legacy = load(REPORT / 'legacy_160.json')
    assert legacy['failures'] == 0 and len(legacy['cases']) == 160
    summary = read_log(REPORT / 'validators/summary.txt')
    assert all(f'pluginval {i} exit=0' in summary for i in range(5, 11)) and 'Steinberg exit=0' in summary
    sdk = read_log(REPORT / 'validators/steinberg.txt')
    assert re.search(r'537\s+tests?\s+passed', sdk, re.I), 'Check Steinberg result'

    before = load(ROOT / 'out/default_patch/before_bank.json')
    bank = load(WORK / 'bank.json')
    assert before['program_count'] == bank['program_count'] == 208
    additions = {'quickEnabled': 0, 'quickStyle': 0, 'quickTime': .5, 'quickMotion': 0, 'quickSpace': 0, 'quickSync': 1}
    for old, new in zip(before['presets'], bank['presets'], strict=True):
        stripped = dict(new)
        stripped['values'] = dict(new['values'])
        for key, expected in additions.items():
            assert stripped['values'].pop(key) == expected, (new['id'], key)
        assert stripped == old, new['id']
    assert len(bank['presets']) == 200
    baseline_dsp = sorted(load(ROOT / 'out/default_patch/before_dsp.json'), key=lambda item: item['name'])
    dsp = [dict(name=p.name, sha256=digest(p)) for p in sorted((ROOT / 'Source/DSP').iterdir()) if p.is_file()]
    assert dsp == baseline_dsp, 'DSP changed: dependent calibration would need to be redone'
    source_id = re.search(r'HAMLO_ECHO_SOURCE_ID "([a-f0-9]+)"', (ROOT / 'build/SourceIdentity.h').read_text())[1]
    assert bank['dsp_source'] == source_id, 'Stale compiled bank dump'
    identity_files = sorted(p for folder in ['DSP', 'Standalone'] for p in (ROOT / 'Source' / folder).rglob('*') if p.is_file())
    identity = hashlib.sha256((version + ''.join(digest(p) for p in identity_files)).encode()).hexdigest()
    assert identity == source_id, 'Sources changed after build'
    for name in ['PresetScreen.cpp', 'PresetScreen.h']:
        original = subprocess.check_output(['git', 'show', f'257a863:Source/UI/{name}'], cwd=ROOT)
        # Git stores LF; normalize checkout line endings before comparing.
        assert original.replace(b'\r\n', b'\n') == (ROOT / 'Source/UI' / name).read_bytes().replace(b'\r\n', b'\n')
    proof = dict(version=version, source_id=source_id, tests_passed=int(tests[1]), legacy_cases=160,
                 dsp_files_identical=True, dsp_files=dsp, old_200_recipe_values_and_metadata_identical=True,
                 new_factory_parameters=additions, programs=208, shared_preset_screen_unchanged=True,
                 scope='Quick-disabled old recipes retain their calibration by explicit equivalence. New Quick moves are tested separately; this is not a new calibration claim.')
    write(REPORT / 'equivalence.json', proof)

    bundle = ROOT / 'build/HamloEcho_artefacts/Release/VST3/HamloEcho.vst3'
    dll = bundle / 'Contents/x86_64-win/HamloEcho.vst3'
    validated = (WORK / 'validated_dll.sha256').read_text(encoding='utf-8-sig').strip().lower()
    assert digest(dll) == validated, 'VST3 changed after validation'
    inventory = [dict(path=p.relative_to(bundle).as_posix(), sha256=digest(p)) for p in sorted(bundle.rglob('*')) if p.is_file()]
    metadata = lambda p: json.loads(re.sub(r',(?=\s*[}\]])', '', p.read_text(encoding='utf-8-sig')))
    assert sorted(c['CID'] for c in metadata(bundle / 'Contents/Resources/moduleinfo.json')['Classes']) == sorted(c['CID'] for c in metadata(ROOT / 'out/m0/HamloEcho.vst3/Contents/Resources/moduleinfo.json')['Classes'])
    stage = WORK / ('portable-' + validated[:12]) / 'HamloEcho.vst3'
    if stage.exists():
        for item in inventory:
            assert digest(stage / item['path']) == item['sha256'], 'Staging differs from the validated binary'
    else:
        shutil.copytree(bundle, stage)
    doc = stage / 'Documentation'
    doc.mkdir(exist_ok=True)
    for name in ['README.txt', 'USER-GUIDE.txt', 'THIRD-PARTY-NOTICES.txt']:
        shutil.copy2(ROOT / 'docs/release' / name, doc / name)
    licence = doc / 'ThirdParty/JUCE'
    licence.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'build/JUCE-LICENSE.md', licence / 'LICENSE.md')
    with (ROOT / 'build/quick_022_installer.log').open('w', encoding='utf-8') as log:
        # Use PowerShell 7, matching the task environment. Windows PowerShell
        # launched through Python inherits an incompatible PSModulePath here.
        subprocess.run(['pwsh', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(ROOT / 'Installer/Build-Installer.ps1'), '-SkipBuild', '-CompilerPath', 'C:/DRUMENGINE/build/tools/InnoSetup/ISCC.exe'], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    for item in inventory:
        assert digest(bundle / item['path']) == item['sha256'], 'Installer input changed'
    archive = ROOT / f'dist/HamloEcho-{version}-win64-vst3.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for p in sorted(stage.rglob('*')):
            if p.is_file():
                z.write(p, 'HamloEcho.vst3/' + p.relative_to(stage).as_posix())
    with zipfile.ZipFile(archive) as z:
        for p in stage.rglob('*'):
            if p.is_file():
                assert hashlib.sha256(z.read('HamloEcho.vst3/' + p.relative_to(stage).as_posix())).hexdigest() == digest(p)
    loose = ROOT / 'dist/HamloEcho.vst3'
    shutil.copytree(stage, loose, dirs_exist_ok=True)
    for p in stage.rglob('*'):
        if p.is_file():
            assert digest(loose / p.relative_to(stage)) == digest(p)
    assert {p.relative_to(loose) for p in loose.rglob('*') if p.is_file()} == {p.relative_to(stage) for p in stage.rglob('*') if p.is_file()}
    artifacts = []
    for p in [ROOT / f'dist/HamloEcho_VST3_Setup_{version}.exe', archive]:
        sha = digest(p)
        Path(str(p) + '.sha256').write_text(f'{sha}  {p.name}\n', encoding='ascii')
        artifacts.append(dict(name=p.name, sha256=sha, bytes=p.stat().st_size))
    manifest = dict(version=version, source_id=source_id, dsp_source=source_id, plugin_sha256=validated,
                    core_payload=inventory, zip_payload_verified=True, installer_input_binary_unchanged=True,
                    installer_executed=False, artifacts=artifacts, owner_fl_audition='pending', portable_bundle=str(stage),
                    loose_bundle=str(loose), loose_payload_verified=True)
    for p in [REPORT / 'package_manifest.json', ROOT / f'dist/release-manifest-{version}.json', ROOT / 'dist/release-manifest.json']:
        write(p, manifest)
    print(json.dumps(dict(tests=proof['tests_passed'], legacy=160, bank=200, source_id=source_id, plugin_sha256=validated, artifacts=artifacts), indent=2))


if __name__ == '__main__':
    main()
