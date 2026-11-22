"""Validate/aggregate the 250 private RackBank records; embed only safe results.

python finish_bank.py output_directory material_directory [--embed]
Requires no third-party modules. Original audio is never copied into source.
"""
from pathlib import Path
import collections
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[2]
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def finish(folder, material, embed=False):
    bank_file=ROOT/'Assets/Presets/HamloRack/Design/recipes.json'
    recipes=json.loads(bank_file.read_text(encoding='utf-8'))['presets']
    bank_hash=digest(bank_file)
    manifest_hash=digest(material/'sources.json')
    definitions={p['id']:p for p in recipes}
    files=sorted((folder/'records').glob('HR*.json'))
    if len(files)!=250:
        raise ValueError(f'Need exactly 250 records, found {len(files)}')
    rows=[json.loads(f.read_text(encoding='utf-8-sig')) for f in files]
    if len({r['id'] for r in rows})!=250 or set(r['id'] for r in rows)!=set(definitions):
        raise ValueError('Missing/duplicate recipe identity')
    base_executables={r['executable_sha256'] for r in rows if 'repair' not in r}
    if len(base_executables)!=1:
        raise ValueError('Mixed baseline laboratory executables')
    base_executable=next(iter(base_executables))
    for row in rows:
        if 'repair' not in row:
            continue
        repair=row['repair'];original_file=folder/'originals'/f"{row['id']}.json"
        if repair['kind']!='master_ceiling_margin_only' or digest(original_file)!=repair['original_receipt_sha256']:
            raise ValueError('Unverified repair / original receipt')
        old=json.loads(original_file.read_text(encoding='utf-8-sig'))
        if old['executable_sha256']!=base_executable or repair['original_executable_sha256']!=base_executable:
            raise ValueError('Repair has wrong baseline executable')
        for key in ('definition_sha256','bank_sha256','material_manifest_sha256','input_pcm_sha256'):
            if row[key]!=old[key]:raise ValueError('Repair changed source/definition')
        for key in ('input_trim_db','output_trim_db'):
            if row['calibration'][key]!=old['calibration'][key]:raise ValueError('Ceiling repair changed gain calibration')
        def overrides(r):
            return {(o['layer'],o['node'],o['slot'],key):value for o in r['calibration']['overrides'] for key,value in o['parameters'].items()}
        before,after=overrides(old),overrides(row);ceiling=(9,8,0,'ceilingDb')
        changed={key for key in before.keys()|after.keys() if before.get(key)!=after.get(key)}
        if changed!={ceiling} or after[ceiling]!=repair['ceiling_after_db'] or abs(repair['ceiling_before_db']-repair['ceiling_after_db']-.05)>1e-9:
            raise ValueError('Repair changed more than the final ceiling')
    executable_hashes=sorted({r['executable_sha256'] for r in rows})
    for row in rows:
        if row['bank_sha256']!=bank_hash or row['material_manifest_sha256']!=manifest_hash:
            raise ValueError(f"Stale record {row['id']}")
        if row['name']!=definitions[row['id']]['name'] or len(row['checks'])!=8:
            raise ValueError('Wrong name/check coverage')
    source_notes={key:dict(description=value['description'],fixture_sha256=value['sha256'],
        source_sha256=[s['sha256'] for s in value['sources']])
        for key,value in json.loads((material/'sources.json').read_text(encoding='utf-8')).items()}
    checks=[c for r in rows for c in r['checks']]
    canonical={r['id']:next(c for c in r['checks'] if c.get('variant')==0 and c.get('level_offset_db')==0) for r in rows}
    safety=[r['id'] for r in rows if not r['safety_pass']]
    loudness=[r['id'] for r in rows if not canonical[r['id']]['loudness_match_pass']]
    dynamics=[dict(id=r['id'],**d) for r in rows for d in r['dynamics'] if not d['pass']]
    summary=dict(recipes=250,renders=len(checks),safety_pass=250-len(safety),safety_failures=safety,
        canonical_loudness_pass=250-len(loudness),canonical_loudness_exceptions=loudness,
        compressor_canonical_targets=sum(len(r['dynamics']) for r in rows),compressor_tuning_failures=dynamics,
        owner_intent_verdicts='pending; each recipe has its own intent and rejection check',
        owner_listening='pending; local dry/processed/loudness-matched WAVs exist for all recipes',
        host_validators='deferred to the owner VST build and host check')
    result=dict(format='HamloRackBankReceipt',method='RACK_BANK_METHOD.md / material-v2',sample_rate=48000,
        channels=2,prepare_block=256,host_block=127,bank_sha256=bank_hash,
        material_manifest_sha256=manifest_hash,executable_sha256=executable_hashes,
        native_sources=json.loads((ROOT/'Assets/Presets/HamloRack/Design/native-contract.json').read_text(encoding='utf-8'))['sources'],
        material=source_notes,summary=summary,presets=rows)
    (folder/'aggregate.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    lines=['# HamloRack bank numerical receipt','',
        'Engineering calibration on local recordings and sample arrangements. Musical intent and owner FL listening remain individual checks.',
        '',f"250 recipes; {len(checks)} verification renders; {250-len(safety)}/250 numerical safety passes.",
        f"Canonical loudness: {250-len(loudness)}/250 within +/-0.5 LU. Exceptions below remain exceptions.",
        f"Compressor tuning targets: {summary['compressor_canonical_targets']}; failed targets: {len(dynamics)}.",
        '', 'The compressor tuning result is measured before final master input trim; each final render also reports actual maximum Comp/RedWall GR. Source-separated stems and sample arrangements do not certify genre suitability.',
        '', '| ID | Preset | LU delta | Peak 8x dBTP | Comp GR | Limiter GR | Numerical safety | Loudness / owner check |',
        '|---|---|---:|---:|---:|---:|---|---|']
    for r in rows:
        c=canonical[r['id']];p=definitions[r['id']]
        reason='matched; owner audition pending'
        if not c['loudness_match_pass']:
            if p['output_mode']=='wet_only':reason='wet-only return; owner audition pending'
            elif p['category']=='Master':reason='master GR/peak priority; loudness exception'
            elif any(x.get('variant') is not None and x['output']['true_peak_8x_dbtp']>=x['input']['sample_peak_dbfs']+2.95 for x in r['checks']):reason='peak-growth constraint; loudness exception'
            else:reason='level/intent-dependent; loudness exception requires owner assessment'
        lines.append(f"| {r['id']} | {r['name']} | {c['loudness_delta_lu']:.3f} | {c['output']['true_peak_8x_dbtp']:.3f} | {c['compressor_peak_gr_db']:.3f} | {c['limiter_peak_gr_db']:.3f} | {'pass' if r['safety_pass'] else 'FAIL'} | {reason} |")
    lines+=['','All per-clip/per-level/tempo measurements, source and executable hashes, overrides and pending individual intent verdicts are in `RACK_BANK_RECEIPT.json`. Audio stays under the private output folder.','']
    (folder/'report.md').write_text('\n'.join(lines),encoding='utf-8')
    print(json.dumps(summary,indent=2))
    if embed:
        if safety or dynamics:
            raise ValueError('Resolve numerical safety / compressor tuning failures before embedding')
        if any('audition' not in r for r in rows):
            raise ValueError('Prepare safe matched audition pairs before embedding')
        calibration=dict(format='HamloRackCalibration',version=2,status='numerically_calibrated; owner audition pending',
            bank_sha256=bank_hash,material_manifest_sha256=manifest_hash,
            laboratory_executable_sha256=executable_hashes,entries={r['id']:r['calibration'] for r in rows})
        destination=ROOT/'Assets/Presets/HamloRack/calibration.json'
        destination.write_bytes((json.dumps(calibration,indent=2)+'\n').encode('utf-8'))
        docs=ROOT/'docs/hamlorack'
        (docs/'RACK_BANK_RECEIPT.json').write_bytes((json.dumps(result,indent=2)+'\n').encode('utf-8'))
        (docs/'RACK_BANK_REPORT.md').write_bytes('\n'.join(lines).encode('utf-8'))

if __name__=='__main__':
    finish(Path(sys.argv[1]),Path(sys.argv[2]),'--embed' in sys.argv[3:])
