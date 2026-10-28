# HamloRedWall

HamloProd's limiter as its own VST3 plug-in (Windows x64), extracted from [HPEQ](../HPEQ) on 2026-10-28 like
HamloComp and HamloUdarnik; developed separately from here, improvements carried back
([docs/SYNC_WITH_HPEQ.md](docs/SYNC_WITH_HPEQ.md)).

- **DSP**: the M6 look-ahead limiter and the RedWall core: LOUDNESS push, GLASNOST / STAKHANOV / OCTOBER, TRUE PEAK
  (4x polyphase), AUTO release, LINK, MONITOR unity / delta, OUTPUT (`Source/DSP/`, every law in `HamloRedWall.h`).
- **Page**: Black Steel (1180 x 700, resizable 0.5 .. 2 x) (`Source/UI/`).
- **Plug-in**: the IDs and identity of HPEQ's HamloRedWall module VST3 (HamloProd / BBLB / HmRw), so its projects open
  here; MIX, ON, 6 presets, mono and stereo, a fixed latency of 488 samples at 48 kHz (`Source/Plugin/`).

## Build

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 [-DHAMLOREDWALL_JUCE_DIR=<JUCE 8.0.12 checkout>]
cmake --build build --config Release --target HamloRedWall_VST3 HamloRedWall_Tests HamloRedWall_Reference
build\Release\HamloRedWall_Tests.exe
```

Installer: `powershell -ExecutionPolicy Bypass -File Installer\Build-Installer.ps1`.

Status: [docs/reports/M0_EXTRACTION_REPORT.md](docs/reports/M0_EXTRACTION_REPORT.md).
