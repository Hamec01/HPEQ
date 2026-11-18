# HamloTochka 0.1.0

HamloTochka is a Windows x64 VST3 dynamic EQ built around HPEQ's PRE-EQ.
Build from the HPEQ checkout with its JUCE 8.0.12 configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release --target HamloTochka_VST3 HamloTochka_HostCheck HamloTochka_LocalCheck
ctest --test-dir build -C Release -R HamloTochka --output-on-failure
```

The VST3 bundle is under `build/Plugins/HamloTochka/HamloTochka_artefacts/Release/VST3`.
Double-click in the graph to add a band, drag a point to move it, and use
the band selector to edit any of the 24 slots. The Black Steel point panel
provides the shape, slope, channel, dynamic controls, solo and de-ess.
A/B switches between dry input and EQ output. Learn Match measures both signals
over the same passage and holds a bounded output correction when pressed again.
Check the correction after changing the EQ. The toolbar also clears the correction.

## First-stage acceptance

- A distinct Windows x64 VST3 with 24 fixed parameter slots, zero reported
  latency and supported mono/stereo layouts.
- Bit-exact neutral pass-through and sample-for-sample agreement with HPEQ's
  PRE-EQ for the same non-neutral parameters and test signal.
- Finite, block-size-independent processing under automation, including block
  lengths 1, 64, 127, 512 and 4096 following prepare with 256.
- State round-trip and stable host parameter identifiers, including active bands
  and held loudness correction.
- Held loudness match within 0.3 LU on the stationary test signal.
- Validate VST3 scanner, editor and state round-trip in a host. FL Studio
  playback and project reopening are separate manual checks.

The next DSP stage is an external sidechain bus feeding individual dynamic-band
detectors. This first stage uses the existing Band and Voice detectors.

## Release build checks

- The JUCE VST3 host passed 25 checks at 48 kHz, including mono/stereo unity,
  exact PRE-EQ comparisons, automation, VST3 state restore, and editor creation.
- The native processor rendered identical samples with blocks of 1, 64, 127,
  512 and 4096 after being prepared with 256. The VST3 host test prepares the
  wrapper for each negotiated block limit.
- Learn Match reduced the measured difference to less than 0.001 LU on the
  1 kHz test tone with a +6 dB bell, after the gain settled.
- FL Studio playback and project reopening are pending.
