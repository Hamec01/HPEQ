# HamloRedWall lab: after the presets pass (colour stage starts full)

8 / 10 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| R1 | Sample peak, drums +18 dB, every style, true peak on / off, DRIVE / TUBE 0 and 100 % | -1.000 dBFS | <= -1 dBFS (LIMIT) | yes |
| R2 | True peak (8x), pushed 9 dB, mixes + drum bus, TRUE PEAK on | -0.916 dBTP | <= -0.9 dBTP (LIMIT + 0.1) | yes |
| R3 | 50 Hz sine at 6 dB GR (LOUDNESS -17.0), AUTO, GLASNOST: THD | 0.231 % | <= 0.3 % | yes |
| R4 | 60 Hz + 3 kHz at 6 dB GR: the largest sideband 3 kHz +- 60 / 120 Hz re 3 kHz | -57.8 dB | <= -40 dB | yes |
| R5 | 1 kHz tone under kicks, pushed 6 dB: deepest dip; time back within 1 dB | -7.6 dB; 65 ms | no worse than before | yes |
| R6 | The 3 mixes, LOUDNESS -6, LIMIT -1 dBTP: integrated loudness gain | 3.95 LU | >= +4.5 LU | **no** |
| R7 | DRIVE 25 / 50 / 100 %, 1 kHz at -12 dBFS: THD (at 100 %: H2 / H3) | 1.33 / 2.47 / 5.42 % (-224.3 / -25.3 dB) | rising, 2 .. 10 % at 100 %, H3 > H2 | yes |
| R8 | DRIVE 100 %, 9 kHz at -6 dBFS: the largest aliased harmonic re the fundamental | -109.4 dB | <= -80 dB | yes |
| R9 | TUBE 100 %, 1 kHz at -12 dBFS: THD; H2 / H3 | 2.28 %; -33.1 / -46.5 dB | H2 > H3, 0.5 .. 5 % | yes |
| R10 | TUBE 100 %, LOUDNESS 0, LIMIT 0: crest factor lowered (mixes / drum bus); peak not raised | 2.0 / -0.7 / 0.1 / 3.6 dB | >= 2 dB, peak <= input + 0.1 | **no** |
