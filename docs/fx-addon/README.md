# HPDG FX Add-on — пакет исследовательской документации

Подготовлено **2026-10-21** на основании обсуждения, рукописного рисунка, mockup и актуального GitHub `Hamec01/HPDG`. **Это ТЗ и исследование, не готовый код VST3.** Название будущего плагина пока не утверждено.

## Файлы

| Файл | Для чего нужен |
|---|---|
| [`01_RESEARCH_ARCHITECTURE.md`](01_RESEARCH_ARCHITECTURE.md) | Обоснование алгоритмов, изучение текущего HPDG и конкурентов, формулы, риски, источники |
| [`02_PROJECT_ROADMAP.md`](02_PROJECT_ROADMAP.md) | Milestones M0–M9, что строить и в какой последовательности, критерии перехода |
| [`03_AGENT_TASK.md`](03_AGENT_TASK.md) | **Основной файл для Copilot/Codex/другого агента**; строгая постановка задачи и запреты |
| [`04_ACCEPTANCE_MATRIX.md`](04_ACCEPTANCE_MATRIX.md) | Математические и референсные критерии качества, таблица PASS/FAIL/BLOCKED |
| [`assets/hand_sketch_reference.jpeg`](assets/hand_sketch_reference.jpeg) | Исходный рисунок автора с компоновкой окна |
| [`assets/ui_mockup_reference.png`](assets/ui_mockup_reference.png) | Пример визуального оформления, не алгоритмическая спецификация |
| [`assets/one_layer_wireframe.svg`](assets/one_layer_wireframe.svg) | Размеченная техническая структура рабочего пространства одного Layer |
| [`assets/signal_flow.svg`](assets/signal_flow.svg) | Схема PRE-EQ → FX Nodes → latency-aligned delta mix |

## Главное решение владельца продукта

**Вариант B:** EQ всегда **до FX Nodes** внутри каждого Layer. Так, Low Cut в 200 Hz физически изменяет сигнал до того, как Compressor 100–250 Hz получает материал на обработку.

## Как передать агенту

Скопируй все файлы и папку `assets/` в репозиторий, например в `docs/fx-addon/`. Отправь агенту:

> Изучи `docs/fx-addon/03_AGENT_TASK.md` и три связанных документа; используй исходный эскиз и схемы. Начни с аудита актуального HPDG и Milestones M0–M1. В каждой фазе предоставляй реальные WAV/JSON измерения и отчёт PASS/FAIL/BLOCKED. Существующий HPDG не ломать. Главный принцип routing — вариант B: PRE-EQ до FX. Не выдавай недоделанный GUI за готовую DSP-обработку.

Для просмотра `.svg` подходят GitHub и браузер. Все ссылки внутри Markdown относительные, поэтому **не отделяй `.md` от папки `assets`**.
