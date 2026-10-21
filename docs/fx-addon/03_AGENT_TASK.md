# AGENT TASK — HPDG FX ADD-ON: самостоятельный частотно-многослойный VST3

> **ЭТО ОСНОВНОЕ ЗАДАНИЕ АГЕНТУ.** Исполнять как engineering spec. Не заменять его общими идеями или UI-макетом. Исходники: https://github.com/Hamec01/HPDG. Дата постановки: 2026-10-21. Цель — новая сборочная цель **VST3 audio effect** рядом с существующим HPDG, а не переписывание HPDG и не показ его скрытого Sound Module.
>
> Прочитай **полностью** документы [`01_RESEARCH_ARCHITECTURE.md`](01_RESEARCH_ARCHITECTURE.md), [`02_PROJECT_ROADMAP.md`](02_PROJECT_ROADMAP.md), [`04_ACCEPTANCE_MATRIX.md`](04_ACCEPTANCE_MATRIX.md). Посмотри [`assets/hand_sketch_reference.jpeg`](assets/hand_sketch_reference.jpeg), [`assets/one_layer_wireframe.svg`](assets/one_layer_wireframe.svg), [`assets/signal_flow.svg`](assets/signal_flow.svg); `assets/ui_mockup_reference.png` — стилистическое вдохновение, **не** утверждённая спецификация DSP/параметров.

![Предполагаемая структура UI](assets/one_layer_wireframe.svg)

## Часть A. ЧТО ИМЕННО СТРОИТЬ

Создай новый вставной аудиоэффект VST3 (название на время разработки `HpdgFx`, графическое название может быть `HPDG FX` — финальное имя выберем потом). Он ставится на **любую аудиодорожку / bus** в DAW и принимает живой звук от host. Плагин должен уметь изолировать диапазоны спектра, назначать каждому диапазону эффекты, применять общий PRE-EQ, смешивать обработанные результаты и визуализировать реальные PCM до/после.

### Непереговорное решение B

**ВНУТРИ КАЖДОГО LAYER** путь аудио такой:

```text
Layer audio input
   -> PRE-EQ (все EQ Nodes: Bell / Cut / Shelf / Notch / etc.)
   -> post-EQ signal xE
   -> parallel frequency-selective FX Nodes receiving xE
   -> latency-compensated delta/wet mixer
   -> Layer Mix / Pan / Width / Channel Mode
   -> Layer audio output
```

В V1 **нельзя** переставить EQ после Compressor/Saturation/Reverb. Если на графике Low Cut выставлен на 200 Hz, и рядом есть Compressor с диапазоном 100–250 Hz, Compressor получает **уже отфильтрованное** звучание. Такое поведение сохраняется в `Serial Layers` и `Parallel Layers` **относительно каждого Layer**.

**Вопрос «один node — много FX?»** В первой реализации **один FX на node**, но архитектура поддерживает будущие chains. Не реализовывать «6 FX на каждой точке» без отдельной проверки нагрузки и UX. Эффектов во всём модуле десять видов: EQ, Compressor, Limiter, Reverb, Delay, Transient Shaper, Saturator, Monsta FX, Stereo Processor, Gate/Expander.

### Два типа объектов

`EqNode`: только PRE-EQ, тип фильтра (Bell, shelves, cuts, notch, band-pass, tilt, all-pass), частота, Gain, Q, Slope, Enabled, Channel Mode. EQ Node не обязан иметь FX Range.

`FxNode`: после PRE-EQ, диапазон `[lowHz, highHz]`, type, amount/mix, dry/wet semantics, channel mode, optional output band limit, Enabled/Bypass/Solo, размер и position в графическом workspace. Границы должны редактироваться мышью и численно. Центральная точка — UI handle, но **реальный селектор** задаётся диапазоном.

`LayerState`: стабильный индекс 0..9, имя, Enabled/Mute/Solo, PRE-EQ, collection FX Nodes, Gain, Mix, Pan, Width, ChannelMode; routing mode Parallel/Serial на уровне общей группы слоёв.

### Визуальная компоновка (по эскизу заказчика)

**Header:** logo/working name, preset, A/B, Undo/Redo, quality.

**Top strip:** осциллограмма real Input grey / real Processed cyan; Overlay / Delta / Freeze; input/output meters; справа **10 кнопок Layer в два ряда по пять**.

