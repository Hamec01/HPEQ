# HamloTochka 0.2.0

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

## External sidechain

Route a kick, vocal, or other trigger to the VST3 Sidechain input. Select a
band, turn on Dynamic, and click **Key: Ext** to use the external detector.
Band mode follows the filtered key level. Voice mode compares the key band
with its vocal-body level. The blue Key trace shares the main spectrum scale.
An inactive or silent key releases the dynamic gain toward the static setting.

Acceptance at 48 kHz uses a synthetic 125 Hz kick/808 pair and a 2 kHz probe
outside the band. A keyed bell reaches −6 ±0.2 dB at 125 Hz, changes the 2 kHz
probe by at most 0.1 dB, and returns within 0.1 dB after release. Silence or a
key beyond the detector band keeps the 125 Hz level within 0.1 dB. A stereo
key, a mono key and a disabled key bus are tested. The range, output latency
and internal-detector baseline match the first release. Saving and reopening
state preserves each band's key mode.

## Release build checks

- The JUCE VST3 host passed 46 checks at 48 kHz. With a 0.1.0 bundle supplied
  as the second argument, it passed 50 checks. The migrated session was
  bit-exact to 0.1.0, including when reloaded into an existing keyed instance.
- The 125 Hz synthetic key produced a −6.00 dB cut and changed the 2 kHz probe
  by −0.0016 dB. A separate vocal-like key produced a −6.00 dB cut at 2.5 kHz
  and changed the 250 Hz probe by −0.0041 dB. Both cuts released to unity.
- The native processor rendered identical samples with blocks of 1, 64, 127,
  512 and 4096 after being prepared with 256. The VST3 host test prepares the
  wrapper for each negotiated block limit.
- Learn Match reduced the measured difference to less than 0.001 LU on the
  1 kHz test tone with a +6 dB bell, after the gain settled.
- FL Studio playback and project reopening are pending.

`KeyedEq` was copied from HPEQ's `PreEqProcessor` at the 0.1.0 baseline
and extended with a separate key input. The original PRE-EQ is the
internal-detector audio reference in `HamloTochka_HostCheck`.
