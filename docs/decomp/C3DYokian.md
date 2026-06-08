# C3DYokian

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokian` |
| Base chain | `C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c019c`, `004c01ac`, `004c05fc`, `004c0638`, `004c064c` |
| Ctor(s) | constructor at `0044a730`; no direct FourCC binding |
| Dtor(s) | adjusted scalar deleting destructor at `0044a980`; cleanup/vtable reset helper at `0044a9b0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokian` is the shared enemy base for the Yokian humanoid family, including `C3DFleetCommander`, `C3DYokianGuard`, `C3DYokianSoldier`, and `C3DYokianSpy`. It inherits the general AI state machine from `C3DAI`, the pickup/progress gate from `C3DPickupType`, and the two tail enemy flags from `C3DEnemy`. This class adds Yokian-specific helper sprites, hit reaction behavior, shadow/shield maintenance, and the attack effect handle used by descendants.

## Field Map

Offsets below are byte offsets from the active `C3DAI`/animated subobject pointer used by slot-1 methods. Constructor-only evidence is also listed with the outer object equivalent, which is `active + 0xc0`.

| Active Offset | Outer Offset | Type | Name | Source | Meaning |
|---:|---:|---|---|---|---|
| inherited `0x600..0x89c` | inherited `0x6c0..0x95c` | mixed | `C3DAI` state block | `C3DAI`; descendant `.gam` rows | Target, patrol, state, FOV, range, animation-state, and wander fields. |
| inherited `0x8d4..0x8dc` | inherited `0x994..0x99c` | mixed | `C3DPickupType` optional pickup fields | `C3DPickupType` | Available through the base, but `C3DYokian` does not enable pickup fields in its constructor. |
| `0x874` | `0x934` | float | `goddard_hit_reaction_scalar` | touch slot `0044b070` | Set to `2.0` when the other object is `C3DGODDARD`. Final effect is inherited AI/movement tuning. |
| `0x880` | `0x940` | float | `reaction_or_attack_range` | ctor `0044a730`; update `0044aa00`; touch `0044b070` | Constructor seeds inherited AI tuning. Baseball/hit reaction sets it to `2200.0`; recovery setup sets it to `1500.0`. |
| `0x898` | `0x958` | byte/bool | `yokian_enabled_flag` | ctor `0044a730`; inherited `C3DEnemy` flag | `C3DEnemy` clears this flag, then `C3DYokian` sets it true. Exact semantic name remains open. |
| `0x8ac` | `0x96c` | float | `baseball_hit_delay` | touch slot `0044b070` | Set to `5.0` after a `C3DBASEBALL` hit; likely participates in inherited AI reaction timing. |
| `0x8b0` | `0x970` | byte/bool | `hit_recovery_active` | ctor `0044a730`; update `0044aa00` | Enables the Yokian hit/recovery branch. The update decrements `hit_recovery_timer`, restores AI state, and clears this flag when the timer expires. |
| `0x8b4` | `0x974` | float | `hit_recovery_timer` | ctor `0044a730`; update `0044aa00` | Counts down while `hit_recovery_active` is set. |
| `0x8d0` | `0x990` | float | `yokian_default_range_900` | ctor `0044a730` | Constructor seeds `900.0`; exact consuming helper remains unresolved. |
| `0x8e0` | `0x9a0` | pointer | `shield_or_bubble_child` | ctor `0044a730`; update `0044aa00` | Adjusted pointer returned by `FUN_0044b510`. Vtables identify `C3DYokianShield`, while the constructor path still passes `C3DBubble` strings. The update positions it between Yokian/player when active. |
| `0x8e4` | `0x9a4` | pointer | `attached_visible_child` | ctor `0044a730`; update `0044aa00`; toggle `0044afa0` | Optional adjusted child pointer. When non-null, update copies this Yokian's transform/rotation to it and toggle logic shows/hides it with the Yokian. Descendants may populate it. |
| `0x8ec` | `0x9ac` | pointer | `shadow_child` | ctor `0044a730`; update `0044aa00`; toggle `0044afa0` | Adjusted `C3DShadow` pointer created unless global flag `DAT_00509a13` disables it. The update terrain-projects and shows/hides it. |
| `0x8f0` | `0x9b0` | byte/bool | `yokian_runtime_flag_0x8f0` | ctor `0044a730` | Constructor clears it. No owned branch was isolated yet. |
| `0x8f4` | `0x9b4` | handle/int | `attack_effect_handle` | ctor `0044a730`; slots 241, 272, 273; vtable4 slot 91 | Initialized to `-1`. Hit/update paths release it; pause/resume hooks release/recreate it; attack hook creates effect id `0x38`. |
| `0x8e8` | `0x9a8` | word | `yokian_word_0x8e8` | ctor `0044a730` | Constructor clears it. No owned branch was isolated yet. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0044a730` | `CtorYokianBase` | Constructs `C3DEnemy`, installs five `C3DYokian` vtables, seeds Yokian AI defaults, creates a shield/bubble child, optionally creates a `C3DShadow` child, seeds animation strings, and initializes effect/runtime fields. | non-trivial |
| 16 | `0044b070` | `ReactToHitObject` | Runs base contact handling. If touched by `C3DBASEBALL` and the ball is not already inactive, plays sound `0x2f`, forces AI state `5`, sets reaction range `2200.0`, sets `baseball_hit_delay=5.0`, and starts delay `10`. If touched by `C3DJIMMY`, sets `goddard_hit_reaction_scalar`-adjacent field to `2.0`. | non-trivial |
| 241 | `0044aa00` | `UpdateYokian` | Runs `C3DAI::UpdateAIStateMachine`, terrain-projects/maintains the shadow child, syncs optional child transforms, updates shield placement, manages hit recovery, controls child visibility, and applies a vertical-motion response when needed. | non-trivial |
| 259 | `0044b060` | `PostLoadAIPickupTypeThunk` | Tail jump to `C3DPickupType::PostLoadAIPickupType` at `00436b10`. | inherited thunk |
| 272 | `0044b140` | `ReleaseAttackEffectHandle` | Runs `C3DAnimated` slot 272, then releases `attack_effect_handle` with `FUN_0047d7a0` when present. | non-trivial |
| 273 | `0044b160` | `ReacquireAttackEffectHandle` | Runs `C3DAnimated` slot 273, then recreates effect id `0x38` through `FUN_004589c0` when the handle field is active. | non-trivial |
| vtable 3 slot 2 | `0044a980` | scalar deleting destructor | Runs cleanup helper `0044a9b0`, destroys the adjusted streamer/string subobject at outer `0x9bc`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 68 | `0044afa0` | `ToggleYokianChildren` | Shows/hides the Yokian object, calls paired inherited active/inactive hooks on the animated subobject, and toggles `attached_visible_child` / `shadow_child` visibility with the parent. | raw block |
| vtable 4 slot 91 | `0044b100` | `StartYokianAttackEffect` | Selects animation `ATTACK` and creates effect id `0x38` into `attack_effect_handle` if no handle is active. | non-trivial |

