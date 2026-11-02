# HamloVerb acoustic laboratory: V2-first-full

Fixed protocol in docs/HAMLOVERB_V2.md. 48 kHz stereo, prepare(256), one second of silent settling before impulses. Legacy wet is recovered by subtracting the direct impulse; modern uses Send. Neutral decay disables modulation, uses bass/HF ratios 1, Early -60 dB, cuts 10/20000 Hz. CPU warm-up excludes 200 blocks. NED uses a normalized 25 ms Hann window, 1 ms sampling, threshold .9 relative to wet onset. These numerical checks do not replace listening or the separate reference, preset and validator reports.

| Goal | Case | Measurement | Result |
|---|---|---|---|
| V6 | Clean | duration=300 s, output drift=-0.021 dB, state drift=0.000 dB, peak=0.1051 | PASS |
| V6 | Clean release | final-second energy=-261.55 dB | PASS |
| V6 | Warm | duration=300 s, output drift=0.171 dB, state drift=0.000 dB, peak=0.0901 | PASS |
| V6 | Warm release | final-second energy=-258.02 dB | PASS |
| V6 | Velvet | duration=300 s, output drift=-0.024 dB, state drift=0.000 dB, peak=0.0888 | PASS |
| V6 | Velvet release | final-second energy=-250.56 dB | PASS |
| V6 | Crisp | duration=300 s, output drift=0.015 dB, state drift=0.000 dB, peak=0.1255 | PASS |
| V6 | Crisp release | final-second energy=-243.34 dB | PASS |
| V6 | Deep | duration=300 s, output drift=0.026 dB, state drift=0.000 dB, peak=0.0812 | PASS |
| V6 | Deep release | final-second energy=-263.95 dB | PASS |
| V6 | Bloom | duration=300 s, output drift=0.033 dB, state drift=0.000 dB, peak=0.1124 | PASS |
| V6 | Bloom release | final-second energy=-260.56 dB | PASS |

**12 checks passed; 0 failed.** INFO/n/a rows are not passes. Section: `freeze`.
