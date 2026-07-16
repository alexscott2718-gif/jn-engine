# CGameType

## Identity

| Item | Value |
|---|---|
| RTTI name | `CGameType` |
| Base chain | `OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d641c`, `004d642c`, `004d687c`, `004d6890` |
| Ctor(s) | constructor/factory at `004745a0` for `GAME` |
| Dtor(s) | adjusted scalar deleting destructor at `004746b0`; cleanup helper at `004746e0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`CGameType` is the base controller for `CJimmyGame`, the level controllers, VR controllers, and `CMainMenu`. It is not a level-placeable `.gam` object; it is the game-mode/lifecycle object that owns global database setup, world init/uninit, terrain-object loading, pause/help state, and per-frame callback dispatch.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x078` | FourCC/raw id | `game_class_id` | slot 43 `00474790`; ctor | Set to `GAME` by the constructor. |
| `0x08c` | subobject | `local_game_object` | ctor/dtor | Embedded `CLocalGameObject`/`CGameObject` subobject used for inherited properties, tracing, and update behavior. |
| `0x490` | FourCC/raw id | `game_class_id_mirror` | slot 43 `00474790`; ctor | Mirrors `game_class_id`; likely for lookup paths that expect the id at the CGameObject-style offset. |
| `0x544` | subobject | `class_streamer` | ctor/dtor | Optional `OMediaClassStreamer` subobject, constructed when the constructor flag is non-zero. |
| static `DAT_00509948` | pointer | `current_game_type` | ctor | Set to the constructed `CGameType`; many inherited object services reach the current game/controller through this global. |
| static `DAT_005099a9` | byte | `pause_or_game_blocked` | `InitGame`; helper `00475a30` | Cleared on `InitGame`; set by a small helper. Exact pause semantics are still tied to menu/help flow. |
| static `DAT_005099c8..DAT_005099dc` | pointers | `database_streams_or_sources` | database init slots | Lazily allocated database source objects for effects, object, song, sound, terrain, and vehicle databases. |
| static `DAT_005099b0..DAT_005099c4` | pointers | `database_handles` | database init slots | Runtime database handles paired with the source objects above. |
| static `DAT_005099e4` | pointer | `global_player_or_update_target` | slot 70; update slot 241 | Optional object notified during `CGameType` update; also recognized by AI/pickup code as the player pointer. |
| static `DAT_005099e8` | pointer | `active_terrain_object` | terrain slots 61/62 | Set while resolving `3TER` terrain objects from a loaded database. |
| static `DAT_005099ee` | byte | `help_visible` | slot 78 | Toggle flag used by the help overlay open/close slots. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `004745a0` | `CtorGameTypeGAME` | Constructs inherited OMedia/`CLocalGameObject` state, installs all four `CGameType` vftables, traces/registers `CGameType()`/`CGAMETYPE`, sets class id `GAME`, attaches the global world/supervisor object, clears the player/update-target pointer, and stores `this` in `DAT_00509948`. | non-trivial |
| dtor | `004746b0` | `ScalarDeletingDestructor` | Adjusts from the embedded subobject back to the primary pointer, runs cleanup helper `004746e0`, destroys `class_streamer`, and frees the primary allocation when requested. | non-trivial |
| cleanup | `004746e0` | `CleanupGameType` | Reinstalls `CGameType` vftables/displacement entries, traces `~CGameType()`, clears `DAT_005099e4`, destroys the embedded `CLocalGameObject`, and tail-calls the inherited OMedia destructor path. | non-trivial |
| vtable 3 slot 43 | `00474790` | `SetGameClassId` | Writes the supplied id to primary `0x78` and mirror `0x490`; constructor passes canonical `GAME`. | trivial |
| vtable 1 slot 7 | `004749a0` | `InitObject` | Traces `InitObject()`, runs two global reset/setup helpers, calls `InitGame`, then runs the inherited no-op hook. | non-trivial |
| vtable 1 slot 8 | `004749e0` | `UnInitObject` | Traces `UnInitObject()`, calls `UnInitGame`, then runs the inherited no-op hook. | non-trivial |
| vtable 3 slot 45 | `00474a10` | `InitGame` | Traces `InitGame()`, enables the main game/update gate, seeds `DAT_00509a50` camera/global target offsets (`+0x44=0`, `+0x48=10000.0`, `+0x4c=0`), and clears `pause_or_game_blocked`. | non-trivial |
| vtable 3 slot 46 | `00474a50` | `UnInitGame` | Traces `UnInitGame()`, runs global reset/uninit helpers for gameplay and input/control state. | non-trivial |
| vtable 1 slot 13 | `00474a80` | `UnInitDatabases` | Traces `UnInitDatabases()`, then calls the database uninit slots for effects, object, song, sound, terrain, and vehicle databases. | non-trivial |
| vtable 3 slots 49/51/53/55/57/59 | `00474b60`, `00474d00`, `00474ea0`, `00475040`, `00475210`, `004753b0` | `InitDatabaseFamily` | Lazily allocates and opens paired source/handle globals for effects, object, song, sound, terrain, and vehicle databases. The sound variant logs `InitSoundDatabase(%s)` and also writes `DAT_00696008`. | non-trivial |
| vtable 3 slots 61/62 | `00475550`, `004757a0` | `LoadTerrainObjectsFromDatabase` | Iterates database chunks with raw id `sd3E`, resolves canonical class id `3TER`, calls terrain object lifecycle/load slots, and stores the current terrain object in `DAT_005099e8`. Slot 62 performs one extra post-load call `00477550`. | non-trivial |
| vtable 1 slot 241 | `00475a70` | `UpdateGameType` | Stores a frame/update predicate into `DAT_00509aec`, delegates to `CLocalGameObject` update, calls a primary update hook, then forwards the frame argument to `global_player_or_update_target` slot `0x3cc` when that pointer is set. | non-trivial |
| vtable 3 slot 70 | `00475a40` | `SetGlobalPlayerOrUpdateTarget` | Stores the supplied pointer in `DAT_005099e4`. | trivial |
| vtable 3 slot 78 | `00475ce0` | `ToggleHelp` | Traces `ToggleHelp()`, calls either the hide-help or show-help slot, and flips `help_visible`. | non-trivial |

