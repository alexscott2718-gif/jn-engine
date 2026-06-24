# The SCENE Sequencer (task-state story progression)

> RE recovered 2026-06-24 via `tools/ghidra/DumpFunctions.java` against
> `JN_decomp.rep` (Neutron.exe). This documents the global **task-state** machine
> that the per-class SCENE gates (`C3DFowl`, `C3DYokCargo`, `C3DHumphrey`,
> `C3DCindy`, `C3DCarl`, …) read. Native port: `task_loader.c` (store),
> `game_flow.c` (accessors), `behavior_ai_trigger.c` (the writers).

## What SCENE is

`SCENE` is one tag in the **CTaskList entity-state table** (`docs/decomp/CTaskList.md`):
a global linked list of `(name[100], int state)` records seeded once at new-game
from `NewGame.tsk` (`SCENE=30`/`0x1e`). It is the campaign's story-progress
counter. There is **no autonomous / per-frame driver** — SCENE only moves when a
story event writes it.

## The task-state API (measured)

| Fn | Addr | Role |
|---|---|---|
| `get_task_state(name)` | `FUN_0045fea0` | Walk list `DAT_004fc5fc`; return `*(entry+100)` for the name, else `0`. |
| `set_task_state(name,v)` | `FUN_0045f990` | (1) For SCENE, map `v`→an objective ordinal and flag it (`FUN_00406f50`). (2) Write `*(entry+100)=v` for the matching name (existing entries **only** — never appends). (3) Notify every live object whose `+0x10c` TaskName == name via its vtable `+0x424`. |
| inventory/picture flag | `FUN_004038c0(list,slot,v)` | Reward grid write (HUD). |
| counter popup | `FUN_004061d0(id,_)` | On-screen "+counter" notify queue. |
| story screen | `FUN_00406f90(id)` | Show a menu/bonus/story screen. |

Native port keeps the **store + name write**; consumers poll `get_task_state`
each frame, so the object-notify push (`+0x424`) and the objective-ordinal flag
(`FUN_00406f50`) and the reward/counter/menu helpers are **deferred** (HUD/menu
subsystems not yet ported) — they do not affect SCENE or the gates.

## The writers

### 1. NPC talk-progress rewards (deferred — needs the talk system)
Carl/Cindy/Benny/Judy/Sheen/Libby/Nick each `set_task_state("SCENE", …)` at
scripted dialog gates (e.g. Cindy `vfunc_04_096`: at `SCENE==0x104` → `0x10e`;
Carl: `0x3c`/`0x46`/`0x118`; Benny: `0x73`/`0x8d`/`0x15e`). These fire from the
friend talk-reward path, which the native port has **not** landed (`vt_friend`
is idle-only). Not ported here.

### 2. C3DAITrigger story-progress patch table — **PORTED**
`C3DAITrigger::ApplyAITriggerStoryProgress` (`FUN_0040caa0`) is a hardcoded
`ObjectTag × current-SCENE → new-SCENE` table, run by `ActivateAITrigger`
(`FUN_0040c300`) when the player trips the trigger volume. This is the
**player-driven SCENE sequencer**. Full table (verified against the decomp; the
in-corpus level placements were confirmed with `strings` over `assets/gam/*.gam`):

| ObjectTag | Gate (SCENE==) | New SCENE | Level(s) | Extra (deferred) |
|---|---|---|---|---|
| `teleportexplanation` | `0x1e`/`0x1f` | `0x23` | level1b | — |
| `CARLOUT` | `0x46` | `0x4b` | Level1 | — |
| `CALLFROMNICK` | `0x5a` | `0x64` | Level1 | — |
| `INVISPART` | `0xa2`→`0xa8`, `0x1ea`→`500` | — | — | — |
| `GOGODDARD` | `0xa8` | `0xaa` | — | flag(0,2)+menu(6) |
| `ESCAPESHIP` | `0xac` | `0xb2` | Level7 | — |
| `GOINGHOME` | `0xb2` | `0xc8` | — | — |
| `GIVEKEY` | `0xcd` | `0xd2` | level4a | flag(2,0x11)+counter |
| `TICKETBOOTH` | `0x140` | `0x14a` | Level3 | flag(2,0x1c)+counter |
| `GIVEAUTO` | `0x14a` | `0x154` | Level3 | flag(2,0x16)+counter |
| `CARLDIS` | `0x17c` | `0x186` | Level3 | — |
| `GETFUEL4` | `0x186` | `400` | Level3 | menu(5) |
| `BEAMOFF` | `0x19a` | `0x1a4` | Level3/3D/4c | flag(2,0x18)+counter |
| `FOWLINV` | `0x1cc` | `0x1d6` | level4c | — (closes the fowl window) |
| `RESCUECARL` | `0x1e0` | `0x1ea` | level4c | — |
| `LANDSHIP`/`SAVECARL` | `0x1fe` | `0x208` | level5a/5b | — |
| `SEECARL` | `0x208` | `0x212` | Level5b | — |
| `ABDUCTED` | `0x212` | `0x21c` | — | — |
| `EVADEYOKES` | `0x21c` | `0x226` | level6 | — |
| `KITEND1/2/3` | `KITTYn==0` | `KITTYn=10` | level4 | — |
| `REMOTE`/`PUTGODDARD`/`JIMEND`/`RECHARGE`/`BONUSSCREEN`/`RESTARTGAME` | — | (no SCENE write) | — | Goddard/energy/bonus/reload — deferred |

### 3. ActivateObject0..4 state machine (deferred)
`FindActivateObjectForState` (`FUN_0040c230`) picks the next trigger from
`ActivateObject0..4` whose `ActivateState*` matches the trigger's internal state
field (`+0x55c`). We don't model that state field, so the native dispatch keeps
the simpler `NextTrigger`/`ToggleObject` forwarding already in
`behavior_ai_trigger.c`.

## The consumers (SCENE gates)

| FourCC | Class | Gate (decompiled `vfunc_01_265`) |
|---|---|---|
| `3HUM` | `C3DHumphrey` | reveal CLONE1..7 when `SCENE==0x5a` (already wired) |
| `3FOW` | `C3DFowl` | show iff: LEV4 `0x121<SCENE<0x136`; LV4A `SCENE>499`; LV4C `0x1cb<SCENE<0x1d6`; else hide |
| `3YCA` | `C3DYokCargo` | show unless level==LEV5; on LEV5 show iff `SCENE>0x1e9` (489) |

**Regression-safety:** every gate first treats `get_task_state("SCENE") < 0`
(no CTaskList loaded — i.e. a direct `--level` / audit / screenshot launch) as
"show", matching the existing `C3DCindy` guard. The gates therefore only bite in
campaign (`--newgame`/`--menu`) runs where SCENE is real; the audit and
screenshot harnesses (`--level X`, SCENE unloaded) are unaffected.

## Reachability (verified)

`strings assets/gam/*.gam` confirms the story-progress ObjectTags are authored in
the campaign levels exactly where the table expects them — notably
`teleportexplanation` in `level1b.gam` (the campaign start, where SCENE seeds to
`0x1e`), which is the end-to-end validation entry point: at new-game,
tripping it advances `SCENE 0x1e → 0x23`.
