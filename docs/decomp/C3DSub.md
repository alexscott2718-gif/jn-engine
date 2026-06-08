# C3DSub

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSub` |
| Base chain | `C3DFlyingObject -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b817c`, `004b818c`, `004b85dc`, `004b8618`, `004b862c` |
| Ctor(s) | constructor/factory block `00443350`; registers FourCC `3SUB` at `004434d7` |
| Dtor(s) | scalar deleting destructor at `004435a0`; cleanup helper `004435d0`; adjusted destructor thunks at `00443d70`, `00443d80`, `00443d90` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSub` is the concrete `3SUB` flying-sub leaf. It derives from `C3DFlyingObject`, uses inherited flight controls, loads the same `Strato.ase` / `strato.png` visual path used by `C3DRocketShip`, allocates a twenty-object `C3DSmokePuff` pool, and adds sub-specific per-frame smoke/wake and linked-object transform sync.

## Field Map

Offsets are byte offsets from the active `C3DSub` / `C3DFlyingObject` pointer unless marked outer. The active pointer is at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x5fc` | float | `AccelRate` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `50.0`; consumed by inherited flight integrator. |
| inherited `0x600` | float | `DecelRate` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `20.0`; consumed by inherited flight integrator. |
| inherited `0x604` | float | `MaxSpeed` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `200.0`; inherited horizontal speed cap. |
| inherited `0x608` | float | `current_speed` | ctor clears; update `00443810`; `C3DFlyingObject` | Compared against `20.0`; smoke puffs are emitted only above that threshold. |
| inherited `0x620` | float | `MaxHeight` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `4000.0`; inherited height clamp. |
| inherited `0x624` | float | `UpRate` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `80.0`; inherited vertical velocity target. |
| inherited `0x628` | float | `DownRate` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `-80.0`; inherited vertical velocity target. |
| inherited `0x62c` | float | `MaxVertVelocity` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `110.0`; inherited vertical velocity clamp. |
| inherited `0x630` | float | `NewGravity` | ctor clears; `C3DFlyingObject` | Registered inherited tuning field; no Sub-owned consumer found. |
| inherited `0x660` | float | `AccelLean` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `10.0`; inherited lean tuning. |
| inherited `0x664` | float | `DecelLean` | ctor `00443350`; `C3DFlyingObject` | Constructor default is `-10.0`; inherited lean tuning. |
| `0x67c` | int/pointer | `sub_runtime_link_or_flag` | ctor clears | Cleared with the linked-object field; no confirmed Sub-owned read in this pass. |
| `0x680` | pointer | `linked_sub_actor` | ctor clears; update `00443810` | Optional linked object. When present, update copies Sub position, angle, and vector state into it. |
| `0x684..0x6d0` | pointer[20] | `smoke_puff_pool` | ctor `00443350`; update `00443810` | Twenty `C3DSmokePuff` children allocated by the constructor and reused by the Sub update. |
| `0xe7c` | word/int | `smoke_puff_index` | ctor clears outer `0xf3c`; update `00443810` | Ring index over `smoke_puff_pool`, incremented and wrapped at twenty. |
| outer `0xf44` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

Several constructor-written inherited/auxiliary fields at active `0x60c`, `0x640`, `0x644`, `0x650`, and `0x654` are left unnamed here. They are stable Sub tuning constants, but their final consumers belong to inherited OMedia/flying helpers and need a broader base-struct pass before assigning gameplay names.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00443350` | `CtorSub3SUB` | Constructs `C3DFlyingObject`, installs Sub vtables, registers `C3DSUB` / `C3DSub()`, seeds inherited flight tuning, runs `InitObjectSub`, applies inherited setup constants, registers FourCC `3SUB`, clears Sub link state, allocates twenty `C3DSmokePuff` children, and clears the smoke ring index. | non-trivial |
| 7 | `00443670` | `InitObjectSub` | Runs `C3DFlyingObject::InitObjectFlying`, initializes the visual database/shape path, registers `HIDEFAULT -> Strato.ase`, loads `strato.png`, assigns the texture/material, applies scalar `20.0`, selects `DEFAULT`, and seeds an initial transform `(0, 600, 100)`. | non-trivial |
| 10 | `0041a0b0` | `C3DFlyingObject::ResetFlyingRuntime` | Inherited flying reset. | inherited |
| 16 | `00443800` | `HandleSubPickupCollision` | Thin wrapper around `C3DFlyingObject::HandlePickupCollision`; preserves inherited barrel-roll pickup handling. | inherited wrapper |
| 241 | `00443810` | `UpdateSub` | Runs inherited flying movement, updates three global wake/effect records, emits or hides one pooled `C3DSmokePuff` based on `current_speed > 20.0`, advances the twenty-slot ring index, and syncs `linked_sub_actor` when present. | raw block |
| 243 | `0041a6c0` | `C3DFlyingObject::UpdateFlyingCameraRecord` | Inherited flying camera/record update. | inherited |
| 246 | `00443ba0` | `UpdateSubRotateToDest` | Sub-specific rotate-to-destination helper. Computes shortest-arc deltas from destination angle fields, wraps across `+/-180`, scales by `dt` with constants `0.9`, `4.0`, and `1.5`, then forwards the Euler delta through inherited slot `0x334`. | raw block |
| 257 | `0041afb0` | `C3DFlyingObject::ResetSelfHook` | Direct inherited reset/self hook. | inherited |
| 259 | `00418950` | `C3DAnimated::PostLoadAnimated` | Direct inherited animated post-load helper. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Direct inherited level/progress gate. | inherited |
| vtable 3 slot 2 | `004435a0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `004435d0`, destroys the tail subobject at outer `0xf44`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `004435d0` | `CleanupSub` | Reinstalls Sub vtables, traces `~C3DSub()`, then tail-calls inherited `C3DFlyingObject` cleanup at `00419ed0`. | non-trivial |

## Runtime Behavior

```c
C3DSub::CtorSub3SUB():
    C3DFlyingObject::Ctor()
    install_sub_vtables()
    register_runtime_strings("C3DSUB", "C3DSub()")

    AccelRate = 50.0
    DecelRate = 20.0
    MaxSpeed = 200.0
    MaxHeight = 4000.0
    UpRate = 80.0
    DownRate = -80.0
    MaxVertVelocity = 110.0
    NewGravity = 0.0
    AccelLean = 10.0
    DecelLean = -10.0

    InitObjectSub()
    apply inherited setup scalar 0.3
    enable inherited toggle
    register_fourcc("3SUB")
    apply inherited scalar 150.0

    linked_sub_actor = NULL
    for i in 0..19:
        smoke_puff_pool[i] = new C3DSmokePuff(1)
        register_or_attach_child(smoke_puff_pool[i], "C3DSmokePuff")
        smoke_puff_pool[i].state_or_visibility = 2
        smoke_puff_pool[i].timer_or_flag = 0
    smoke_puff_index = 0
