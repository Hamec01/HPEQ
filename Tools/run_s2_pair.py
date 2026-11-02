"""Run a fresh paired S2 experiment; restore the candidate even on failure.

The aligned-dry fixture is not a production source. Do not run concurrently with
another build: the source identity intentionally binds each role to its own DSP.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('label', help='New immutable run suffix, e.g. v5')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_-]+', args.label):
        parser.error('Use only letters, numbers, underscores and hyphens')
    folders = [ROOT / 'out/s2' / f'{role}_{args.label}' for role in ('before', 'final')]
    reports = [ROOT / 'docs/reports/s2' / f'{role}_{args.label}.json' for role in ('before', 'final')]
    archive = ROOT / 'out/s2' / f'sources_{args.label}'
    for path in folders + reports + [archive]:
        if path.exists():
            raise RuntimeError(f'Refusing to overwrite {path}')
    archive.mkdir(parents=True)
    dsp = ROOT / 'Source/DSP/SpatialPrototype.cpp'
    candidate = dsp.read_bytes()
    baseline = (ROOT / 'Tests/SpatialBaseline.cpp').read_bytes()
    for name in ['Source/DSP/SpatialPrototype.h', 'Tests/SpatialLabMain.cpp',
                 'Tools/analyze_s2.py', 'docs/S2_METHODS.md']:
        target = archive / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / name).read_bytes())
    (archive / 'candidate.cpp').write_bytes(candidate)
    (archive / 'baseline.cpp').write_bytes(baseline)

    def command(argv, log):
        with (ROOT / 'build' / log).open('w', encoding='utf-8') as output:
            subprocess.run(argv, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)

    def build(role):
        command(['cmake', '--build', 'build', '--config', 'Release', '--target',
                 'HamloShirota_SpatialLab', '--', '/m:2', '/v:minimal'], f's2_{role}_{args.label}_build.log')

    def measure(index, role):
        command(['build/Release/HamloShirota_SpatialLab.exe', str(folders[index])], f's2_{role}_{args.label}.log')
        command([sys.executable, '-I', '-X', 'utf8', 'Tools/analyze_s2.py', str(folders[index]),
                 str(reports[index])] + (['--baseline'] if index == 0 else []), f's2_{role}_{args.label}_analysis.log')

    try:
        dsp.write_bytes(baseline)  # Fresh timestamp; never Copy-Item an older source.
        build('before')
        measure(0, 'before')
    finally:
        dsp.write_bytes(candidate)
        build('final')
    measure(1, 'final')
    print('Paired experiment complete:', *reports)


if __name__ == '__main__':
    main()
