# CJimmyGame

## Identity

| Item | Value |
|---|---|
| RTTI name | `CJimmyGame` |
| Base chain | `CGameType -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c2c74`, `004c2c84`, `004c30d4`, `004c30e8` |
| Ctor(s) | inherits the `CGameType` (`GAME`) construction path; installs the four `CJimmyGame` vftables |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `0044d360` (destroys the `OMediaClassStreamer` subobject at `this+0x163`) |
| Ledger row | `docs/decomp_ledger.csv` |

`CJimmyGame` is the **per-level game-mode controller** and the parent of all 28
`CLevel*Game` level controllers, the 8 `CLevelVR0N` VR controllers, and `CMainMenu`.
It derives from `CGameType` (the lifecycle/database/world-init base — see
[`CGameType.md`](./CGameType.md)) and adds the Jimmy-game mission layer: per-frame
update of the active player/controller object, mission timers/counters seeded in
`InitGame`, and the `SoundEffects.omt` audio bank. The concrete `CLevel*Game`
subclasses are near-empty leaves (0 owned methods) that only bind a specific level's
vtable + `.gam`/`.tsk`; the behavior lives here.

## Field Map

Offsets are from the primary `CJimmyGame` pointer. Ghidra prints them as
`this[N].vftable` against the incomplete struct; `this[N]` is `N * sizeof(slot)` byte
arithmetic, so the named byte offsets are approximate pending struct application.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this[0x23]` | subobject | `init_registrar` | `vfunc_03_045`, `vfunc_03_046` | Inherited registrar/streamer subobject; `InitGame`/`UnInitGame` call its slot `0x3f8` with the `"InitGame..."` / `"UnInitGame..."` trace strings. |
| `this[0x148]` | pointer | `pending_controller` | `vfunc_01_259` | Cleared by the reset slot. |
| `this[0x149]` | pointer | `active_controller` | `vfunc_01_241` (update) | The active player/level controller object; update ticks its slot `0x334` when present. |
| `this[0x14a]` | bool | `active_controller_enabled` | `vfunc_01_241` | Gate checked before ticking `active_controller`. |
| `this[0x150]`, `this[0x151]` | float | `mission_scale_a/b` | `vfunc_03_045` | Both seeded to `1.0` (`0x3f800000`) by `InitGame`. |
| `this[0x152]`, `this[0x153]` | int | `mission_counter_a/b` | `vfunc_03_045` | Both seeded to `5` by `InitGame`. |
| `this[0x154]` | pointer | `init_slot` | `vfunc_03_045` | Cleared at `InitGame` entry. |
| `this[0x16a]` | int | `mission_value` | `vfunc_03_045` | Seeded to `0x64` (100) by `InitGame`. |
| `this[0x16c]` | int | `mission_flag` | `vfunc_03_045` | Cleared by `InitGame`. |
| `this[0x16d]` | bool | `mission_active` | `vfunc_03_045` | Set to 1 by `InitGame`. |
| `this+0x163` | subobject | `class_streamer` | `vfunc_02_002` | `OMediaClassStreamer` tail destroyed by the deleting destructor. |
| `DAT_00696008` | int | `sound_bank_handle` | `vfunc_03_046` | Cleared by `UnInitGame` after releasing `SoundEffects.omt`. |
| `DAT_004f83d8` | pointer | `global_secondary_target` | `vfunc_01_241` | Optional global object pinged (slot `0x200`) at the end of update. |

## Vtable Methods

374 slots walked; 11 owned. Key owned methods:

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 241 | `0044df80` | `UpdateGame` | Runs `FUN_0045f510`, the inherited `CGameType::Update` (slot 241), and `FUN_0047d8f0`; if `active_controller` is set and enabled, ticks its slot `0x334`; runs `FUN_00478530`; if `DAT_004f83d8` is set, calls its slot `0x200`. The per-frame mission/controller pump. | non-trivial |
| vtable 1 slot 259 | `0044df30` | `ResetGame` | Inherited `CGameObject::vfunc_00_013`, then clears `pending_controller` (`this[0x148]`). | trivial |
| vtable 2 slot 2 | `0044d360` | `ScalarDeletingDestructor` | Runs `FUN_0044d390`, destroys `OMediaClassStreamer` at `this+0x163`, frees the adjusted allocation (`this-0xc`) when the deleting flag is set. | non-trivial |
| vtable 3 slot 45 | `0044d3d0` | `InitGame` | Registers the `"InitGame..."` trace, clears `init_slot`, calls `CGameType::InitGame`, runs `FUN_0045f4c0`, zeroes a global vec (`_DAT_005cfc00..08`, with `DAT_005cfc04 = -800.0`), seeds `mission_scale_a/b = 1.0`, runs `FUN_00476930(DAT_00509a3c, 0x20)`, calls inherited slots `0x1b0`/`0x204`, seeds `mission_counter_a/b = 5`, `mission_value = 100`, clears `mission_flag`, sets `mission_active = 1`. | non-trivial |
| vtable 3 slot 46 | `0044d4a0` | `UnInitGame` | Registers `"UnInitGame..."`, releases the `SoundEffects.omt` bank (`FUN_0046aef0`), clears `sound_bank_handle`, calls `CGameType::UnInitGame`. | non-trivial |
| vtable 3 slot 57 | `0044e7b0` | `LoadByName` | Copies a caller-supplied name string into a local buffer (default `&DAT_004c3260`), then dispatches through inherited slot `0x16c`. Name-keyed load path (level/task load by string). | raw block |

The remaining owned slots are mission/registration helpers in the same `0044d…`
range; the six above carry the gameplay-relevant behavior.

## Per-Frame Behavior

```c
CJimmyGame::UpdateGame():               // vfunc_01_241 @ 0044df80
    FUN_0045f510()
    CGameType::Update()                 // inherited world/database/help pump
    FUN_0047d8f0()
    if active_controller != NULL and active_controller_enabled:
        active_controller->slot_0x334(0)   // tick the active player/level controller
    FUN_00478530()
    if global_secondary_target != NULL:
        global_secondary_target->slot_0x200()
