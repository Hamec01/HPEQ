# HPDG FX ADD-ON — матрица приёмки и экспериментальный протокол

**Важно:** каждая строка в этой таблице — **требование/проектная гипотеза**. Значение `PLANNED` не означает прохождение теста. Заполнять `PASS`, `FAIL`, `BLOCKED` и evidence только после запуска тестов. Документы: [научный анализ](01_RESEARCH_ARCHITECTURE.md), [roadmap](02_PROJECT_ROADMAP.md), [задача агенту](03_AGENT_TASK.md).

## 1. Обязательная таблица по фазам

| Test ID | Milestone | Методика и вход | Критерий / expected | Статус | Evidence path |
|---|---:|---|---|---|---|
| M0-A | 0 | SHA, JUCE, CMake, build/log | Протокол исходного HPDG, actual logs | PASS | [M0_BASELINE](reports/M0_BASELINE.md) |
| M0-B | 0 | Отдельный offline signal generator | Reproducible inputs + fixtures + metrics JSON | PASS | [M0_BASELINE](reports/M0_BASELINE.md), `reports/m0/` |
| M1-A | 1 | Insert FX in/out 2ch 44.1/48k | Unity passthrough, отсутствие нового звука | PASS | [M1_REPORT](reports/M1_REPORT.md) |
| M1-B | 1 | VST3 host scan | Отдельный уникальный plugin ID, загрузка | PASS (JUCE host); pluginval BLOCKED | [M1_REPORT](reports/M1_REPORT.md) |
| M1-C | 1 | Десятикратные/100× editor open/close | No crash/hang/deadlock, state restore | PASS (in-process + JUCE host); DAW BLOCKED | [M1_REPORT](reports/M1_REPORT.md) |
| T01 | 2 | Stereo sine/noise, EQ off, FX bypass | Для exact copy равенство samples; для aligned nontrivial paths normalized RMS error ≤ −100 dB ref | PASS (H3 engine; passive nodes vs documented all-pass) | [M2_REPORT](reports/M2_REPORT.md) |
| T02 | 2 | 2–8 overlapping regions `[250,700]`, `[450,1200]`, disabled | Нет dry duplication; output = aligned PRE-EQ | PASS (H3 engine; passive nodes vs documented all-pass) | [M2_REPORT](reports/M2_REPORT.md) |
| T03 | 3 | Sweeps/sine grid for 6–96 dB/oct | Actual response matches derived coefficients; slope evaluation with defined valid frequency interval and tolerance ±6 dB/oct target | PASS | [M3_REPORT](reports/M3_REPORT.md) |
| T04 | 2+ | 100 000 deterministic random params & 1..2048 blocks | No NaN/Inf, exceptions, audio callback allocations; no runaway | PASS (M2 params) | [M2_REPORT](reports/M2_REPORT.md) |
| T05 | 3 | Left-only, Right-only, M-only, S-only, mono | Exact channel-selection semantics; roundtrip M/S within float tolerance | PASS | [M3_REPORT](reports/M3_REPORT.md) |
| T06 | 4 | Burst/step tones, > and < threshold | Compressor GR and envelope shape follow published detector/ratio rules | PLANNED | — |
| T07 | 4 | Single impulse then 10 s silence | Reverb/Delay tails audible/quantified and no abrupt cut in normal bypass | PLANNED | — |
| T08 | 6 | High sine/multitone, 1x and OS4x | Aliasing energy and headroom measured; OS4x target demonstrably better in tested conditions | PLANNED | — |
| T09 | 5 | Offline PCM vs displayed capture | Waveform input/output/Delta are actual aligned PCM; displayed spectrum corresponds FFT of captures | PLANNED | — |
| T10 | 2 | Identical input with block 1, 32, 127, 256, 1024 | Offline outputs numerically consistent with documented tolerance | PASS (bit-exact) | [M2_REPORT](reports/M2_REPORT.md) |
| T11 | 8 | pluginval, host reopen, mutate/restore | pluginval level≥5; no crashes/hangs; 100 editor cycles, no state loss | PLANNED | — |
| T12 | 7–8 | 1/5/10 layers × 1/5 nodes × multiple fs | p50/p95/p99 CPU, peak RSS, latency documented on same host | PLANNED | — |
| B-ORDER | 3 | 80 Hz input, Pre-EQ HP200 + FX comp 100–250 | Compressor receives PRE-filtered 80 Hz (not original); layer EQ before FX | PASS (LowCut 200 Hz) | [M3_REPORT](reports/M3_REPORT.md) |
| MIX-0 | 2 | FX Node Mix=0, layer Mix=0 | No contribution from node/layer, independent of hidden DSP state | PASS (node) | [M2_REPORT](reports/M2_REPORT.md) |
| METER | 5 | Signal at known dBFS | Input/output peak & RMS within calibrated accuracy; display never synthetic | PLANNED | — |
| PDC | 4/8 | Turn on/lookahead/FIR/oversampling | Host latency equals effective processing delay; dry and wet aligned | PLANNED | — |
| STATE | 1–8 | Save/restore state; automation gesture | Parameters, IDs, sound, selected Layer, nodes fully restore | PASS (M1 scope) | [M1_REPORT](reports/M1_REPORT.md) |
| HPDG | all | Build and run unchanged old tests | Original HPDG generator/editor/wav unaffected | PASS (untouched, separate repo) | [M0](reports/M0_BASELINE.md), [M1_REPORT](reports/M1_REPORT.md) |

