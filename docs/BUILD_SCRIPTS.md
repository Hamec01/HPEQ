# Windows build and installation launchers

Checked 2026-11-25. The BAT files use the included Windows PowerShell and share
`scripts/build_hamlo.ps1`. They build current standalone sources from `Plugins/`
and the root HamloRack target, not the older root compatibility-module targets.

## Build

Use Visual Studio 2026 with C++ tools and Windows SDK, and a CMake version that
lists `Visual Studio 18 2026` in `cmake --help`. Rack configure also needs Git and
Python 3 for native source isolation. JUCE 8.0.12 is fetched by rack configure
unless an existing local checkout is selected. The manual CMake presets remain
available; the BAT generator is set in the shared PowerShell launcher.

| Command | Result |
|---|---|
| `build_hamlo_plugins.bat` | Menu: one product or all eleven |
| `build_hamlo_plugins.bat all` | Rack and all ten standalone plug-ins |
| `build_hamlo_plugins.bat comp` | HamloComp only |
| `build_hamlo_plugins.bat rev` | HamloRevolution only |
| `build_vst.bat` | HamloRack only |
| `build_hamlo_plugins.bat check all` | Print planned commands, without compilation, copying or installation |
| `build_vst.bat check` | Print the rack's planned commands |

Aliases and menu numbers retain their previous meanings. `check` validates the
selection and available CMake generator, and displays the selected cache paths;
compiler/SDK availability is established by actual CMake configuration, not by
the command-only check. A normal build always reconfigures first, preserves
existing cache options and stops on failed configure/build/copy.

All VST targets and asset stages use `--parallel 4` and `/p:CL_MPCount=2`.
Standalone generated assets finish before VST compilation. A first standalone
build with no shared JUCE configures the rack to obtain JUCE, without compiling
the rack VST. A cache moved from another PC/source/generator is left intact;
the launcher selects a separate `build-rebuild/<product>` directory. The
consistently named output bundles are copied into `batvst/` in either case.

## Install

Close the DAW before replacing loaded VSTs. Build-and-install commands require an
administrator terminal and check this before compiling:

```
build_hamlo_plugins.bat install all
build_hamlo_plugins.bat install comp
build_hamlo_plugins.bat comp install
build_vst.bat install
```

The complete requested set is built before installation begins. The ten
standalones and the rack are separate installation transactions. A failure in
the rack transaction does not undo a completed standalone transaction.

`install_hamlo_plugins.bat` installs the ten already prepared standalone bundles
from `batvst`, without building. It requests Windows elevation if needed. The
rack is installed by the explicit rack/build-and-install commands above.

Installation uses the Windows 64-bit Common Files VST3 directory, stages complete
bundles before replacement and moves old bundles into backups. A failed
transaction restores earlier replacements; a locked old file is left intact.
Successful replacements retain their backups in the printed
`Common Files/Hamlo-VST3-install-<id>/backup` directory. Rack installation also
retires the old `HPEQ.vst3` duplicate. All installers share the same mutex.

For automation, set `HAMLO_NO_PAUSE=1` to suppress the BAT's final pause.

## Verification scope

`Tests/Tools/BuildScriptsSmoke.ps1` runs the real BAT/PowerShell control flow with
a native fake CMake and fake bundles under an isolated `out` directory. It
checks all target/JUCE-variable mappings, bounded compilation, source paths with
spaces/parentheses/ampersand/exclamation, single/all selection, JUCE bootstrap,
relocated caches, errors, native stderr warnings, backups and rollback with a
locked file. Installation functions and the real rack installer wrapper use a
fake Common Files destination. No real installed VST is replaced by these tests.

Final smoke result: **82 assertions passed**. Real CMake configuration also
passed for HamloRack and standalone HamloComp, including juceaide setup and all
ten isolated native wrappers in the rack. Full VST compilation was not run as
part of this launcher review. Evidence: `BUILD_SCRIPTS_VERIFICATION.json`.
