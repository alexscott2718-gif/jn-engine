# C3DSumo

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSumo` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b876c`, `004b877c`, `004b8bcc`, `004b8c08`, `004b8c1c` |
| Ctor(s) | constructor/factory block `FUN_00443dc0`; registers FourCC `3SUM` at `00443e7e` |
| Dtor(s) | adjusted scalar deleting destructor and cleanup path inherited/generated with the animated allocation layout |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSumo` is the `3SUM` placeable animated Sumo actor. It loads `sumo.png`/`sumo.ase` and implements a Jimmy-only escort/exit interaction. When Jimmy touches an eligible Sumo actor with the required counter available, Sumo captures Jimmy, runs a short carried/exit sequence, resolves the serialized `StartPoint`, moves Jimmy there, then restores Jimmy control flags and starts a release cooldown.

## Field Map

Offsets below are byte offsets from the outer `C3DSumo` allocation pointer. The raw update slot is called through the adjusted animated subobject pointer; objdump converts those adjusted offsets to the outer offsets listed here.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3SUM` | Rows use `"none"` and `"scene"`. Sumo-specific code does not branch on it directly. |
| `0x57c` | handle/pointer | `sumo_texture_canvas_handle` | `00443fa0` | Loaded from `sumo.png` and attached to the animated material/canvas. |
| `0x6bc` | pointer | `attached_jimmy` | ctor `00443dc0`; touch `004442f0`; raw `00444040` | Holds the Jimmy object subpointer while the escort sequence is active. Cleared after release. |
| `0x6c0` | byte/bool | `escort_active` | ctor `00443dc0`; touch `004442f0`; raw `00444040` | Set by eligible Jimmy contact; cleared once the exit/teleport handoff completes. |
| `0x6c4` | float | `release_cooldown` | ctor `00443dc0`; raw `00444040` | Counts down from `10.0` after releasing Jimmy. While zero and inactive, Sumo runs a slower idle motion. |
| `0x6c8` | char buffer/string | `StartPoint` | `.gam` `3SUM`; `00443fa0`; raw `00444040` | Serialized destination tag. Runtime resolves it with `FUN_00474070`; observed values include `"none"` and `"sumoexit"`. |

No other Sumo-owned serialized fields were observed. Constructor toggles inherited render/collision/range setup, including one extra inherited toggle relative to Pirate, then initializes the fields above.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00443fa0` | `InitObjectSumo` | Traces `InitObject()`, calls `C3DAnimated::InitObject`, registers the `StartPoint` property at `0x6c8`, runs inherited setup, registers `HIDEFAULT -> sumo.ase`, loads `sumo.png`, attaches the texture, selects `DEFAULT`, and finalizes. | non-trivial |
| 16 | `004442f0` | `TouchSumoJimmyExit` | Touch/collision handler. If the toucher is `C3DJIMMY` and inventory/picture counter `0x1b` is available, starts the escort sequence, captures Jimmy, changes Jimmy state fields, delays `100`, consumes counter `0x1b`, and sets Jimmy's control/transition flag. | non-trivial |
| 241 | `00444040` | `UpdateSumoEscort` | Raw per-frame update. Runs the animated update, advances Sumo motion, positions and animates captured Jimmy while `escort_active`, then on completion resolves `StartPoint`, moves Jimmy, clears Jimmy flags, releases `attached_jimmy`, and starts a `10.0` cooldown. | raw block |
| 259 | `004443d0` | `PostLoadOrRangeSumo` | Calls inherited `C3DAnimated` slot 259, then applies inherited slot `0x110(680.0)`. | non-trivial |
| 265 | `0040e340` | `C3DAnimated` slot 265 | Inherited animated/progress behavior. | inherited |
| vtable 3 slot 2 | `00443e90` family | scalar deleting destructor | Runs generated animated cleanup/vtable reset and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Sumo registers its concrete assets directly from slot 7; no separate asset-registration override. | inherited |

## Runtime Behavior

Sumo is a focused interaction actor. It has a normal inactive motion, a faster active escort motion, and a Jimmy handoff that is close to Pirate's exit flow but with different offsets, animations, and state timing.

```c
C3DSumo::InitObjectSumo():
    trace("InitObject()")
    C3DAnimated::InitObject()
    register_property("StartPoint", &StartPoint, string, default_or_flags=0)

    inherited_adjusted_setup_slot()
    register_anim("HIDEFAULT", "sumo.ase")
    load_texture("sumo.png", 0)
    attach_texture_canvas(sumo_texture_canvas_handle, 0)
    select_animation("DEFAULT", true)
    CGameObject::vfunc_00_013()
```

```c
C3DSumo::TouchSumoJimmyExit(other):
    CGameObject::Touch(other)
    if !other.is("C3DJIMMY"):
        return
    if !inventory_or_picture_has(2, 0x1b):
        return
    if inventory_or_picture_count(0x1b) <= 0:
        return

    other.slot_0x178()
    escort_active = true
    attached_jimmy = other
    this.slot_0x224(false)
    other.state_word_0x7c4 = 3
    other.flag_0x880 = false
    delay_or_cooldown(100)
    inventory_or_picture_add(0x1b, -1)
    other.flag_0x1e59 = true
