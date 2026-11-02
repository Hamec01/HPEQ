"""Audit S2 artifacts, export measured figures and convenient listening copies."""
import hashlib
import json
import re
import shutil
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'docs/reports/s2'
WORK = ROOT / 'out/s2'


def load(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    final = load(REPORT / 'final_v4.json')
    assert final['passed'] == 181 and final['failed'] == final['unavailable'] == 0
    routing = load(REPORT / 'routing_failed_v4.json')
    assert routing['passed'] == 169 and routing['failed'] == 12
    assert sha(WORK / 'routing_before.cpp') == routing['files']['Source/DSP/SpatialPrototype.cpp']
    for name, value in final['files'].items():
        if name != 'Source/DSP/SpatialPrototype.cpp':
            assert routing['files'][name] == value, 'Routing regression method changed after failure'
    guard_tests = load(REPORT / 'guard_tests.json')
    assert guard_tests['source_id'] == final['source_id'] and len(guard_tests['checks']) == 4
    assert all(row['passed'] for row in guard_tests['checks'])
    assert load(REPORT / 'candidate_01.json')['failed'] == 1
    assert load(REPORT / 'candidate_02.json')['failed'] == 0
    assert sha(WORK / 'candidate_01_source.cpp') == load(REPORT / 'candidate_01.json')['files']['Source/DSP/SpatialPrototype.cpp']
    before = load(REPORT / 'before_v4.json')
    assert before['passed'] == 129 and before['failed'] == 0 and before['unavailable'] == 9
    for name, value in final['files'].items():
        assert sha(ROOT / name) == value, 'Stale final prototype/method'
        if name != 'Source/DSP/SpatialPrototype.cpp':
            assert before['files'][name] == value, 'Final before/after method differs'
    # Logging/provenance changes did not change the baseline stimulus or PCM.
    for p in (WORK / 'before').glob('*.f32'):
        assert sha(p) == sha(WORK / 'before_v4' / p.name), 'Repeated dry baseline differs'
    for p in (WORK / 'before_v4').glob('*.input.f32'):
        assert sha(p) == sha(WORK / 'final_v4' / p.name), 'Paired input differs'
    manifest = load(WORK / 'final_v4/manifest.json')
    assert manifest['source_id'] == manifest['dsp_identity'] == final['source_id'] and manifest['implemented']
    assert load(REPORT / 'legacy_160.json')['failures'] == 0
    assert '115 expectations passed, 0 failed' in (ROOT / 'build/s2_legacy_tests.log').read_text(encoding='utf-8-sig')
    s1 = load(ROOT / 'docs/reports/s1/acceptance.json')
    for item in s1['production_files']:
        assert sha(ROOT / item['path']) == item['sha256'], 'Legacy production source changed'
    dll = ROOT / 'build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3/Contents/x86_64-win/HamloShirota.vst3'
    assert sha(dll) == s1['validated_dll_sha256'], 'Production VST3 differs from S1 validators'
    production = (ROOT / 'CMakeLists.txt').read_text().split('add_library(HamloShirota_Shared STATIC', 1)[1].split('target_include_directories', 1)[0]
    assert 'SpatialPrototype' not in production and len(re.findall(r'Source/[\w/]+\.cpp', production)) == 10
    demos = load(REPORT / 'demos.json')
    assert demos['source_id'] == final['source_id']
    listen = WORK / 'listen'
    listen.mkdir(exist_ok=True)
    aliases = []
    for group in demos['groups']:
        assert group['identity']['source_id'] == final['source_id'] and len(group['recipes']) == 10
        folder = Path(group['recipes'][0]['path']).parent
        for name, value in group['files'].items():
            assert sha(folder / name) == value
        for row in group['recipes']:
            assert row['prepared_peak_dbtp'] <= -1
            target = listen / (group['material'] + '_' + row['scene'] + '.wav')
            shutil.copy2(row['path'], target)
            assert sha(target) == sha(Path(row['path']))
            aliases.append({**row, 'source_path': row['path'], 'path': str(target), 'sha256': sha(target)})
    (listen / 'manifest.json').write_text(json.dumps(dict(source_id=final['source_id'], files=aliases), indent=2) + '\n')

    checks = {c['name']: c for c in final['checks']}
    gains = [0, checks['width_2']['gain_db'], checks['width_4']['gain_db']]
    guard_gain = checks['width_4']['gain_db'] + checks['guard_0_meter']['actual_db']
    plt.rcParams.update({'font.size': 11, 'axes.spines.top': False, 'axes.spines.right': False})
    fig, ax = plt.subplots(figsize=(8, 4.4))
    bars = ax.bar(['1x / off', '2x / off', '4x / off', '4x / guard on'], gains + [guard_gain], color=['#557a67'] * 3 + ['#b68a3a'])
    for b, value in zip(bars, gains + [guard_gain]):
        ax.text(b.get_x() + b.get_width() / 2, value + .25, f'{value:.2f}', ha='center')
    ax.set(title='Measured stereo width response', ylabel='Side/Mid increase relative to source (dB)', xlabel='Width / energy-budget guard', ylim=(-.2, 14))
    ax.grid(axis='y', alpha=.2)
    fig.text(.13, .01, '48 kHz mixed stereo fixture; no raw-output gain compensation.', fontsize=9)
    fig.tight_layout(rect=(0, .03, 1, 1));fig.savefig(REPORT / 'width_response.png', dpi=180);plt.close(fig)

    fig, ax = plt.subplots(figsize=(8, 4.4))
    for c, label, color in [(0, 'Tight', '#4d9a77'), (1, 'Open', '#5579b1'), (2, 'Scatter', '#af784b')]:
        row = next(r for r in manifest['cases'] if r['id'] == 'space_impulse_' + str(c))
        y = np.fromfile(WORK / 'final_v4' / ('space_impulse_' + str(c) + '.f32'), dtype='<f4').reshape(-1, 2).astype(float)
        direct = int(.25 * 48000) + row['latency'];y = y[direct + 1:]
        e = np.convolve(np.sum(y * y, axis=1), np.ones(48) / 48, mode='same')
        level = 10 * np.log10(np.maximum(e / np.max(e), 1e-12))
        ax.plot((np.arange(len(e)) + 1) / 48, level, label=label, color=color, lw=1.1)
    ax.set(title='Early spatial fields: normalized wet impulse energy', xlabel='Time after aligned direct impulse (ms)', ylabel='1 ms energy relative to wet maximum (dB)', xlim=(0, 250), ylim=(-100, 2))
    ax.grid(alpha=.2);ax.legend();fig.tight_layout();fig.savefig(REPORT / 'spatial_fields.png', dpi=180);plt.close(fig)

    proof = dict(stage='S2 offline prototype; production VST3 remains S1', source_id=final['source_id'], prototype_checks=181,
                 prototype_cases=final['cases'], same_final_before_after_method=True, initial_baseline_pcm_identical=True,
                 provenance_fault_tests=4, prototype_executable_sha256=sha(ROOT / 'build/Release/HamloShirota_SpatialLab.exe'),
                 demonstration_executable_sha256=sha(ROOT / 'build/Release/HamloShirota_SpatialDemo.exe'),
                 failed_candidate_retained=True, routing_regressions=12, mixed_incremental_build_rejected=True,
                 native_legacy_expectations=115, m0_bit_exact_cases=160, production_dll_unchanged=True,
                 production_dll_sha256=sha(dll), production_units=10, prototype_linked_into_vst=False,
                 demos=40, worst_demo_peak_dbtp=max(row['prepared_peak_dbtp'] for g in demos['groups'] for row in g['recipes']),
                 listening_folder=str(listen), owner_listening='pending; no HRTF/binaural or trajectory yet',
                 cpu=[c for c in final['checks'] if c['goal'] == 'P12'],
                 artifacts={p.name: sha(p) for p in REPORT.iterdir() if p.is_file() and p.name != 'acceptance.json'})
    (REPORT / 'acceptance.json').write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8')
    print('S2 acceptance PASS:181prototype checks,115legacy expectations,160M0renders,40source-pinned demonstrations; production unchanged.')


if __name__ == '__main__':
    main()
