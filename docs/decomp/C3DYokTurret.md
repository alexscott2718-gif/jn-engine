# C3DYokTurret

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokTurret` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c2638`, `004c2648`, `004c2a98`, `004c2ad4`, `004c2ae8` |
| Ctor(s) | constructor/factory block `FUN_0044c6f0`; registers FourCC `3TUR` at `0044c89f` |
| Dtor(s) | adjusted scalar deleting destructor at `0044c9f0`; cleanup/vtable reset helper at `0044ca20` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokTurret` is the concrete `3TUR` placeable Yokian turret. It inherits `C3DAI` targeting and animated-object transforms, binds an `objectslevel5a.omt` shape during init, tracks a firing timer, adjusts its aim offset by the current target class, and cycles through three preallocated `C3DMissile` children when firing.

## Field Map

Offsets below are byte offsets from the outer `C3DYokTurret` allocation pointer unless marked active. The constructor uses the outer pointer; most vtable slots enter through the active AI pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3TUR` | Rows use `"none"` and `"scene"`; turret-specific code does not branch on it directly. |
| active `0x644` | float | `VisibleRange` | ctor outer `0x704`; `.gam` `3TUR` | Constructor default is `15000.0`; rows range `7500.0..18000.0`. |
| active `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3TUR` | All rows use `"none"`. |
| active `0x6ac` | char buffer/string | `TargetName` | `.gam` `3TUR` | All rows target `"JIM1"`. |
| active `0x80c` | float | `FOV` | ctor outer `0x8cc`; `.gam` `3TUR` | Constructor default is `359.0`; rows range `359.0..9000.0`. |
| active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0044c6f0`; `.gam` `3TUR` | Constructor and current rows use `6`. |
| active `0x89c` | float | `WanderRange` | `.gam` `3TUR` | Current rows use `1500.0`. |
| active `0x4a8` / outer `0x568` | pointer/handle | `objectslevel5a_database` | init slot `0044ca70` | Result of `FUN_0046a910("objectslevel5a.omt")`. |
| active `0x574` / outer `0x634` | byte/bool | `turret_asset_flag` | init slot `0044ca70` | Cleared after the OMT database lookup. |
| active `0x5fc` / outer `0x6bc` | float | `fire_cycle_timer` | ctor `0044c6f0`; raw update `0044cb00` | Accumulates frame delta. At `1.8` seconds it refreshes orientation data; at `4.5` seconds it resets and fires. |
| active `0x608` / outer `0x6c8` | int | `turret_mode_or_state` | ctor `0044c6f0`; raw update `0044cb00` | Constructor writes `6`. Update checks for zero before applying one inherited offset/impulse call. Exact meaning unresolved. |
| active `0x614..0x61c` / outer `0x6d4..0x6dc` | vec3 | `target_aim_offset` | ctor `0044c6f0`; raw update `0044cb00` | Constructor seeds `(0, -500, 2000)`. Update rewrites it for `C3DJIMMY`, `C3DROCKET`, and `C3DJEEP` targets. |
| outer `0x7d0`, `0x7f8`, `0x820`, `0x848`, `0x870`, `0x898` | char buffers/strings | `turret_animation_names` | ctor `0044c6f0` | Constructor copies `"none"` into six animation/name slots. |
| active `0x8d4..0x8dc` / outer `0x994..0x99c` | vec3 | `initial_world_position` | ctor `0044c6f0`; slot `0044ccf0` | Stores the turret's current OMedia position after construction or reset. |
| outer `0x9a4..0x9ac` | pointer[3] | `missile_pool` | ctor `0044c6f0`; fire helper `0044cd50` | Three adjusted `C3DMissile` children allocated at construction, hidden/disabled until fired. |
| outer `0x9b0` | int16 | `next_missile_index` | ctor `0044c6f0`; fire helper `0044cd50` | Current missile-pool cursor; incremented modulo `3` after each shot. |
| outer `0x9b2` | byte/bool | `missile_pool_ready` | ctor `0044c6f0` | Set to `1` after missile-pool allocation. Exact consumers outside the constructor are not named yet. |

No additional turret-owned serialized fields were observed beyond inherited common/AI properties.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044c6f0` | `CtorYokTurret3TUR` | Constructs `C3DAI`, applies turret vtables and tuning values, seeds animation names, sets AI defaults, registers `3TUR`, records initial transform, seeds aim offsets/ranges, allocates three `C3DMissile` children, hides/disables each missile, and finalizes. | non-trivial |
| 7 | `0044ca70` | `InitObjectYokTurret` | Runs `C3DAI::InitObjectAI`, loads `objectslevel5a.omt`, binds database index `5` to the adjusted shape/canvas path, applies `115.0` tuning through inherited slot `0x108`, and finalizes. | non-trivial |
| 8 | `0040e670` | `C3DAnimated::UnInitObjectAnimated` | Inherited animated uninit. | inherited |
| 16 | `0040a3c0` | `C3DAI` collision/AI reaction slot | Inherited AI behavior. | inherited |
| 241 | `0044cb00` | `UpdateYokTurret` | Raw update slot. Runs inherited AI update, maintains orientation, increments `fire_cycle_timer`, rewrites aim offsets by target class, fires when the timer reaches `4.5`, and flips a small active flag at `0x8f2`. | raw block |
| 257 | `0044ccf0` | `SnapshotTurretPosition` | Raw helper. Reads current OMedia world position and copies X/Y/Z into active `0x8d4..0x8dc`. | raw block |
| 259 | `00409480` | `C3DAI::PostLoadAI` | Inherited AI post-load. | inherited |
| 260 | `0040a6b0` | `C3DAI` post-load/update helper | Inherited AI behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated` level/visibility gate | Inherited animated behavior. | inherited |
| vtable 3 slot 2 | `0044c9f0` | scalar deleting destructor | Runs the turret cleanup/vtable reset helper, destroys the adjusted streamer/string subobject at outer `0x9b8`, and frees the allocation when requested. | non-trivial |
| vtable 4 slot 95 | `0044cd50` | `FireYokTurretMissile` | Raw helper. Selects `missile_pool[next_missile_index]`, places it using turret muzzle transforms, triggers effect/sound id `0x89`, marks missile runtime flags, shows/enables it, applies `50.0` tuning, and advances the cursor modulo `3`. | raw block |

## Runtime Behavior

```c
C3DYokTurret::CtorYokTurret3TUR():
    C3DAI::Ctor()
    install_turret_vtables()
    apply inherited tuning: 150.0, 0.8, 0.7
    copy "none" into six animation/name slots
    fire_cycle_timer = 0.0f
    turret_mode_or_state = 6
    AIState = 6
    InitObjectYokTurret()
    initial_world_position = current_world_position()
    register_fourcc("3TUR")
    VisibleRange = 15000.0f
    FOV = 359.0f
    target_aim_offset = (0.0f, -500.0f, 2000.0f)
    missile_pool_ready = true
    for i in 0..2:
        missile_pool[i] = new C3DMissile(1)->active
        show(missile_pool[i], true)
        disable missile update/render flags