```

```c
CJimmyGame::InitGame():                 // vfunc_03_045 @ 0044d3d0
    trace("InitGame...")
    init_slot = NULL
    CGameType::InitGame()               // database/world/terrain setup
    mission_scale_a = mission_scale_b = 1.0
    mission_counter_a = mission_counter_b = 5
    mission_value = 100
    mission_flag = 0
    mission_active = 1
```

## Constants And Wiring

| Property | Value | Consuming Logic |
|---|---|---|
| `mission_scale_a/b` | `1.0` | Seeded by `InitGame`; mission rate/scale. |
| `mission_counter_a/b` | `5` | Seeded by `InitGame`; mission counters (lives/retries class default). |
| `mission_value` | `100` | Seeded by `InitGame`. |
| `SoundEffects.omt` | audio bank | Loaded for the mode, released by `UnInitGame`. |
| `CTaskList` | task/state object | Owned per level; supplies spawn + entity-state table (see [`CTaskList.md`](./CTaskList.md)). |
| `.gam` `TaskName` / `NewTaskState` | per-object | Drive mission-state transitions over the task tag namespace. |

`CJimmyGame` itself is not a level-placeable `.gam` object — it is the game-mode
controller instantiated per level by the load path, not by a FourCC row.

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| Audio bank | `SoundEffects.omt` | `vfunc_03_046`; `.rdata:004ee08c` | Loaded for the active game mode; released on uninit. |
| Task file | `NewGame.tsk` / `RestartLevel.tsk` | via owned `CTaskList` | Spawn + entity-state source. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CJimmyGame` (`slots=374`, `owned_methods=11`,
`offsets=2`); decompiled `InitGame`/`UnInitGame`/`UpdateGame`/destructor; base-chain
and `CGameType` cross-reference. `this[N]` offsets are slot-arithmetic approximations
pending struct application. Not runtime-validated.

Open questions:
- Resolve the producer of `active_controller` (`this[0x149]`) — is it the `C3DJimmy`
  player or a level-specific controller? Slot `0x334` is the per-frame tick.
- Name the `0044d…` mission helper slots beyond the six characterized here.
- Confirm `LoadByName` (`vfunc_03_057`) is the level/task-by-name entry and identify
  its default string `&DAT_004c3260`.
