# HamloVerb acoustic laboratory: V2-first

Fixed protocol in docs/HAMLOVERB_V2.md. 48 kHz stereo, prepare(256), one second of silent settling before impulses. Legacy wet is recovered by subtracting the direct impulse; modern uses Send. Neutral decay disables modulation, uses bass/HF ratios 1, Early -60 dB, cuts 10/20000 Hz. CPU warm-up excludes 200 blocks. NED uses a normalized 25 ms Hann window, 1 ms sampling, threshold .9 relative to wet onset. These numerical checks do not replace listening or the separate reference, preset and validator reports.

| Goal | Case | Measurement | Result |
|---|---|---|---|
| V3 | Clean decay 0.10 | T30=0.1095 s, T20=0.1144 s, R2=0.9983 | PASS |
| V3 | Clean decay 0.25 | T30=0.2515 s, T20=0.2521 s, R2=1.0000 | PASS |
| V3 | Clean decay 1.00 | T30=0.9966 s, T20=0.9951 s, R2=1.0000 | PASS |
| V3 | Clean decay 2.00 | T30=2.0007 s, T20=2.0007 s, R2=1.0000 | PASS |
| V3 | Clean decay 10.00 | T30=10.0013 s, T20=10.0060 s, R2=1.0000 | PASS |
| V3 | Clean decay 30.00 | T30=29.9980 s, T20=29.9968 s, R2=1.0000 | PASS |
| V3 | Warm artistic decay | T30=2.0070 s, R2=1.0000 | INFO |
| V3 | Velvet artistic decay | T30=2.0114 s, R2=1.0000 | INFO |
| V3 | Crisp artistic decay | T30=2.0195 s, R2=1.0000 | INFO |
| V3 | Deep artistic decay | T30=1.9970 s, R2=1.0000 | INFO |
| V3 | Bloom artistic decay | T30=2.0016 s, R2=1.0000 | INFO |
| V3 | Decay EQ 0 | Broadband T30=2.6298 s | PASS |
| V3 | Decay EQ 1 | Broadband T30=2.6143 s | PASS |
| V3 | Decay EQ 2 | Broadband T30=3.9785 s | PASS |
| V5 | Clean | NED .9 at 17.50 ms after onset | PASS |
| V5 | Crisp | NED .9 at 21.50 ms after onset | PASS |
| V5 | Velvet | NED .9 at 23.50 ms after onset | PASS |
| V4 | Predelay 20 ms | shift=960 samples; expected=960 | PASS |
| V4 | Predelay 250 ms | shift=12000 samples; expected=12000 | PASS |
| V4 | Predelay 500 ms | shift=24000 samples; expected=24000 | PASS |
| V7 | Clean | correlation=-0.0131, mono energy=-3.07 dB; width0 exact=yes | PASS |
| V7 | Warm | correlation=0.0072, mono energy=-2.98 dB; width0 exact=yes | PASS |
| V7 | Velvet | correlation=-0.0100, mono energy=-3.05 dB; width0 exact=yes | PASS |
| V7 | Crisp | correlation=-0.0075, mono energy=-3.04 dB; width0 exact=yes | PASS |
| V7 | Deep | correlation=0.0756, mono energy=-2.69 dB; width0 exact=yes | PASS |
| V7 | Bloom | correlation=-0.0035, mono energy=-3.03 dB; width0 exact=yes | PASS |
| V6 | Clean | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.0943 | INFO |
| V6 | Clean release | final-second energy=-261.51 dB | PASS |
| V6 | Warm | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.0861 | INFO |
| V6 | Warm release | final-second energy=-257.64 dB | PASS |
| V6 | Velvet | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.0722 | INFO |
| V6 | Velvet release | final-second energy=-250.35 dB | PASS |
| V6 | Crisp | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.1122 | INFO |
| V6 | Crisp release | final-second energy=-243.35 dB | PASS |
| V6 | Deep | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.0714 | INFO |
| V6 | Deep release | final-second energy=-263.94 dB | PASS |
| V6 | Bloom | duration=8 s, output drift=0.000 dB, state drift=0.000 dB, peak=0.0996 | INFO |
| V6 | Bloom release | final-second energy=-260.95 dB | PASS |
| V8 | Shape 0 | early=8.4514, rising window=4.8772, residual after400ms=-11.13 dB | PASS |
| V8 | Shape 1 | early=8.4501, rising window=4.8772, residual after400ms=-85.68 dB | PASS |
| V8 | Shape 2 | early=0.0581, rising window=2.0760, residual after400ms=-90.02 dB | PASS |
| V8 | Shape 3 | early=0.0000, rising window=2.9164, residual after400ms=-3.47 dB | PASS |
| V9 | Shimmer max-feedback stress | peak=0.0196, tail energy=0.0000 | PASS |
| V10 | Clean | mean=3.91%, maximum=7.40% | PASS |
| V10 | Warm | mean=4.24%, maximum=9.19% | PASS |
| V10 | Velvet | mean=5.50%, maximum=12.09% | PASS |
| V10 | Crisp | mean=5.08%, maximum=11.21% | PASS |
| V10 | Deep | mean=3.29%, maximum=11.31% | PASS |
| V10 | Bloom | mean=4.17%, maximum=8.87% | PASS |
| V10 | Two-path transition | mean=10.54% | PASS |

**39 checks passed; 0 failed.** INFO/n/a rows are not passes. Section: `quick`.
