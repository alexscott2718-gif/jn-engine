# Native Linux Port Plan — Gameplay Classes → Running Engine

> **Successor to** [`codex_full_decomp_plan.md`](./codex_full_decomp_plan.md). That plan's
> deliverable (a faithful spec for all 208 `Neutron.exe` gameplay classes) is **DONE** —
> `docs/decomp/*.md` + `docs/decomp_ledger.csv` are at `status=spec` for 208/208. This plan
> consumes those specs to build **runtime behavior in the native C engine** (`jnengine`).
>
> **Artifact decision (2026-06-22):** the Godot bridge is **retired**. The decomp specs now
> feed the **native Linux port** — the C engine in `src/` is the product, not a foundry for a
> Godot game. `docs/godot_bridge_plan.md` is superseded; do not start new work against it.

## 0. Where the engine stands (survey, 2026-06-22)

The engine resolves each `.gam` object through **two independent tables**, so "implemented"
has two meanings:

| Layer | Where | Coverage |
|---|---|---|
| **Spec** (behavioral doc) | `docs/decomp/*.md` + ledger | **208 / 208 ✅** |
| **Visual** (mesh/sprite/scale/orient) | `src/game/entity_visual.c` | ~120 FourCCs (broad) |
| **Behavior** (runtime logic) | `src/game/behaviors/` via `entities.c` | **31 FourCCs → 21 vtables (narrow)** |

Anything outside the behavior table falls through to `vt_default` (inert) or `vt_static`
(solid prop). **Implemented well:** mechanisms/moving parts (~12 of 21), player movement,
triggers/sound/music/load, Carl's patrol walker. **Essentially unimplemented:** enemies/AI
(20 specs, only Carl), friends/NPCs (20, only the player), vehicles (12, all visual-only),
level/game controllers (43 — `main.c` is a *generic* loop, not a port of `CJimmyGame` / the
task-objective / menu / cutscene system). ~95 of 208 specs have a doc but no runtime behavior.

## 1. The implementation contract (read before writing any behavior)

A native gameplay class is an `EntityVTable` (`src/engine/world.h`) registered against its
FourCC in `src/game/entities.c`:

```c
typedef struct EntityVTable {
    void (*on_spawn) (Entity *e, World *w);            /* once, after gam_load */
    void (*on_update)(Entity *e, World *w, float dt);  /* per frame, DT fixed step */
    void (*on_trigger)(Entity *e, Entity *by);         /* on player overlap if FLAG_TRIGGER */
    unsigned int flags;  /* PHYSICS | SOLID | TRIGGER | PLAYER */
} EntityVTable;
```

- **Lifecycle:** `entity_bind_vtables()` resolves the vtable + runs `on_spawn`;
  `entity_update()` calls `on_update` each fixed tick (`main.c:1418`).
- **Authored parameters** come from the `.gam` property bag — read by name with
  `gam_prop_f(e,"FanSpeed",def)` / `gam_prop_i(...)`; well-known props are pre-mapped onto
  named `Entity` fields (see `world.h`). The validated property set for each class is in its
  `docs/decomp/<Class>.md` (§Constants).
- **Per-instance scratch:** `home[3]`, `patrol_to[3]`, `link_target`, `user_flag`,
  `user_float`, `vx/vy/vz`. Add new shared fields to `Entity` only when several behaviors need
  them; otherwise reuse `props`/scratch.
- **Queries:** `world_find_type`, tag scan over `w->head`, `world_query_segment` (raycast
  through SOLID AABBs — the basis for line-of-sight and projectile hits).
- **Exemplars to copy:** `behavior_walker.c` (AI + nav graph), `behavior_player.c` (physics +
  input), `behavior_button.c` (`link_target` activation wiring).

## 2. Leverage rule (same as the decomp plan): bases → families → leaves

The 208 classes are an inheritance DAG. Port **shared base behavior once** as a reusable
native helper; every leaf is then a thin override. Never start a leaf before its base exists
natively. This is why Wave N1 is bases, not the flashiest enemy.

