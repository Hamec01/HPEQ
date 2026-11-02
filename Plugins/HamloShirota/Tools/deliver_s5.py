"""Copy only a validated VST bundle; no installer or installed-host mutation."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'build/HamloShirota_artefacts/Release/VST3/HamloShirota.vst3'
DESTINATION=ROOT/'dist/HamloShirota-0.2.0-VST3/HamloShirota.vst3'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def inventory(folder):return {str(p.relative_to(folder)).replace('\\','/'):sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def main():
    validators=json.loads((ROOT/'docs/reports/s5/validators_release/identity.json').read_text(encoding='utf-8-sig'))
    dll='Contents/x86_64-win/HamloShirota.vst3';assert sha(SOURCE/dll)==validators['dll_sha256']
    assert all(row.endswith('exit=0') for row in validators['results']) and len(validators['results'])==7
    assert DESTINATION.resolve().is_relative_to((ROOT/'dist').resolve())
    before=inventory(SOURCE)
    if DESTINATION.exists():assert inventory(DESTINATION)==before,'Refusing to replace a different delivered build'
    else:shutil.copytree(SOURCE,DESTINATION)
    assert inventory(DESTINATION)==before and sha(SOURCE/dll)==validators['dll_sha256']
    data=dict(version='0.2.0',bundle=str(DESTINATION),dll_sha256=validators['dll_sha256'],files=before,installer_built=False)
    (ROOT/'docs/reports/s5/delivery.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8',newline='\n')
    checksum=''.join(value+'  HamloShirota.vst3/'+name+'\n' for name,value in before.items())
    (DESTINATION.parent/'SHA256SUMS.txt').write_text(checksum,encoding='utf-8',newline='\n')
    (DESTINATION.parent/'READ-ME.txt').write_text('HamloShirota 0.2.0 VST3 for Windows x64\n\nCopy the entire HamloShirota.vst3 folder to your VST3 directory, then rescan in FL Studio.\n207 host programs: Default, six original presets,200new genre recipes.\nQUICK / STEREO / SPACE / MOTION / LEGACY; three horizontal headphone profiles.\nAll engines retain the original2.5ms rounded-sample PDC.\nOwner FL/K712 listening and live-host deadline verification remain pending; no personalized headphone EQ.\nNo installer included. SHA256SUMS.txt covers the complete validated bundle.\n',encoding='utf-8',newline='\n')
    print(DESTINATION)
    print('Validated DLL SHA256:',validators['dll_sha256'])


if __name__=='__main__':main()
