# HamloRedWall vs reference

Reference: `E:/HPEQ/build/HamloModules/Release/HamloRedWall.vst3`

192 cases: 184 bit-identical, worst difference -157.6 dB (limit -120 dB), 0 over the limit

| Rate / block | Signal | Case | Bit-identical from 0.5 s | Difference from 0.5 s | First 0.5 s (start-up, not judged) |
|---|---|---|---|---|---|
| 48.0 kHz / 512 | drums | Default | yes | -inf | -400.0 dB |
| 48.0 kHz / 512 | drums | Preset Safe Ceiling | yes | -inf | -26.9 dB |
| 48.0 kHz / 512 | drums | Preset Streaming -1 dBTP | yes | -inf | -24.7 dB |
| 48.0 kHz / 512 | drums | Preset Loud Master | yes | -inf | -19.7 dB |
| 48.0 kHz / 512 | drums | Preset Soft Clip Push | yes | -inf | -19.8 dB |
| 48.0 kHz / 512 | drums | Preset Drum Peak Catcher | yes | -inf | -19.7 dB |
| 48.0 kHz / 512 | drums | Preset Broadcast -2 dB | yes | -inf | -36.5 dB |
| 48.0 kHz / 512 | drums | Style 0, true peak 0, auto 0 | yes | -inf | -24.9 dB |
| 48.0 kHz / 512 | drums | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 512 | drums | Style 0, true peak 1, auto 0 | yes | -inf | -25.9 dB |
| 48.0 kHz / 512 | drums | Style 0, true peak 1, auto 1 | yes | -inf | -25.5 dB |
| 48.0 kHz / 512 | drums | Style 1, true peak 0, auto 0 | yes | -inf | -22.9 dB |
| 48.0 kHz / 512 | drums | Style 1, true peak 0, auto 1 | yes | -inf | -22.1 dB |
| 48.0 kHz / 512 | drums | Style 1, true peak 1, auto 0 | yes | -inf | -23.6 dB |
| 48.0 kHz / 512 | drums | Style 1, true peak 1, auto 1 | yes | -inf | -22.6 dB |
| 48.0 kHz / 512 | drums | Style 2, true peak 0, auto 0 | yes | -inf | -19.3 dB |
| 48.0 kHz / 512 | drums | Style 2, true peak 0, auto 1 | yes | -inf | -19.3 dB |
| 48.0 kHz / 512 | drums | Style 2, true peak 1, auto 0 | yes | -inf | -19.7 dB |
| 48.0 kHz / 512 | drums | Style 2, true peak 1, auto 1 | yes | -inf | -19.7 dB |
| 48.0 kHz / 512 | drums | Link 0.3, release 15 ms, lookahead 0.5 ms | no | -157.6 dB | -22.2 dB |
| 48.0 kHz / 512 | drums | Lookahead 10 ms, release 400 ms | yes | -inf | -31.4 dB |
| 48.0 kHz / 512 | drums | Monitor unity | no | -160.8 dB | -14.9 dB |
| 48.0 kHz / 512 | drums | Monitor delta | yes | -inf | -15.9 dB |
| 48.0 kHz / 512 | drums | Output -4, mix 0.6 | yes | -inf | -24.9 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Default | yes | -inf | -400.0 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Safe Ceiling | yes | -inf | -36.9 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Streaming -1 dBTP | yes | -inf | -28.2 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Loud Master | yes | -inf | -23.6 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Soft Clip Push | yes | -inf | -28.0 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Drum Peak Catcher | yes | -inf | -27.5 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Preset Broadcast -2 dB | yes | -inf | -31.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 0, true peak 0, auto 0 | yes | -inf | -24.6 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 0, true peak 1, auto 0 | yes | -inf | -24.7 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 0, true peak 1, auto 1 | yes | -inf | -24.7 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 1, true peak 0, auto 0 | yes | -inf | -24.5 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 1, true peak 0, auto 1 | yes | -inf | -24.5 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 1, true peak 1, auto 0 | yes | -inf | -24.6 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 1, true peak 1, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 2, true peak 0, auto 0 | yes | -inf | -25.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 2, true peak 0, auto 1 | yes | -inf | -25.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 2, true peak 1, auto 0 | yes | -inf | -25.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Style 2, true peak 1, auto 1 | yes | -inf | -25.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Link 0.3, release 15 ms, lookahead 0.5 ms | yes | -inf | -23.8 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Lookahead 10 ms, release 400 ms | yes | -inf | -32.3 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Monitor unity | yes | -inf | -36.0 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Monitor delta | yes | -inf | -7.9 dB |
| 48.0 kHz / 512 | sine -6 dBFS | Output -4, mix 0.6 | yes | -inf | -40.8 dB |
| 48.0 kHz / 97 | drums | Default | yes | -inf | -400.0 dB |
| 48.0 kHz / 97 | drums | Preset Safe Ceiling | yes | -inf | -26.9 dB |
| 48.0 kHz / 97 | drums | Preset Streaming -1 dBTP | yes | -inf | -24.7 dB |
| 48.0 kHz / 97 | drums | Preset Loud Master | yes | -inf | -19.7 dB |
| 48.0 kHz / 97 | drums | Preset Soft Clip Push | yes | -inf | -19.8 dB |
| 48.0 kHz / 97 | drums | Preset Drum Peak Catcher | yes | -inf | -19.7 dB |
| 48.0 kHz / 97 | drums | Preset Broadcast -2 dB | yes | -inf | -36.5 dB |
| 48.0 kHz / 97 | drums | Style 0, true peak 0, auto 0 | yes | -inf | -24.9 dB |
| 48.0 kHz / 97 | drums | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 97 | drums | Style 0, true peak 1, auto 0 | yes | -inf | -25.9 dB |
| 48.0 kHz / 97 | drums | Style 0, true peak 1, auto 1 | yes | -inf | -25.5 dB |
| 48.0 kHz / 97 | drums | Style 1, true peak 0, auto 0 | yes | -inf | -22.9 dB |
| 48.0 kHz / 97 | drums | Style 1, true peak 0, auto 1 | yes | -inf | -22.1 dB |
| 48.0 kHz / 97 | drums | Style 1, true peak 1, auto 0 | yes | -inf | -23.6 dB |
| 48.0 kHz / 97 | drums | Style 1, true peak 1, auto 1 | yes | -inf | -22.6 dB |
| 48.0 kHz / 97 | drums | Style 2, true peak 0, auto 0 | yes | -inf | -19.3 dB |
| 48.0 kHz / 97 | drums | Style 2, true peak 0, auto 1 | yes | -inf | -19.3 dB |
| 48.0 kHz / 97 | drums | Style 2, true peak 1, auto 0 | yes | -inf | -19.7 dB |
| 48.0 kHz / 97 | drums | Style 2, true peak 1, auto 1 | yes | -inf | -19.7 dB |
| 48.0 kHz / 97 | drums | Link 0.3, release 15 ms, lookahead 0.5 ms | no | -157.6 dB | -22.2 dB |
| 48.0 kHz / 97 | drums | Lookahead 10 ms, release 400 ms | yes | -inf | -31.4 dB |
| 48.0 kHz / 97 | drums | Monitor unity | no | -160.8 dB | -14.9 dB |
| 48.0 kHz / 97 | drums | Monitor delta | yes | -inf | -15.9 dB |
| 48.0 kHz / 97 | drums | Output -4, mix 0.6 | yes | -inf | -24.9 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Default | yes | -inf | -400.0 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Safe Ceiling | yes | -inf | -36.9 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Streaming -1 dBTP | yes | -inf | -28.2 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Loud Master | yes | -inf | -23.6 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Soft Clip Push | yes | -inf | -28.0 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Drum Peak Catcher | yes | -inf | -27.5 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Preset Broadcast -2 dB | yes | -inf | -31.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 0, true peak 0, auto 0 | yes | -inf | -24.6 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 0, true peak 1, auto 0 | yes | -inf | -24.7 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 0, true peak 1, auto 1 | yes | -inf | -24.7 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 1, true peak 0, auto 0 | yes | -inf | -24.5 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 1, true peak 0, auto 1 | yes | -inf | -24.5 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 1, true peak 1, auto 0 | yes | -inf | -24.6 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 1, true peak 1, auto 1 | yes | -inf | -24.6 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 2, true peak 0, auto 0 | yes | -inf | -25.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 2, true peak 0, auto 1 | yes | -inf | -25.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 2, true peak 1, auto 0 | yes | -inf | -25.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Style 2, true peak 1, auto 1 | yes | -inf | -25.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Link 0.3, release 15 ms, lookahead 0.5 ms | yes | -inf | -23.8 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Lookahead 10 ms, release 400 ms | yes | -inf | -32.3 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Monitor unity | yes | -inf | -36.0 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Monitor delta | yes | -inf | -7.9 dB |
| 48.0 kHz / 97 | sine -6 dBFS | Output -4, mix 0.6 | yes | -inf | -40.8 dB |
| 44.1 kHz / 512 | drums | Default | yes | -inf | -400.0 dB |
| 44.1 kHz / 512 | drums | Preset Safe Ceiling | yes | -inf | -27.2 dB |
| 44.1 kHz / 512 | drums | Preset Streaming -1 dBTP | yes | -inf | -25.0 dB |
| 44.1 kHz / 512 | drums | Preset Loud Master | yes | -inf | -20.1 dB |
| 44.1 kHz / 512 | drums | Preset Soft Clip Push | yes | -inf | -20.2 dB |
| 44.1 kHz / 512 | drums | Preset Drum Peak Catcher | yes | -inf | -20.0 dB |
| 44.1 kHz / 512 | drums | Preset Broadcast -2 dB | yes | -inf | -36.3 dB |
| 44.1 kHz / 512 | drums | Style 0, true peak 0, auto 0 | yes | -inf | -25.1 dB |
| 44.1 kHz / 512 | drums | Style 0, true peak 0, auto 1 | yes | -inf | -24.9 dB |
| 44.1 kHz / 512 | drums | Style 0, true peak 1, auto 0 | yes | -inf | -26.2 dB |
| 44.1 kHz / 512 | drums | Style 0, true peak 1, auto 1 | yes | -inf | -25.8 dB |
| 44.1 kHz / 512 | drums | Style 1, true peak 0, auto 0 | yes | -inf | -23.2 dB |
| 44.1 kHz / 512 | drums | Style 1, true peak 0, auto 1 | yes | -inf | -22.4 dB |
| 44.1 kHz / 512 | drums | Style 1, true peak 1, auto 0 | yes | -inf | -23.8 dB |
| 44.1 kHz / 512 | drums | Style 1, true peak 1, auto 1 | yes | -inf | -22.9 dB |
| 44.1 kHz / 512 | drums | Style 2, true peak 0, auto 0 | yes | -inf | -19.7 dB |
| 44.1 kHz / 512 | drums | Style 2, true peak 0, auto 1 | yes | -inf | -19.7 dB |
| 44.1 kHz / 512 | drums | Style 2, true peak 1, auto 0 | yes | -inf | -20.1 dB |
| 44.1 kHz / 512 | drums | Style 2, true peak 1, auto 1 | yes | -inf | -20.1 dB |
| 44.1 kHz / 512 | drums | Link 0.3, release 15 ms, lookahead 0.5 ms | no | -157.6 dB | -22.6 dB |
| 44.1 kHz / 512 | drums | Lookahead 10 ms, release 400 ms | yes | -inf | -31.8 dB |
| 44.1 kHz / 512 | drums | Monitor unity | no | -161.1 dB | -15.2 dB |
| 44.1 kHz / 512 | drums | Monitor delta | yes | -inf | -15.8 dB |
| 44.1 kHz / 512 | drums | Output -4, mix 0.6 | yes | -inf | -25.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Default | yes | -inf | -400.0 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Safe Ceiling | yes | -inf | -36.9 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Streaming -1 dBTP | yes | -inf | -28.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Loud Master | yes | -inf | -23.6 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Soft Clip Push | yes | -inf | -28.0 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Drum Peak Catcher | yes | -inf | -27.5 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Preset Broadcast -2 dB | yes | -inf | -31.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 0, true peak 0, auto 0 | yes | -inf | -24.6 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 0, true peak 1, auto 0 | yes | -inf | -24.7 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 0, true peak 1, auto 1 | yes | -inf | -24.7 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 1, true peak 0, auto 0 | yes | -inf | -24.5 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 1, true peak 0, auto 1 | yes | -inf | -24.5 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 1, true peak 1, auto 0 | yes | -inf | -24.6 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 1, true peak 1, auto 1 | yes | -inf | -24.6 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 2, true peak 0, auto 0 | yes | -inf | -25.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 2, true peak 0, auto 1 | yes | -inf | -25.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 2, true peak 1, auto 0 | yes | -inf | -25.4 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Style 2, true peak 1, auto 1 | yes | -inf | -25.4 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Link 0.3, release 15 ms, lookahead 0.5 ms | yes | -inf | -23.7 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Lookahead 10 ms, release 400 ms | yes | -inf | -32.3 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Monitor unity | yes | -inf | -36.0 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Monitor delta | yes | -inf | -7.9 dB |
| 44.1 kHz / 512 | sine -6 dBFS | Output -4, mix 0.6 | yes | -inf | -40.8 dB |
| 44.1 kHz / 97 | drums | Default | yes | -inf | -400.0 dB |
| 44.1 kHz / 97 | drums | Preset Safe Ceiling | yes | -inf | -27.2 dB |
| 44.1 kHz / 97 | drums | Preset Streaming -1 dBTP | yes | -inf | -25.0 dB |
| 44.1 kHz / 97 | drums | Preset Loud Master | yes | -inf | -20.1 dB |
| 44.1 kHz / 97 | drums | Preset Soft Clip Push | yes | -inf | -20.2 dB |
| 44.1 kHz / 97 | drums | Preset Drum Peak Catcher | yes | -inf | -20.0 dB |
| 44.1 kHz / 97 | drums | Preset Broadcast -2 dB | yes | -inf | -36.3 dB |
| 44.1 kHz / 97 | drums | Style 0, true peak 0, auto 0 | yes | -inf | -25.1 dB |
| 44.1 kHz / 97 | drums | Style 0, true peak 0, auto 1 | yes | -inf | -24.9 dB |
| 44.1 kHz / 97 | drums | Style 0, true peak 1, auto 0 | yes | -inf | -26.2 dB |
| 44.1 kHz / 97 | drums | Style 0, true peak 1, auto 1 | yes | -inf | -25.8 dB |
| 44.1 kHz / 97 | drums | Style 1, true peak 0, auto 0 | yes | -inf | -23.2 dB |
| 44.1 kHz / 97 | drums | Style 1, true peak 0, auto 1 | yes | -inf | -22.4 dB |
| 44.1 kHz / 97 | drums | Style 1, true peak 1, auto 0 | yes | -inf | -23.8 dB |
| 44.1 kHz / 97 | drums | Style 1, true peak 1, auto 1 | yes | -inf | -22.9 dB |
| 44.1 kHz / 97 | drums | Style 2, true peak 0, auto 0 | yes | -inf | -19.7 dB |
| 44.1 kHz / 97 | drums | Style 2, true peak 0, auto 1 | yes | -inf | -19.7 dB |
| 44.1 kHz / 97 | drums | Style 2, true peak 1, auto 0 | yes | -inf | -20.1 dB |
| 44.1 kHz / 97 | drums | Style 2, true peak 1, auto 1 | yes | -inf | -20.1 dB |
| 44.1 kHz / 97 | drums | Link 0.3, release 15 ms, lookahead 0.5 ms | no | -157.6 dB | -22.6 dB |
| 44.1 kHz / 97 | drums | Lookahead 10 ms, release 400 ms | yes | -inf | -31.8 dB |
| 44.1 kHz / 97 | drums | Monitor unity | no | -161.1 dB | -15.2 dB |
| 44.1 kHz / 97 | drums | Monitor delta | yes | -inf | -15.8 dB |
| 44.1 kHz / 97 | drums | Output -4, mix 0.6 | yes | -inf | -25.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Default | yes | -inf | -400.0 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Safe Ceiling | yes | -inf | -36.9 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Streaming -1 dBTP | yes | -inf | -28.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Loud Master | yes | -inf | -23.6 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Soft Clip Push | yes | -inf | -28.0 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Drum Peak Catcher | yes | -inf | -27.5 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Preset Broadcast -2 dB | yes | -inf | -31.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 0, true peak 0, auto 0 | yes | -inf | -24.6 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 0, true peak 0, auto 1 | yes | -inf | -24.6 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 0, true peak 1, auto 0 | yes | -inf | -24.7 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 0, true peak 1, auto 1 | yes | -inf | -24.7 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 1, true peak 0, auto 0 | yes | -inf | -24.5 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 1, true peak 0, auto 1 | yes | -inf | -24.5 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 1, true peak 1, auto 0 | yes | -inf | -24.6 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 1, true peak 1, auto 1 | yes | -inf | -24.6 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 2, true peak 0, auto 0 | yes | -inf | -25.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 2, true peak 0, auto 1 | yes | -inf | -25.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 2, true peak 1, auto 0 | yes | -inf | -25.4 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Style 2, true peak 1, auto 1 | yes | -inf | -25.4 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Link 0.3, release 15 ms, lookahead 0.5 ms | yes | -inf | -23.7 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Lookahead 10 ms, release 400 ms | yes | -inf | -32.3 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Monitor unity | yes | -inf | -36.0 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Monitor delta | yes | -inf | -7.9 dB |
| 44.1 kHz / 97 | sine -6 dBFS | Output -4, mix 0.6 | yes | -inf | -40.8 dB |
