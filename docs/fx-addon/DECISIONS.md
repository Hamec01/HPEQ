# HPEQ — owner decisions that change the spec

The spec (`01`–`04`) stays as written; this file records where the product owner decided differently.
Each entry: date, decision, what it overrides, consequences for the engineering plan.

## D1 — 2026-10-21 — Separate repository

**Decision:** the plugin lives in its own repository `HPEQ` (`E:\HPEQ`), not under `HPDG/Addons/HpdgFx`.

**Overrides:** `02_PROJECT_ROADMAP.md` "Структура репозитория" and `03_AGENT_TASK.md` Part B
("независимый source set в `Addons/HpdgFx`").

**Consequences:**
- HPDG cannot be regressed by this work at the build level: nothing in the HPDG tree is edited.
  The `HPDG` acceptance row is still reported (HPDG tests are run read-only as a baseline).
- JUCE is not vendored here. CMake takes `HPEQ_JUCE_DIR` (a local JUCE checkout, e.g. the one inside
  HPDG) or fetches the same pinned version (JUCE 8.0.12) with FetchContent.
- HPDG DSP is **not copied**. The M0 harness can compile selected HPDG blocks read-only from
  `HPEQ_HPDG_SOURCE_DIR` to measure them; anything later reused is ported deliberately, with tests,
  and the RT problems found in M0 fixed in the port.
- Working name in code/UI: `HPEQ` (from the owner's sketch). Final product name is still open.

## D2 — 2026-10-21 — Up to 6 FX processors per FX Node

**Decision:** an FX Node carries a **chain of 1–6 processors** (the sketch: "на одной точке их может
быть максимум 6"), not one processor.

**Overrides:** `03_AGENT_TASK.md` "один FX на node" and `01` §3 "Для V1 один FX-процессор на FX Node".

**Semantics (keeps every invariant of the delta formula):** the node selects its band once,
`b = B(xE)`, then runs the chain serially on the band signal, each slot with its own mix:

```text
s0 = b
for slot k = 1..K (K <= 6), in chain order:
    if slot bypassed or mix_k == 0:  s_k = Align(s_{k-1})
    insert/replace processor:        s_k = Align(s_{k-1}) + mix_k * (Align(P_k(s_{k-1})) - Align(s_{k-1}))
    wet-send processor (reverb/delay): s_k = Align(s_{k-1}) + mix_k * Align(W_k(s_{k-1}))
delta_node = s_K - Align(b)
y = Align(xE) + sum_nodes nodeMix * delta_node
```

- All slots bypassed / mix 0 → `s_K = Align(b)` → `delta_node = 0` → T01/T02/MIX-0 unchanged.
- Chain latency = sum of slot latencies; the node reports it to the layer's `LatencyCoordinator`.
- A node with one slot is exactly the spec's single-FX node, so M2 starts with K = 1 and the chain
  is enabled for K > 1 only after its own tests (order matters, bypass of any slot, latency sum) and a
  CPU measurement of the 6-slot worst case (T12 extended).
- State schema v1 stores nodes as a slot array from the start, so no migration is needed later.

## D3 — 2026-10-21 — Rewrite the processors, do not port HPDG's

**Decision:** the owner considers HPDG's EQ, MonstaFX, Compressor "and everything else" raw. HPEQ
implements all processors anew, to a measured quality bar.

**Overrides:** `01` §1 "Кандидат на extraction" and `03` M4.1 "Исследуй reuse `DrumCompressorBlock`".

**Consequences:**
- HPDG blocks are used only as a **measured baseline to beat** (M0 `hpdg_block_audit`): every new
  processor's report shows the same test on the HPDG block and on the HPEQ processor.
- Known defects that the rewrite must not repeat (from the M0 audit): allocations on the audio
  thread (scratch `setSize`, per-call `std::vector`, `IIR::Coefficients` creation), no parameter
  smoothing on EQ coefficients (zipper on automation), only 3 EQ shapes at fixed 12 dB/oct, a
  compressor whose real attack/release/makeup differ from the displayed values (drum "character"
  scaling, block-peak gain reduction, no knee), Monsta randomness tied to HPDG's generator.
- The milestone order is unchanged: the processors come after the M2 routing gate (EQ in M3,
  Compressor/Reverb in M4, Monsta and the rest in M6).
- HPDG itself is not changed by this decision; improving HPDG's own Sound Module is a separate task.