## Runtime Behavior

`C3DYokian` keeps the base AI loop, but layers visual helpers and hit reaction on top of it.

```c
C3DYokian::CtorYokianBase():
    C3DEnemy::CtorEnemy()
    install C3DYokian vtables

    current_state = 2
    AIState = 2
    reaction_or_attack_range = 200.0
    hit_recovery_timer = 0.0
    hit_recovery_active = false
    yokian_enabled_flag = true

    shield_or_bubble_child = new FUN_0044b510() + adjusted_sprite_offset
    shadow_child = global_no_shadow ? NULL : new C3DShadow() + adjusted_sprite_offset
    if shadow_child:
        shadow_child.show_or_enable(true)
        shadow_child.set_sprite_size_or_offset(0x8c, 0x32)

    copy_string(anim_attack, "ATTACK")
    copy_string(anim_walk, "WALK")
    copy_string(anim_stop, "STOP")
    attack_effect_handle = -1
    yokian_default_range_900 = 900.0
```

```c
C3DYokian::ReactToHitObject(other):
    CGameObject::Touch(other)
    if other.is("C3DBASEBALL") and !other.already_handled():
        play_effect_or_sound(0xffffffff, 0x2f, 0)
        this.slot_0x124(5)
        reaction_or_attack_range = 2200.0
        baseball_hit_delay = 5.0
        delay_or_cooldown(10)

    if other.is("C3DJIMMY"):
        goddard_hit_reaction_scalar = 2.0
```

```c
C3DYokian::UpdateYokian(dt):
    C3DAI::UpdateAIStateMachine(dt)
    clear_some_transient_movement_outputs()

    if shadows_enabled:
        terrain_project_shadow_below_yokian()

    if current_state in {2, 3, 6}:
        release attack_effect_handle

    if attached_visible_child:
        attached_visible_child.position = this.position
        attached_visible_child.rotation = this.rotation

    if shield_or_bubble_child is active and target_object:
        place shield_or_bubble_child near Yokian, offset away from player by 100.0

    if hit_recovery_active:
        release attack_effect_handle
        maybe play one of sounds 0xf3..0xf6
        hide_or_disable shadow/attached child
        if current_state == 0:
            this.slot_0x124(5)
            reaction_or_attack_range = 1500.0
            current_state_timer = 200.0
        hit_recovery_timer -= dt
        if hit_recovery_timer <= 0.0:
            hit_recovery_active = false
            clear temporary target/vector state
            this.slot_0x124(AIState)
            select_animation("WALK", true)
            current_state_timer = 200.0
            restore child visibility

    sync shadow visibility with parent active flag
    if upward_velocity_or_motion.y > 0:
        apply vertical response with constant 250.0
```

The helper names above reflect static behavior. Several exact inherited slots (`0x124`, `0x178`, `0x214`, `0x3a4`, `0x3ac`) still need final names from the base object/movement specs before porting.

## Constants And Wiring

`C3DYokian` has no direct placeable FourCC row in `docs/gam_schema.md` and no direct class-id scan row in `docs/_gam_classids.tsv`. It is a concrete base constructor used by placeable descendants.

Known descendant rows using this behavior:

| Descendant | FourCC / Notes |
|---|---|
| `C3DFleetCommander` | `3FLE`; already documented as a Yokian-derived commander/talk actor. |
| `C3DYokianGuard` | `3GUA`; documented Yokian guard leaf. |
| `C3DYokianSoldier` | `3SOL`; documented Yokian soldier leaf. |
| `C3DYokianSpy` | `3SPY`; documented captain/spy leaf; populates `attached_visible_child` with a `C3DYokHelmet`. |

Runtime constants:

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DYOKIAN` | Base object/type string. | string `.data:004ee2ec`; ctor `0044a730` |
| `ATTACK` | Attack animation selected by vtable4 slot 91 and copied by the constructor. | string `.data:004ee550`; `0044b100`; ctor `0044a730` |
| `WALK` | Recovery animation selected after hit recovery ends. | string `.data:004eca54`; `0044aa00` |
| `STOP` | Constructor-copied stop/idle animation string. | string `.data:004ed040`; ctor `0044a730` |
| `C3DBASEBALL` | Collision class that triggers baseball-hit reaction. | string `.data:004ecc50`; touch slot `0044b070` |
| `C3DJIMMY` | Collision class that tweaks a hit/reaction scalar. | string `.data:004ecb20`; touch slot `0044b070` |
| `C3DShadow` / `3SHA` | Runtime shadow child constructor. | `FUN_0043f640`; constructor stores adjusted pointer at active `0x8ec` |
| `FUN_0044b510` helper | Runtime shield/bubble child constructor. | Constructor stores adjusted pointer at active `0x8e0`; vtables match `C3DYokianShield`, strings still name `C3DBubble` |
| `0x38` | Effect id used for attack effect handle allocation. | `0044b100`; `0044b160` |
| `0x2f` | Sound/effect id played on baseball hit. | touch slot `0044b070` |
| `0xf3..0xf6` | Randomized sound/effect ids played when hit recovery starts. | update `0044aa00`; RNG thresholds `0.25`, `0.5`, `0.75` |
| `200.0` | Constructor/current-state timer default and recovery reset value. | ctor `0044a730`; update `0044aa00`; immediate `0x43480000` |
| `2200.0` | Baseball-hit reaction range/distance value. | touch slot `0044b070`; immediate `0x45098000` |
| `1500.0` | Hit recovery range/distance value. | update `0044aa00`; immediate `0x44bb8000` |
| `900.0` | Constructor-seeded Yokian tuning field. | ctor `0044a730`; immediate `0x44610000` |
| `5.0` | Baseball-hit delay scalar. | touch slot `0044b070`; immediate `0x40a00000` |
| `2.0` | Jimmy-contact scalar. | touch slot `0044b070`; immediate `0x40000000` |
| `250.0` | Vertical response constant at the end of update. | update `0044aa00`; immediate `0x437a0000` |
| `100.0` | Shield/bubble placement offset away from the player. | update `0044aa00`; vector math constants |

## Assets

`C3DYokian` itself names no humanoid mesh or texture. Leaf classes supply guard/soldier/commander/spy assets. The base owns or creates helper visual objects:

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| helper object | `C3DShadow` via `FUN_0043f640` | ctor `0044a730`; `docs/_gam_classids.tsv` `3SHA` | Runtime shadow/ground marker. The base stores its adjusted pointer at active `0x8ec`. |
| helper object | `FUN_0044b510` shield/bubble child | ctor `0044a730`; vtables `004c0e14..004c1288` | Vtables identify `C3DYokianShield`, but constructor strings still identify `C3DBubble`; leave the FourCC/schema normalization for the `C3DYokianShield` leaf spec. |
| retained sprites | `RetainedSprites.omt` | helper constructors | Used by the shadow/bubble/sprite helper family. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, existing base specs, descendant `C3DFleetCommander` cross-check, and `.gam` schema context only; not runtime-validated.

Open questions:
- Name the exact inherited movement/visibility slots used as `0x124`, `0x178`, `0x214`, `0x3a4`, and `0x3ac`.
- Confirm the final semantic names of `reaction_or_attack_range`, `yokian_default_range_900`, and the hit recovery fields from runtime traces or leaf behavior.
- Resolve the `FUN_0044b510` identity conflict when documenting `C3DYokianShield`: vtables are `C3DYokianShield`, but the constructor path still uses `C3DBubble` strings and the current schema map names `3YSH` as `C3DBubble`.
- `C3DYokianSpy` populates `attached_visible_child` at active `0x8e4`; check whether other descendants also use it.
- Runtime-check the baseball hit reaction and attack effect handle before marking this family `validated`.

## Notes

- Evidence: `DumpClass.java C3DYokian /tmp/decomp_C3DYokian.md` (`slots=392`, `owned_methods=4`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DYokian_funcs.md`, local disassembly over `0044a730..0044b194`, string-table scans around `004ee2ec`, `004eca54`, `004ecc50`, `004f0a50`, and `004f17ec`, and descendant cross-checks from `C3DFleetCommander`.
- The active-pointer/outer-pointer split matters: slot-1 methods use the active AI pointer, while constructor and vtable4 slot 68 use the outer allocation pointer.
