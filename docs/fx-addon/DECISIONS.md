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

## D4 — ACCEPTED 2026-10-21 — Frequency core H3 instead of H1

**Evidence:** `reports/M2_REPORT.md`. H1 ("delta bands", spec default) is bit-exact at bypass but adds
up to +2.3 dB (LR8: +3.9 dB) of false boost when a node cuts, because the minimum-phase band rotates
in phase against the dry signal. H2 has a correct, smooth response but cannot overlap. H3 (one shared
Linkwitz-Riley tree over all node edges, nodes = sums of elementary bands, deltas added to the
all-pass dry) keeps H2's response and supports overlap.

**Would override:** `01` §5 "default для перекрывающихся FX Nodes в V1 — H1" and the exact-identity
reading of T01/T02 for "node bypassed while other nodes are active" (compared to the documented
all-pass result instead, 04 §3). "All FX off" stays bit-exact: the tree is not run.

**Owner:** accepted 2026-10-21 ("внедряй H3 в движок ..., кроссфейд при смене набора границ").

**Implementation note (measured, same principle, different transition method):** the planned
crossfade between the old and the new tree on a structure change produced a **17.8 dB level dip**
on a 1 kHz tone (two differently phased all-pass signals cancel mid-fade). The engine therefore uses
the order-free form of H3: each node's band is its own LR4 band times the all-passes of all other
nodes' edges, the dry is the product of all edge all-passes; band/dry stays real, so the response is
the same phase-coherent H3 response, and edges are born and retired at 1 Hz (where an all-pass is
numerically an identity) and glide, so no crossfade is ever needed (worst dip 0.52 dB). Overlap and
edge crossing need no special handling. Evidence: `reports/M2_REPORT.md` §5.

**Implementation note 2 (CPU pass, 2026-10-21):** the tree became O(N) (Horner recombination: a node
processes its LR4 band carrying only the earlier nodes' edge all-passes, and its delta is all-passed by
the later nodes'). The linear response is unchanged (engine vs offline H3 formula -142.7...-155.4 dB).
For non-linear processors the detector now sees the band with a different all-pass phase than before;
on drums the first node's peak gain reduction moved from 14.19 to 15.70 dB in the M4 scenario. The
delta stays phase-coherent with the dry. Evidence: `reports/CPU_REPORT.md`.

## D5 — 2026-10-22 — Layer semantics for M7 (agent decision, owner may revise)

**Context:** `01` §8 defines Parallel (delta) and Serial routing and lists layer controls, but not what
Bypass / Mute / Solo mean in a delta mix, nor the latency rule across layers or memory sharing.

**Decision:**
- **Weight** `w_j = mix_j · heard_j`, `heard_j = !mute_j && (no enabled layer soloed || solo_j)`.
  Parallel `y = D_L(x) + Σ w_j (D_{L−L_j}(L_j{x}) − D_L(x))`; Serial `x_{j+1} = D_{L_j}(x_j) + w_j (L_j{x_j} − D_{L_j}(x_j))`.
- **Mute / Solo / Mix** only change `w_j` (20 ms ramp); the layer keeps running, so un-muting is seamless
  (tested bit-exact after the fade). Solo is diagnostic and does not change stored mutes.
- **Bypass (layer off)** fades `w_j` out and then stops processing the layer (CPU saved). Its tails are
  cut by that fade; re-enabling resets the layer. (Node bypass still keeps tails.)
- **Latency:** `L_j` counts whether the layer is on, muted or soloed (same rule as nodes), so toggling
  never changes the host latency. Parallel `L = max L_j`, Serial `L = Σ L_j` (a bypassed Serial layer is a
  pure delay).
- **Lazy layers:** a layer with neutral PRE-EQ, no running node and identity controls is skipped; its
  delta is exactly 0, so empty / bypassed layers give the input bit-exactly.
- **Layer channel mode** = which component the layer processes (Stereo / Left / Right / Mid / Side); the
  rest passes delayed by `L_j`. Linked/Dual Mono are node (detector) concepts and act as Stereo here.
- **Controls order:** gain → M/S width → linear balance (pan), after the layer engine.
- **Pools** (Reverb 16, Delay 8, Monsta 8) are shared by the whole project instead of 8/4/4 per layer:
  per-layer pools would multiply memory by 10. Starvation is reported per slot as before.
- **Routing change** restarts the alignment lines (the latency changes anyway); not a crossfade.
- Selecting a layer in the editor is saved with the state but is not an undo step.

**Consequences:** M4/M6 pool tests were rewritten for the shared sizes (`kSize + 1` starved).

## D6 — 2026-10-22 — Modules on EQ points; module-grid inspector (owner feedback F1–F3)

**Owner answers:** chain limit stays **6** per point / region (grid 3 × 2); the band of an EQ point's
modules comes **from the point's frequency and Q**.

**Decision:**
- An EQ point may carry a chain of 0–6 modules. The point's EQ stays in PRE-EQ (decision B); its modules
  run like an FX region on the band `[f·(√(1 + 1/(4Q²)) − 1/(2Q)), f·(√(1 + 1/(4Q²)) + 1/(2Q))]` (the
  edges of a band-pass with that centre and Q; for every shape), on the point's channel component, node
  mix 1. Turning the point off turns its modules off (tails ring out as for a region).
- FX regions and points with modules share the layer's 8 engine nodes. Each gets a **stable engine
  slot** (0–7) stored in the state, so adding or deleting one never moves another's processing state
  (before D6, deleting region 1 shifted all later regions to other engine nodes). A point with no
  modules uses no slot. When all 8 are used, adding modules to another point is refused in the UI.
- Inspector: the chain is a 3 × 2 grid of tiles in processing order (left to right, then the next row);
  an empty tile is "+" (adds a module at the end). Clicking a tile opens that module's page in the same
  panel (On, Mix, type, move earlier/later, remove, parameters) with a back arrow to the grid.
- An FX region keeps at least one module (as before); a point may have none.
