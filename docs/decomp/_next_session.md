# Next Session — Kickoff Prompt (Native Linux Port)

> Paste this (or point the agent at this path) to resume work. The decomp **spec** campaign
> is finished; the active campaign is now the **native C port** of the gameplay classes.
> Living doc: update "Current state" + "Your task" at each wave boundary so the next fresh
> session starts oriented. Source of truth: the per-class specs (`docs/decomp/*.md`), the
> ledger, `PROJECT_HISTORY.md`, and the wave plan (`docs/native_port_plan.md`).

---

You are porting `Neutron.exe`'s gameplay classes into **runtime behavior in the native C
engine** (`jnengine`), using the already-written behavioral specs as the source of truth.
This is a shared, committed campaign — read the shared docs, don't rely on tool-private memory.

## Orient yourself first (read in this order)
1. `~/AGENTS.md` (== `~/CLAUDE.md`; machine/workflow conventions, anti-silo policy)
2. `~/jn-engine/docs/native_port_plan.md` — **the execution plan** (§1 implementation
   contract, §3 validation, §4 the waves, §5 sequencing). **Start here.**
3. `~/jn-engine/docs/PROJECT_HISTORY.md` + `docs/ARCHITECTURE.md` — engine state + invariants.
4. `~/jn-engine/docs/decomp_ledger.csv` — 208 rows, all `status=spec`; per-class behavior
   source of truth is `docs/decomp/<Class>.md`.
5. Exemplar behaviors before writing any: `src/game/behaviors/behavior_walker.c` (AI + nav),
   `behavior_player.c` (physics/input), `behavior_button.c` (activation wiring).

## Current state (branch: decomp-campaign → recommend branching `native-port`)
- **Decomp spec campaign DONE:** all 208 `C*` gameplay classes have `docs/decomp/<Class>.md`
  at `status=spec`. Do not re-derive specs from the binary; consume them.
- **Godot retired (2026-06-22):** `docs/godot_bridge_plan.md` is superseded. The specs feed
  the **native port**; the C engine in `src/` is the product.
- **Native engine today (see plan §0 survey):** renderer + level geometry + mechanical props
  (fan/switch/geyser/pendulum/button/door/platform/checkpoint) + player movement + Carl's
  patrol walker all work. **Unimplemented:** enemies/AI (only Carl), friends/NPCs (only the
  player), vehicles (visual-only), and the game-flow/level-controller layer (`main.c` is a
  generic loop, not a port of `CJimmyGame`/tasks/menus/cutscenes). ~95 of 208 specs have a
  doc but no runtime behavior.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: start Wave N1 (base behavior framework)
Per plan §4, port the shared bases first so later leaves are thin overrides:
- `C3DObject`/`C3DAnimated` lifecycle → shared `on_update` prologue (visibility gates,
  `InitiallyVisible`, collision pass, anim advance).
- `C3DFlyingObject` movement base → extract a reusable `movement_base` helper from
  `physics.c` + `behavior_player.c`.
- `C3DAI` core state machine → new `behavior_ai.c` (seek/patrol/idle base); **re-point
  `vt_walker` onto it** as the proof that the base works.
- `C3DTriggerType`/`C3DSpriteType` overlap base → consolidate the trigger/pickup overlap test.

**Done when:** Carl still patrols correctly through `behavior_ai.c`, the shared prologue is
wired into at least the mechanism behaviors, and there is **no visual regression**
(`tools/audit_faithfulness.py` = 0 findings; spot-check levels via `JN_SCREENSHOT`).

## Inner loop (per class)
1. Read `docs/decomp/<Class>.md` (§Field map, §Per-frame behavior, §Constants). Confirm
   behavior against the documented decompiled body — not an offset/immediate scan.
2. Write/extend `src/game/behaviors/behavior_<x>.c`; read authored params with `gam_prop_*`.
   Add an `Entity` field only when several behaviors need it; else reuse scratch/`props`.
3. Register the FourCC → vtable in `src/game/entities.c`.
4. Build (`make`), then validate per plan §3: `JN_SCREENSHOT` on affected levels,
   `tools/audit_faithfulness.py`, and motion-diff vs the marked `.omtc` where dynamics matter.
5. Commit per class: `port(<Class>): <one-line behavior>`.

## Hard rules
- Commit ONLY port artifacts: `src/**`, `tools/**`, and (at wave end) `PROJECT_HISTORY.md`.
  There is a large PRE-EXISTING dirty asset tree — do NOT stage or touch it.
- Don't relitigate settled invariants (matrix convention, `PROJ[3][3]=1`, no X-mirror,
  `canvas_id=Canv+1`, DIFFUSE alpha 0, no fog) — see `PROJECT_HISTORY.md` §Invariants.
- Capture (`.omtc`) is a **validator**, never a runtime dependency.
- The per-class specs + ledger + `PROJECT_HISTORY.md` + `native_port_plan.md` are the only
  source of truth.

## Definition of done for this session
Wave N1 base modules exist and build; `vt_walker` runs on the new AI base with Carl's patrol
intact; faithfulness sweep clean; per-class commits pushed on `native-port`. If the wave
closes, append a Wave N1 paragraph to `PROJECT_HISTORY.md` and update this handoff to point at
Wave N2 (enemies/AI).
