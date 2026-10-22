# HPEQ — owner feedback backlog

Items from the owner's own testing. Order agreed on 2026-10-22: **finish the milestones first
(M6 → M8), then rework these "по факту"**. Each item gets its own measured change and report when done.

## FL Studio test of M5, 2026-10-22 (owner)

Overall: works, "не могу сказать что прям нравится".

| # | Feedback | Notes for the rework |
|---|---|---|
| F1 | **An EQ point must also carry modules** (compressor, reverb, any FX) "точечно", not only FX regions | Model change: a point with an optional FX chain. Its band must be defined (e.g. from frequency + Q/bandwidth, or an explicit width) so the H3 tree can select it. Keep decision B: the point's EQ stays PRE-EQ; its modules run on the band after PRE-EQ. Needs a short design note before code. |
| F2 | **Chain slots do not fit the inspector; no scrolling** — looks bad | Replaced by F3. |
| F3 | **Inspector as a module grid** (owner sketch below): tiles "Comp", "Reverb", "+", … Clicking a tile opens that module's editor in the same panel; a **back arrow** returns to the grid | Sketch shows 3 columns × 3 rows = 9 tiles; the agreed chain limit is 6 per node (D2) — confirm 6 vs 9 with the owner before building. "+" opens the module type list. Order of tiles = processing order (needs a clear left-to-right/top-to-bottom rule and reordering). |

Owner sketch for F3: `assets/owner_feedback_2026-10-22_module_grid.webp`.
