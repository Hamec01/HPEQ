# M0: HamloRedWall extracted from HPEQ into its own repository

Date 2026-10-28. Source: HPEQ `b277315`. Owner: "now RedWall - it must be better than the other limiters". This
stage only moves it, with the sound proven unchanged; the improvement pass follows.

| Part | From HPEQ | Here |
|---|---|---|
| DSP | `RedWallCore` (HamloRedWall.*), `LimiterProcessor` (ProcessorsM6) | the same code; `LimiterSettings` (10 slot values) |
| Plug-in | `ModulePluginProcessor` + one-slot `LayerEngine` | `HamloRedWallProcessor`: the limiter directly, the same fixed latency (10 ms + 8 = 488 samples at 48 kHz), MIX, ON |
| Parameters, state, identity | makeRedWall, BBLB / HmRw / com.hamloprod.hamloredwall, tag `HamloRedWall` | the same |
| Page, presets | `HamloRedWallPanel`, `kLimiter` (6) | the same ([panel.png](panel.png)) |

## Same sound as HPEQ's HamloRedWall (`HamloRedWall_Reference`, limit -120 dB)

2 signals x 24 settings (Default, 6 presets, 3 styles x TRUE PEAK x AUTO release pushed 6 dB, link 0.3 / release
15 ms / lookahead 0.5 ms, lookahead 10 ms / release 400 ms, MONITOR unity / delta, OUTPUT -4 with MIX 0.6) x 48 /
44.1 kHz x blocks 512 / 97: **192 cases, 184 bit-identical, worst -157.6 dB** ([reference_check.md](reference_check.md)).

### Disclosed

1. The first run differed by up to -1.2 dB: a bug in the check, not in the limiter. RedWall has two parameters named
   "Release" (the time and Manual / Auto); the tool matched hosted parameters by name and set both from the second, so
   the reference ran with a 1000 ms release. Now matched by name and occurrence.
2. One new test was wrong as written: the true-peak check with a 0.97 fs / 4 sine, whose inter-sample overs were too
   small; now fs / 4 at 45 degrees (samples at 0.707 of the peak): TRUE PEAK on -1.00 dBTP, off +2.10 dBTP (ceiling -1).

## Checks

`HamloRedWall_Tests` 144 / 144 (M6 path, ceiling over every style / true peak / auto / link on drums +18 dB, true peak,
LINK 0, OUTPUT, block sizes; plug-in: parameters, fixed latency, MIX / ON / quiet = delayed input, pushed 9 dB stays
under -1 dB, HPEQ state, host blocks, mono, programs, page). pluginval v1.0.4 strictness 5 .. 10 (10 with Steinberg's
validator): SUCCESS; Steinberg validator `-e`: 537 / 537.