**Large central spectrum:** частоты логарифмически 20 … 20k, вертикальная ось dB, analyzer, EQ response (только статические фильтры), цветные FX regions с границами/точками/подписью Processor. `Stereo / Left / Right / Mid / Side` и `Spectrum / Spectrogram / Difference` — view/channel controls.

**Bottom panel:** список Nodes активного Layer, большая inspector-панель **ТОЛЬКО выбранного** процессора, Layer Controls. Не отображать одновременно все ручки всех десяти эффектов.

**UX контракты:** двойной щелчок создаёт node; контекстное меню выбирает EQ/FX; drag point — частота; drag handles — low/high; Shift — fine; Ctrl/Cmd — альтернативное fine/precision значение; числовой ввод всегда доступен; right click — context; Bypass/Solo в node/layer, Undo и keyboard-focus безопасны для DAW. Touch/multi-DPI опционально после desktop UI.

### Обязательные filter/channel modes

Low/High Cut slopes `6,12,18,24,36,48,72,96 dB/oct`; отдельный R&D `Brickwall` с transition width/stopband attenuation/latency — **никакого псевдо-Brickwall на основе одних маркетинговых подписей**. Остальные формы: Bell, Low Shelf, High Shelf, Notch, Band Pass, Tilt, All Pass. В зависимости от математической формы slope может иметь другую семантику, UI показывает только реально поддерживаемые значения.

Channel modes `Stereo, Left, Right, Mid, Side, Linked Stereo, Dual Mono`, где применимо. Для M/S четко определить нормировку и round-trip tests. Возможность обрабатывать только Side в Low Cut обязательна; Side является отдельной компонентой M/S, не простой «правой стороной» стерео.

### 10 Layers

Все десять кнопок существуют в UI/state. В Milestone 1–5 только Layer 1 реально обрабатывается; остальные не маскируются «успешным» демо. В Milestone 7 вводятся Serial и Parallel. В Parallel слои получают общий dry и добавляют **разницу**, а не копию dry; иначе десять bypassed слоёв умножат громкость. Для Serial цепь `L1 → L2 → …` с Mix и latency alignment. Hybrid, Morph, LFO, External Sidechain — **Phase 9**; не блокировать beta этими функциями.

## Часть B. АРХИТЕКТУРА И ФОРМУЛЫ

### Core graph, зависимый только от audio state

Вместо использования `PatternProject`, `TrackType`, `PreviewEngine`, `SampleLibraryManager` и иных структур MIDI-генератора — создать чистый `FxRoutingGraph`. Модули DSP не должны импортировать UI, MIDI engine или состояние генерации паттернов HPDG.

C++17 / JUCE; допускается общая библиотека reusable DSP при условии отсутствия регрессий HPDG. В первом PR безопаснее создать независимый source set в `Addons/HpdgFx` и позже переносить proven portions текущего кода.

### Формула частотного FX (исполнить, не только написать в комментарии)

```text
xE = PreEQ(x)
base = DelayToCommonLatency(xE)
for each active FxNode i:
    bi = FrequencySelect_i(xE)
    if processor is INSERT/REPLACE (compressor, transient, saturator, monsta, stereo):
        di = Align(Process_i(bi)) - Align(bi)
    else if processor is WET-SEND (reverb, delay):
        di = Align(ProcessWetOnly_i(bi))
    else:
        di = 0
    base += SmoothedMix_i * di
layerOutput = LayerControls(base)
```

Обязательные инварианты: при `FxNode Mix=0/Bypass` её `di=0`, при всех отключенных FX результат совпадает с PRE-EQ выходом (после выравнивания delay), при PRE-EQ=identity и layer bypass весь плагин передаёт исходник точно/в пределах численного порога. Ревербератор должен возвращать **wet-only**, иначе нельзя использовать это правило без ошибочного вычитания dry.

Для Parallel Layers аналогично:

```text
base = Aligned(x)
for layer in 1..10:
    base += SmoothedLayerMix[layer] * (Aligned(ProcessFullLayer(layer,x)) - Aligned(x))
y = base
```

В Serial Layers каждый слой получает выход предыдущего. Всегда pre-EQ до FX **внутри** каждого слоя. Порядок слоёв может влиять на аудио; тест на правильную последовательность.

### Контракт процессора (пример интерфейса, не обещание окончательного API)

