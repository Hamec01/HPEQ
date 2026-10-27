# HamloUdarnik lab: before (the HPEQ laws, engine CLASSIC)

7 / 25 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| U1 | SNAP +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 3.0 / 3.3 / 3.0 / 3.3 / 3.2 dB | >= +8 dB | **no** |
| U1 | SNAP +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 4.4 / 8.0 / 14.6 / 15.1 / 7.5 dB | >= +8 dB | **no** |
| U1 | SNAP +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.0 / 12.0 / 10.9 / 11.6 / 7.4 dB | >= +8 dB | **no** |
| U2 | SNAP +6 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 1.7 / 1.9 / 1.8 / 2.0 / 1.8 dB | >= +4 dB | **no** |
| U2 | SNAP +6 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 3.6 / 5.8 / 10.3 / 11.1 / 4.6 dB | >= +4 dB | **no** |
| U2 | SNAP +6 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 4.0 / 6.0 / 5.5 / 5.8 / 3.7 dB | >= +4 dB | **no** |
| U3 | SNAP -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -4.5 / -6.3 / -6.0 / -6.5 / -5.3 dB | <= -8 dB | **no** |
| U3 | SNAP -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -9.7 / -13.9 / -13.2 / -14.1 / -8.0 dB | <= -8 dB | yes |
| U3 | SNAP -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -8.0 / -12.0 / -10.9 / -11.6 / -7.2 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -0.1 / -0.1 / -0.2 / -0.1 / -0.0 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -0.2 / -0.1 / -0.5 / -0.3 / -0.0 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -0.2 / -0.1 / -0.5 / -0.3 / -0.0 dB | <= -8 dB | **no** |
| U4 | BODY +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 0.7 / 0.4 / 0.9 / 0.3 / 0.0 dB | >= +8 dB | **no** |
| U4 | BODY +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 0.9 / 0.5 / 1.2 / 0.8 / 0.0 dB | >= +8 dB | **no** |
| U4 | BODY +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 1.0 / 0.5 / 1.2 / 0.9 / 0.0 dB | >= +8 dB | **no** |
| U5 | Level independence: drum bus at -6 vs -30 dBFS RMS, SNAP +9 (PRAVDA): contrast | 2.58 vs 2.58 dB | within 1 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PRAVDA: level change; residual | 2.28 dB; -10.4 dB | <= 0.5 dB; <= -50 dB | **no** |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, UDARNIK: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PERESTROIKA: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PRAVDA: output peak | 11.00 dBFS | <= 0 dBFS | **no** |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, UDARNIK: output peak | -2.20 dBFS | <= 0 dBFS | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PERESTROIKA: output peak | 11.00 dBFS | <= 0 dBFS | **no** |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PRAVDA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, UDARNIK: output - input - latency | -1.312 ms | within 0.1 ms | **no** |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PERESTROIKA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