## Per-Frame Behavior

```c
CGameType::InitObject():
    trace("InitObject()")
    reset_global_gameplay_state()
    reset_global_control_state()
    InitGame()

CGameType::InitGame():
    enable_game_update_gate(true)
    DAT_00509a50->camera_or_target_offset = (0, 10000.0, 0)
    pause_or_game_blocked = false

CGameType::UpdateGameType(dt_or_event):
    DAT_00509aec = compute_update_predicate()
    CLocalGameObject::Update(dt_or_event)
    primary_update_hook()
    if global_player_or_update_target != null:
        global_player_or_update_target->slot_0x3cc(dt_or_event)
```

The terrain loader slots are the reason `_gam_classids.tsv` has `3TER` hits inside `CGameType` methods. Those are database lookups/load loops, not the identity of `CGameType`.

## Constants And Wiring

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `GAME` | FourCC | ctor immediate `0x47414d45` | no `.gam` rows | Bound by constructor/factory `004745a0`. |
| `CGAMETYPE` | class string | `0x4f644c` | trace/registration | Constructor class registration. |
| `CGameType()` | trace string | `0x4f6440` | trace/registration | Constructor trace/registration. |
| `~CGameType()` | trace string | `0x4f6458` | cleanup | Destructor trace. |
| `InitGame()` / `UnInitGame()` | trace strings | `0x4f1acc`, `0x4f1ad8` | lifecycle slots | Logged by game init/uninit. |
| `UnInitDatabases()` | trace string | `0x4f6480` | slot 13 | Logged before database cleanup chain. |
| `InitSoundDatabase(%s)` | trace string | `0x4f64dc` | database slot 55 | Sound database init trace with source name. |
| `PauseGame(%d)` | trace string | `0x4f653c` | helper `00475a00` | Pause trace helper; exact state write is separate. |
| `ToggleHelp()` | trace string | `0x4f654c` | slot 78 | Help overlay toggle trace. |
| `3TER` | FourCC | terrain slots 61/62 | raw immediate `RET3` | Terrain database resolution; not a `CGameType` constructor id. |

