"""Verify integrated renderer against all retained, independently analysed attack-stage outputs."""
import csv,hashlib,json,argparse
from pathlib import Path
root=Path(__file__).resolve().parents[1]
groups={'original':'verified_original','attacks':'verified_attacks','edges':'verified_edges','wrap':'verified_wrap'}
parser=argparse.ArgumentParser();parser.add_argument('directory',nargs='?',type=Path,default=root/'out/score/prototype_regression');parser.add_argument('report',nargs='?',type=Path,default=root/'docs/reports/integration/prototype_regression.json');args=parser.parse_args()
records=[];failed=[]
for group,baseline in groups.items():
    before=root/'out/attacks'/baseline;after=args.directory/group
    for file in sorted(before.glob('*.f32')):
        expected=hashlib.sha256(file.read_bytes()).hexdigest();target=after/file.name
        actual=hashlib.sha256(target.read_bytes()).hexdigest() if target.exists() else 'missing'
        row=dict(group=group,case=file.stem,sha256=actual,bit_exact=expected==actual);records.append(row)
        if not row['bit_exact']:failed.append(row)
    for tsv in after.glob('*.tsv'):
        with tsv.open() as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                if 'allocations' in row and int(row['allocations']):failed.append(dict(group=group,allocation=row))
source_hashes={name:hashlib.sha256((root/'Source/DSP'/name).read_bytes()).hexdigest() for name in ['ReflectionPrototype.cpp','ReflectionPrototype.h']}
report=dict(renderer_source_sha256=source_hashes,records=len(records),passed=sum(r['bit_exact'] for r in records),failures=failed,results=records)
dest=args.report;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(json.dumps(report,indent=2)+'\n')
print(f"{report['passed']}/{report['records']} archived render hashes identical; {len(failed)} failures")
raise SystemExit(bool(failed))