```cpp
struct FxProcessContext {
    double sampleRate;
    int maxBlockSize;
    double bpm;
    bool transportRunning;
    std::int64_t timelineSamples;
};

class IFxProcessor {
public:
    virtual ~IFxProcessor() = default;
    virtual void prepare (const juce::dsp::ProcessSpec&) = 0;
    virtual void reset() noexcept = 0;
    virtual void process(juce::AudioBuffer<float>&, const FxProcessContext&) noexcept = 0;
    virtual int latencySamples() const noexcept = 0;
    virtual double tailSeconds() const noexcept = 0;
};
```

`noexcept` — только если код действительно не выделяет память / не кидает exceptions. Имплементация может разделять mono/stereo, SIMD, float/double. Избегать RTTI и динамического создания FX на audio callback. Изменение topology — подготовка графа на non-RT thread, RT-safe передача готового графа, освобождение старых экземпляров только вне audio callback.

### Калибровка анализа

**Waveform:** PCM snapshots до и после, компенсация фактического общего latency; Input/Output/Delta, вертикальная ось реальная. Сэмпловые буферы передавать в UI через bounded lock-free SPSC queue или заранее выделенные snapshots; FFT на non-RT worker/thread; дропнуть UI frame разрешается, дропнуть audio buffer — нет.

**EQ response:** сгенерированная кривая должна совпадать с реально установленными coefficients фильтров. FX processing НЕ представлять как EQ-график. `Difference Spectrum` — измеренная величина спектральной энергии, с low-energy masking и временем усреднения, а не полная «частотная характеристика» нелинейной системы.

**Meters:** RMS, sample peak, oversampled true peak (phase 8), optional LUFS, compressor reduction, L/R correlation; корректная калибровка dBFS.

## Часть C. ПОРЯДОК ВЫПОЛНЕНИЯ — ИЗМЕРЕНИЯ СНАЧАЛА

### ЭТАП 0: только аудит, baseline, тестовый стенд

1. `git status`, branch, `git rev-parse HEAD` и версия JUCE; создай новую ветку `feature/hpdg-fx-addon` **только если разрешено политикой репозитория**; не перезаписывай чужие изменения и не делай `reset --hard`.
2. Проверь настоящее дерево репозитория, прочитай исходники перечисленных выше FX и `CMakeLists.txt`; не ориентируйся на старый Source.zip.
3. Запусти исходные HPDG build/test, где инфраструктура доступна; сохрани stdout/stderr, commit SHA, окружение и actual exit codes; отметь `BLOCKED`, если не запускал.
4. Создай offline unit-test harness и генераторы сигналов: impulse, step, sine, sweep, multitone, white/pink noise, silence, stereo L/R/M/S, drum WAV. Без копирования чужих защищённых треков в репо.
5. Создай baseline метрик WAV/JSON, спектральных/фазовых свойств, CPU/p99 и количества RT allocations.
6. Задокументируй найденные угрозы, измеряемые пороги и всю воспроизводимость в `docs/fx-addon/reports/M0_BASELINE.md`.

**Gate 0:** тестовый harness и baseline существуют, результаты не вымышлены.

### ЭТАП 1: отдельный VST3 Insert Skeleton

1. Добавь вторую `juce_add_plugin` цель с другим plugin code/manufacturer/bundle ID, независимую от HPDG `IS_SYNTH TRUE`; НОВАЯ `IS_SYNTH FALSE`, настроенный stereo/mono input/output.
2. Сделай no-op audio-through и нейтральный Gain, полноценный bypass, save/restore state, metadata, editor. Зафиксируй стабильную схему сериализации `schemaVersion=1`, на случай добавления новых нод.
3. Настрой CTest, pluginval по доступности, debug asserts, logs вне RT; защиту от открывания/закрывания editor.
4. Собери существующий HPDG, запуск smoke test (`HPDG` и `HpdgFx` не должны конфликтовать по UID).

**Gate 1:** DSP unity/host stable; отдельный .vst3 реально собирается. Если не можешь собрать — не объявляй его работающим.

### ЭТАП 2: 1 PRE-EQ + 1 FX Node научный прототип

1. Реализуй core routing delta, один предвыделенный mono/stereo буфер, тип selector (первый `[250,700]`), mock processor Gain -6 dB. Затем замени mock на Compressor.
2. Создай тесты перекрытия двух nodes: `[250,700]` и `[450,1200]`, `Mix 0/100`, bypass, dry preservation, latency.
3. Сравни H1 (`delta-band`) и H2 (комплементарные crossover) на одинаковом наборе, выбери стратегию. Принять для перекрывающихся H1, если тесты не опровергнут.
4. Проверь изменяемые диапазоны при воспроизведении; сдвиг cutoff обязан быть smooth/crossfaded без RT alloc.

