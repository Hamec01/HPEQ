"""Prepare private 48 kHz stereo fixtures from an explicit local JSON manifest.

Usage: python prepare_material.py sources.json output_directory
The manifest maps category names to exactly two {sources:[...]} arrangements.
Each source has path, optional start_seconds, gain, and optional onsets_seconds.
Original files are read only; the output directory must be new.
Requires numpy, scipy, soundfile. No audio or absolute source paths go in Git.
"""
from pathlib import Path
import hashlib
import json
import math
import sys
import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

def prepare(manifest, output):
    definitions = json.loads(manifest.read_text(encoding="utf-8-sig"))
    output.mkdir(parents=True, exist_ok=False)
    receipts = {}
    length = 8 * 48000
    for category, variants in definitions.items():
        if len(variants) != 2:
            raise ValueError(f"{category} needs two variants")
        for variant, arrangement in enumerate(variants):
            result = np.zeros((length, 2), dtype=np.float64)
            sources = []
            for source in arrangement["sources"]:
                path = Path(source["path"])
                info = sf.info(path)
                start = round(source.get("start_seconds", 0) * info.samplerate)
                audio, rate = sf.read(path, start=start, frames=round(8 * info.samplerate), always_2d=True, dtype="float64")
                if audio.shape[1] == 1:
                    audio = np.repeat(audio, 2, axis=1)
                if audio.shape[1] != 2 or not np.isfinite(audio).all():
                    raise ValueError(f"Invalid source: {path}")
                divisor = math.gcd(rate, 48000)
                audio = resample_poly(audio, 48000 // divisor, rate // divisor, axis=0)
                audio *= source.get("gain", 1)
                # A sample arrangement must specify every onset; continuous
                # recordings use one onset at zero. Do not relabel a mix as a stem.
                for onset in source.get("onsets_seconds", [0]):
                    offset = round(onset * 48000)
                    count = min(len(audio), length - offset)
                    if offset < 0 or count <= 0:
                        raise ValueError("Invalid onset")
                    result[offset:offset + count] += audio[:count]
                sources.append(dict(source, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
            fade = np.minimum(1, np.minimum(np.arange(length), np.arange(length)[::-1]) / 480)
            result *= fade[:, None]
            peak = np.max(np.abs(result))
            if peak < 1e-8:
                raise ValueError(f"Silent fixture: {category}")
            result *= .5 / peak
            destination = output / f"{category.replace(' ', '_')}-{variant}.wav"
            sf.write(destination, result, 48000, subtype="FLOAT")
            receipts[destination.name] = dict(description=arrangement.get("description", ""), sources=sources,
                sha256=hashlib.sha256(destination.read_bytes()).hexdigest())
    (output / "sources.json").write_text(json.dumps(receipts, indent=2, ensure_ascii=False), encoding="utf-8")

if __name__ == "__main__":
    prepare(Path(sys.argv[1]), Path(sys.argv[2]))
