# C3DPirate

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPirate` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004af0cc`, `004af0dc`, `004af52c`, `004af568`, `004af57c` |
| Ctor(s) | constructor/factory block `FUN_00436c40`; registers FourCC `3PIR` at `00436cfe` |
| Dtor(s) | adjusted scalar deleting destructor at `00436da0`; cleanup/vtable reset helper at `00436dd0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPirate` is the single `3PIR` placeable animated actor in the ship area. Despite the class name, its concrete asset set is `viking.png`/`viking.ase`. Its unique behavior is an exit/escort interaction: on eligible Jimmy contact it captures Jimmy, drives him through a short sequence, resolves the serialized `StartPoint`, moves Jimmy there, and then releases control.

## Field Map

Offsets below are byte offsets from the outer `C3DPirate` allocation pointer. The update slot is called through the adjusted animated subobject pointer; objdump converts those adjusted offsets to the outer offsets listed here.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3PIR` | The one row uses `"none"`. Pirate does not use task-state wiring directly. |
| `0x57c` | handle/pointer | `viking_texture_canvas_handle` | `00436e20` | Loaded from `viking.png` and attached to the animated material/canvas. |
| `0x6bc` | float | `bob_phase` | ctor `00436c40`; raw `00436ec0` | Accumulates frame delta and feeds a sine bob applied through transform slot `0x330`. |
| `0x6c0` | pointer | `attached_jimmy` | ctor `00436c40`; touch `00437110`; raw `00436ec0` | Holds the Jimmy object subpointer during the ship-exit escort sequence. Cleared when the sequence releases Jimmy. |
| `0x6c4` | byte/bool | `escort_active` | ctor `00436c40`; touch `00437110`; raw `00436ec0` | Set when eligible Jimmy contact starts the exit sequence; cleared once the exit/teleport handoff completes. |
| `0x6c8` | float | `release_cooldown` | ctor `00436c40`; raw `00436ec0` | Counts down from `10.0` after releasing Jimmy. |
| `0x6cc` | char buffer/string | `StartPoint` | `.gam` `3PIR`; `00436e20`; raw `00436ec0` | Serialized destination tag. Current row uses `"shipexit"`; runtime resolves it with `FUN_00474070` and moves Jimmy to that transform. |

No other Pirate-owned serialized fields were observed. Constructor toggles several inherited render/collision/range slots and initializes the fields above.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00436e20` | `InitObjectPirate` | Traces `InitObject()`, calls `C3DAnimated::InitObject`, registers the `StartPoint` property at `0x6cc`, runs inherited setup, registers `HIDEFAULT -> viking.ase`, loads `viking.png`, attaches the texture, selects `DEFAULT`, and finalizes. | non-trivial |
| 16 | `00437110` | `TouchPirateJimmyExit` | Touch/collision handler. If the toucher is `C3DJIMMY` and inventory/picture counter `0x1b` is available, starts the escort sequence, captures Jimmy, changes Jimmy state fields, delays `100`, consumes counter `0x1b`, and sets Jimmy's control/transition flag. | non-trivial |
| 241 | `00436ec0` | `UpdatePirateEscort` | Raw per-frame update. Runs `C3DAnimated` update, applies sine bobbing, keeps captured Jimmy positioned/animated while `escort_active`, then on completion resolves `StartPoint`, moves Jimmy, clears Jimmy flags, releases `attached_jimmy`, and starts a `10.0` cooldown. | raw block |
| 259 | `00437200` | `PostLoadOrRangePirate` | Calls inherited `C3DAnimated` slot 259, then applies inherited slot `0x110(800.0)`. | non-trivial |
| 265 | `0040e340` | `C3DAnimated` slot 265 | Inherited animated/progress behavior. | inherited |
| vtable 3 slot 2 | `00436da0` | scalar deleting destructor | Runs the Pirate cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Pirate registers its concrete assets directly from slot 7; no separate asset-registration override. | inherited |

## Runtime Behavior

Pirate is a small interaction actor. It bobs visually every frame, and only starts the special exit flow when Jimmy touches it with the required counter/state available.

```c
C3DPirate::InitObjectPirate():
    trace("InitObject()")
    C3DAnimated::InitObject()
    register_property("StartPoint", &StartPoint, string, default_or_flags=0)

    inherited_adjusted_setup_slot()
    register_anim("HIDEFAULT", "viking.ase")
    load_texture("viking.png", 0)
    attach_texture_canvas(viking_texture_canvas_handle, 0)
    select_animation("DEFAULT", true)
    CGameObject::vfunc_00_013()
```

