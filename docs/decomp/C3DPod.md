# C3DPod

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPod` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004afd48`, `004afd58`, `004b01a8`, `004b01e4`, `004b01f8` |
| Ctor(s) | constructor/factory block `0043b990`; registers FourCC `3POD` at `0043ba5a` |
| Dtor(s) | scalar deleting destructor at `0043bb90`; cleanup helper `0043bbc0`; adjusted destructor thunks at `0043bc40`, `0043bc50`, `0043bc60` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPod` is a concrete `C3DAI` leaf for the pod ship object. It registers `3POD`, seeds inherited AI state and movement tuning, binds `objects.omt` entry id `6`, and otherwise relies on inherited `C3DAI` update/targeting behavior. No `3POD` objects appear in the current `.gam` corpus.

## Field Map

Offsets are byte offsets from the outer `C3DPod` allocation unless marked `active`. Primary slot-1 methods enter through the active AI pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x600` | pointer | `target_object` | `C3DAI` | Runtime target resolved from `TargetName`. |
| active `0x604` / outer `0x6c4` | float | `ai_speed_tuning` | ctor `0043b990`; inherited AI/trigger paths | Constructor writes `500.0`; this is the inherited AI speed/tuning field also mutated by `C3DAITrigger::AISpeed`. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0043b990`; `C3DAI` | Constructor writes `1`. |
| active `0x710..0x7d8` / outer `0x7d0..0x898` | char buffers/strings | `pod_animation_names` | ctor `0043b990` | Constructor overwrites the six inherited `C3DAI` animation-state strings with `"none"`. |
| active `0x814` / outer `0x8d4` | float | `pod_transform_gain_y` | ctor `0043b990` | Constructor writes `0.3`. Exact inherited consumer is unresolved; nearby AI/transform helpers use fields in the active `0x810..0x818` range. |
| active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0043b990`; `C3DAI` | Constructor writes serialized/default AI state `1`. |
| active `0x4a8` / outer `0x568` | pointer/handle | `objects_database` | ctor `0043b990` | Result of `FUN_0046a910("objects.omt")`, used immediately to bind OMT entry id `6`. |
| active `0x574` / outer `0x634` | byte/bool | `pod_asset_flag` | ctor `0043b990` | Cleared after the OMT database lookup. Exact inherited meaning is unresolved. |
| outer `0x998` | subobject/tail | class streamer tail | constructor/destructor scaffolding | Tail cleanup/streamer allocation handled around construction and destruction; not gameplay tuning. |

## Vtable Methods

`C3DPod` owns only construction/destruction logic. Slot 7 remains `C3DAI::InitObjectAI`; the constructor calls that base init directly before binding its OMT asset.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0043b990` | `CtorPod3POD` | Constructs `C3DAI`, installs Pod vtables, sets runtime type `C3DPOD` / `C3DPOD()`, runs `C3DAI::InitObjectAI`, finalizes base object setup, registers `3POD`, seeds inherited AI speed/state defaults, clears inherited animation strings to `"none"`, applies inherited scalar/toggle defaults, binds `objects.omt` entry id `6`, and finalizes. | non-trivial |
| 7 | `00407ee0` | `InitObjectAI` | Inherited `C3DAI` property registration for `PatrolPoint`, `VisibleRange`, `FOV`, `TargetName`, `AIState`, and `WanderRange`. | inherited |
| 10 | `00407eb0` | `ResetAIState` | Inherited reset; copies serialized `AIState` into runtime state and clears patrol/waypoint cache. | inherited |
| 16 | `0040a3c0` | `HandleAITouch` | Inherited `C3DAI` touch/target reaction slot. | inherited |
| 17 | `0040a390` | `ClearAITouchMarker` | Inherited `C3DAI` contact-end marker clear. | inherited |
| 241 | `00408000` | `UpdateAIStateMachine` | Inherited per-frame AI update and movement/facing output. | inherited |
| 259 | `00409480` | `PostLoadAI` | Inherited target resolution and initial AI state sync. | inherited |
| 260 | `0040a6b0` | `StopAIMotion` | Inherited zero-motion helper. | inherited |
| vtable 3 slot 2 | `0043bb90` | scalar deleting destructor | Adjusts from the secondary subobject pointer, calls cleanup helper `0043bbc0`, destroys the tail subobject at outer `0x998`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0043bbc0` | `CleanupPod` | Reinstalls Pod vtables during destruction and tail-jumps to the `C3DAI` cleanup helper at `00407e60`. | non-trivial |

## Runtime Behavior

