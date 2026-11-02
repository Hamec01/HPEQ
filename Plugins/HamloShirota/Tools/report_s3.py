"""Audit final S3 evidence and export measurements without changing accepted DSP."""
import hashlib
import importlib.util
import json
import re
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'docs/reports/s3'
WORK = ROOT / 'out/s3'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def load(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def main():
    final, before = load(REPORT / 'final.json'), load(REPORT / 'before_final.json')
    assert (final['passed'], final['failed'], final['unavailable'], final['cases']) == (211, 0, 0, 67)
    assert (before['passed'], before['failed'], before['unavailable']) == (155, 0, 4)
    assert load(REPORT / 'before.json')['passed'] == 141
    assert load(REPORT / 'candidate_01.json')['passed'] == 190
    for name, value in final['files'].items():
        assert sha(ROOT / name) == value, 'Changed accepted sources/data'
        if name != 'Source/DSP/MotionPrototype.cpp':
            assert before['files'][name] == value, 'Paired methods/data differ'
    for file in (WORK / 'before').iterdir():
        if file.suffix in ('.f32', '.f64'):
            assert sha(file) == sha(WORK / 'before_final' / file.name), 'Original baseline changed'
    for file in (WORK / 'before_final').glob('*.input.f32'):
        assert sha(file) == sha(WORK / 'final' / file.name), 'Paired stimulus changed'
    # The DSP recipe did not change when coverage was extended.
    assert load(REPORT / 'candidate_01.json')['files']['Source/DSP/MotionPrototype.cpp'] == final['files']['Source/DSP/MotionPrototype.cpp']
    s2 = load(ROOT / 'docs/reports/s2/acceptance.json')
    for name, value in s2['artifacts'].items():
        assert sha(ROOT / 'docs/reports/s2' / name) == value, 'S2 evidence changed'
    for name, value in load(ROOT / 'docs/reports/s2/final_v4.json')['files'].items():
        assert sha(ROOT / name) == value, 'S2 source changed'
    s1 = load(ROOT / 'docs/reports/s1/acceptance.json')
    for item in s1['production_files']:
        assert sha(ROOT / item['path']) == item['sha256'], 'Legacy source changed'
    dll = ROOT / 'build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3/Contents/x86_64-win/HamloShirota.vst3'
    assert sha(dll) == s1['validated_dll_sha256']
    production = (ROOT / 'CMakeLists.txt').read_text().split('add_library(HamloShirota_Shared STATIC', 1)[1].split('target_include_directories', 1)[0]
    assert len(re.findall(r'Source/[\w/]+\.cpp', production)) == 10 and 'MotionPrototype' not in production
    assert '115 expectations passed, 0 failed' in (ROOT / 'build/s3_legacy_tests.log').read_text(encoding='utf-8-sig')
    assert load(REPORT / 'legacy_160.json')['failures'] == 0
    faults = load(REPORT / 'guard_tests.json')
    assert faults['source_id'] == final['source_id'] and len(faults['checks']) == 5 and all(c['passed'] for c in faults['checks'])
    exports = load(ROOT / 'Assets/Hrtf/manifest.json')
    assert sha(ROOT / 'Assets/Hrtf/horizontal.bin') == exports['bank_sha256']
    assert sha(ROOT / 'Assets/Hrtf/LICENSE-SADIE-II.txt') == exports['license_sha256']
    assert sha(ROOT / 'Tools/prepare_s3_hrtf.py') == exports['exporter_sha256']
    for row in exports['downloads']:
        assert sha(WORK / 'downloads' / row['file']) == row['sha256']
    demos = load(REPORT / 'demos.json');assert demos['source_id'] == final['source_id'] and len(demos['groups']) == 4
    for group in demos['groups']:
        assert group['identity']['source_id'] == final['source_id'] and len(group['recipes']) == 17
        folder = Path(group['recipes'][0]['path']).parent.parent
        for file, value in group['files'].items():
            assert sha(folder / file) == value
        for row in group['recipes']:
            assert row['prepared_peak_dbtp'] <= -1
    aliases = load(WORK / 'listen/manifest.json');assert aliases['source_id'] == final['source_id'] and len(aliases['files']) == 68
    for row in aliases['files']:
        assert sha(Path(row['path'])) == row['sha256'] == sha(Path(row['source_path']))

    spec = importlib.util.spec_from_file_location('s3_numeric', ROOT / 'Tools/analyze_s3.py')
    numeric = importlib.util.module_from_spec(spec);spec.loader.exec_module(numeric)
    filters = numeric.bank()[48000]
    plt.rcParams.update({'font.size': 10, 'axes.spines.top': False, 'axes.spines.right': False})
    trace = np.fromfile(WORK / 'final/path_free_48000.trace.f64', dtype='<f8').reshape(-1, 4)
    fig, ax = plt.subplots(figsize=(8.4, 4))
    plotted = trace[:, 0].copy()
    plotted[np.flatnonzero(np.abs(np.diff(plotted)) > 180) + 1] = np.nan
    ax.plot(np.arange(len(trace)) / 48000, plotted, color='#4a9672', lw=1.6)
    ax.set(title='Programmed holds and travel: measured path clock', xlabel='Time (s)', ylabel='Azimuth (degrees)', yticks=[0, 90, 180, 270, 360], xlim=(0, 3))
    ax.grid(alpha=.25)
    fig.text(.10, .02, '0 front / 90 left / 180 back / 270 right. 360 and 0 are the same direction.', fontsize=9)
    fig.tight_layout(rect=(0, .05, 1, 1));fig.savefig(REPORT / 'path_timing.png', dpi=180);plt.close(fig)
    fig, axes = plt.subplots(3, 1, figsize=(8.4, 7.5), sharex=True)
    hz = np.fft.rfftfreq(4096, 1 / 48000)
    for i, ax in enumerate(axes):
        for direction, name, color in [(0, 'Front', '#4a9672'), (36, 'Back', '#b47944')]:
            response = np.fft.rfft(filters[i, direction], 4096, axis=-1)
            level = 10 * np.log10(np.maximum(np.mean(np.abs(response) ** 2, axis=0), 1e-10))
            ax.semilogx(hz[1:], level[1:], label=name, color=color, lw=1)
        ax.set(title=f'Profile {i+1} / {exports["profiles"][i]}: mean ear magnitude', ylabel='dB', xlim=(100, 16000), ylim=(-35, 20));ax.grid(alpha=.2);ax.legend(loc='lower left')
    axes[-1].set_xlabel('Frequency (Hz)');fig.tight_layout();fig.savefig(REPORT / 'front_back_responses.png', dpi=180);plt.close(fig)
    checks = {c['name']: c for c in final['checks']}
    proof = dict(stage='S3 offline timed paths and horizontal binaural; production remains S1', source_id=final['source_id'],
                 assertions=211, render_cases=67, stress_seconds=300, zero_audio_and_setter_allocations=True,
                 unchanged_original_baseline=True, unchanged_S2=True, native_legacy_expectations=115, M0_bit_exact_cases=160,
                 production_dll_sha256=sha(dll), production_unchanged=True, linked_into_VST=False,
                 prototype_executable_sha256=sha(ROOT / 'build/Release/HamloShirota_MotionLab.exe'),
                 provenance_rejections=5, profiles=exports['profiles'], bank_sha256=exports['bank_sha256'],
                 native_filter_energy_retained=exports['minimum_retained_energy'], magnitude_error_db=exports['worst_magnitude_error_db'],
                 max_clock_error_degrees=max(c['max_angular_error_degrees'] for c in final['checks'] if c['goal']=='M02'),
                 enable_ramp_error_db=checks['enable_ramp_exact']['relative_error_db'],
                 cpu=[c for c in final['checks'] if c['goal']=='M09'],
                 demo_count=68, demo_worst_peak_dbtp=max(row['prepared_peak_dbtp'] for g in demos['groups'] for row in g['recipes']),
                 owner_listening='pending on AKG K712; front/back perception not certified',
                 artifacts={p.name: sha(p) for p in REPORT.iterdir() if p.is_file() and p.name!='acceptance.json'})
    (REPORT / 'acceptance.json').write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('S3 acceptance PASS:211assertions/67cases,5rejection tests,68auditions,115native/160M0. Production unchanged.')


if __name__ == '__main__':
    main()
