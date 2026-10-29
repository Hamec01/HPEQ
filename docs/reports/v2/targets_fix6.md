# HamloMag lab: ADAA and full residual MATCH

15 / 15 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| G1 | New controls at their defaults: every model x 1x / 2x / 4x on the drums vs the stored 0.1.0 renders | 18 / 18 bit-identical | bit-identical | yes |
| G2 | Aliasing, TUBE DRIVE 24 dB, 9 kHz at -6 dBFS: largest alias re the tone, 4x / 8x | -47.2 / -105.6 dB | 4x no worse than before; 8x <= -90 dB | yes |
| G3 | SP-1200, 15 kHz at -12 dBFS: alias at 11.04 kHz re the tone | -4.7 dB | >= -30 dB | yes |
| G4 | MPC60 21 kHz -> 19 kHz / S950 18 kHz -> 14 kHz: input alias re the tone (images of 15 kHz: -33.6 / -36.2 dB) | -240.0 / -240.0 dB | <= -50 dB | yes |
| G5 | Response at 1 kHz / 15 kHz: SP-1200; MPC60; S950 at 1 kHz | -0.1 / -14.2; -0.0 / -3.1; -0.0 dB | SP-1200 15 kHz <= -10; MPC60 15 kHz >= -6; 1 kHz within +-1 | yes |
| G6 | 1 kHz at -60 dBFS: THD + N re the tone: off; SP-1200 / MPC60 / S950 | -75.8; -17.6 / -27.4 / -18.1 dB | off < -60; linear 12-bit -6 .. -24; MPC60 -20 .. -40 | yes |
| G7 | PHONE 100 %, 50 Hz at -12 dBFS: H2 + H3 + H4; fundamental change; above 1 kHz (re the fundamental) | -7.6; -0.11; -61.1 dB | >= -12; +-0.5; <= -60 | yes |
| G8 | PHONE 100 %, bass clips through a 200 Hz phone speaker: loudness gain | 4.2 / 13.1 LU | >= +4 LU | yes |
| G9 | SUB 120 Hz, TUBE DRIVE 24 dB, 40 Hz + 1 kHz: THD of the 40 Hz part, off / on | 13.77 / 0.00 % | <= 1 %, >= 10 x lower | yes |
| G10 | SUB 120 Hz, CLASSIC DRIVE 0: magnitude against SUB off, 20 Hz .. 18 kHz | 0.00 dB | within 0.2 dB | yes |
| G11 | DYNAMICS +100 / -100 %, TUBE 12 dB: non-linear energy attack - body, against 0: hits (drum bus 1.7 / -3.1) | 5.7 / -13.1 dB | +100 >= +3; -100 <= -3 | yes |
| G12 | MATCH, the mixes, TAPE / TUBE / GERMANIUM at DRIVE 0 / 12 / 24: absolute loudness out - in (after 3 s) | 0.49 LU | <= 0.5 LU | yes |
| G13 | MATCH, steady pink noise, TUBE 24 dB, after 3 s: gain movement (400 ms windows) | +-0.08 dB | within +-0.3 dB | yes |
| G14 | DELTA + the aligned input against the output without DELTA (TAPE 18 dB, drums) | -148.7 dB | <= -120 dB | yes |
| G15 | Blocks 1 / 64 / 512 / 4096 after 256, every new control there is on | bit-identical | bit-identical | yes |