## 3. Validation (every wave)

- **Visual:** `JN_SCREENSHOT=1 JN_SCREENSHOT_PATH=... ./jnengine` per affected level; eyeball
  against the XP capture / existing QA pages.
- **In-engine QA:** the `qa.c` annotate tool (B-key) for picking + tagging regressions.
- **Faithfulness sweep:** `python3 tools/audit_faithfulness.py` must stay at 0 findings across
  the 35 levels.
- **Motion ground-truth:** where a class's dynamics matter (AI paths, projectile arcs,
  vehicle drive), diff produced motion against the marked `.omtc` capture — the same method
  that validated the player jump arc. Capture is the **validator**, not a runtime dependency.
- **Web regression:** `python3 tools/qa_web_verify.py` (16 checks) before any deploy.

## 4. The waves

Each wave: new `behavior_*.c` files, registered in `entities.c`, faithful to the matching
`docs/decomp/<Class>.md`, validated per §3, **one commit per class** on the `native-port`
branch (recommend branching off `decomp-campaign`). Commit only `src/**`, `tools/**`, and
wave-end `PROJECT_HISTORY.md` — leave the pre-existing dirty asset tree alone.

### Wave N1 — Base behavior framework *(unblocks every later wave)*
Turn the shared bases into reusable native modules instead of per-leaf copies.
- `C3DObject` / `C3DAnimated` lifecycle → a shared `on_update` prologue: level-visibility
  gates, `InitiallyVisible`, collision-pass flag, anim advance. (`docs/decomp/C3DObject.md`,
  `C3DAnimated.md`.)
- `C3DFlyingObject` movement base (`3FLY`: MaxSpeed/UpRate/DownRate/NewGravity/lean) →
  extract a `movement_base` helper from `physics.c` + `behavior_player.c` so vehicles/AI can
  reuse it. (`docs/decomp/C3DFlyingObject.md`, `C3DPlayer.md`.)
- `C3DAI` core state machine (target/range/FOV/state/wander) → `behavior_ai.c` exposing a
  reusable seek/patrol/idle base; **re-point `vt_walker` onto it** to prove the base.
  (`docs/decomp/C3DAI.md`, `C3DAIOmtObj.md`.)
- `C3DTriggerType` / `C3DSpriteType` overlap base → consolidate the trigger/pickup overlap
  test used by items, buttons, triggers.
- **Done when:** Carl still patrols correctly through the new `behavior_ai.c` base, and the
  shared prologue is wired into at least the mechanism behaviors with no visual regression.

### Wave N2 — Enemies / AI *(20 specs; build on N1's `behavior_ai.c`)*
Order by level reach. Yokian family first (it owns the back third of the game):
`C3DYokian`, `C3DYokianSoldier`, `C3DYokianSpy`, `C3DYokianGuard`, `C3DYokTurret`,
`C3DYokianShield`, `C3DYokianShip`. Then one-off threats: `C3DDigger`, `C3DTank`, `C3DTesla`,
`C3DHarrier`, `C3DEnemyAircraft`, `C3DMine`, `C3DLaserTrigger`.
- New shared module **`behavior_projectile.c`** (missiles, baseballs, shrink-ray bolts):
  spawn → integrate → `world_query_segment` hit test → effect. Needed by enemies *and* Wave N3.
- Minimal **health/hit model** in `gamestate.c` (player damage, enemy knockout) — keep it data-
  driven from `.gam` where the props exist.
- **Done when:** at least the Yokian soldier seeks + attacks the player and can be defeated,
  with motion sanity-checked against capture.

### Wave N3 — Player combat + the pickups family *(12 specs)*
Replace the generic `vt_item` with per-type behaviors where the spec differs:
`C3DBalloon`, `C3DBaseball`/`C3DBaseballPickup`, `C3DBubble`/`C3DBubblePickup`,
`C3DGraplingHook`/`C3DHook`, `C3DShrinkRay`, `C3DMetalPickup`, `C3DHelmet`. Wire inventory →
ability, and the player throw/attack using `behavior_projectile.c`. (`docs/decomp/C3DPickupItem.md`,
`C3DPickupType.md`, `CPickupType.md`.)
- **Done when:** picking up an item grants its ability and the player can use the thrown/fired
  items against Wave N2 enemies.

