# HamloVerb0.2.2 - explicit wet return level

2026-11-04. Parent115db13. Owner reports that the reverb itself is too quiet.
Screenshot:TRAP Memphis Reverse Horizon/Bloom/Reverse Audio, Mix0.7906208,
Early-9dB,Tail0dB,Output0dB,Duck0.08. No exact FL source clip/state supplied.

## Diagnosis

WET-40.7dB in the screenshot is an absolute RMS meter, not a wet/dry ratio.
KEEP DRY affects the direct source, not the return. Existing Early/Tail controls
only boost their respective components, while OUTPUT also raises dry. There was
no single exposed overall return trim.

The unchanged WetLevelTests measured actual Send output including8s of tail on
the first8s of owner's keys2, normalized to-18dBFS RMS subject to-3dBFS peak.
Source SHA256:21a33d4574e2724773a5d500d77c0b9c1261fc5bcb08cdf5d510dc126d0b55b1.
Default lab transport120BPM; this preset uses free predelay/shape times, matching
the displayed3.642857s decay and857.142857ms fragments without tempo scaling.

| Condition | Full wet/dry energy ratio |
|---|---:|
| Screenshot recipe, Time Shape off | -11.552388dB |
| Screenshot recipe, Reverse Audio | -12.540777dB |
| Same Reverse return with WET LEVEL+12 | -0.540777dB (gain identity) |

Reverse adds0.988389dB loss on this material, rather than a major silent-path
failure. Six neutral characters measured-3.150844..-7.786163dB. The owner's exact
FL clip may differ; this measurement does not prove every source has this ratio.
No Reverse normalization/FDN injection/decay/preset recipe was changed.

## Change

Appended wetDb(-24..+24dB,default0) after KEEP DRY. A20ms sample-clocked gain
ramp trims the wet output after temporal effects/EQ/Duck/Width, before Insert/
Send mixing. It does not feed the tank, Shimmer, Reverse buffer or Freeze loop.
At unity the old float arithmetic is retained. Legacy ignores the control.

Main's final knob is now WET LEVEL; SHIMMER remains in Advanced/Effects. OUTPUT
still controls total output. Meter now measures the trimmed return before MIX/
Output/bypass. Factory preset loads reset trim0; edited-state marker and project
save/restore include it. Loading old state on an edited instance resets trim0.

Suggested use:modern character, Insert, KEEP DRY on, OUTPUT0, WET LEVEL+6/+12dB,
then set MIX to taste. Raising wet gain raises wet peaks. This is deliberate
manual gain, not automatic makeup, normalization, loudness matching or limiting.

## Acceptance

- 1634 native expectations,0 failures, including48 measured gain combinations
  across six characters, Reverse on/off and-24/+6/+12/+24dB, within0.01dB.
- Insert additive identity within2e-6/sample; ordinary Insert dry coefficient,
  Output and bypass verified. Predelay dry,MIX0 and Legacy unchanged.
- Wet meter includes trim within0.001dB; Freeze output scales without changing
  tank timing/tail contract. Core Freeze algorithms are untouched.
- Mono/stereo automation at exact sample positions:blocks1/64/127/512/4096
  match reference256 bit-for-bit; zero latency. Allocation audit enables+12dB.
- 208/208 old program PCM hashes exact vs0.2.1;390/390 M0 cases exact.
- pluginval5..10 exit0;Steinberg537passed,0failed on the delivered DLL.
- Main(+12dB),Effects andLegacy screenshots reviewed:no overlaps;Shimmer
  remains accessible,Legacy WET LEVEL disabled. Shared PresetScreen unchanged.

Before/after method:WetLevelTests.cpp SHA256
86bb6e554c53a432874da03834802426c8bc86ebc02ea47380c16e49ee19d0d2.
Parameter-absent baseline reportsn/a for the new control. Its six neutral and
two screenshot diagnostics remain identical after the change. Supplementary
WetLevelWorkflowTests verifies UI workflow/state/Freeze/meter and produces
optional private auditions; it is not represented as a before/after instrument.
No failed acceptance thresholds or methods were adjusted after measurements.
The first isolated Python packaging invocation could not import its sibling
verifier; an explicit local tool directory fixes isolated-mode loading. It
failed before creating any package/receipt; no sound, method or goal changed.

## Auditions and delivery

Private owner-material pairs inout/wetlevel_demo_final:first0dB,0.4s silence,
then+12dB. Each pair has one common gain, not independent loudness normalization.
Insert/KEEP DRY common gain0.706268787;Send1. Independent scipy8x true peaks:
Insert-6.014821dBTP,Send-7.118709dBTP. Send measured gain12.000000018dB;
Insert total-energy increase1.764050dB because dry remains the same.
Audition source/common-gain/hash records:auditions.json.

VST3,portableZIP,SHA256:dist/WetLevel0.2.2. No installer,installed-copy overwrite,
private audio in Git or new GitHub push. Original200 recipes and their original
balance-law calibration remain at trim0, not a claim that boosted recipes retain
their original depth ranges. Manual FL loading/listening remains for the owner.
No fresh complete acoustic/CPU run claimed for this wrapper-only return trim.
Historical Keep Dry receipts certify their frozen copies; the new acceptance
receipt pins this source,tests,raw reports,tools,baseline,delivery and auditions.
