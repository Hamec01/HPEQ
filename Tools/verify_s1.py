"""Audit the isolated M0 checkpoint without rewriting references or results."""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'docs/reports/s1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    before, after = load(REPORT / 'before.json'), load(REPORT / 'after.json')
    assert before['failures'] == after['failures'] == 0
    assert len(before['cases']) == len(after['cases']) == 160
    assert load(REPORT / 'selfcheck.json')['failures'] == 0
    for a, b in zip(before['cases'], after['cases'], strict=True):
        assert a['case'] == b['case'] and a['render_sha256'] == b['render_sha256'] and a['state_sha256'] == b['state_sha256']
        assert b['bit_exact'] and b['state_exact'] and b['latency_exact'] and b['finite']
        assert a['latency_events'] == b['latency_events']
        for suffix, expected in [('.f32', a['render_sha256']), ('.state', a['state_sha256'])]:
            assert sha(ROOT / 'out/s1/reference' / (a['case'] + suffix)) == expected
    tests = (ROOT / 'build/s1_final_tests.log').read_text(encoding='utf-8-sig')
    assert '115 expectations passed, 0 failed' in tests
    assert '59 expectations passed, 0 failed' in (ROOT / 'build/s1_original_tests.log').read_text(encoding='utf-8-sig')
    validation = (REPORT / 'validators/summary.txt').read_text(encoding='utf-8-sig')
    assert all(f'pluginval {i} exit=0' in validation for i in range(5, 11)) and 'Steinberg exit=0' in validation
    assert '537 tests passed, 0 tests failed' in (REPORT / 'validators/steinberg.txt').read_text(encoding='utf-8-sig')

    cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    block = cmake.split('add_library(HamloShirota_Shared STATIC', 1)[1].split('target_include_directories', 1)[0]
    sources = re.findall(r'Source/[\w/]+\.cpp', block)
    assert len(sources) == 10
    assert not any(any(word in s for word in ['LayerEngine', 'FxNodeProcessor', 'HamloVerb', 'HamloEcho', 'HamloMag', 'HamloComp', 'RedWall', 'Udarnik', 'Zastava']) for s in sources)
    closure = set(sources)
    pending = list(sources)
    while pending:
        name = pending.pop()
        for inc in re.findall(r'#include\s+"([^"]+)"', (ROOT / name).read_text(encoding='utf-8-sig')):
            included = 'Source/' + inc
            if (ROOT / included).is_file() and included not in closure:
                closure.add(included)
                pending.append(included)
    assert not any(Path(n).name in ['LayerEngine.h', 'FxNodeProcessor.h', 'LayerSnapshot.h', 'FxProjectState.h', 'Processors.h', 'ProcessorsM6.h'] for n in closure)
    original = ROOT / 'out/s1/original_sources'
    assert sha(ROOT / 'Source/DSP/HamloShirota.cpp') == sha(original / 'Source/DSP/HamloShirota.cpp')
    for name in ['Source/UI/PresetScreen.cpp', 'Source/UI/PresetScreen.h', 'Source/UI/HamloShirotaPanel.cpp', 'Source/UI/HamloShirotaPanel.h', 'Source/UI/BlackSteel.cpp']:
        import subprocess
        old = subprocess.check_output(['git', 'show', '93d965f:' + name], cwd=ROOT)
        assert old.replace(b'\r\n', b'\n') == (ROOT / name).read_bytes().replace(b'\r\n', b'\n')

    bundle = ROOT / 'build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3'
    old_bundle = ROOT / 'out/s1/binaries/HamloShirota.vst3'
    dll = Path('Contents/x86_64-win/HamloShirota.vst3')
    assert sha(bundle / dll) == (ROOT / 'out/s1/validated_dll.sha256').read_text().strip()
    metadata = lambda p: json.loads(re.sub(r',(?=\s*[}\]])', '', p.read_text(encoding='utf-8-sig')))
    ids = lambda base: sorted(c['CID'] for c in metadata(base / 'Contents/Resources/moduleinfo.json')['Classes'])
    assert ids(bundle) == ids(old_bundle)
    legacy_blocks = {}
    for event in range(4):
        hashes = [c['render_sha256'] for c in before['cases'] if c['case'].startswith(f'events_{event}_')]
        legacy_blocks[f'event_family_{event}'] = len(set(hashes))
    proof = dict(baseline_commit='93d965f', stage='S1 isolated M0, not spatial V2', cases=160,
                 native_expectations=115, original_expectations=59, cpp_audio_allocations=0,
                 contract=load(ROOT / 'out/s1/reference/contract.json'),
                 contract_sha256=sha(ROOT / 'out/s1/reference/contract.json'),
                 lab_source_sha256=sha(ROOT / 'Tests/ShirotaLabMain.cpp'),
                 processor_object_bytes_before=before['processor_bytes'], processor_object_bytes_after=after['processor_bytes'],
                 legacy_distinct_renders_across_blocks=legacy_blocks,
                 production_translation_units=sources, production_include_closure=sorted(closure),
                 production_files=[dict(path=n, sha256=sha(ROOT / n)) for n in sorted(closure)],
                 class_ids=ids(bundle), original_dll_sha256=sha(old_bundle / dll),
                 validated_dll_sha256=sha(bundle / dll), dll_bytes_before=(old_bundle / dll).stat().st_size,
                 dll_bytes_after=(bundle / dll).stat().st_size,
                 installer_built=False, owner_fl_audition='not requested for unchanged-sound checkpoint',
                 validator_results=validation.splitlines())
    (REPORT / 'acceptance.json').write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: proof[k] for k in ['cases', 'native_expectations', 'processor_object_bytes_before', 'processor_object_bytes_after', 'validated_dll_sha256', 'legacy_distinct_renders_across_blocks']}, indent=2))


if __name__ == '__main__':
    main()