### Wave N4 — Vehicles *(12 specs)*
A shared **`behavior_vehicle.c`** ride base (mount/dismount, drive physics over `movement_base`),
then leaves: `C3DBus`/`C3DAICar` (AI-driven), `C3DJeep`, `C3DNeuCar`/`C3DNeuCar2`,
`C3DSailBoat`, `C3DSub`, `C3DSkateBoard`, `C3DRocket`/`C3DRocketShip`, `C3DPod`, `C3DWheel`.
Distinguish AI-driven (3SBU bus) from player-driven. (`docs/decomp/C3DVehicle.md`,
`C3DAICar.md`.)
- **Done when:** the player can mount and drive one vehicle end-to-end on its level.

### Wave N5 — Game-flow / level controllers *(43 specs; the biggest structural gap)*
Today `main.c` is a generic loop. Port the controller layer:
- `CJimmyGame` master controller + `CLoadLevel` + `CTaskList` → a real **objective / win-
  condition / task-state** layer (replacing ad-hoc level logic).
- `CMainMenu` / `CMenuElement` / `C2DInGameMenu` → menu, pause, in-game HUD menu.
- 40 `CLevel*Game` / `CLevelVR*` → a **data-driven per-level script table** (most are thin
  hooks; a few carry real logic, e.g. `CLevel01FGame` death/restart). Don't hand-port 40
  near-identical controllers — table-drive them and special-case the outliers.
- Cutscene sequencing (`CMultiCutSceneCamera` + the cutscene-camera classes) → a scripted
  camera timeline.
- **Done when:** a level can be entered from a menu, its objectives tracked to a win state, and
  it transitions to the next level — without per-level C hardcoding.

## 5. Sequencing rationale

Bases (N1) unlock the rest. Enemies (N2) + combat/items (N3) make the world *react* — the
largest perceived-completeness jump. Vehicles (N4) are self-contained and ride on N1's movement
base. Game-flow (N5) is last because it's the most structural and benefits from having real
gameplay to wrap. Within each wave, hold the decomp discipline: **confirm every behavior against
the decompiled body in `docs/decomp/<Class>.md`, not an offset scan**, and don't relitigate the
settled invariants (matrix convention, `PROJ[3][3]=1`, no X-mirror, `canvas_id=Canv+1`, DIFFUSE
alpha 0, no fog — see `PROJECT_HISTORY.md` §Invariants).

## 6. Deferred polish — rideable rocket (do toward the end)

The rocket flight model (forward-thrust on SPACE / "go", W·S pitch the nose, A·D yaw, Ctrl·Q
brake) and the visible seated Jimmy landed 2026-06-23. Two polish items were explicitly deferred
to the end of the rocket work:

1. **Engine audio.** Loop `rocket_fly` (soundeffects.omt handle **195**) while thrust (SPACE) is
   held, with a `rocket_blast` (handle **196**) one-shot on ignition (thrust off→on); **halt the
   loop the instant the engine is cut** so no-thrust = silence. Also halt on dismount. The mixer
   API is ready: `audio_play_db("soundeffects", 195, -1, gain)` for the loop, `audio_channel_halt`
   for the cut. Track the loop channel + an `engine_on` latch in `behavior_vehicle.c` so ignition
   fires exactly once. (Investigated + asset-confirmed 2026-06-23; not yet wired.)
2. **Engine exhaust effect.** A fire/exhaust visual behind the rocket while thrusting — emitted at
   the tail (rocket center minus the forward aim vector `(sin ry·cos rx, -sin rx, cos ry·cos rx)`),
   scaled with `move_speed`/thrust, hidden when the engine is cut. Reuse the existing
   billboard/effect draw path.