## 2. Test signal definitions

Определить в коде и зафиксировать входные параметры/seed (не редактировать input corpus между baseline и контрольным сравнением):

```yaml
fixture_set: fx_addon_v1
sample_rates: [44100, 48000, 96000, 192000]
block_sizes: [1, 16, 32, 64, 127, 256, 512, 1024, 2048]
channels: [1, 2]
durations_s: [0.2, 1.0, 5.0, 10.0]
seed: 24681357
test_signals:
  - silence
  - dc
  - subnormal
  - impulse
  - step
  - sine_20_50_80_100_200_450_700_1k_5k_10k_15k
  - exponential_sweep_20_to_nyquist_margin
  - white_noise
  - pink_noise
  - dual_tone_imd
  - gated_burst_attack_release
  - stereo_left_only
  - stereo_right_only
  - stereo_mid_only
  - stereo_side_only
  - synthetic_drums_kick_snare_hihat
```

Обратить внимание: некоторые сочетания `frequency` с sampleRate > Nyquist недопустимы; генератор обязан отбрасывать или помечать их `N/A`, не использовать alias как ожидаемый сигнал.

## 3. Метрики с формулами

### Сравнение aligned bypass

`e[n] = y_aligned[n] − x_reference[n]`

`error_dB = 20 log10( RMS(e) / max(RMS(x_reference), epsilon) )`

Критерий T01, стартовое предложение: `error_dB ≤ −100 dB` для регулярных не-нулевых референсов. На тишине проверять отдельные абсолютные peak/RMS и отсутствие NaN; не делить на почти нулевой rms. Для разных фазовых режимов проводить comparison к документированному expected delayed/allpass result, а не требовать произвольного time-domain null.

### АЧХ для теста фильтра

`H(f) = FFT(output)[f] / FFT(input)[f]`, при обязательной маске ненадёжных бинов. `mag_dB(f) = 20 log10 |H(f)|`; `phase(f) = arg H(f)`; `group_delay ≈ −d(unwrap phase)/dω`. Отдельно логировать фазу/групповую задержку. Для фильтра 16-го порядка целевой асимптотический slope ~`96.33 dB/oct`, если участок, где это применимо, не упирается в цифровой noise floor. Не оценивать наклон на cutoff, где transition behaviour зависит от прототипа.

### Вычисление времени обратного вызова

`block_budget_us = 1e6 * block_size / sample_rate`

`dsp_cpu_fraction = measured_process_us / block_budget_us`

Сохранять `p50`, `p95`, `p99`, maximum на реальной target machine. Пиковое значение может меняться с ОС/host/scheduler; сравнивать в одинаковой среде. При наличии профайлера проверить именно allocations внутри RT, а не только общую память процесса.

### Aliasing

Для нелинейных эффектов сравнивать спектральную энергию **неожиданных компонентов** с аналитически ожидаемыми гармониками/IMD на valid frequency bins, учитывать отражение за Nyquist. В каждом отчёте приводить до/после oversampling и целевой диапазон частот. Не делать вывод «OS лучше» только из того, что вырос CPU.

## 4. Отчётная таблица эксперимента (шаблон)

```text
Experiment ID: [T03 / T08 / ...]
Test commit SHA:
Reference commit SHA:
OS / CPU / Audio driver / Compiler / flags:
Sample rate / block / channels:
Input WAV SHA256:
Params and random seed:
Expected analytic behaviour:
Measured response:
Mean / RMS / Peak / p50 / p95 / p99:
Fail cases / anomalies:
Raw JSON metrics:
WAV/PNG/SVG output:
PASS / FAIL / BLOCKED:
Conclusion and next action:
```

## 5. Правила научной честности

1. Не заменяй выполненное измерение фразами «звучит хорошо», «почти как FabFilter», «на глаз нормально».
2. Не применяй шумовую маску после теста для того, чтобы искусственно получить PASS; зафиксируй маску заранее.
3. Не меняй reference inputs и seed при сравнении baseline и новой версии.
4. Для любого не выполненного теста статус только `BLOCKED / NOT RUN`, не `PASS`.
5. Записывай погрешности, невозможность абсолютных утверждений о «нулевой фазе», «Brickwall без задержки», «полной ликвидации алиасинга».
6. Отрицательные тесты (фейл) — полезный результат исследования; сперва определи воспроизводимый дефект, потом исправь.
7. Не забывай регрессии исходного HPDG.

## 6. Критерий релизного допуска

- Нет критических `FAIL` для correctness/RT/host стабильности.
- Рендеры и графики по T01–T12 воспроизводимы из команд сборки / test harness.
- В DAW плагин вставляется именно как audio effect, не меняет MIDI routing основной HPDG.
- Pre-EQ → FX соблюдается в каждом Layer (решение B).
- Все заявленные в UI эффекты действительно обрабатывают audio; скрытые/непроверенные функции не называются готовыми.
- Работа со всеми десятью слоями измерена под профилируемой нагрузкой, а не только проверена кликом кнопок.
- Документация ограничения и preset backward compatibility подготовлены.
