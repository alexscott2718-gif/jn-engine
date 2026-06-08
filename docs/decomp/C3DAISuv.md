# C3DAISuv

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAISuv` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048f278`, `0048f288`, `0048f6d8`, `0048f714`, `0048f728` |
| Ctor(s) | constructor/factory block `0040b1b0`; registers FourCC `3SUV` at `0040b26e` |
| Dtor(s) | scalar deleting destructor at `0040b3e0`; cleanup helper `0040b410`; adjusted destructor thunks `0040ba20`, `0040ba30`, `0040ba40` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DAISuv` is the concrete `3SUV` AI obstacle/vehicle. It inherits the `C3DAI` targeting and patrol schema, binds a `jeep.omt` visual, creates a `C3DLightCone` child, reacts to AI trigger/rocket/Jimmy contacts, and maintains the child light cone during the inherited AI update loop.

## Field Map

Offsets below are byte offsets from the active `C3DAI` subobject unless marked outer. The constructor writes through the outer allocation pointer and the owned vtable bodies enter through the active pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3SUV`; slot 259/265 helpers | Passed to `FUN_0045fea0` for progress/task gating. |
| inherited `0x578`, `0x57c`, `0x580` | int | `RequiredLevel`, `ExactLevel`, `RemoveLevel` | `.gam` `3SUV`; slot 259/265 helpers | Inherited progress gates; SUV also uses `RequiredLevel`/`RemoveLevel` to mark the light-cone child when outside range. |
| inherited `0x584..0x590` | int | `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass` | `.gam` `3SUV`; `C3DAnimated` | Inherited collision, visibility, movement, and render/update pass gates. |
| inherited `0x644` | float | `VisibleRange` | `.gam` `3SUV`; `C3DAI` | AI target range; rows use `2500..5000`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3SUV`; touch slot `0040b4f0` | Patrol target. The `GOAWAY` trigger branch overwrites this buffer from the touching AI trigger. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `.gam` `3SUV`; `C3DAI` | Rows target `"JIM1"`. Resolved by inherited `C3DAI::PostLoadAI`. |
| inherited `0x80c` | float | `FOV` | `.gam` `3SUV`; `C3DAI` | Field of view; rows use `90..300`. |
| inherited `0x87c` | int | `AIState` | `.gam` `3SUV`; ctor | Constructor and rows seed state `2`. |
| inherited `0x89c` | float | `WanderRange` | `.gam` `3SUV`; `C3DAI` | Rows use `1500.0`. |
| active `0x4a8` / outer `0x568` | pointer/handle | `jeep_database` | init slot `0040b460` | Result of `FUN_0046a910("jeep.omt")`. |
| active `0x574` / outer `0x634` | byte/bool | `suv_asset_flag` | init slot `0040b460` | Cleared after database lookup. Exact inherited consumer unresolved. |
| active `0x604` / outer `0x6c4` | float | `suv_ai_tuning_700` | ctor `0040b1b0` | Constructor seeds `700.0`; exact base-field name unresolved. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0040b1b0`; inherited `C3DAI` | Constructor seeds state `2` before `.gam`/post-load processing. |
| active `0x848` / outer `0x908` | byte/bool | `suv_visual_flag_0x848` | init slot `0040b460` | Cleared after visual setup. Exact consumer unresolved. |
| active `0x8d4` / outer `0x994` | pointer | `light_cone_child` | ctor `0040b1b0`; update/post-load slots | Adjusted `C3DLightCone` child pointer. The constructor hides/initializes it; update maintains its transform. |
| active `0x8d8` / outer `0x998` | float | `goaway_timer` | ctor `0040b1b0`; touch slot `0040b4f0`; update slot `0040b7a0` | Set to `10.0` by the `GOAWAY` AI-trigger branch. Update decrements it and returns state to `2` when it expires. |
| outer `0x7d0..0x898` | char buffers/strings | `suv_animation_names` | ctor `0040b1b0` | Constructor copies `"none"` into six inherited AI animation/name slots. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0040b1b0` | `CtorAISuv3SUV` | Constructs `C3DAI`, installs SUV vtables, sets object tag `C3DSUV`, runs `InitObjectAISuv`, registers `3SUV`, seeds AI state/tuning fields, allocates and initializes a `C3DLightCone` child, clears six animation/name strings to `"none"`, and initializes runtime fields. | non-trivial |
| 7 | `0040b460` | `InitObjectAISuv` | Runs `C3DAI::InitObjectAI`, loads `jeep.omt`, binds database entry `2`, normalizes the bound visual, clears a visual flag, applies tuning values `500.0` and `0.2`, and returns. | non-trivial |
| 16 | `0040b4f0` | `HandleSuvContact` | Raw vtable target. Runs inherited AI touch handling, then handles `C3DAITRIGGER`/`GOAWAY`, `C3DROCKET`, `C3DROCKETSHIP`, and `C3DJIMMY` contact cases. Several branches resolve a tag through `FUN_00474070`, copy the target object's position into the touching object, call a local hook, and play id `0xb4`. | raw block |
| 17 | `0040a390` | `C3DAI::ClearAITouchMarker` | Inherited AI contact-exit behavior. | inherited |
| 241 | `0040b7a0` | `UpdateSuvLightConeAndTimer` | Raw update slot. Applies an inherited offset, runs the `C3DAI` update, maintains the light-cone child transform/orientation using several SUV-specific offsets, decrements `goaway_timer`, and returns state to `2` when the timer expires. | raw block |
| 259 | `0040b8e0` | `PostLoadSuvProgressLightCone` | Runs `C3DAI::PostLoadAI`, then if progress/task gating falls outside `RequiredLevel..RemoveLevel`, marks the light-cone child at outer offset `0x6d0`. | non-trivial |
| 265 | `0040b960` | `ApplySuvProgressLightConeAfterGate` | Raw target. Runs `C3DAnimated` slot 265, then repeats the same progress-gate/light-cone-child mark used by slot 259. | raw block |
| 260 | `0040a6b0` | `C3DAI::StopAIMotion` | Inherited AI helper. | inherited |
| vtable 3 slot 2 | `0040b3e0` | scalar deleting destructor | Runs cleanup/vtable reset logic and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

```c
C3DAISuv::UpdateSuvLightConeAndTimer(dt):
    apply_inherited_offset(0, -100, 0)
    C3DAI::UpdateAIStateMachine(dt)

    if light_cone_child:
        position = transform_suv_lightcone_offset(0, 160, 110)
        light_cone_child->set_position(position)
        adjust_light_cone_angles_with_offsets(-1400, -200)

    if goaway_timer > 0:
        goaway_timer -= dt
        if goaway_timer <= 0:
            set_ai_state(2)
            goaway_timer = 0
