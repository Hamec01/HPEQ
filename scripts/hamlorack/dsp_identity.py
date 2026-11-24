"""Portable build-time identity of audio and calibration inputs (no private data)."""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[2]
EXTRA = (
    'Source/Core/FxProjectState.cpp', 'Source/Core/FxProjectState.h',
    'Source/Plugin/FxPluginProcessor.cpp', 'Source/Plugin/FxPluginProcessor.h',
    'Source/Plugin/FxParameterContract.h',
    'Source/Plugin/MacroRouting.cpp',
    'Source/Analysis/LoudnessMeter.cpp', 'Source/Analysis/LoudnessMeter.h',
    'Assets/Presets/HamloRack/Design/native-contract.json',
    'scripts/hamlorack/generate_native.py', 'cmake/HamloNative.cmake',
    'scripts/hamlorack/control_adapters.py',
)

def normalized(data):
    return data.decode('utf-8-sig').replace('\r\n', '\n').replace('\r', '\n').encode('utf-8')

def identity(root=ROOT):
    files = set(root / name for name in EXTRA)
    for directory in ('Source/DSP', 'Source/Rack'):
        files.update(p for p in (root/directory).rglob('*')
                     if p.is_file() and p.suffix in ('.h', '.cpp', '.inc')
                     and p.stem not in ('NativeModulePanel', 'PresetCatalogue'))
    sources = {p.relative_to(root).as_posix(): hashlib.sha256(normalized(p.read_bytes())).hexdigest()
               for p in sorted(files)}
    match = re.search(r'set\(HPEQ_JUCE_VERSION\s+([^\s)]+)',
                      (root/'CMakeLists.txt').read_text(encoding='utf-8-sig'))
    if match is None:
        raise ValueError('Missing pinned JUCE version')
    result = dict(format='HamloRackNormalizedDSP1', juce_version=match[1].strip('"'), sources=sources)
    result['sha256'] = hashlib.sha256(json.dumps(result, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    return result

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--header', type=Path)
    parser.add_argument('--json', type=Path)
    args = parser.parse_args()
    result = identity()
    if args.header:
        content = ('#pragma once\n// Generated from normalized audio source inputs.\n'
                   '#define HPEQ_RACK_DSP_SOURCE_SHA256 "'+result['sha256']+'"\n').encode()
        args.header.parent.mkdir(parents=True, exist_ok=True)
        if not args.header.exists() or args.header.read_bytes() != content:
            args.header.write_bytes(content)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(result['sha256'])

if __name__ == '__main__':
    main()