**Gate 2:** no dry-doubling, нет фазовой катастрофы при unity/bypass, no nonfinite, T01/T02/T04/T10. Если не проходит — СТОП, не строить UI.

### ЭТАП 3: PRE-EQ с графическими типами и slopes

1. `EqNode` исключительно в PRE-EQ; структурный порядок подтверждать unit-test: clip 80 Hz -> LowCut 200 Hz -> Compressor 100–250 Hz; compressor detector получает ослабленный сигнал.
2. Реализуй basic Bell, LP/HP cuts, shelf, notch и другие по вехам. 96 dB/oct — реальные SOS (пример: 16-й порядок), не просто EQ curve.
3. Доступные slope и Brickwall обозначать честно: Brickwall после отдельной FIR validation, можно оставить `not available` до R&D.
4. Channel modes и M/S тесты, частоты около Nyquist, alpha smoothing параметров.

**Gate 3:** T03/T05, phase/impulse/sweep evidence, предсказуемый CPU.

### ЭТАП 4: Compressor / Reverb как регионные обработчики

1. Исследуй reuse `DrumCompressorBlock`: его `ensureScratchCapacity` в обработке может аллоцировать; новый контракт исправляет это.
2. Compressor: threshold, ratio, knee, attack, release, wet/dry, gain reduction; сравни с понятной контрольной моделью.
3. Reverb: обработка **wet-only send**, predictably decaying tails, предсказуемый bypass mode, input/output band filter choice.

**Gate 4:** T06/T07, поведение на kick/snare/full-mix и сохранение mono/stereo.

### ЭТАП 5: UI одного Layer и измерители

1. Построй интерфейс по структуре рисунка, с 10 кнопками Layer, в начале реально работает только Layer 1; остальные ясно помечены как `not yet implemented` / disabled и не должны незаметно имитировать функциональность.
2. Реализуй SpectrumWorkspace, live FFT, реальные Waveform overlay/delta (compensated delay), point/range gestures, editable inspector.
3. Сделай spectrum view и theoretical PRE-EQ curve разными визуальными слоями; частоту x нормируй логарифмически.
4. Freeze с корректным real PCM, undo, A/B state, save/load, resize, UI event throttling.

**Gate 5:** T09 + screenshot, animations на реальных данных; UI не вызывает RT-alloc/stalls.

### ЭТАП 6: остальные процессоры

Задействуй отдельно Saturator → Transient → Delay → Monsta FX → Stereo → Gate/Expander → Band Limiter (в таком порядке по умолчанию). Каждый эффект отдельно тестируется. Monsta должен работать на выделенной частоте и принимать host BPM/tempo fallback. Saturator — хотя бы optional oversampling 2x/4x, alias tests. No fake control knobs.

**Gate 6:** каждый выбранный эффект изменяет PCM как заявлено, имеет тесты; при no-op/disabled нет влияния.

### ЭТАП 7: 10 Layers, Serial / Parallel

1. Ввести стабильный `LayerState` всех десяти; delta-mix без dry duplication, latency cross-layer align.
2. Слева/справа панели Gain/Mix/Pan/Width/Mono/Stereo/Side/Bypass/Mute/Solo. UI слой переключает view/state, а не глобально изменяет всякий раз другой Layer.
3. Serial/Parallel выбор явно меняет маршрут. Никакого скрытого master limiter; предупреждать meter об overshoots.
4. Lazy-process inactive layers; профилировать тяжёлые комбинации.

**Gate 7:** 10 bypassed slots = unity, 10 layers state round-trip, T12; DAW host stress.

### ЭТАП 8: release/quality

VST3 validator/pluginval, стресс open/close, hosts, PDC, tails, sample-rates, CPU profiling, automation and preset compatibility, baseline regression. Документировать сведения для Windows x64 и других реально собранных платформ (не обещать Mac/Linux, если нет запуска).

**Gate 8:** матрица завершена `PASS`, непроверенное = `BLOCKED`, high-severity bugs отсутствуют.

### НЕ ДЕЛАТЬ В ПЕРВЫХ ВЕХАХ

