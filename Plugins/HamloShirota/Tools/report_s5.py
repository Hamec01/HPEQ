"""Final S5 evidence/delivery audit; does not weaken failed or pending checks."""
import importlib.util
import json
import re
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('s5_lab',ROOT/'Tools/s5_calibrate.py')
lab=importlib.util.module_from_spec(spec);spec.loader.exec_module(lab)


def main():
    report=lab.REPORT;identity=lab.identities()
    before=lab.read(report/'catalog_before.json');catalog=lab.read(report/'catalog_final.json')
    assert before['program_count']==7 and before['genre_count']==0
    assert catalog['program_count']==207 and catalog['genre_count']==200
    assert before['method_id']==catalog['method_id']==identity['method_id']
    assert catalog['sound_id']==identity['sound_id']
    assert before['programs']==catalog['programs'][:7]
    recipes=lab.author.recipes();assert len(recipes)==200 and len({r['id'] for r in recipes})==200
    expected={name:len(rows) for name,rows in lab.author.GROUPS}
    for genre in ['Boom Bap','Trap','Techno','DnB']:
        assert Counter(r['group'] for r in recipes if r['genre']==genre)==expected
    calibration=lab.read(report/'calibration.json');verified=lab.read(report/'verified.json')
    assert calibration['identity']==verified['identity']==identity
    assert not calibration['failures'] and len(calibration['presets'])==200
    prior=lab.read(report/'before_state_fix_calibration.json')
    for identifier,row in calibration['presets'].items():
        old=prior['presets'][identifier]
        assert next(v for p,v in row['files'].items() if p.endswith('.f32'))==next(v for p,v in old['files'].items() if p.endswith('.f32')),'State repair changed full-strength processing'
    assert verified['passed']==verified['unique_recipes']==len(verified['presets'])==200
    for identifier,result in verified['presets'].items():
        assert result['passed'] and result['roundtrip'] and result['peak_dbtp']<=-1
        for name,value in result['files'].items():assert lab.sha(Path(name))==value,'Changed verified output'
    for result in calibration['presets'].values():
        for name,value in result['files'].items():assert lab.sha(Path(name))==value,'Changed calibration output'
    filters=lab.read(report/'filters.json');assert filters['identity']==identity and filters['passed']==len(filters['cases'])==96
    for result in filters['cases']:assert lab.sha(Path(result['raw']))==result['sha256']
    demos=lab.read(report/'demos.json');assert demos['sound_id']==identity['sound_id'] and demos['count']>=40
    for group in demos['groups']:
        assert all(row['peak_dbtp']<=-1 for row in group['clips'])
        for name,value in group['files'].items():assert lab.sha(Path(name))==value

    native=(report/'native_final.txt').read_text(encoding='utf-8-sig')
    result=re.search(r'HamloShirota_Tests: (\d+) expectations passed, 0 failed',native);assert result and int(result[1])>=451
    legacy=lab.read(report/'legacy_final.json');assert len(legacy['cases'])==160 and legacy['failures']==0
    assert all(all(row[k] for k in ['bit_exact','state_exact','latency_exact']) for row in legacy['cases'])
    integration=lab.read(report/'integration_release.json');assert integration['passed']==106 and integration['failed']==0 and integration['cases']==31
    guards=lab.read(report/'guard_tests.json');assert guards['source_id']==integration['source_id'] and len(guards['checks'])==5 and all(r['passed'] for r in guards['checks'])
    pin=lab.read(ROOT/'out/s5/integration_release/identity.json');assert lab.sha(ROOT/'out/s5/integration_release/identity.json')==integration['identity_sha256']
    assert pin['renders']==lab.read(ROOT/'out/s4/stage_final/identity.json')['renders'],'Existing modern processing changed'
    for name,value in pin['renders'].items():assert lab.sha(ROOT/'out/s5/integration_release'/name)==value
    # S4 source guards pin catalog/editor too, so its old audit cannot be run on S5.
    # Verify unchanged accepted kernels, parameters, assets and shared UI directly.
    for name,value in lab.read(ROOT/'docs/reports/s4/acceptance.json')['source_files'].items():
        if name.startswith(('Source/DSP/','Source/Standalone/V2Parameters','Assets/Hrtf/')):
            assert lab.sha(ROOT/name,not name.endswith('.bin'))==value,'Accepted DSP/data changed'
    for name in ['Source/Core/ModulePresets.cpp','Source/Core/ModulePresets.h','Source/UI/PresetScreen.cpp','Source/UI/PresetScreen.h']:
        import subprocess
        original=subprocess.check_output(['git','show','5765c5e:'+name],cwd=ROOT)
        assert original.replace(b'\r\n',b'\n')==(ROOT/name).read_bytes().replace(b'\r\n',b'\n')

    validators=lab.read(report/'validators_release/identity.json')
    assert validators['results']==[f'pluginval {i} exit=0' for i in range(5,11)]+['Steinberg exit=0']
    assert 'Result: 537 tests passed, 0 tests failed' in (report/'validators_release/steinberg.txt').read_text()
    delivery=lab.read(report/'delivery.json');assert not delivery['installer_built']
    folder=Path(delivery['bundle']);assert delivery['dll_sha256']==validators['dll_sha256']
    for name,value in delivery['files'].items():assert lab.sha(folder/name)==value
    module=(folder/'Contents/Resources/moduleinfo.json').read_text();assert '"Version": "0.2.0"' in module
    assert sorted(re.findall(r'"CID": "([A-F0-9]+)"',module))==sorted(lab.read(ROOT/'docs/reports/s1/acceptance.json')['class_ids'])
    for name,original in [('LICENSE-SADIE-II.txt','LICENSE-SADIE-II.txt'),('SADIE-II-NOTICE.md','NOTICE.md'),('PRODUCTION-NOTICE.md','PRODUCTION-NOTICE.md')]:
        assert lab.sha(folder/'Contents/Resources'/name)==lab.sha(ROOT/'Assets/Hrtf'/original)
    timing=lab.read(report/'timing_final.json');assert len(timing)==3 and all(r['mean_fraction']<=.25 for r in timing)
    screenshots=list(report.glob('library_*.png'));assert len(screenshots)>=4 and all(p.stat().st_size>10000 for p in screenshots)
    artifacts={str(p.relative_to(report)).replace('\\','/'):lab.sha(p) for p in sorted(report.rglob('*')) if p.is_file() and p.name not in ['acceptance.json','REPORT.md']}
    summary=dict(stage='S5 VST-only library',parent='5765c5e',version='0.2.0',identity=identity,
                 host_programs=207,new_presets=200,verified_presets=200,unique_recipes=200,native_expectations=int(result[1]),
                 legacy_cases=160,integration_checks=106,existing_31_PCM_unchanged=True,resampled_filters=96,
                 maximum_filter_error_db=max(r['magnitude_error_db'] for r in filters['cases']),
                 maximum_ITD_error_samples=max(r['itd_error_samples'] for r in filters['cases']),
                 worst_preset_peak_dbtp=max(r['peak_dbtp'] for r in verified['presets'].values()),demo_count=demos['count'],
                 validated_dll_sha256=validators['dll_sha256'],delivery=delivery,cpu=timing,artifacts=artifacts,
                 installer_built=False,owner_FL_K712_listening='pending',live_host_callback_deadlines='unverified; earlier outlier remains disclosed')
    lab.write(report/'acceptance.json',summary)
    write_documentation(summary,catalog,verified,filters)
    print('S5 audit passed: 200/200 library,207 programs,96 filters,160 M0,106 integration; VST-only delivery verified')