```

```c
C3DYokTurret::UpdateYokTurret(dt):
    C3DAI::Update(dt)
    if !global_runtime_allows_turret_update():
        return

    fire_cycle_timer += dt
    if fire_cycle_timer >= 1.8:
        refresh inherited orientation/transform state

    face/aim through inherited OMedia transform slots

    if fire_cycle_timer < 4.5:
        return

    fire_cycle_timer = 0.0f
    if current_target is C3DJIMMY:
        target_aim_offset = (0, 0, 200)
    else if current_target is C3DROCKET:
        target_aim_offset = (0, 0, distance_to_target * 0.35)
    else if current_target is C3DJEEP:
        target_aim_offset = (0, 0, 400)

    if turret_mode_or_state == 0:
        apply inherited offset/impulse (0, 0, 320)

    fire_missile()
    active_flag_0x8f2 = !active_flag_0x8f2
```

```c
C3DYokTurret::FireYokTurretMissile():
    missile = missile_pool[next_missile_index]
    if missile == NULL:
        return

    hide(missile)
    missile->arm_or_launch_flag = true
    play_effect_or_sound(0x89, muzzle_position)
    copy muzzle transform and orientation to missile
    clear missile runtime flags
    show/enable(missile)
    apply missile tuning 50.0

    next_missile_index = (next_missile_index + 1) % 3
