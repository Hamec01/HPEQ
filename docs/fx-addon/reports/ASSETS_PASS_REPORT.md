# Soviet hardware assets pass

Дата проверки: 2026-10-24

Ветка: `feature/hpeq-m0-m1`

## Что изменено

- Все PNG из `docs/fx-addon/assets/soviet_hardware_pack/` добавлены в `HPEQ_Assets` через `juce_add_binary_data`; рядом с `.vst3` ничего не требуется.
- `BlackSteel::brassKnob`, `lever` и `lamp` теперь используют общий кэш изображений, созданный один раз на UI-потоке. Аудиопоток к этим функциям не обращается.
- `HamloCompPanel` переведён на те же общие функции; hit-зоны, drag/wheel/double-click и ввод значения не менялись.
- Материалы ручек: Comp/RedWall/Udarnik/Zastava/Shirota — `knob_aluminum_brass_insert`; Verb — `knob_bakelite_ribbed`; Echo — `knob_lathe_steel_rust`; Mag — `knob_chipped_iron_dark`. Указатель — `pointer_white_enamel`.
- Для тумблеров используется общая TP1-2 планка с отдельными кэшированными рычагами UP/DOWN; нейтральное положение показывает только планку с центральным гнездом.
- Красные пилотные лампы используют `lamp_panel_red_on/off`. Синяя FREEZE сохранена на старой цветной отрисовке, чтобы не потерять синий цвет.
- `scale_arc_ticks.png` оставлен неиспользованным: файл поставлен как вертикальная дуга 284×540 и не укладывается в существующую круговую 270° шкалу без изменения размеров/геометрии интерфейса; текущая гравировка сохранена.

## Скриншоты до/после

| Страница | До | После |
| --- | --- | --- |
| HamloComp | [PNG](assets_pass/before/hpeq_panel_hamlocomp.png) | [PNG](assets_pass/after/hpeq_panel_hamlocomp.png) |
| HamloVerb | [PNG](assets_pass/before/hpeq_panel_hamloverb.png) | [PNG](assets_pass/after/hpeq_panel_hamloverb.png) |
| HamloEcho | [PNG](assets_pass/before/hpeq_panel_hamloecho.png) | [PNG](assets_pass/after/hpeq_panel_hamloecho.png) |
| HamloMag | [PNG](assets_pass/before/hpeq_panel_hamlomag.png) | [PNG](assets_pass/after/hpeq_panel_hamlomag.png) |
| HamloRedWall | [PNG](assets_pass/before/hpeq_panel_hamloredwall.png) | [PNG](assets_pass/after/hpeq_panel_hamloredwall.png) |
| HamloUdarnik | [PNG](assets_pass/before/hpeq_panel_hamloudarnik.png) | [PNG](assets_pass/after/hpeq_panel_hamloudarnik.png) |
| HamloZastava | [PNG](assets_pass/before/hpeq_panel_hamlozastava.png) | [PNG](assets_pass/after/hpeq_panel_hamlozastava.png) |
| HamloShirota | [PNG](assets_pass/before/hpeq_panel_hamloshirota.png) | [PNG](assets_pass/after/hpeq_panel_hamloshirota.png) |

## Проверки

| Проверка | Результат |
| --- | --- |
| `cmake --build --preset release` | PASS, без предупреждений |
| `build/Release/HPEQ_Tests.exe` | PASS — 1452 expectations, 0 failed |
| `cmake --build --preset debug` | PASS, без предупреждений |
| `build/Debug/HPEQ_Tests.exe` | PASS — 1452 expectations, 0 failed |
| `HPEQ_Lab.exe ui --out .../assets_pass/after` | PASS — все семь страниц сохранены |
| UI frame, 1400×900 | p50 8.91 мс; p95 10.91 мс; max 14.66 мс; бюджет 33.3 мс |
| Встроенные ресурсы | PASS — имена ручек, указателей, `toggle_switch_plate`, ламп, шкалы и кнопки присутствуют в сгенерированном `BinaryData.h` |