def write_documentation(summary,catalog,verified,filters):
    cpu=summary['cpu'];rows='\n'.join(f'| {i+1} | {r["mean_fraction"]*100:.3f}% | {r["largest_fraction"]:.5f} | {r["cycles_at_largest_wall"]:,} / {r["median_cycles"]:,} |' for i,r in enumerate(cpu))
    rates=[]
    for rate in sorted({r['rate'] for r in filters['cases']}):
        cases=[r for r in filters['cases'] if r['rate']==rate]
        rates.append(f'| {rate} | {max(r["magnitude_error_db"] for r in cases):.6f} | {max(r["itd_error_samples"] for r in cases)} |')
    text=f'''# HamloShirota 0.2.0: VST-only library acceptance

2026-11-02; parent5765c5e. The owner's latest request excludes the installer.
The delivered Windows x64 VST contains207host programs: Default, six original
presets and200new spatial recipes. No installed plug-in, other product, push or
subtree was changed. FL/K712 listening and live-host deadline checks remain
pending; this report does not substitute offline tests for owner audition.

## Library and measured results

Before:7programs/new0; library checks n/a. After:207programs/new200. The same
native catalog/render driver and final method are used for both. Old first7names,
values and indices are unchanged. Four genres x50: Boom Bap, Trap, Techno, DnB.
Each genre has Kick6, Snare/Clap6, Hats/Cymbals5, Percussion4, Drum Bus7,
808/Bass5, Vocals5, Keys/Piano4, Samples/Melody4, Master4. Each entry has a
permanent ID, purpose/note, genre/group, material, tempo and depth. See
`docs/GENRE_PRESETS.md` and `catalog_final.json` for the complete list/settings.

| Check | Result |
|---|---|
| Calibrated and verified genre programs | **200/200** |
| Unique sound recipes, excluding Mix/Output/UI-only fields | **200/200** |
| Actual saved/restored parameters, selected program and audio | **200/200 exact** |
| Native expectations | **{summary['native_expectations']} passed,0failed** |
| Original M0 audio/state/latency | **160/160 bit exact** |
| Existing modern31PCM renders versus S4 | **31/31 identical SHA256** |
| Final integration lab | **106/106** |
| Source/cache/actual-embedded-bank rejection | **5/5** |
| Resampled integrated binaural filters | **96/96** |
| Worst preset8xtrue peak | **{summary['worst_preset_peak_dbtp']:.6f}dBTP**, target<=-1 |
| Production owner-material listening files | **{summary['demo_count']}**, common gain within each material |
| pluginval | **5..10, all exit0** |
| Steinberg validator | **537/537** |

Spatial depth is the energy of the output change relative to PDC-aligned dry,
with Output gain removed. This is not a reverb wet/dry or perceived loudness
metric. Gentle:-24..-16dB, Medium:-16..-8, Heavy:-8..0; calibration targets
-20/-12/-4. Each author recipe was rendered at full strength; Spatial Mix was
derived from that residual and then checked in a fresh actual program render.
Output can only trim peaks and cannot manufacture the depth result. Inputs use
the owner's highest-energy10s window plus1s silence, prepared at48k with recorded
source/input hashes and peak cap. Bounds/methods preceded measurement in
`docs/S5_METHODS.md`; none were relaxed. All final audio processing/setter
allocation checks pass. Static block tests exercise every207programs.

Cache records bind sound/parameter/routing/build sources, method, recipe, input
bytes and tempo; each native call records its executable SHA. Generated catalog
and UI are intentionally outside the sound fingerprint, while every compiled
recipe is independently compared with author/calibration settings. Interrupted
native attempts are retained; only complete hash-verified records resume.

## Failures found and corrected

The first BB Bass Answer reached-12.097dB at full strength, narrowly missing
the chosen Medium center(-12). The target band was already reachable, but the
calibration-center rule still failed. Its room amount and beat-related predelay
were increased; other199valid recipes resumed from verified cache. Original
failed output and `calibration_initial.json` remain retained.

The first project-state pass stopped on BB Open Tiles: saved Width3 could show
Width1 after loading into an edited instance, even though DSP audio was already
identical. A silent reset before APVTS state replacement left the parameter and
its adapter out of sync when the pre-load value equaled the saved value. Reset
now notifies the parameter listeners. A regression covers equal pre-load values,
stable-ID selection with a stale numeric program index, and return to the exact
compact Default. Temporary diagnostic instrumentation was restored byte-for-byte
before acceptance; final before/after laboratory method fingerprints match.

Changing this state code conservatively invalidated the sound fingerprint.
All200calibrations,96filter checks and200state/audio validations were repeated.
The200full-strength PCM hashes remain identical to before the state repair.
Prior reports/attempts are retained under `before_state_fix_*` and `out/s5`.

Four-node UI inspection also caught the rear node number overlapping BACK.
Node numbers now sit inside the circle; the initial screenshot is retained.
QUICK/STEREO/SPACE/MOTION/LEGACY snapshots were inspected. Shared PresetScreen
stays unchanged at56,20,336x116; genre/group/note and LOADED/MODIFIED are visible.
Final validators target the corrected UI binary in `validators_release/`;
the earlier `validators_final/` is historical, not the delivered fingerprint.

## Binaural rates and performance

Actual integrated Point impulses cover three profiles and front/left/back/right
at eight rates. Magnitude is compared independently to the native48k bank in
100Hz..min(16k,.4fs), above-40dB source magnitude. ITD is compared with an
independently resampled native reference. All meet<=0.5dB and<=1sample.

| Rate(Hz) | Maximum magnitude error(dB) | Maximum ITD error(samples) |
|---|---:|---:|
{chr(10).join(rates)}

The original SADIE II source bank, production bank, S4 DSP kernels and parameter
layout remain unchanged. Full upstream/modified-data notices and license are
included in the complete VST bundle. No runtime data download is needed.

Combined worst-case48k/256CPU, three8s runs after1s warmup, on the available
i5-10300H; all means meet the pre-recorded<=25%block-period target:

| Run | Mean / block period | Largest wall callback / period | Thread cycles at largest wall / median |
|---|---:|---:|---:|
{rows}

This is not proof of FL callback deadlines. S4 previously recorded a1.0062period
callback, and earlier S3 larger outliers remain unexplained. Final lower numbers
do not erase them. Owner FL Studio/K712 audition is still pending. Horizontal
profiles are not personalized HRTF or headphone EQ; perceived behind-head
placement varies. No elevation/head tracking is provided. Legacy retains its
original block dependence and compact-state omission of program metadata; modern
genre projects preserve their permanent ID/program and exact settings/audio.

## Delivery and reproduction

Delivered bundle: `{summary['delivery']['bundle']}`.
DLL SHA256: `{summary['validated_dll_sha256']}`.
Keep/copy the entire HamloShirota.vst3 folder, including Resources. The adjacent
SHA256SUMS.txt covers every file. No installer was built, as requested.
`acceptance.json` and `delivery.json` verify the copied bundle against validators.

Run commands from E:/HamloShirota, using isolated build/s3_venv Python:

```powershell
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py calibrate
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/s5_calibrate.py verify
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/prepare_s5_demos.py
build/s3_venv/Scripts/python.exe -I -X utf8 Tools/report_s5.py
```

Do not overwrite native run directories or old evidence. Changed DSP/method/
material requires a new matching cache identity. Full build/validator commands,
final commit and open owner checks are recorded in `docs/NEXT_STAGE.md`.
'''
    (lab.REPORT/'REPORT.md').write_text(text,encoding='utf-8',newline='\n')
    lines=['# HamloShirota genre presets','',
           '200 authored spatial recipes, appended at host programs7..206. Original0..6 stay unchanged.',
           'Offline owner-material/state/audio checks pass; personal FL/K712 audition remains pending.',
           'Depth is output-change energy relative to aligned dry, excluding Output gain; see S5_METHODS.md.','']
    for genre in ['Boom Bap','Trap','Techno','DnB']:
        lines+=['## '+genre,'','| Program / ID | Preset | Group | Tempo | Depth / measured dB | Purpose |','|---|---|---|---:|---|---|']
        for row in catalog['programs'][7:]:
            if row['genre']!=genre:continue
            measured=verified['presets'][row['id']]['residual_db']
            lines.append(f'| {row["program"]} / `{row["id"]}` | {row["name"]} | {row["group"]} | {row["bpm"]:g} | {row["depth"]} / {measured:.3f} | {row["note"]} |')
        lines.append('')
    (ROOT/'docs/GENRE_PRESETS.md').write_text('\n'.join(lines).rstrip()+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
