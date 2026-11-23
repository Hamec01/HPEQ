# Current HamloRack integration

Owner clarification: 2026-11-23. One HamloRack VST3 must embed the latest versions
of our standalone plug-ins, complete with their own panels and capabilities,
while retaining HPEQ's EQ, frequency regions, ten layers and routing. It must not
ask the owner to replace legacy modules manually or reset their settings. Presets
must demonstrate the plug-ins and rack, and the main selector must have arrows.
The owner authorized commit and push after completion.

## Implementation

- The build uses `Plugins/Hamlo*` as the source of all ten products. The native
  contract pins exact clean source trees and versions; generation fails when an
  imported source differs. The generator compiles original editors instead of
  replacing them. Plug-in namespaces and each product's assets are isolated to
  prevent collisions. No external VST files or installations are required.
- An embedded editor controls its actual prepared rack processor. Its parameter
  gestures flow into HPEQ's undoable project state. Pending edits survive audio
  blocks before the message callback, and host save captures pending controls
  and native non-parameter state. Shared runtime ownership keeps an editor safe
  across topology retirement. Host reprepare keeps the native processor object
  and workbench alive, reallocating its DSP via the original prepare API; state
  loaded before host preparation defers DSP allocation until that first prepare.
  Native pages resize in
  the rack, including Revolution's Simple/Advanced/Transition pages.
- Native MIX, BYPASS and insert/send controls are allowed. Explicit native
  wet-only returns use HPEQ's additive send law; complete native inserts use its
  aligned crossfade.
  Native send BYPASS fades the additional dry reference over the same 20 ms as
  the original wrapper, preserving one dry reference throughout both transitions.
  Fader's native action buttons are bridged to the existing rack Match/dual-mono
  controller. Revolution's live instance includes its sample
  player, HPDG follower and original workbench; offline factory inspection still
  uses its original offline constructor.
- Imported HPEQ projects and user presets are mapped to the current wrappers on
  load. The migration retains enabled state, rack mix, finite matching physical
  parameter IDs and explicit legacy Revolution action vocabulary. Unmatched old
  fields use current defaults and out-of-range values clamp to current bounds.
  Missing old physical values come from HPEQ's actual former snapshot defaults,
  generated from `LayerSnapshot.cpp`; this avoids injecting the newer standalone
  factory's drive/noise settings into an imported state. Where available, the
  current plug-in's compatibility controls retain the meaning of old parameter
  IDs. These are engines inside the current full plug-in, not separate old rack
  modules. Fresh rack modules and Rack Power recipes use the current defaults.
  The first migration test incorrectly expected implicit old hiss to be zero;
  `LayerSnapshot.cpp` actually specifies 0.15 with Classic model 0. The test now
  checks those actual former defaults; no audio acceptance bound was relaxed.
  This is not a claim of bit-identical old-project sound. The old exact legacy
  rendering guarantee in RACK0/RACK1 is superseded by this owner clarification.
  Low-level legacy DSP remains for reference tools and compatibility targets;
  normal rack creation and imported states use current modules. The manual
  `Upgrade to native module` menu item is removed.
- The main browser leads with twenty Rack Power recipes: 2–5 active layers,
  multi-band/parallel/serial processing and 103 module instances across all ten
  products. The 250 task recipes remain available; their original data and
  calibration are unchanged. The old 224-preset bank is no longer offered in the
  main browser. Rack Power's readable signal chains are in [RACK_POWER.md](RACK_POWER.md).
- The green preset display has previous/next buttons within the current bank,
  a position counter, bank/category labels, wheel navigation and a name menu.
- Rack version is 0.2.0, with the existing VST3 class IDs. Tall original panels
  scroll instead of shrinking below 65% scale; ordinary panels fit the editor.

## Verification and continuation

The current machine-readable evidence is `CURRENT_VERIFICATION.json`, separate
from the historical `RACK_COMPLETE.json`. The complete Release suite passed
7851 expectations with zero failures. The Debug HamloRack suite passed 4708
expectations with zero failures, using the CRT hook for both malloc and new
(Release instruments operator new only). The Release VST3 binary is version 0.2.0. This
machine used `HPEQ_ENABLE_LTO=OFF`; the repository's production default remains
ON. The receipt identifies the tested binaries and their hashes.

- The independent VST3 host loaded that binary, completed 100/100 instance cycles
  and 100/100 desktop editor open/close cycles, and passed identity, mono/stereo,
  block-size, output-gain and exact saved-state audio checks.
- The final compiled rack replayed all 250 existing task recipes and all 20 Rack
  Power recipes bit-exactly against their respective saved canonical renders.
- Original-panel snapshots cover all ten products at 1000x700, 1280x840 and
  1920x1200. The panels and scroll behavior were visually inspected. Thirteen
  Python structural tests and the source/tree/parameter bank checks pass.
- The original-editor tests cover the actual audio-instance binding, pending
  edits before message dispatch, immediate host save, undo, preservation across
  host sample-rate changes, and dry-reference continuity through native send
  bypass. The native regression retains strict public-API parity, block-size,
  allocation, scene, state, retirement and Fader Match checks.

The first full Release run had 74 failures from superseded test contracts: old
module identity after public state import, the former single preset button, and
reading old packed Revolution keys instead of its native scene state. The
updated tests retain the immutable v1 model fixture, check its explicit migration
to current wrappers, and require exact audio and structural state restoration
for current native projects. The preset UI test now operates both arrows; native
Revolution scenes use the native public scene reader. No numerical audio, peak,
allocation or CPU acceptance threshold was relaxed.

Rack Power method v1 is declared in `RackBankMain.cpp` before its first run:
keep authored DSP parameters, change output trim only, use two independent local
category clips at -6/0/+6 dB, require finite active output and <= -0.3 dBTP,
check host tempo at 80/120%, and render eight seconds of silent input after the
phrase with the final-second sample peak <= -45 dBFS. Matched A/B files receive
common attenuation for headroom. Each recipe records metrics, hashes, failures
and an explicit pending owner-listening verdict. Numerical difference does not
establish musical suitability.

All 20 Rack Power recipes passed 160 source/level/tempo checks and 20 silent-tail
checks. The worst independent 8x true peak was -0.34999762 dBTP; the worst final
tail was -113.43057 dBFS. Only output trim was calibrated. Ten recipes meet the
0.5 LU loudness target; ten documented peak/headroom exceptions remain. Full
metrics are in `RACK_POWER_RECEIPT.json`; owner listening is pending. Private
source audio and rendered A/B files stay outside Git.

No new pluginval or Steinberg-validator run is claimed. The desktop host check
does not replace the owner's FL Studio check. Native sample/workbench file
persistence follows each native product; rack sample-asset packaging is not
claimed. Importing old HPEQ projects preserves shared controls, but does not
promise their former sound bit-for-bit.

Build on another PC after pulling:

```
build_vst.bat
```

Output: `build/HPEQ_artefacts/Release/VST3/HamloRack.vst3` and `batvst/HamloRack.vst3`.
The installer script backs up/replaces old HPEQ and HamloRack bundles; builds do
not silently overwrite an installed plug-in. Close the old rack instance before
replacing a loaded DLL. Full FL Studio listening remains the owner's check.