```

```c
C3DAISuv::HandleSuvContact(other):
    C3DAI::HandleAITouch(other)

    if other->IsA("C3DAITRIGGER") and this.ObjectTag == "GOAWAY":
        set_ai_state(3)
        PatrolPoint = other->ai_trigger_target_or_next_string
        goaway_timer = 10.0
        return

    if other is active player/rocket/rocketship/Jimmy case:
        tag = select_redirect_tag_from_other_or_player_state(other)
        target = lookup_object_by_tag(tag)
        if target:
            other->set_position(target->position())
        local_contact_hook()
        play_effect_or_sound(-1, 0xb4, 0)
```

The redirect-tag branches are stable at the class/string level, but the exact source fields on `C3DROCKET`/`C3DROCKETSHIP` remain unnamed. The code copies strings from object subfields, resolves them with `FUN_00474070`, and writes the resolved target position back through the touching object's transform slot.

## Constants And Wiring

`3SUV` appears five times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` targeting and patrol fields; no unique `.gam` properties are registered by `C3DAISuv` itself.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DSUV"` | Base object tag and lookup identity. The contact handler has a special branch when the tag is `"GOAWAY"`, but current `3SUV` rows do not use that tag. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101`, `29010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861099350` | FourCC/object id value for `3SUV`. |
| `PositionX` | float | inherited | `828..16200` | Base placement transform. |
| `PositionY` | float | inherited | `-776..128` | Base placement transform. |
| `PositionZ` | float | inherited | `-11900..14000` | Base placement transform. |
| `RotationX`, `RotationZ` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..120` | Base placement transform and initial facing. |
| `TaskName` | str | inherited `0x430` | `"none"`, `"scene"` | Used by slot 259/265 progress gating through `FUN_0045fea0`. |
| `Debug` | int | inherited | `0` | Base debug flag; no SUV-owned branch found. |
| `RequiredLevel` | int | inherited `0x578` | `0..1` | Inherited animated progress gate and SUV light-cone gating. |
| `ExactLevel` | int | inherited `0x57c` | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited `0x580` | `-1..320` | Inherited animated progress gate and SUV light-cone gating. |
| `HasCollision` | int | inherited `0x584` | `-1..1` | Inherited collision gate. |
| `InitiallyVisible` | int | inherited `0x588` | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited `0x58c` | `1` | Inherited movement/update gate. |
| `SecondPass` | int | inherited `0x590` | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited `0x595` | `"none"` | No SUV-owned consumer found. |
| `PatrolPoint` | str (`1`) | inherited `0x648` | `"MIB1"`, `"PATA1"`, `"none"` | Resolved by inherited `C3DAI`; `GOAWAY` contact branch can overwrite it. |
| `VisibleRange` | float (`3`) | inherited `0x644` | `2500..5000` | Used by inherited AI target/range logic. |
| `FOV` | float (`3`) | inherited `0x80c` | `90..300` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str (`1`) | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int (`6`) | inherited `0x87c` | `2` | Constructor and rows seed state `2`. |
| `WanderRange` | float (`3`) | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SUV` | Concrete placeable class id for AI SUV. | ctor `0040b1b0`; `push 0x33535556` at `0040b26e` |
| `C3DSUV` | Concrete object/type string. | string `.data:004ecd20`; constructor path |
| `jeep.omt` | Visual database loaded by SUV init. | string `.data:004ecd28`; init slot `0040b460` |
| OMT entry `2` | Visual entry bound by SUV init. | `FUN_00477ba0(jeep_database, 2)` |
| `C3DLightCone` | Runtime child object allocated by constructor. | string `.data:004ecd10`; constructor allocation path |
| `C3DAITRIGGER`, `GOAWAY` | Trigger/contact branch. | raw touch slot `0040b4f0`; strings `.data:004ecd4c`, `.data:004ecd44` |
| `C3DROCKET`, `C3DROCKETSHIP`, `C3DJIMMY` | Contact classes that can redirect and play id `0xb4`. | raw touch slot `0040b4f0`; strings `.data:004ecc80`, `.data:004ecd34`, `.data:004ecb20` |
| `0xb4` | Effect/sound id played after redirect/contact handling. | raw touch slot `0040b4f0`; `FUN_00458980(-1, 0xb4, 0)` |
| `10.0` | `GOAWAY` timer duration. | raw touch slot `0040b4f0`; update slot `0040b7a0` |
| `500.0`, `0.2` | Visual/tuning values applied during init. | init slot `0040b460` |
| `700.0` | Constructor-seeded SUV AI tuning field at active `0x604`. | ctor `0040b1b0` |
| `140.0`, `0.2`, `0.2` | Constructor values written to the light-cone child. | ctor `0040b1b0` |
| `(0, -100, 0)`, `(0, 160, 110)`, `-1400`, `-200` | SUV update offsets used to maintain the child/light-cone transform. | raw update slot `0040b7a0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `jeep.omt` | init slot `0040b460`; parsed metadata `assets/parsed/jeep/jeep.json` | Original source path in metadata is `/home/scotty/xp-jnbg-original/omt/jeep.omt`. |
| OMT entry | index `2` | `FUN_00477ba0(db, 2)` | Local parsed image metadata exposes indices `0` (`mib`) and `1` (`lawnmower`); executable passes `2`, so the entry may be a non-image chunk or parser gap. |
| helper object | `C3DLightCone` | ctor `0040b1b0`; class hierarchy row | Runtime child allocated and stored at active `0x8d4`. |
| sound/effect | id `0xb4` | raw touch slot `0040b4f0` | Exact parsed sound/effect name still needs subsystem mapping. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local disassembly of constructor/raw vtable bodies, `.gam` schema cross-check, parsed `jeep.omt` metadata, and `C3DAI` base spec cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0040b4f0`, `0040b7a0`, and `0040b960` and re-run decompilation.
- Name SUV-specific inherited fields at active `0x604`, `0x848`, and the light-cone child flag at child outer `0x6d0`.
- Resolve the exact redirect-tag fields read from `C3DROCKET` and `C3DROCKETSHIP`.
- Confirm whether `jeep.omt` entry `2` is a parser gap/non-image entry or a database index with a different numbering convention.
- Runtime-check the `GOAWAY` branch; current `3SUV` rows all use `ObjectTag="C3DSUV"`, so the branch may be scripted by a non-`.gam` spawn or unused.

## Notes

- Evidence: `DumpClass.java C3DAISuv /tmp/decomp_C3DAISuv.md` (`slots=391`, `owned_methods=2`, `offsets=1`), local objdump windows over `0040b1b0..0040ba80`, string scans around `004ecd10..004ecd54`, parsed `jeep.omt` metadata, `.gam` schema for `3SUV`, and `C3DAI` field-map cross-check.
- `3SUV -> C3DAISuv` was backfilled in `docs/_gam_classids.tsv` from constructor/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
