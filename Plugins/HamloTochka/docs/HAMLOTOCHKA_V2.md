# HamloTochka graph and factory library

The graph follows the documented conventions for choosing Q with the wheel,
adjusting a cut slope in steps, and using modifier keys for precision or range.
[FabFilter display and workflow](https://prod.fabfilter.com/help/pro-q/using/eqdisplay)

| Area | Acceptance |
| --- | --- |
| Dynamic range | Selected eligible band shows the area between static response and the signed range limit. Other dynamic bands show their range handles. The amber curve follows the measured detector offset. |
| Graph editing | Wheel changes Q in both directions and keeps it between 0.1 and 18. Shift applies one fifth of the usual change. High/low cut uses its eight discrete slopes. Alt-wheel edits dynamic range. Ctrl-wheel edits gain and Ctrl-drag edits Q. Pointer and range-handle gestures publish automation. |
| Difference spectrum | Display measured output minus input on the dB scale, sign-distinguished. Omit bins at or below −80 dBFS input. A bin-centred −6 dB bell reads −6 ±0.15 dB and silence remains masked. Saved projects and editor reopen preserve the display mode. |
| Factory library | Append 200 named recipes, 50 per genre across the existing ten instrument groups. Keep the first ten program indices. At 48 kHz, each recipe loads and produces finite output on synthetic material. Dynamic bell presets follow their signed range when measured with a steady centre-frequency probe. |
| Compatibility | Neutral and existing 0.2 renders compare bit-for-bit. Preserve sidechain layout, zero reported latency and the 24 band slots. |

Recipe checks use synthetic material. Song-specific thresholds, frequency
choices, and listening comparisons require programme material.