## Assets

`CGameType` names databases and database chunk/class ids rather than individual meshes or textures. Concrete levels and descendants provide the level-specific data.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| game controller id | `GAME` | constructor | Base game-controller identity. |
| terrain class id | `3TER` | terrain loader slots | Used to instantiate/load terrain objects from database chunks. |
| database chunk id | raw `sd3E` | terrain loader slots | Chunk id passed to the database iterator; canonical name still needs confirmation. |
| sound chunk id | raw `Wave` | slot `00474af0` | Used by a sound/wave helper when sound database globals are available. |

## Confidence

Confidence: Medium

Validation: Static Ghidra dump + local `objdump` disassembly + string/FourCC cross-check only; not runtime-validated.

Open questions:
- Give semantic names to the database global pairs `DAT_005099b0..c4` and `DAT_005099c8..dc` after mapping the OMedia database/file stream types.
- Name the imported OMedia calls in the constructor/destructor (`0x48d278`, `0x48d284`, `0x48d320`, etc.).
- Confirm whether `DAT_005099a9` is strictly pause state or a broader gameplay-block flag.
- Confirm the canonical name and data type for raw chunk id `sd3E`.
- Resolve the help show/hide target slots `0x13c` and `0x140` once UI/menu classes are documented.

## Notes

- Evidence: `DumpClass.java CGameType /tmp/decomp_CGameType.md` (`slots=366`, `owned_methods=17`, `offsets=1`).
- Constructor/destructor evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `004745a0..00474790`.
- Terrain-loader false-positive evidence: raw class-id rows `@004756be` and `@0047590e` are inside slots `00475550` and `004757a0`, after database iteration and before terrain object lifecycle calls.

## Native Linkage

### Aspect: `initgame-camera-record-seed` -- status `linked`

The address-backed `InitGame` body at `00474a10` writes the global camera
record position at `DAT_00509a50+0x44/+0x48/+0x4c` to
`(0.0f, 10000.0f, 0.0f)`.  The native method map is:

| Decomp method | Native entry point | Deliberate deviation |
|---|---|---|
| `CGameType::InitGame` `00474a10` camera-record writes | `camera_record_init_game` in `src/game/camera_record.c` | None within this aspect. Adjacent angle and mode fields are preserved because the recovered body does not write them. |

`tools/linkage_oracles/CGameType.py` compiles the real native module, seeds
non-zero sentinels into the position, angle, and mode fields, calls the method
twice, and checks bit-exact position writes plus preservation of every adjacent
field.  No authored asset table feeds this lifecycle seed, so the oracle uses
decomp-derived state vectors.  Its self-test rejects both a `10000 -> 9999`
mutation and the previous native over-reset of an adjacent angle field.

### Aspect: `pause-help-update-gates` -- status `linked-blocked`

The immutable `9a2b908` evidence is not sufficient for an L1/L2 claim over the
combined menu gate.  It identifies the `PauseGame(%d)` trace helper at
`00475a00`, a separate writer at `00475a30`, `UpdateGameType` at `00475a70`,
and `ToggleHelp` at `00475ce0`, but it explicitly leaves the exact meaning of
`DAT_005099a9`, the update predicate producer, and the help show/hide target
slots `0x13c`/`0x140` unresolved.  Those bodies/slots must be recovered before
native menu input can be wired without guessing; visual help layout would then
need original-game/capture evidence.
