# HamloComp lab: before (today's compressor: MODEL and SOURCE not implemented)

5 / 13 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| T2 | VCA static gain at -10 dBFS (square, -30 dB, 4:1) | -15.00 dB (law -15.00) | within 0.5 dB | yes |
| T3 | VCA GR left 300 ms after a 3 s vs a 30 ms burst (release 100 ms) | 0.90 vs 0.90 dB | long >= 2 x short | **no** |
| T4 | VCA THD at 6 dB GR | 0.022 % (H2 -240.0, H3 -73.4 dB) | <= 0.05 % | yes |
| T5 | FET attack 0.1 ms: time to 90 % of the GR (+20 dB step) | 0.23 ms | <= 2 ms | yes |
| T6 | FET THD at 6 dB GR | 0.022 % (H2 -240.0, H3 -73.4 dB) | 0.3 .. 3 %, H2 and H3 >= -70 dB | **no** |
| T7 | OPTO 30 ms burst: time to recover 50 % (release 60 ms) | 41.5833 ms (GR 17.1 dB) | 40 .. 120 ms | yes |
| T8 | OPTO 3 s burst: time to recover 90 %, vs after 30 ms | 0.14 s vs 0.14 s | >= 1 s and >= 4 x | **no** |
| T9 | OPTO attack 10 ms: time to 63 % of the GR | 9.9 ms | 5 .. 20 ms | yes |
| T10 | TUBE effective ratio 4 dB vs 20 dB over the threshold (ratio 8) | 8.00 vs 8.00 | low <= 0.6 x high | **no** |
| T11 | TUBE colour at 6 dB GR | 0.024 % (H2 -240.0, H3 -72.7 dB) | H2 >= H3 + 6 dB, 0.2 .. 3 % | **no** |
| T12 | DRUMS: GR from 50 Hz vs 1 kHz (-10 dBFS) | 14.6 vs 14.6 dB | 50 Hz >= 8 dB less | **no** |
| T13 | VOC: GR from 3 kHz / 1 kHz / 80 Hz | 14.5 / 14.6 / 14.6 dB | 3 kHz >= 2 dB more; 80 Hz >= 6 dB less | **no** |
| T14 | SAMPLES: GR from 30 Hz vs 1 kHz | 14.6 vs 14.6 dB | 30 Hz >= 4 dB less | **no** |
