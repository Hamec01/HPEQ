# HamloVerb acoustic laboratory: V2-isolated

Fixed protocol in docs/HAMLOVERB_V2.md. 48 kHz stereo, prepare(256), one second of silent settling before impulses. Legacy wet is recovered by subtracting the direct impulse; modern uses Send. Neutral decay disables modulation, uses bass/HF ratios 1, Early -60 dB, cuts 10/20000 Hz. CPU warm-up excludes 200 blocks. NED uses a normalized 25 ms Hann window, 1 ms sampling, threshold .9 relative to wet onset. These numerical checks do not replace listening or the separate reference, preset and validator reports.

| Goal | Case | Measurement | Result |
|---|---|---|---|
| V10 | Clean | mean=3.51%, maximum=9.58% | PASS |
| V10 | Warm | mean=4.14%, maximum=9.75% | PASS |
| V10 | Velvet | mean=4.91%, maximum=41.58% | PASS |
| V10 | Crisp | mean=4.38%, maximum=17.10% | PASS |
| V10 | Deep | mean=2.91%, maximum=6.20% | PASS |
| V10 | Bloom | mean=3.62%, maximum=7.16% | PASS |
| V10 | Two-path transition | mean=8.89% | PASS |

**7 checks passed; 0 failed.** INFO/n/a rows are not passes. Section: `cpu`.
