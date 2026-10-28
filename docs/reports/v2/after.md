# HamloUdarnik lab: after (v2 engine)

24 / 25 checks pass

| # | Check | Measured | Target | Pass |
|---|---|---|---|---|
| U1 | SNAP +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 8.1 / 12.0 / 12.0 / 10.8 / 12.0 dB | >= +8 dB | yes |
| U1 | SNAP +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 8.9 / 11.9 / 12.3 / 10.8 / 12.4 dB | >= +8 dB | yes |
| U1 | SNAP +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.1 / 12.0 / 12.0 / 10.8 / 12.0 dB | >= +8 dB | yes |
| U2 | SNAP +6 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 4.0 / 6.0 / 6.0 / 5.6 / 6.0 dB | >= +4 dB | yes |
| U2 | SNAP +6 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 4.5 / 6.0 / 6.3 / 5.7 / 6.2 dB | >= +4 dB | yes |
| U2 | SNAP +6 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 4.0 / 6.0 / 6.0 / 5.6 / 6.0 dB | >= +4 dB | yes |
| U3 | SNAP -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB | yes |
| U3 | SNAP -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB | yes |
| U3 | SNAP -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB | yes |
| U4 | BODY -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB | yes |
| U4 | BODY -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB | yes |
| U4 | BODY -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB | yes |
| U4 | BODY +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB | yes |
| U4 | BODY +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB | yes |
| U4 | BODY +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB | yes |
| U5 | Level independence: drum bus at -6 vs -30 dBFS RMS, SNAP +9 (PRAVDA): contrast | 9.00 vs 9.00 dB | within 1 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PRAVDA: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, UDARNIK: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PERESTROIKA: level change; residual | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PRAVDA: output peak | 0.00 dBFS | <= 0 dBFS | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, UDARNIK: output peak | -1.00 dBFS | <= 0 dBFS | yes |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PERESTROIKA: output peak | 0.00 dBFS | <= 0 dBFS | yes |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PRAVDA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, UDARNIK: output - input - latency | -1.167 ms | within 0.1 ms | **no** |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PERESTROIKA: output - input - latency | 0.000 ms | within 0.1 ms | yes |