```

```c
C3DSumo::UpdateSumoEscort(dt):
    copy_or_stabilize_transform()
    C3DAnimated::Update(dt)

    if escort_active:
        this.slot_0x334(0, dt * 60.0, 0)
        move_attached_jimmy_to_local_offset(260.0, 185.0, 550.0)
        attached_jimmy.slot_0x410()
        attached_jimmy.select_animation("RUN", true)
        attached_jimmy.position = this.position with y += 90.0

        if exit_input_or_anim_done():
            escort_active = false
            this.slot_0x224(true)

            dest = find_object(StartPoint)
            if dest:
                attached_jimmy.position = dest.position

            attached_jimmy.flag_0x880 = true
            attached_jimmy.state_word_0x7c4 = 0
            attached_jimmy.flag_0x1e59 = false
            attached_jimmy = NULL
            release_cooldown = 10.0
    else if release_cooldown > 0.0:
        release_cooldown = max(0.0, release_cooldown - dt)
    else:
        this.slot_0x334(0, dt * 20.0, 0)

    copy_or_stabilize_transform()
```

The exact identities of `exit_input_or_anim_done()` (`FUN_0042a7a0`) and the inventory/picture helpers are inherited project services; this spec records the control flow and constants but leaves those helpers for their owning subsystem.

## Constants And Wiring

### `.gam` Placeable Properties

`3SUM` appears three times across the level `.gam` files. It serializes common object/animated fields plus the Sumo-specific `StartPoint` destination.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DSUMO"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010100`, `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861099341` | FourCC/object id value for `3SUM`. |
| `PositionX` | float | inherited | `-9.52e+03` .. `-1.72e+03` | Base placement transform. |
| `PositionY` | float | inherited | `-108` .. `57.9` | Base placement transform. |
| `PositionZ` | float | inherited | `-4.21e+03` .. `4.86e+03` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` .. `66.5` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"none"`, `"scene"` | Not used by Sumo-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1` .. `0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `-1` | Sumo must receive touch/collision events for the Jimmy exit flow. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Present on two rows; Sumo-specific code does not consume it directly. |
| `StartPoint` | str | `0x6c8` | `"none"`, `"sumoexit"` | Resolved by raw update `00444040` after the escort sequence completes. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SUM` | Concrete placeable class id for Sumo. | ctor `00443dc0`; `push 0x3353554d` at `00443e7e` |
| `C3DSUMO` | Concrete object/type string. | string `.data:004f0fbc`; constructor string path |
| `C3DSUMO()` | Concrete class string. | string `.data:004f0fb0`; constructor string path |
| `StartPoint` | Serialized destination property. | string `.data:004efb3c`; `00443fa0` |
| `HIDEFAULT` | Sumo animation alias. | `00443fa0` |
| `DEFAULT` | Default animation selected after asset setup. | `00443fa0` |
| `RUN` | Jimmy animation selected while the escort is active. | string `.data:004eca5c`; raw `00444040` |
| `C3DJIMMY` | Required toucher class for the exit interaction. | `004442f0` |
| `0x1b` | Inventory/picture counter gate consumed by the touch flow. | `004442f0` |
| `0.05` | Constructor-applied inherited tuning constant. | ctor `00443dc0`; immediate `0x3d4ccccd` |
| `1000.0` | Constructor-applied inherited range/tuning constant. | ctor `00443dc0`; immediate `0x447a0000` |
| `680.0` | Slot 259 inherited range/tuning constant. | `004443d0`; immediate `0x442a0000` |
| `(260.0, 185.0, 550.0)` | Local offset used to position captured Jimmy during active escort. | raw `00444040`; immediates `0x43820000`, `0x43390000`, `0x44098000` |
| `90.0` | Vertical offset applied when copying Sumo position to Jimmy. | raw `00444040`; float constant at `.rdata:004a7388` |
| `60.0` | Active escort motion scale. | raw `00444040`; float constant at `.rdata:0049d5b4` |
| `20.0` | Inactive idle motion scale after cooldown. | raw `00444040`; float constant at `.rdata:00495324` |
| `10.0` | Cooldown after releasing Jimmy. | raw `00444040`; immediate `0x41200000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `sumo.png` | `00443fa0`; `assets/png/sumo.png` | Loaded during Sumo init and attached as texture page `0`. |
| animation | `HIDEFAULT` -> `sumo.ase` | `00443fa0`; `assets/ase/sumo.ASE` | Default animated shape. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw update target `00444040`; `DumpFunctions` could not decompile it because it is not currently function-defined.
- Name helper `FUN_0042a7a0`, which decides when the escort sequence completes.
- Confirm the semantic name of inventory/picture counter `0x1b` from UI/inventory docs before porting the touch gate.
- Runtime-check the `StartPoint=sumoexit` handoff and the rows with `StartPoint=none` before marking this class `validated`.
- Confirm the exact semantic names of motion slot `0x334` and the transform-copy bookends used around the update.

## Notes

- Evidence: `DumpClass.java C3DSumo /tmp/decomp_C3DSumo.md` (`slots=368`, `owned_methods=3`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DSumo_funcs.md`, local `objdump` windows over `00443dc0..00444440`, asset scan, and `.gam` schema for `3SUM`.
- `3SUM -> C3DSumo` was normalized in `docs/_gam_classids.tsv` from the RTTI/class dump evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
