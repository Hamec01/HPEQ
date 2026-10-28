# HamloRedWall lab: before (RedWall as in HPEQ)

4 / 10 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| R1 | Sample peak, drums +18 dB, every style, true peak on / off, DRIVE / TUBE 0 and 100 % | -1.000 dBFS | <= -1 dBFS (LIMIT) | yes |
| R2 | True peak (8x), pushed 9 dB, mixes + drum bus, TRUE PEAK on | -0.885 dBTP | <= -0.9 dBTP (LIMIT + 0.1) | **no** |
| R3 | 50 Hz sine at 6 dB GR (LOUDNESS -17.0), AUTO, GLASNOST: THD | 0.231 % | <= 0.3 % | yes |
| R4 | 60 Hz + 3 kHz at 6 dB GR: the largest sideband 3 kHz +- 60 / 120 Hz re 3 kHz | -57.7 dB | <= -40 dB | yes |
| R5 | 1 kHz tone under kicks, pushed 6 dB: deepest dip; time back within 1 dB | -7.6 dB; 65 ms | no worse than before | yes |
| R6 | The 3 mixes, LOUDNESS -6, LIMIT -1 dBTP: integrated loudness gain | 3.95 LU | >= +4.5 LU | **no** |
| R7 | DRIVE: harmonics | n/a (no DRIVE) | rising, 2 .. 10 % at 100 %, H3 > H2 | **no** |
| R8 | DRIVE: aliasing | n/a (no DRIVE) | <= -80 dB | **no** |
| R9 | TUBE: harmonics | n/a (no TUBE) | H2 > H3, 0.5 .. 5 % | **no** |
| R10 | TUBE: density | n/a (no TUBE) | >= 2 dB, peak <= input + 0.1 | **no** |
