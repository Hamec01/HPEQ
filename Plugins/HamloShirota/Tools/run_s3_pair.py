"""Fresh immutable S3 pair, using the tracked dry fixture and restoring sources.

Do not run concurrently with another build. Reports/audits target the accepted
checkpoint explicitly; a new label never overwrites/relabels existing results.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('label', help='New suffix, e.g. next01')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_-]+', args.label):
        parser.error('Use letters, numbers, underscores or hyphens')
    folders = [ROOT / 'out/s3' / f'{role}_{args.label}' for role in ['before', 'final']]
    reports = [ROOT / 'docs/reports/s3' / f'{role}_{args.label}.json' for role in ['before', 'final']]
    archive = ROOT / 'out/s3' / ('sources_' + args.label)
    for p in folders + reports + [archive]:
        if p.exists():
            raise RuntimeError('Refusing to overwrite ' + str(p))
    archive.mkdir(parents=True)
    dsp = ROOT / 'Source/DSP/MotionPrototype.cpp';actual = dsp.read_bytes()
    (archive / 'candidate.cpp').write_bytes(actual)
    for name in ['Assets/Hrtf/horizontal.bin', 'Assets/Hrtf/manifest.json', 'Source/DSP/MotionPrototype.h',
                 'Tests/MotionLabMain.cpp', 'Tools/analyze_s3.py', 'docs/S3_METHODS.md', 'Tests/MotionBaseline.cpp']:
        target = archive / name;target.parent.mkdir(parents=True, exist_ok=True);target.write_bytes((ROOT / name).read_bytes())

    def command(argv, log):
        with (ROOT / 'build' / log).open('w', encoding='utf-8') as out:
            subprocess.run(argv, cwd=ROOT, stdout=out, stderr=subprocess.STDOUT, check=True)

    def build(role):
        command(['cmake', '--build', 'build', '--config', 'Release', '--target', 'HamloShirota_MotionLab', '--', '/m:2', '/v:minimal'], f's3_{role}_{args.label}_build.log')

    def measure(i, role):
        command(['build/Release/HamloShirota_MotionLab.exe', 'Assets/Hrtf/horizontal.bin', str(folders[i])], f's3_{role}_{args.label}.log')
        command([sys.executable, '-I', '-X', 'utf8', 'Tools/analyze_s3.py', str(folders[i]), str(reports[i])] + (['--baseline'] if i == 0 else []), f's3_{role}_{args.label}_analysis.log')

    try:
        dsp.write_bytes((ROOT / 'Tests/MotionBaseline.cpp').read_bytes());build('before');measure(0, 'before')
    finally:
        dsp.write_bytes(actual);build('final')
    measure(1, 'final')
    print('Paired run complete:', *reports)


if __name__ == '__main__':
    main()