```

```c
C3DSub::InitObjectSub():
    trace("InitObject()")
    C3DFlyingObject::InitObjectFlying()
    init visual database/shape path
    register_anim("HIDEFAULT", "Strato.ase")
    create_texture_slot("strato.png", 0)
    assign_texture_to_current_material()
    apply_shape_scalar(20.0)
    set_anim("DEFAULT", true)
    set_initial_transform(0.0, 600.0, 100.0)
```

```c
C3DSub::UpdateSub(dt):
    C3DFlyingObject::UpdateFlyingMovement(dt)
    update three global wake/effect records from Sub transform and scaled Y

    if current_speed > 20.0:
        pos = transform_offset(local=(0.0, 0.0, -150.0))
        smoke_puff_pool[smoke_puff_index].place_or_start(pos)
        smoke_puff_pool[smoke_puff_index].state_or_visibility = 1
        smoke_puff_pool[smoke_puff_index].visibility_slot(0)
    else:
        smoke_puff_pool[smoke_puff_index].visibility_slot(1)

    smoke_puff_index = (smoke_puff_index + 1) % 20

    if linked_sub_actor:
        linked_sub_actor.copy_position_from(this)
        linked_sub_actor.copy_angle_from(this)
        linked_sub_actor.copy_vector_fields_from(this)
