# HamloEcho 0.2.1: neutral Default

The owner's requested Default is now a clean starting point. New instances and explicit program 0 open NOTE SCORE with Mix, feedback, space, glitch, modulation, drive and pitch shifts at zero. Input passes bit-identically. Output stays at 0 dB, Width at 100%, filters open and note targets equal SOURCE; these are neutral settings rather than literal zero values. Times use their minimum valid values.

The first unpitched reflection is prepared, with the other seven disabled. Raise Mix and Time to begin exploring. Default is selectable under Factory / General in the plugin's preset screen. It deliberately resets Send, Mix Lock, Freeze and bypass, and clears outgoing tails at the next audio callback. Other presets keep their previous Mix Lock/routing behaviour.

Saved projects restore their stored engine and parameters, including old audible Default states. Select Default again to apply the new initialization. Parameter IDs, order, ranges and host metadata defaults are unchanged; initialization is a separate recipe. Original programs 1..7 and all 200 genre recipes retain their values and positions. Host program count remains 208. Shared PresetScreen files are unchanged.

## Verification

- 1234 expectations passed, 0 failed. New coverage verifies fresh dry output, reset of a playing Send/locked/frozen instance, cleared tails, preset modification state, and an audible 1 ms unpitched reflection when Mix rises.
- 160 immutable M0 states/renders pass. Since Default is intentionally redefined, the existing lab compares expected old values against archived XML rather than today's initialization. Original signals, event schedules and reference bytes remain unchanged.
- All Source/DSP file hashes and the complete compiled 200-program parameter/metadata payload match the before-patch snapshot. See equivalence.json. Existing calibrated values are retained by this equivalence proof; the 0.2.0 rendering/calibration reports remain historical evidence, not a claimed new DSP calibration. Strict stale-calibration guards are unchanged.
- Pluginval 5–10 and Steinberg validator 537/537 pass. Default screenshot inspected.
- First unit run: one routing-reference fixture assumed the former 2 s / 6 kHz spatial defaults. Set those reference conditions explicitly (independent of the new init recipe); retain its tolerance and all production DSP. Final run passes. Logs: build/default_021_*.

The inherited wrapper Program-selector scale exception and Windows CPU outliers from 0.2.0 remain documented in the earlier V2 report. Owner FL Studio listening remains pending; the running FL session was not modified.

## Artifacts and identity

Build/source identity: `d9e24f36675b7dc3335a98f1bdf4b2efcfed3bb2934e34d923b62e2833209c18`.
VST3 binary SHA256: `07b376bd78928777347d5b1f2e80b62dfad261e3ccaaf37b45bc6012a0b4bb05`.

Updated VST3: build/HamloEcho_artefacts/Release/VST3/HamloEcho.vst3.
Installer and portable ZIP: dist/HamloEcho_VST3_Setup_0.2.1.exe and dist/HamloEcho-0.2.1-win64-vst3.zip, with SHA256 files. Package inventory and hashes are in package_manifest.json. ZIP entries were decompressed and compared to staging; the installer input binary stayed identical to the validated VST3 before/after ISCC. Installer not executed.

No Verb/HPEQ, subtree or push work is included.