- Не внедрять в существующий HPDG VST3 SoundModuleComponent, если это не отдельный осознанный будущий запрос.
- Не переписывать генераторы Boom Bap, Trap, DnB, Techno, SampleAnalyzer, GUI HPDG.
- Не копировать звуковой движок с глобальными mutable state, проектными mutex и RT allocations.
- Не делать 10 processing layers до доказательства single-layer architecture.
- Не создавать fictitious `Brickwall`, `Linear Phase`, `True Peak`, `Sidechain`, `Spectral` controls без DSP и проверки.
- Не объявлять низкую загрузку CPU без времени processBlock на реальной целевой машине.
- Не хранить тестовые WAV или измерения, лицензии на которые отсутствуют.
- Не генерировать бинарный результат «из головы», не выдавать mockup за работающий VST3.

## Часть D. ПРОВЕРКИ, МЕТРИКИ, РЕПРОДУЦИРУЕМОСТЬ

Использовать `04_ACCEPTANCE_MATRIX.md`, тесты T01–T12 из исследования и дополнительные шаги:

- **Стабильность:** отсутствие NaN/Inf при 100k random parameter mutations в тестовом режиме, ограничение amplification для extreme configs, real-time safety.
- **Цифровой аудиофакт:** offline WAV с phase/time aligned A/B, input/output/difference arrays, измеренные frequency responses; plots генерируются инструментально, не рисуются вручную.
- **Filter math:** для 96 dB/oct нет обещания −96 dB attenuation прямо на cutoff; тестировать асимптотический slope в валидном измерительном диапазоне.
- **Восстановление:** весь bypass с `PreEQ=identity` должен быть unity; отдельный `FX disabled` должен восстанавливать именно Pre-EQ signal, даже если Pre-EQ активен.
- **Динамика:** correct compressor makeup/mix, attack/release и detector logic. Различать «Band Limiter» и общую защиту пиков.
- **Визуализация:** oscilloscope — реальные PCM, FFT — результаты преобразования; `Delta` требует синхронизации и (по желанию) level match — разные режимы явно видны.
- **Реальное время:** никаких heap allocations, file access, GUI calls, shared mutable UI mutex в `processBlock`. Тестировать `blockSize 1..2048`, sampleRate `44.1..192k` и паузы.
- **Host stability:** `pluginval --strictness-level 5 <path>`, более высокий уровень по мере прохождения, FL Studio/REAPER если установлены, сохранение/открытие проекта, transport absent, tail after stop, 100 повторных загрузок VST.
- **CPU:** профили один/пять FX, один/десять Layers, p50/p95/p99, max allocs and memory peak; сравнивать с тестовым baseline по тому же железу и commit SHA.

Для числовых допусков смотри acceptance matrix; цель без воспроизводимого результата — **не PASS**.

## Часть E. ОТЧЁТ АГЕНТА (ОБЯЗАТЕЛЕН ПОСЛЕ КАЖДОЙ ФАЗЫ)

По окончании **каждой** вехи создать `docs/fx-addon/reports/M{N}_REPORT.md` со следующими разделами:

```text
1. Milestone / Date / Git SHA / Branch / Target Platform
2. Изменённые файлы (папки + мотив)
3. Что реально реализовано / что только scaffolding
4. Команды сборки/тестов и exit code
5. Точные числа: THD/alias/null/latency/CPU где уместно
6. Ссылки на WAV, JSON, plot, screenshot артефакты
7. Acceptance Matrix: PASS/FAIL/BLOCKED с доказательствами
8. Существующий HPDG: build/tests результат и регрессии
9. Ограничения / открытые риски / вопросы решения
10. Следующая веха; разрешено ли переходить gate?
```

**Критерий завершения задачи:** не «UI выглядит интересно», а отдельный VST3, работающий с host audio in/out и реально выполняющий спектральные FX-операции; измерения подтверждают архитектурные инварианты. Не позволяй возможной недоступности DAW/SDK подменить тесты фиктивными результатами. Если окружение не может выполнить gate — фиксируй блокер и остановись.

## С чего начать прямо сейчас

**Выполни M0 полностью, затем M1.** Не начинай внедрение всех 10 FX и 10 Layers до лабораторной валидации M2. В первом сообщении о выполнении покажи фактическую структуру репозитория, найденные компоненты пригодные для reuse, риски RT-allocation и конкретный дизайн offline DSP harness. Дальше работай по roadmap с доказательствами.