```c
C3DPod::CtorPod3POD():
    C3DAI::Ctor()
    install_pod_vtables()
    set_runtime_type("C3DPOD")
    register_class_string("C3DPOD()")
    C3DAI::InitObjectAI()
    finalize inherited base object setup
    register_fourcc("3POD")
    ai_speed_tuning = 500.0f
    current_state = 1
    AIState = 1
    copy "none" into inherited AI animation strings
    apply inherited scalar 100.0f
    apply inherited scalar 100.0f
    clear three inherited flags/toggles
    pod_transform_gain_y = 0.3f
    db = lookup_omt_database("objects.omt")
    objects_database = db
    pod_asset_flag = false
    bind_omt_entry(db, 6)
    finalize inherited object/asset state
```

```c
C3DPod::Update(dt):
    // No owned update override.
    C3DAI::UpdateAIStateMachine(dt)
```

## Constants And Wiring

`C3DPod` registers FourCC `3POD`, but `assets/gam/*.gam` currently contain no `3POD` objects. There are therefore no serialized per-instance Pod properties in `docs/gam_schema.md`; the known tuning below comes from the constructor.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3POD` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0043b990`; backfilled in `docs/_gam_classids.tsv` as `C3DPod()`. |
| `ai_speed_tuning` | float | active `0x604` | constructor `500.0` | Inherited AI movement/speed tuning. |
| `current_state` | int | active `0x608` | constructor `1` | Inherited AI state. |
| `AIState` | int | active `0x87c` | constructor `1` | Inherited serialized/default AI state. |
| `pod_transform_gain_y` | float | active `0x814` | constructor `0.3` | Unresolved inherited transform/AI helper tuning. |
| OMT database | str | active `0x4a8` | `objects.omt` | Looked up by `FUN_0046a910`, then passed to OMT entry lookup. |
| OMT entry id | int | n/a | `6` | Passed to `FUN_00477ba0(db, 6)` and then into inherited object binding. Parsed metadata names chunk id `6` as `podship`. |
| constructor scalars | float | inherited slots | `100.0`, `100.0` | Applied through inherited slots `00471240` and `00471b40`; exact inherited names still open. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DPOD` | Concrete object/type string set by the constructor. | string `.data:004f073c`; constructor `0043b990` |
| `C3DPOD()` | Constructor/class string used by the executable. | string `.data:004f0730`; constructor `0043b990` |
| `objects.omt` | OMT database for the pod ship visual. | string `.data:004ecca4`; constructor `0043b990` |
| `podship` | Parsed OMT image/chunk name for id `6`. | `assets/parsed/objects/objects.json`, `chunk_id: 6`, `name: "podship"` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objects.omt` | constructor `0043b990`; parsed metadata `assets/parsed/objects/objects.json` | Original source path is `/home/scotty/xp-jnbg-original/omt/objects.omt`. |
| OMT entry/chunk | id `6` / `podship` | constructor `0043b990`; parsed `objects.json` | Local parsed image file is `assets/parsed/objects/objects_images/0017_128x128d32.png`. |
| level model candidate | `assets/glb/omt/level7/pod.glb` | asset scan only | Level 7 contains a separate OMT/GLB pod asset; no direct constructor reference was found in this class body. |
| voice references | `pod escape`, `pod crashing` | asset scan only | Present in `voicepowerplant`; not directly referenced by `C3DPod` constructor/update. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, string/asset scans, and class-id scan backfill only; not runtime-validated.

Open questions:
- Name the inherited scalar/toggle slots called after constructor asset setup (`00471240`, `00471b40`, `004710c0`, `00470fd0`, `00470fa0`, and `00470f10`).
- Confirm the exact inherited consumer of active `0x814 = 0.3`.
- Runtime-check whether Pod is spawned by script/cutscene code instead of direct `.gam` placement.
- Confirm whether the level 7 `pod.glb` asset is related to `C3DPod` or a separate level prop.

## Notes

- Evidence: `DumpClass.java C3DPod /tmp/decomp_C3DPod.md` (`slots=391`, `owned_methods=1`, `offsets=1`), local objdump window `0043b990..0043bc70`, string scans around `004f0730`, asset metadata for `objects.omt`, and `.gam` byte scans showing no `3POD` rows.
- `docs/_gam_classids.tsv` was backfilled for `3POD -> C3DPod()` from RTTI/string/vtable evidence, then `python3 tools/gam_schema.py` was rerun. `docs/gam_schema.md` did not change because the current level corpus has no `3POD` instances.
