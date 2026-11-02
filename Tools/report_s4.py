"""Audit the accepted production integration; never reuse another DSP's renders."""
import hashlib
import importlib.util
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('s4_analysis', ROOT / 'Tools/analyze_s4.py')
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)
identities, digest = analysis.identities, analysis.digest
REPORT = ROOT / 'docs/reports/s4'
RUN = ROOT / 'out/s4/stage_final'
BUNDLE = ROOT / 'build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3'


def load(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    source, method, files, methods = identities()
    final = load(REPORT / 'stage_final.json')
    before = load(REPORT / 'before_final_v2.json')
    require((final['passed'], final['failed'], final['unavailable'], final['cases']) == (106, 0, 0, 31), 'Integration checks')
    require((before['passed'], before['failed'], before['unavailable'], before['cases']) == (94, 0, 3, 31), 'Baseline checks')
    require(final['source_id'] == source and final['source_files'] == files, 'Production source changed')
    require(final['method_id'] == before['method_id'] == method and final['methods'] == before['methods'] == methods, 'Paired method changed')
    for folder, report in [(RUN, final), (ROOT / 'out/s4/before_final_v2', before)]:
        pin = load(folder / 'identity.json')
        require(digest(folder / 'identity.json') == report['identity_sha256'], 'Cached identity changed')
        require(digest(folder / 'manifest.json') == pin['manifest'], 'Manifest changed')
        for name, value in pin['renders'].items():
            require(digest(folder / name) == value, 'Raw render changed: ' + name)
    manifest = load(RUN / 'manifest.json')
    require(manifest['parameter_count'] == 159 and len(manifest['cases']) == 31, 'Parameter/case count')
    previous = load(ROOT / 'out/s4/accepted_final/identity.json')
    require(previous['renders'] == load(RUN / 'identity.json')['renders'], 'Formatting changed accepted audio')

    legacy = load(REPORT / 'stage_final_legacy.json')
    require(legacy['failures'] == 0 and len(legacy['cases']) == 160, 'Legacy count')
    require(all(all(row[k] for k in ['bit_exact', 'state_exact', 'latency_exact', 'finite']) for row in legacy['cases']), 'Legacy identity')
    native = REPORT / 'native_stage_final.txt'
    require('451 expectations passed, 0 failed' in native.read_text(encoding='utf-8-sig'), 'Native checks')

    frozen = load(ROOT / 'out/s4/frozen/source.json')
    require(frozen['commit'] == 'd1ef423', 'Baseline revision')
    for name, value in frozen['files'].items():
        require(digest(ROOT / 'out/s4/frozen' / name) == value, 'Frozen facade changed')
        original = subprocess.check_output(['git', 'show', 'd1ef423:' + name], cwd=ROOT)
        require(hashlib.sha256(original).hexdigest() == value, 'Frozen facade differs from git')
    s1 = load(ROOT / 'docs/reports/s1/acceptance.json')
    for row in s1['production_files']:
        if not row['path'].startswith('Source/Standalone/'):
            require(digest(ROOT / row['path']) == row['sha256'], 'Original common source changed: ' + row['path'])
    for stage, filename in [('s2', 'final_v4.json'), ('s3', 'final.json')]:
        folder = ROOT / 'docs/reports' / stage
        for name, value in load(folder / filename)['files'].items():
            require(digest(ROOT / name) == value, 'Accepted prototype changed: ' + name)
        for name, value in load(folder / 'acceptance.json')['artifacts'].items():
            require(digest(folder / name) == value, 'Accepted evidence changed: ' + name)

    bank = load(ROOT / 'Assets/Hrtf/production.json')
    for name, key in [('Assets/Hrtf/production.bin', 'production_bank_sha256'), ('Assets/Hrtf/horizontal.bin', 'source_bank_sha256'), ('Tools/prepare_s4_hrtf.py', 'tool_sha256')]:
        require(digest(ROOT / name) == bank[key], 'Prepared bank provenance: ' + name)
    for name in ['LICENSE-SADIE-II.txt', 'SADIE-II-NOTICE.md', 'PRODUCTION-NOTICE.md']:
        original_name = 'NOTICE.md' if name == 'SADIE-II-NOTICE.md' else name
        require(digest(BUNDLE / 'Contents/Resources' / name) == digest(ROOT / 'Assets/Hrtf' / original_name), 'Bundled notice changed')
    dll = BUNDLE / 'Contents/x86_64-win/HamloShirota.vst3'
    validated = load(REPORT / 'validators_stage_final/identity.json')
    require(digest(dll) == validated['dll_sha256'], 'Validated binary changed')
    require(validated['results'] == [f'pluginval {level} exit=0' for level in range(5, 11)] + ['Steinberg exit=0'], 'Validator failures')
    require('Result: 537 tests passed, 0 tests failed' in (REPORT / 'validators_stage_final/steinberg.txt').read_text(), 'Steinberg test count')
    info = (BUNDLE / 'Contents/Resources/moduleinfo.json').read_text()
    require(sorted(re.findall(r'"CID": "([A-F0-9]+)"', info)) == sorted(s1['class_ids']), 'Class IDs changed')

    guards = load(REPORT / 'guard_tests.json')
    require(guards['source_id'] == source and guards['method_id'] == method and len(guards['checks']) == 5 and all(c['passed'] for c in guards['checks']), 'Identity rejection checks')
    timing = load(REPORT / 'timing_stage_final.json')
    require(len(timing) == 3 and all(t['mean_fraction'] <= .25 for t in timing), 'Mean CPU target')
    screenshots = [REPORT / f'stage_final_page_{i}.png' for i in range(5)] + [REPORT / 'stage_final_binaural.png']
    require(all(p.stat().st_size > 10000 for p in screenshots), 'Missing UI evidence')

    artifacts = {str(p.relative_to(REPORT)).replace('\\', '/'): digest(p) for p in sorted(REPORT.rglob('*')) if p.is_file() and p.name not in ['acceptance.json', 'REPORT.md']}
    binaries = {p.name: digest(p) for p in [dll, ROOT / 'build/Release/HamloShirota_Tests.exe', ROOT / 'build/Release/HamloShirota_IntegrationLab.exe', ROOT / 'build/Release/HamloShirota_LegacyCompat.exe', ROOT / 'build/Release/HamloShirota_Snapshot.exe', ROOT / 'build/Release/HamloShirota_IntegrationTiming.exe']}
    result = dict(stage='S4 production integration', parent='d1ef423', product_version='0.1.0 development', source_id=source, method_id=method, source_files=files, methods=methods,
                  native_expectations=451, M0_bit_exact_cases=160, paired_cases=31, before_checks=94, before_unavailable=3, production_checks=106,
                  provenance_rejections=5, original_parameters=12, appended_parameters=147, total_parameters=159, host_programs=7,
                  same_final_method=True, formatting_preserves_audio=True, unchanged_legacy_common_sources=True, unchanged_S2_S3=True,
                  class_ids=s1['class_ids'], validated_dll_sha256=digest(dll), bank=bank, cpu=timing, artifacts=artifacts, binaries=binaries,
                  owner_FL_K712_listening='pending', installer_built=False, new_genre_programs=0,
                  remaining=['200 genre programs and calibration', 'Full resampled-rate spectral validation', 'Live-host callback deadline assessment', 'Owner FL/K712 listening', 'Final 0.2.0 installer/ZIP/SHA256'])
    (REPORT / 'acceptance.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('S4 audit passed: 451 native, 160 M0, 106 integration, 5 rejection checks; validated DLL ' + digest(dll))


if __name__ == '__main__':
    main()
