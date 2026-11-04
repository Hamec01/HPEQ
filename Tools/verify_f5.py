"""Replay Fader0.2.0 acceptance from frozen source, measurements and delivery."""
import hashlib,json,pathlib,re,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
REPORT=ROOT/'docs/reports/f5'
def sha(path,normalized=False):
    data=path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n',b'\n') if normalized else data).hexdigest()
def load(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def check():
    receipt=load(REPORT/'acceptance.json')
    assert receipt['stage']=='F5' and receipt['version']=='0.2.0'
    for name,pin in receipt['files'].items():assert sha(ROOT/name,pin['normalized'])==pin['sha256'],name
    native=(REPORT/'native_delivery.txt').read_text(errors='replace')
    count=re.search(r'HamloFader_Tests: (\d+) expectations passed, (\d+) failed',native)
    assert count and int(count[1])==receipt['native_expectations'] and int(count[2])==0
    assert int(count[1])>=17000
    assert sha(ROOT/'Tests/F5LibraryTests.cpp',True)==sha(ROOT/'out/f5/F5LibraryTests-before.cpp',True)
    before=load(REPORT/'legacy_before.json');after=load(REPORT/'legacy_delivery.json')
    assert before['method_id']==after['method_id'] and before['contract']==after['contract']
    assert len(before['cases'])==len(after['cases'])==226 and after['failures']==0
    for old,new in zip(before['cases'],after['cases']):
        assert old==new,old['case']
    host=load(REPORT/'host_delivery.json');assert host['passed']==224 and host['failed']==0
    for file in (ROOT/'Source/DSP').rglob('*'):
        if file.is_file():assert sha(file,True)==sha(ROOT/'out/f4/accepted/Source/DSP'/file.relative_to(ROOT/'Source/DSP'),True),file
    cal=load(REPORT/'calibration_v4/result.json');checked=load(REPORT/'check_final.json');analysis=load(REPORT/'analysis_final.json')
    assert cal['passed']==checked['passed']==analysis['passed']==200
    assert len(cal['recipes'])==len(checked['recipes'])==len(analysis['recipes'])==200
    for key in ('dsp_sha256','method_sha256','skeleton_sha256','materials'):assert cal['identity'][key]==checked['identity'][key],key
    dsp='FaderPresetDSPV1'+''.join(sha(p,True) for p in sorted(p for p in (ROOT/'Source/DSP').rglob('*') if p.is_file()))
    for name in ('Source/Standalone/ModulePluginProcessor.cpp','Source/Standalone/ModulePluginProcessor.h','Source/Standalone/FaderHostControls.cpp','Source/Standalone/FaderHostControls.h','Source/Standalone/ModuleVariant.cpp'):dsp+=sha(ROOT/name,True)
    method='FaderPresetMethodV1'+''.join(sha(ROOT/name,True) for name in ('Tests/F5LibraryTests.cpp','docs/F5_PRESETS.md'))
    assert digest(dsp)==cal['identity']['dsp_sha256'] and digest(method)==cal['identity']['method_sha256']
    assert analysis['exact_calibration_pcm']==200 and analysis['max_work_delta_db']==0
    for row in checked['recipes']:
        assert row['passed'] and row['finite'] and row['state_exact'] and row['workflow_valid']
        low,high={'Gentle':(1,3),'Medium':(3,6),'Heavy':(6,12)}[row['depth']]
        assert low<=row['work_db']<=high
        assert sha(ROOT/f"out/f5/check_final/audio/{row['id']}.wav")==row['wav_sha256']
    assert max(row['true_peak_8x_dbtp'] for row in analysis['recipes'])<=-1
    # Resume provenance: only byte-equivalent measurements for unchanged recipes.
    for number in (2,3,4):
        folder=REPORT/f'calibration_v{number}';prior=REPORT/f'calibration_v{number-1}'
        migration=load(folder/'migration.json');old=load(prior/'result.json');new=load(folder/'result.json')
        for key in ('dsp_sha256','method_sha256','materials'):assert old['identity'][key]==new['identity'][key]
        a={r['id']:r for r in load(prior/'uncalibrated.json')};b={r['id']:r for r in load(folder/'uncalibrated.json')}
        x={r['id']:r for r in old['recipes']};y={r['id']:r for r in new['recipes']}
        for key in migration['copied_exact_recipe_ids']:assert a[key]==b[key] and x[key]==y[key] and x[key]['passed']
        assert len(migration['copied_exact_recipe_ids'])==(198 if number==2 else 199)
    assert load(REPORT/'calibration_v1/result.json')['passed']==198
    assert load(REPORT/'calibration_v2/result.json')['passed']==199 and load(REPORT/'calibration_v3/result.json')['passed']==199
    identity=load(REPORT/'validators_delivery/identity.json')
    assert len(identity['results'])==7 and all('exit=0' in result for result in identity['results'])
    assert '537 tests passed, 0 tests failed' in (REPORT/'validators_delivery/steinberg.txt').read_text(errors='replace')
    assert sha(ROOT/receipt['build_dll'])==sha(ROOT/receipt['delivery_dll'])==identity['dll_sha256']
    rc=(ROOT/'build/HamloFader_artefacts/JuceLibraryCode/HamloFader_resources.rc').read_text()
    assert '0,2,0,0' in rc and '0.2.0' in rc
    with zipfile.ZipFile(ROOT/receipt['archive']) as archive:
        assert archive.testzip() is None
        for name in archive.namelist():assert archive.read(name)==(ROOT/receipt['package']/name).read_bytes(),name
    demos=load(REPORT/'demos_final.json');assert len(demos)==12
    for row in demos:
        assert row['true_peak_8x_dbtp']<=-1
        assert sha(ROOT/'out/f5/demos_final'/row['file'])==row['sha256']
    print(f"F5 PASS:{count[1]}native,224host,678exactLegacy,200compiled/calibrated/8xchecked recipes,12auditions,validators,source/package identities")
if __name__=='__main__':check()
