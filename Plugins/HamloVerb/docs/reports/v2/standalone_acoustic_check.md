# HamloVerb acoustic laboratory: standalone

Fixed protocol in docs/HAMLOVERB_V2.md. 48 kHz stereo, prepare(256), one second of silent settling before impulses. Legacy wet is recovered by subtracting the direct impulse; modern uses Send. Neutral decay disables modulation, uses bass/HF ratios 1, Early -60 dB, cuts 10/20000 Hz. CPU warm-up excludes 200 blocks. NED uses a normalized 25 ms Hann window, 1 ms sampling, threshold .9 relative to wet onset. These numerical checks do not replace listening or the separate reference, preset and validator reports.

| Goal | Case | Measurement | Result |
|---|---|---|---|
| V3 | Legacy Hall decay 0.10 | T30=0.1011 s, T20=0.1423 s, R2=0.9422 | INFO |
| V3 | Legacy Hall decay 0.25 | T30=0.2019 s, T20=0.1574 s, R2=0.9685 | INFO |
| V3 | Legacy Hall decay 1.00 | T30=0.9987 s, T20=0.9962 s, R2=1.0000 | INFO |
| V3 | Legacy Hall decay 2.00 | T30=2.0033 s, T20=1.9982 s, R2=1.0000 | INFO |
| V3 | Legacy Hall decay 10.00 | T30=10.0075 s, T20=9.9956 s, R2=1.0000 | INFO |
| V3 | Legacy Hall decay 30.00 | T30=30.0317 s, T20=30.0185 s, R2=1.0000 | INFO |
| V5 | Legacy Hall | NED .9 at 97.50 ms after onset | INFO |
| V4 | Predelay 20 ms | shift=960 samples; expected=960 | PASS |
| V4 | Predelay 250 ms | shift=12000 samples; expected=12000 | PASS |
| V4 | Predelay 500 ms | shift=24000 samples; expected=24000 | PASS |
| V7 | Legacy | correlation=-0.0178, mono energy=-3.09 dB; width0 exact=yes | PASS |
| V6 | New Freeze | n/a: V2 engine absent | INFO |
| V8/V9 | Time shapes and Shimmer | n/a: V2 parameters absent | INFO |
| V10 | Legacy | mean=1.87%, maximum=13.82% | INFO |

**4 checks passed; 0 failed.** INFO/n/a rows are not passes. Section: `all`.