```

The fire helper is still raw, but the missile pool, muzzle transform copy, id `0x89`, and modulo-three cursor are direct disassembly evidence.

## Constants And Wiring

### `.gam` Placeable Properties

`3TUR` appears eighteen times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` patrol/targeting fields. Turret adds no unique serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DYOKTURRET"`, `"yokturr"`, `"yokturret"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861164882` | FourCC/object id value for `3TUR`. |
| `PositionX` | float | inherited | `-58900..46100` | Base placement transform. |
| `PositionY` | float | inherited | `-5020..3600` | Base placement transform. |
| `PositionZ` | float | inherited | `-39700..39400` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..270` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"none"`, `"scene"` | Not used by turret-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag; no turret-owned branch found. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated progress gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `HasCollision` | int | inherited | `0..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1..1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `0..1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Present on twelve rows; turret-specific code does not consume it directly. |
| `PatrolPoint` | str | inherited `0x648` | `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited active `0x644` | `7500..18000` | Compared by inherited AI target/range logic; constructor default `15000`. |
| `FOV` | float | inherited active `0x80c` | `359..9000` | Used by inherited AI facing/visibility helpers; constructor default `359`. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited active `0x87c` | `6` | Constructor and current rows both seed state `6`. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3TUR` | Concrete placeable class id for Yokian Turret. | ctor `0044c6f0`; `push 0x33545552` at `0044c89f` |
| `?333` | False-positive class-id scan hit; actually float `0.7`. | constructor immediate `0x3f333333` at `0044c7cb` |
| `C3DYOKTURRET` | Concrete object/type string. | string `.data:004f1a84`; constructor string path |
| `objectslevel5a.omt` | OMT database for turret visual setup. | string `.data:004ee89c`; init slot `0044ca70` |
| index `5` | OMT database entry bound by init. | init slot `0044ca70`; `FUN_00477ba0(db, 5)` |
| `C3DMissile` | Projectile helper class allocated into the pool. | string `.data:004eebb4`; ctor allocates `0042f500` three times |
| `C3DJIMMY`, `C3DROCKET`, `C3DJEEP` | Target-class checks controlling aim offset. | raw update strings `.data:004ecb20`, `.data:004ed640`, `.data:004ecc80` |
| `1.8` | Orientation refresh threshold. | double at `.rdata:00494cb8`; raw update |
| `4.5` | Fire threshold. | double at `.rdata:004afd38`; raw update |
| `0.35` | Rocket distance multiplier for aim offset. | double at `.rdata:0049b200`; raw update |
| `0x89` | Effect/sound id emitted when firing. | fire helper `0044cd50` |
| `3` | Missile pool size and modulo cursor. | ctor loop and fire helper |
| `115.0` | Visual/tuning value applied during init. | init slot `0044ca70`; immediate `0x42e60000` |
| `150.0`, `0.8`, `0.7` | Constructor tuning values applied through inherited slots. | ctor `0044c6f0` |
| `50.0` | Missile tuning value after launch. | fire helper `0044cd50`; immediate `0x42480000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objectslevel5a.omt` | init slot `0044ca70`; parsed metadata `assets/parsed/objectslevel5a/objectslevel5a.json` | Original source path in metadata is `/home/scotty/xp-jnbg-original/omt/objectslevel5a.omt`. |
| OMT entry | index `5` | `FUN_00477ba0(db, 5)` | Local parsed image entry `5` is named `doorcave`; the OMT entry may include more than image metadata. |
| helper class | `C3DMissile` | ctor `0044c6f0`; class-id row `3MIS` | Three hidden missile children are preallocated and reused. |
| sound/effect candidate | `turret shot` | asset scan only; raw fire helper emits id `0x89` | A parsed sound named `turret shot` exists, but the executable path here uses numeric id `0x89`, so the link is not yet proven. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, local `objectslevel5a.omt` metadata, `C3DMissile` class-id cross-check, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw turret targets `0044cb00`, `0044ccf0`, and `0044cd50`.
- Name inherited transform/muzzle slots used by `FireYokTurretMissile`.
- Runtime-check whether numeric id `0x89` maps to the parsed `turret shot` sound or another effect service.
- Confirm whether `objectslevel5a.omt` index `5` is the actual turret visual despite parsed image metadata naming it `doorcave`.
- Confirm the exact semantics of constructor tuning values `150.0`, `0.8`, `0.7`, `115.0`, and missile tuning `50.0`.

## Notes

- Evidence: `DumpClass.java C3DYokTurret /tmp/decomp_C3DYokTurret.md` (`slots=392`, `owned_methods=1`, `offsets=0`), local objdump windows over `0044c6f0..0044d090`, string scans around `004ee89c`, `004eebb4`, and `004f1a70`, parsed `objectslevel5a.omt` metadata, `.gam` schema for `3TUR`, and `C3DMissile` class-id cross-check.
- `3TUR -> C3DYokTurret` was backfilled in `docs/_gam_classids.tsv` from RTTI/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
