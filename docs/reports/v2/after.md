# HamloUdarnik lab: after (v2 engine)

9 / 25 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| U1 | SNAP +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 8.0 / 11.8 / 10.9 / 10.7 / 3.2 dB | >= +8 dB | **no** |
| U1 | SNAP +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 6.1 / 9.2 / 10.6 / 10.4 / 4.2 dB | >= +8 dB | **no** |
| U1 | SNAP +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.0 / 11.8 / 10.9 / 10.7 / 3.2 dB | >= +8 dB | **no** |
| U2 | SNAP +6 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 4.0 / 5.9 / 5.4 / 5.3 / 2.2 dB | >= +4 dB | **no** |
| U2 | SNAP +6 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 3.5 / 5.4 / 6.3 / 6.0 / 2.8 dB | >= +4 dB | **no** |
| U2 | SNAP +6 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 4.0 / 5.9 / 5.4 / 5.3 / 2.2 dB | >= +4 dB | **no** |
| U3 | SNAP -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -7.9 / -11.6 / -10.4 / -10.2 / -6.9 dB | <= -8 dB | **no** |
| U3 | SNAP -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -7.9 / -11.6 / -10.4 / -10.2 / -6.9 dB | <= -8 dB | **no** |
| U3 | SNAP -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -7.9 / -11.6 / -10.4 / -10.2 / -6.9 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -8.1 / -11.9 / -11.0 / -10.8 / -3.1 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -8.1 / -11.9 / -11.0 / -10.8 / -3.1 dB | <= -8 dB | **no** |
| U4 | BODY -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -8.1 / -11.9 / -11.0 / -10.8 / -3.1 dB | <= -8 dB | **no** |
| U4 | BODY +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 8.0 / 11.8 / 10.5 / 10.4 / 6.9 dB | >= +8 dB | **no** |
| U4 | BODY +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 8.0 / 11.9 / 10.5 / 10.4 / 6.9 dB | >= +8 dB | **no** |
| U4 | BODY +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.0 / 11.8 / 10.5 / 10.4 / 6.9 dB | >= +8 dB | **no** |
| U5 | Level independence: drum bus at -6 vs -30 dBFS RMS, SNAP +9 (PRAVDA): contrast | 2.77 vs 2.77 dB | within 1 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PRAVDA: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, UDARNIK: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PERESTROIKA: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PRAVDA: output peak | 0.00 dBFS | <= 0 dBFS | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, UDARNIK: output peak | -1.00 dBFS | <= 0 dBFS | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PERESTROIKA: output peak | 0.00 dBFS | <= 0 dBFS | yes |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PRAVDA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, UDARNIK: output - input - latency | -1.167 ms | within 0.1 ms | **no** |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PERESTROIKA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