```c
C3DPirate::TouchPirateJimmyExit(other):
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
    other.slot_0x410()
    other.state_word_0x7c4 = 3
    other.flag_0x880 = false
    delay_or_cooldown(100)
    inventory_or_picture_add(0x1b, -1)
    other.flag_0x1e59 = true
```

```c
C3DPirate::UpdatePirateEscort(dt):
    C3DAnimated::Update(dt)

    bob_phase += dt
    apply_bob(sin(bob_phase) * 60.0)

    if escort_active:
        move_attached_jimmy_to_local_offset(-300.0, -650.0, 0.0)
        attached_jimmy.select_animation("DRIVE", true)

        if exit_input_or_anim_done():
            escort_active = false
            this.slot_0x224(true)
            attached_jimmy.flag_0x880 = true
            attached_jimmy.state_word_0x7c4 = 0

            dest = find_object(StartPoint)
            if dest:
                attached_jimmy.position = dest.position

            attached_jimmy.flag_0x1e59 = false
            attached_jimmy = NULL
            release_cooldown = 10.0
    else if release_cooldown > 0.0:
        release_cooldown = max(0.0, release_cooldown - dt)
```

The exact identities of `exit_input_or_anim_done()` (`FUN_0042a7a0`/`FUN_0042a730`) and the inventory/picture helpers are inherited project services; this spec records the control flow and constants but leaves those helpers for their owning subsystem.

## Constants And Wiring

### `.gam` Placeable Properties

`3PIR` appears once across the level `.gam` files. It serializes common object/animated fields plus the Pirate-specific `StartPoint` destination.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DPIRATE"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860899666` | FourCC/object id value for `3PIR`. |
| `PositionX` | float | inherited | `448` | Base placement transform. |
| `PositionY` | float | inherited | `1160` | Base placement transform. |
| `PositionZ` | float | inherited | `3030` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `303` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"none"` | Not used by Pirate-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `-1` | Pirate must receive touch/collision events for the Jimmy exit flow. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `StartPoint` | str | `0x6cc` | `"shipexit"` | Resolved by raw update `00436ec0` after the escort sequence completes. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3PIR` | Concrete placeable class id for Pirate. | ctor `00436c40`; `push 0x33504952` at `00436cfe` |
| `C3DPIRATE` | Concrete object/type string. | string `.data:004f0564`; constructor string path |
| `C3DPIRATE()` | Concrete class string. | string `.data:004f0558`; constructor string path |
| `StartPoint` | Serialized destination property. | string `.data:004efb3c`; `00436e20` |
| `HIDEFAULT` | Viking/Pirate animation alias. | `00436e20` |
| `DEFAULT` | Default animation selected after asset setup. | `00436e20` |
| `DRIVE` | Jimmy animation selected while the escort is active. | string `.data:004ef344`; raw `00436ec0` |
| `C3DJIMMY` | Required toucher class for the exit interaction. | `00437110` |
| `0x1b` | Inventory/picture counter gate consumed by the touch flow. | `00437110` |
| `0.05` | Constructor-applied inherited tuning constant. | ctor `00436c40`; immediate `0x3d4ccccd` |
| `1000.0` | Constructor-applied inherited range/tuning constant. | ctor `00436c40`; immediate `0x447a0000` |
| `800.0` | Slot 259 inherited range/tuning constant. | `00437200`; immediate `0x44480000` |
| `60.0` | Bob amplitude. | raw `00436ec0`; double at `.rdata:004af6a8` |
| `(-300.0, -650.0, 0.0)` | Local offset used to position captured Jimmy during escort. | raw `00436ec0`; immediates `0xc3960000`, `0xc4228000`, `0` |
| `10.0` | Cooldown after releasing Jimmy. | raw `00436ec0`; immediate `0x41200000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `viking.png` | `00436e20`; `assets/png/viking.png` | Loaded during Pirate init and attached as texture page `0`. |
| animation | `HIDEFAULT` -> `viking.ase` | `00436e20`; `assets/ase/viking.ASE` | Default animated shape. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw update target `00436ec0`; `DumpFunctions` could not decompile it because it is not currently function-defined.
- Name helper calls `FUN_0042a7a0`/`FUN_0042a730` that decide when the escort sequence completes.
- Confirm the semantic name of inventory/picture counter `0x1b` from UI/inventory docs before porting the touch gate.
- Runtime-check the `StartPoint=shipexit` handoff before marking this class `validated`.

## Notes

- Evidence: `DumpClass.java C3DPirate /tmp/decomp_C3DPirate.md` (`slots=368`, `owned_methods=3`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DPirate_funcs.md`, local `objdump` windows over `00436c40..00437280`, asset scan, and `.gam` schema for `3PIR`.
- `3PIR -> C3DPirate` was normalized in `docs/_gam_classids.tsv` from the RTTI/class dump evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