```

The boolean polarity of the child visibility slot is inherited and still named conservatively. The stable gameplay point is the branch: one pooled puff is transformed/started when `current_speed > 20.0`; otherwise one pooled child is sent through the opposite visibility/enable path.

## Constants And Wiring

### `.gam` Placeable Properties

`C3DSub` registers FourCC `3SUB`, but the current 35-level `.gam` corpus has no `3SUB` object rows. There are therefore no serialized Sub-only properties to prefill from `docs/gam_schema.md`; the relevant movement constants are constructor defaults and inherited `C3DFlyingObject` fields.

| Property | Type | Offset | Value | Consuming Logic |
|---|---|---:|---|---|
| `MaxHeight` | inherited float | `0x620` | `4000.0` | Inherited flight height clamp. |
| `MaxSpeed` | inherited float | `0x604` | `200.0` | Inherited flight horizontal speed cap. |
| `AccelRate` | inherited float | `0x5fc` | `50.0` | Inherited flight acceleration. |
| `DecelRate` | inherited float | `0x600` | `20.0` | Inherited flight deceleration. |
| `UpRate` | inherited float | `0x624` | `80.0` | Inherited upward velocity target. |
| `DownRate` | inherited float | `0x628` | `-80.0` | Inherited downward velocity target. |
| `MaxVertVelocity` | inherited float | `0x62c` | `110.0` | Inherited vertical velocity clamp. |
| `NewGravity` | inherited float | `0x630` | `0.0` | Registered inherited tuning field. |
| `AccelLean` | inherited float | `0x660` | `10.0` | Inherited flight lean tuning. |
| `DecelLean` | inherited float | `0x664` | `-10.0` | Inherited flight lean tuning. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SUB` | Concrete Sub class id. | ctor `00443350`; `push 0x33535542` at `004434d7` |
| `C3DSUB`, `C3DSub()` | Runtime class/object strings. | strings `.data:004ef33c`, `.data:004f0f80`, `.data:004f0f8c`; constructor/destructor |
| `HIDEFAULT`, `Strato.ase`, `strato.png`, `DEFAULT` | Visual setup. | init slot `00443670`; strings `.data:004ed8e4`, `004f0984`, `004f0978`, `004ee39c` |
| `C3DSmokePuff` | Allocated child/effect class. | constructor allocation size `0x62c`; constructor `00440980`; string `.data:004f0c00` |
| `20` | Smoke puff pool size. | constructor loop count and update wrap check |
| `20.0` | `current_speed` threshold for puff emission path. | update slot `00443810`; compare against `.rdata:00495324` |
| `-150.0` | Local Z offset for emitted puff placement. | update slot `00443810`; immediate `0xc3160000` |
| `0.00025` | Scale applied to transform Y for global effect records. | update slot `00443810`; `.rdata:004b8764` |
| `0.2`, `0.4`, `0.1` | Clamp/phase limits in Sub global effect-record update. | update slot `00443810`; `.rdata:004a2ad0`, `004ab220`, `0048d920` |
| `0.3` | Constructor inherited setup scalar. | ctor `00443350`; immediate `0x3e99999a` |
| `150.0` | Constructor inherited scalar. | ctor `00443350`; immediate `0x43160000` |
| `20.0` | Init visual scalar. | init slot `00443670`; immediate `0x41a00000` |
| `(0, 600, 100)` | Initial transform/vector seeded by init. | init slot `00443670`; immediates `0`, `0x44160000`, `0x42c80000` |
| `180.0`, `360.0`, `0.9`, `4.0`, `1.5` | Rotation-wrap and per-axis scale constants. | raw rotate helper `00443ba0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `Strato.ase` | init slot `00443670`; local file `assets/ase/strato.ASE` | Direct executable reference. Local ASE source scene is `rocketship_5.max`, node `strato01`. This is the same visual path used by `C3DRocketShip`. |
| PNG texture | `strato.png` | init slot `00443670`; local file `assets/png/strato.png` | Direct executable reference. |
| child effect class | `C3DSmokePuff` | constructor `00443350` | Twenty pooled children are allocated and reused by update. Separate wave 8 spec pending. |
| JNvJN candidate ASE | `assets/ase/jnvsjn/substop.ase` | asset scan only | Source scene `substop.max`; materials reference `sub.bmp` and `subsphere.bmp`. Not referenced by this original JNBG constructor. |
| JNvJN candidate PNGs | `assets/png/sub.png`, `assets/png/subsphere.png` | asset scan only | Related Sub art from the later asset corpus; not direct JNBG executable references. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` for constructor/init/cleanup, local `objdump` over raw slots `00443800`, `00443810`, and `00443ba0`, string-table checks, class-id scan backfill, `.gam` schema cross-check, and local asset metadata only; not runtime-validated.

Open questions:
- Name the three global records at `DAT_00509a30`, `DAT_00509a38`, and `DAT_00509a3c` that `UpdateSub` rewrites before the puff branch.
- Identify the producer and exact gameplay role of `linked_sub_actor` at active `0x680`.
- Repair Ghidra function boundaries for raw slots `00443810` and `00443ba0` so decompiler output can confirm argument lists and child visibility polarity.
- Runtime-check whether the `Strato.ase` visual path is an intentional shared placeholder or a dead/code-spawned path unused by shipped levels.
- Confirm whether later JNvJN `substop.ase` art maps to this class or a sequel-only class.

## Notes

- Evidence: `DumpClass.java C3DSub /tmp/decomp_C3DSub.md` (`slots=372`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DSub_raw.md`, local objdump ranges `00443350..00443df0`, and string/asset scans.
- `docs/_gam_classids.tsv` was backfilled for `3SUB -> C3DSub()` from RTTI/string/vtable evidence, then `python3 tools/gam_schema.py` was rerun.
