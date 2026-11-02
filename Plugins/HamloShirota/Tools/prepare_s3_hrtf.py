"""Own horizontal selection of SADIE II v2-1; preserve every native FIR tap.

Modified data: selected 5-degree horizontal plane, common per-subject gain,
float32 packing. No headphone/room response or commercial implementation.
"""
import hashlib
import io
import json
import struct
import urllib.request
import zipfile
from pathlib import Path
import h5py
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / 'out/s3/downloads'
ASSETS = ROOT / 'Assets/Hrtf'
FILES = {'LICENSE.txt': '0ba5044c64ef53cb0189c9546081e228',
         'D1_HRIR_SOFA.zip': '4850c1eb8e63e2d4f605edcdb4d5c883',
         'H3_HRIR_SOFA.zip': '145826ddba3db973bec5340322b27909',
         'H4_HRIR_SOFA.zip': '1e44b127d40f2098263d5f0b76faf2c4'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    CACHE.mkdir(parents=True, exist_ok=True)
    ASSETS.mkdir(parents=True, exist_ok=True)
    downloads = []
    for name, md5 in FILES.items():
        path = CACHE / name
        url = 'https://zenodo.org/records/10886409/files/' + name + '?download=1'
        if not path.exists():
            with urllib.request.urlopen(url, timeout=45) as source:
                path.write_bytes(source.read())
        data = path.read_bytes()
        assert hashlib.md5(data).hexdigest() == md5, 'Corrupt upstream download: ' + name
        downloads.append(dict(file=name, url=url, md5=md5, sha256=sha(data), bytes=len(data)))
    (ASSETS / 'LICENSE-SADIE-II.txt').write_bytes((CACHE / 'LICENSE.txt').read_bytes())
    blob = bytearray(struct.pack('<8sIII', b'HmHrS3A\0', 3, 3, 72))
    records = []
    gains = {}
    worst_error, retained = 0., 1.
    for rate, marker, taps in [(44100, '44K', 256), (48000, '48K', 256), (96000, '96K', 512)]:
        blob.extend(struct.pack('<II', rate, taps))
        for profile in ['D1', 'H3', 'H4']:
            z = zipfile.ZipFile(CACHE / (profile + '_HRIR_SOFA.zip'))
            assert all(n.endswith('.sofa') or n.endswith('/') for n in z.namelist()), 'Inspect additional archive notices'
            member = next(n for n in z.namelist() if '_' + marker + '_' in n)
            sofa = z.read(member)
            with h5py.File(io.BytesIO(sofa), 'r') as f:
                assert float(f['Data.SamplingRate'][0]) == rate
                assert np.all(f['Data.Delay'][:] == 0)
                assert f['SourcePosition'].attrs['Type'] == b'spherical'
                pos = f['SourcePosition'][:]
                indices = []
                for angle in range(0, 360, 5):
                    found = np.flatnonzero((np.abs(pos[:, 0] - angle) < 1e-6) & (np.abs(pos[:, 1]) < 1e-6))
                    assert len(found), (profile, angle)
                    indices.append(int(found[0]))  # First regular-grid measurement if repeated by an Ambisonic configuration.
                raw = f['Data.IR'][:][indices]
                assert raw.shape == (72, 2, taps) and np.isfinite(raw).all()
                if profile not in gains:
                    gains[profile] = 1 / np.sqrt(np.mean(np.sum(raw[0] ** 2, axis=1)))
                gain = gains[profile]  # One gain per subject across every angle/ear/rate.
                prepared = (raw * gain).astype('<f4')
                blob.extend(prepared.tobytes())
                restored = prepared.astype(float) / gain
                source_fft = np.fft.rfft(raw, 8192, axis=-1)
                restored_fft = np.fft.rfft(restored, 8192, axis=-1)
                hz = np.fft.rfftfreq(8192, 1 / rate)
                mask = (hz >= 100) & (hz <= 16000) & (np.abs(source_fft) > np.max(np.abs(source_fft), axis=-1, keepdims=True) * .01)
                error = 20 * np.log10(np.maximum(np.abs(restored_fft), 1e-30) / np.maximum(np.abs(source_fft), 1e-30))
                worst_error = max(worst_error, float(np.max(np.abs(error[mask]))))
                retained = min(retained, float(np.min(np.sum(restored ** 2, axis=-1) / np.sum(raw ** 2, axis=-1))))
                records.append(dict(profile=profile, rate=rate, taps=taps, gain=gain, member=member,
                                    sofa_sha256=sha(sofa), measurement_indices=indices,
                                    license=f.attrs['License'].decode(), upstream_history=f.attrs['History'].decode()))
    bank = ASSETS / 'horizontal.bin'
    if bank.exists():
        assert bank.read_bytes() == blob, 'Refusing to overwrite a different prepared bank'
    else:
        bank.write_bytes(blob)
    manifest = dict(record='https://zenodo.org/records/10886409', profiles=['D1', 'H3', 'H4'],
                    rates=[44100, 48000, 96000], angles=list(range(0, 360, 5)),
                    coordinate='0 front, +90 left, 180 back, 270 right; elevation0',
                    changes='Horizontal selection; first regular-grid occurrence; one per-profile gain from44.1k front; float32. All taps retained, no trimming/resampling.',
                    bank_sha256=sha(bytes(blob)), license_sha256=sha((ASSETS / 'LICENSE-SADIE-II.txt').read_bytes()),
                    exporter_sha256=sha(Path(__file__).read_bytes()), downloads=downloads, records=records,
                    worst_magnitude_error_db=worst_error, minimum_retained_energy=retained,
                    archive_notice='Archives contain only SOFA files; embedded University of York copyright/license retained here.')
    (ASSETS / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
    assert worst_error <= .5 and retained >= .999
    print('M06 asset export:', len(blob), 'bytes; worst magnitude error', worst_error, 'dB; energy retained', retained)


if __name__ == '__main__':
    main()