- Map `mission_counter_a/b`, `mission_value` to on-screen HUD counters in
  [`C2DInGameMenu.md`](./C2DInGameMenu.md).

## Notes

- Evidence: `DumpClass.java CJimmyGame /tmp/decomp_CJimmyGame.md`; executable string
  table `.rdata:004ec71c` (level-file table: `NewGame.tsk`, VR `.gam` list,
  `level1f.gam`, `RestartLevel.tsk`).
- Parent of the `CLevel*Game` / `CLevelVR0N` batch (see those specs — all are
  0-owned-method leaves that inherit this controller).

## Native Linkage (linked-parity branch)

Aspect: **`initgame-seed`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle:
`tools/linkage_oracles/CJimmyGame.py`.

This aspect certifies exactly the mission-seed constants `InitGame`
(`0044d3d0`) writes — the fully decompiled part of `CJimmyGame`'s per-frame/
lifecycle behavior. It does **not** cover the death/restart lives-decrement
flow or the win-condition bridge described in `docs/PROJECT_HISTORY.md` —
neither corresponds to a decompiled `CJimmyGame` method in the Vtable Methods
table above (see "Not covered" below).

### L2 — transcription map

| Decompiled (`CJimmyGame::InitGame` @ `0044d3d0`) | Native (`game_flow_init_game`, `src/game/game_flow.c`) |
|---|---|
| `mission_counter_a = mission_counter_b = 5` | `g_flow.lives = JIMMYGAME_DEFAULT_LIVES` (`#define ... 5`) |
| `mission_value = 0x64` (100) | `g_flow.mission_value = JIMMYGAME_DEFAULT_MISSION_VAL` (`#define ... 100`) |
| `mission_active = 1` | `g_flow.mission_active = 1` |
| `mission_scale_a = mission_scale_b = 1.0` | not ported (no native consumer of a mission "scale" yet) |
| `mission_flag = 0` | not individually ported; `level_won = 0` is the nearest native field but is not a 1:1 map (see deviations) |

### L3 — oracle

`tools/linkage_oracles/CJimmyGame.py` compiles and runs the real, unmodified
`game_flow_init_game`/`game_flow_lives`/`game_flow_mission_value`
(`cjimmygame_dump.c` links `game_flow.c` + its only non-libc dependency,
`task_loader.c`) and asserts: the pre-seed globals read `0` (BSS zero-init,
proving the post-seed values genuinely come from calling `InitGame`, not a
coincidental default); post-seed `lives==5, mission_value==100` (the
decompiled constants); and that re-seeding is idempotent (a second call still
yields `5, 100`, matching the decompiled body's unconditional overwrite, not
an accumulate).

### Deliberate deviations (native-only; outside the linked aspect)

- **`mission_scale_a/b` (seeded to `1.0`) is not ported.** No native consumer
  reads a mission "scale" value yet; nothing to certify against.
- **`mission_flag` (cleared to `0`) has no direct native field.** `level_won`
  is the closest native analogue but serves a different purpose (the win
  latch, a native-port invention — see "Not covered") and is not claimed as
  a 1:1 map to `mission_flag`.

### Not covered by this aspect (still open)

- **Death/restart lives-decrement** (`game_flow_player_died`). Routed through
  "`C2DInGameMenu` semantics" per `docs/PROJECT_HISTORY.md`, but
  `docs/decomp/C2DInGameMenu.md` does not carry a recovered body for this
  specific decrement-and-respawn-or-game-over flow; no decompiled ground
  truth to certify against this pass.
- **Win-condition bridge** (`game_flow_level_objective_met`). Per
  `docs/decomp/_next_session.md`, this "bridges `gamestate`'s level-clear to
  `game_flow_level_objective_met()`" — a native-port design choice for
  wiring level-completion signals (e.g. the VR trophy pickup) into the
  mission layer, not a decompiled `CJimmyGame` method. `CJimmyGame.md`'s
  Vtable Methods table has no owned method matching this behavior. Certifying
  it here would mean treating a native invention as if it were proven
  faithful to Neutron.exe — flagged rather than claimed.
