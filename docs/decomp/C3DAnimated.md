# C3DAnimated

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAnimated` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048fe0c`, `0048fe1c`, `0049026c`, `004902a8`, `004902bc` |
| Ctor(s) | TODO |
| Dtor(s) | adjusted scalar deleting destructor at `0040d2d0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAnimated` pointer in the slot-1 methods. Several OMedia morph/shape methods are called with adjusted `this` pointers, so their same numeric indexes are not treated as the same absolute fields until full structs are applied.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x578` | int | `RequiredLevel` | `.gam` registration at `0040d3c0` | Enables the object when the current level/state is at or after this value, unless removed by `RemoveLevel`. `-1` disables this gate. |
| `0x57c` | int | `ExactLevel` | `.gam` registration at `0040d3c0` | If not `-1`, enables only when current level/state equals this value. |
| `0x580` | int | `RemoveLevel` | `.gam` registration at `0040d3c0` | Upper bound for the `RequiredLevel` range; `-1` means no removal bound. |
| `0x584` | int | `HasCollision` | `.gam` registration at `0040d3c0` | Common collision toggle; consumed by the enable/disable collision helper pair. |
| `0x588` | int | `InitiallyVisible` | `.gam` registration at `0040d3c0`; `0040e7b0` | Initial visibility state applied after object setup. |
| `0x58c` | int | `CanMove` | `.gam` registration at `0040d3c0`; `0040e050` | When zero, per-frame update forces OMedia transform back from the game-object transform path. |
| `0x590` | int | `SecondPass` | `.gam` registration at `0040d3c0`; `0040e7b0` | Enables second-pass/material behavior through an inherited setter. |
| `0x595` | char buffer | `PickupLink` | `.gam` registration at `0040d3c0`; `0040e050` | Object tag/string used for lazy runtime linkup unless equal to `"none"`. |
| adjusted | pointer | `anim3d_database` | `0040e270`, `0040d4a0` | OMedia database used to load `Canv`, `3DSh`, `3DMa`, and `A3dm` objects. |
| adjusted | list | `animation_records` | `0040d4a0`, `0040dd90` | Linked list of loaded animation records; each record stores name, id/index, DB object pointer, and next link. |
| adjusted `+0x588` | int | `current_anim_index` | `0040da30`, `0040dab0` | Numeric OMedia animation index selected from the current animation record. |
| adjusted `+0x58c` | pointer | `current_anim_db_object` | `0040db10`, `0040dd90` | DB object pointer applied from the selected animation record. |
| adjusted `+0x62c` | pointer | `current_anim_record` | `0040dd90`, `0040da30` | Selected animation record; record `+0x48` caches the last-frame/frame-count value used by `UpdateAnimated`. |
| adjusted `+0x634/+0x635` | bytes | `anim_loader_ready_flags` | `0040d9e0`, `0040da30`, `0040dab0`, `0040dd90`, `0040e050` | Guard record lookup, animation selection, and the whole `UpdateAnimated` completion path (`this1[0x15d]` bytes 0/1 with `this1 = adjusted + 0xc0`). |
| adjusted `+0x654` | byte | `anim_paused_mirror` | `0040d350` | `SetAnim3DPaused`'s edge-guard mirror keeping `OMediaAnim::pause`'s count binary. Pause gates frame *advance* (`OMediaAnim::update_logic` returns while `pause_count != 0`); it is **not** the last-frame hook gate — that gate is `play_loop` at adjusted `+0xad` (see below). |
| adjusted `+0x90` | embedded object | `omedia_anim` | `0040da30`, `0040e050`, `0040d350` | Embedded `OMediaAnim` instance (OMT 2.5 source layout): `+0x94` `anim_def`, `+0xa0` `current_sequence` (the frame-count refresh index), `+0xa8` `pause_count`, `+0xad` `play_loop`. Vtable calls: `+0x10` `setcurrentsequence(long, bool restart)`, `+0x1c` `getcurrentframe_pos()`, `+0x24` `setplay_timebased(bool)`. |
| adjusted `+0x580` | short | `shape_mode` | `0040dd90`, `0040e270` | Selects base (`0`) / alternate (`1`) shape and the lookup-key lead string in `SetAnim3DByName`; seeded `0` by `InitAnim3DDatabase`. |
| adjusted `+0x584` | float | `anim_clock` | `0040dd90`, `0040e050` | Per-frame animation clock (`this1[0x131] += dt` in `UpdateAnimated`); zeroed unconditionally by every `SetAnim3DByName` call that passes the ready-flag guard, found or not. |
| adjusted | pointer arrays | `canvas_slots[]`, `material_slots[]` | `0040db20`, `0040dd40`, `0040dd60` | Up to 10 loaded canvas/material pairs used by animation textures. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0040d3c0` | `InitObjectAnimated` | Runs `C3DObject::InitObject`, then registers `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, and `PickupLink`. | non-trivial |
| 8 | `0040e670` | `UnInitObjectAnimated` | Detaches current OMedia animation/shape state, runs `C3DObject::UnInitObject`, then frees loaded canvas/material arrays and animation-list records when the loader was initialized. | non-trivial |
| 241 | `0040e050` | `UpdateAnimated` | Lazily resolves `PickupLink`, delegates to `C3DObject::Update3DObject`, enforces non-moving transform sync when `CanMove == 0`, and fires vtable-4 slot 65 when a **non-looping** current OMedia animation reaches its last frame (gate = `OMediaAnim::play_loop` at adjusted `+0xad`; no completion latch — the hook re-fires every update while the clip sits at its last frame). | non-trivial |
| 242 | `0040d3a0` | `HideIfVisibleFlagSet` | If the adjusted visibility flag is non-zero, marks it as set and calls an inherited visibility setter with false. | TODO |
| 259 | `0040e7b0` | `ApplyInitialAnimatedFlags` | Applies `InitiallyVisible`; if `SecondPass == 1`, calls inherited second-pass/material setup. | non-trivial |
| 265 | `0040e340` | `ApplyLevelGate` | Uses `RequiredLevel`, `ExactLevel`, and `RemoveLevel` to enable or disable the object for the current level/state. | non-trivial |
| 266 | `0040e1f0` | `ApplyAnimatedCollisionVisibleState` | Paired state helper used by the animated enable path; applies collision helpers when `HasCollision` is set and toggles inherited visible/enabled state through the adjusted base object. | non-trivial |
| 272 | `0040e770` | `EnableAnimatedCollision` | Calls inherited collision/interaction setter with true. | trivial |
| 273 | `0040e790` | `DisableAnimatedCollision` | Calls inherited collision/interaction setter with false. | trivial |
| vtable 3 slot 2 | `0040d2d0` | scalar deleting destructor | Runs local cleanup helper, destroys the embedded `OMediaClassStreamer`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 54 | `0040d4a0` | `CreateAnim3DRecord` | Appends an animation record, resolves a file path, opens the source stream, imports the OMedia animation/shape object into the local DB, stores the DB object pointer, and records the caller-supplied animation name. | non-trivial |
| vtable 4 slot 55 | `0040d9e0` | `FindAnim3DRecordByName` | Walks the loaded animation-record list and returns the case-insensitive name match while loader-ready flags are set. | non-trivial |
| vtable 4 slot 56 | `0040dd90` | `SetAnim3DByName` | Takes `(name, loop_flag)`. Zeroes the anim clock, selects base/alternate shape, composes an animation lookup key, finds an animation record, stores it as current, forwards the loop flag to slot 57, and applies the DB object pointer. | non-trivial |
| vtable 4 slot 57 | `0040da30` | `SelectAnim3DRecordIndex` | Takes `(seq_id, loop_flag)`. Writes the current animation id, calls `OMediaAnim::setcurrentsequence(seq_id, loop_flag)`, stores the flag as `play_loop` (`+0xad`), forces `setplay_timebased(true)` (resets the time-base phase), and refreshes the current record's frame-count cache from the imported def's `sequences[current_sequence].size()`. | non-trivial |
| vtable 4 slot 58 | `0040dab0` | `GetCurrentAnim3DRecord` | Returns the animation record whose id matches the current animation index; logs through the inherited trace path on miss. | non-trivial |
| vtable 4 slot 59 | `0040db10` | `GetCurrentAnim3DObject` | Returns the current animation DB object pointer. | trivial |
| vtable 4 slot 60 | `0040db20` | `CreateTextureSlot` | Loads an `OMediaCanvas` from a file path into `canvas_slots[index]`, then creates and initializes the paired `OMedia3DMaterial` in `material_slots[index]`. | non-trivial |
| vtable 4 slot 61 | `0040dd60` | `AssignTextureSlotToMaterial` | If a material is supplied, calls its texture/canvas setter with `material_slots[index]`. | trivial |
| vtable 4 slot 62 | `0040df90` | `EnsureAnim3DDatabase` | Lazily creates global/static `OMediaMemStream` and `OMediaDataBase` objects used by the animation loader. | non-trivial |
| vtable 4 slot 63 | `0040dd40` | `SetTextureSlotModes` | Writes two mode fields at offsets `0x30` and `0x34` in `material_slots[index]`. | trivial |
| vtable 4 slot 64 | `0040df80` | `GetAnim3DNameBuffer` | Returns the adjusted animation name buffer at `this+0x5e8`. | trivial |
| vtable 4 slot 65 | `00472970` | `OnAnimEndedBaseNoop` | Default animation-ended hook invoked by `UpdateAnimated`; base `C3DAnimated` leaves it as the shared no-op/thunk, while `C3DPlayer` overrides this slot at `0043a900`. | trivial |
| vtable 4 slot 66 | `0040e270` | `InitAnim3DDatabase` | Ensures the DB, registers OMedia object builders for `Canv`, `3DSh`, `3DMa`, and `A3dm`, loads default `3DSh`, seeds shape flags, then invokes the shape-selection helper. | non-trivial |
| vtable 4 slot 68 | `0040e3e0` | `ApplyAnimatedEnabledState` | Bridges animated enabled/visible state into inherited visibility, collision, and material/selection hooks. | non-trivial |
| vtable 4 slot 69 | `0040e4a0` | `SetShapeMaterialAlphaOrPass` | Iterates materials in the active shape and writes render/pass fields; non-positive input clears a flag, positive input sets pass and alpha-like value. | non-trivial |
| vtable 4 slot 70 | `0040e5e0` | `ForceShapeMaterialPass` | Iterates current shape materials and marks them for render mode/pass `6/7`. | non-trivial |
| vtable 4 slot 71 | `0040d350` | `SetAnim3DPaused` | Toggles `OMediaAnim::pause` and the local paused byte. | non-trivial |

## Per-Frame Behavior

```c
C3DAnimated::UpdateAnimated(dt):
    if engine_allows_update():
        if pickup_link_not_checked:
            pickup_link_not_checked = true
            if PickupLink != "none":
                target = find_object_or_context_for_pickup_link()
                if target_is_usable_for_current_state(target):
                    inherited_visibility_or_state_hook()
                    selected_animation_hook()
                    inherited_transform_hook()

        C3DObject::Update3DObject(dt)

        if CanMove == 0:
            pos = inherited_get_position_pair()
            inherited_set_position_pair(pos.x, pos.y)
            xform = inherited_get_transform_vector()
            inherited_set_transform_vector(xform)

        if anim_loader_ready_flags_set:            # +0x634 and +0x635
            local_anim_clock += dt                  # +0x584, always
            if !omedia_anim.play_loop:              # +0xad — loops never complete
                if current_anim_record:             # +0x62c
                    frame_pos = omedia_anim.getcurrentframe_pos()
                    if current_anim_record->frame_count - 1 <= frame_pos:
                        vtable4_slot65_OnAnimEnded()   # every update; no latch
```

Frame advance itself happens inside the `Update3DObject` element update
(`OMediaAnim::update_logic`, time-based walk), before the completion check
above. Pause (`pause_count != 0`) stops that advance but not the check, so a
paused one-shot already at its last frame keeps firing the hook.

The default slot-65 implementation is the shared no-op/thunk at `00472970`;
`C3DPlayer` overrides the same vtable-4 slot with `0043a900`
(`OnPlayerAnimEnded`) and consumes the event for Jimmy's FENCE/LADDER and
SPLAT/HIT return-to-STOP paths.

Level gating:

```c
C3DAnimated::ApplyLevelGate(level):
    if ExactLevel != -1:
        enabled = (level == ExactLevel)
    else if RequiredLevel == -1:
        enabled = false
    else:
        enabled = (RequiredLevel <= level) &&
                  (RemoveLevel == -1 || level < RemoveLevel)
    inherited_set_enabled(enabled)
```

## Constants And Wiring

The eight registered properties appear widely across 3D placeable classes. Aggregate counts/ranges below are from all `assets/gam/*.gam` files parsed by `tools/gam_schema.py`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `RequiredLevel` | int (`6`) | `0x578` | 1329 values, `-1..550` | Lower bound in `ApplyLevelGate`; `-1` disables range gating. |
| `ExactLevel` | int (`6`) | `0x57c` | 1222 values, mostly `-1`, max `470` | Overrides range gating when not `-1`. |
| `RemoveLevel` | int (`6`) | `0x580` | 621 values, mostly `-1`, max `500` | Upper bound in range gating. One malformed/extreme value exists in source data. |
| `HasCollision` | int (`6`) | `0x584` | 616 values, `-1..1` | Paired enable/disable collision helpers call inherited setter. |
| `InitiallyVisible` | int (`6`) | `0x588` | 616 values, `-1..1` | Applied by `ApplyInitialAnimatedFlags`. |
| `CanMove` | int (`6`) | `0x58c` | 604 values, `0..1` | When zero, update pins OMedia transform back to game-object transform. |
| `SecondPass` | int (`6`) | `0x590` | 604 values, `0..1` | Enables inherited second-pass/material setup. |
| `PickupLink` | str (`1`) | `0x595` | 527 values; `"none"`, `"hydrant"`, `"water2"` | Lazy link lookup in `UpdateAnimated`; `"none"` bypasses it. |

OMedia class IDs registered by `InitAnim3DDatabase`:

| FourCC | Meaning |
|---|---|
| `Canv` | `OMediaCanvas` objects used by texture slots. |
| `3DSh` | `OMedia3DShape` mesh objects. |
| `3DMa` | `OMedia3DMaterial` objects. |
| `A3dm` | `OMedia3DMorphAnimDef` animation definitions. |

## Assets

No fixed asset filename is embedded in `C3DAnimated`; callers supply animation, shape, and texture path/name strings. Loader evidence strings in `Neutron.exe` include `Anim3D_CreateAnim`, `Anim3D_GetAnim`, and `Anim3D_CreateTexture`.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMedia DB object | `Canv`, `3DSh`, `3DMa`, `A3dm` | `0040e270` | Registered with `OMediaDataBase::register_object`. |
| animation record | caller supplied | `0040d4a0`, `0040dd90` | Linked records store name/id plus DB object pointer. |
| canvas/material slot | caller supplied path + index | `0040db20`, `0040dd40`, `0040dd60` | `OMediaCanvas` is paired with a new `OMedia3DMaterial`. |

## Target 7 Animation Dispatch

Target 7 opened the L1 bodies around `UpdateAnimated` (`0040e050`) and
`SetAnim3DByName` (`0040dd90`). The recovered mechanism is:

1. Load/import animation records into an OMedia database (`CreateAnim3DRecord`,
   `InitAnim3DDatabase`).
2. Compose a shape-specific animation lookup key from a shape-mode-selected
   lead string plus the caller animation name (`SetAnim3DByName`; see the
   Native Linkage key-composition note for the exact copy semantics).
3. Find the record by case-insensitive name, select its OMedia animation id,
   apply its DB object pointer, and cache the last-frame count.
4. On each update, compare the current embedded OMedia animation frame against
   the cached last-frame count and call vtable-4 slot 65 when the clip ends.

Base `C3DAnimated` does not itself implement a behavior event at completion:
slot 65 is `00472970`, the shared no-op/thunk. The concrete consumer found by
this target is `C3DPlayer` vtable-4 slot 65 (`0043a900`,
`OnPlayerAnimEnded`), already documented in `docs/decomp/C3DPlayer.md`.

## Native Linkage

`event-animation-dispatch`: `linked-blocked`.

The original side is now L1-backed, but native has no 1:1 `C3DAnimated`
animation-record subsystem. Current native `behavior_cutscene.c` dispatches
cutscene actor poses through a static `ACTOR_ANIMS[]` alias table (85 rows in
the current tree, not the old worklist's 53-row shape): non-player
targets copy `cutscene_model`, optional texture, loop flag, and reset
`anim_time`; Jimmy maps aliases to the separate `PlayerAnim` enum and calls
`player_anim_advance`. Native `player_anim.c` then advances hardcoded Jimmy
ASE clips with global `g_current_anim`/`g_clip_time`. Native
`behavior_animsprite.c` is the separate `C3DAnimatedSprite`/`3ANI` billboard
frame animator, not the OMedia morph-animation record path.

There is therefore no native `SetAnim3DByName` record lookup, OMedia DB import,
or vtable-4 slot-65 `AnimEnded` hook to certify. An oracle over the current
native cutscene/player pose table would certify a different design, so the
row returns to native-port until that mechanism is ported 1:1.

The existing `C3DAnimated`/`ase-deserialization` certificate remains
`linked-blocked` for the separate self-comparison reason documented in
`docs/linkage_certificates.csv`; target 7 does not change that row.

## Confidence

Confidence: Medium-high

Validation: Static Ghidra recovery plus PE vtable probe; not runtime-validated.

Open questions:
- Apply full `C3DAnimated`/OMedia morph structs so adjusted vtable-4 field indexes can be mapped to absolute offsets without colliding with primary property offsets.
- Identify constructor(s) and the helper at `FUN_0040d300` used by the deleting destructor.
- Name the inherited setters called by slots `0x58`, `0x110`, `0x118`, and `0x11c`.
- Confirm the target lookup semantics behind `PickupLink` and `FUN_00474070`.
- Define the material fields at offsets `0x30`, `0x34`, `0x38`, and `0x4c..0x58`.

## Notes

- Evidence: `DumpClass.java C3DAnimated /tmp/decomp_C3DAnimated.md` (`slots=368`, `owned_methods=18`, `offsets=10`).
- Target 7 evidence: `docs/decomp/evidence/c3danimated_target7.md`.
- Extra raw vtable targets `0040e1f0`, `0040d9e0`, `0040da30`, `0040dab0`, `0040db10`, `0040df80`, `0040e3e0`, and `0040d350` are now function-defined in the Ghidra project and documented above.
- String evidence from `/home/scotty/xp-jnbg-original/Neutron.exe`: `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, `PickupLink`, `MEMLOG Anim3D_CreateAnim`, `Anim3D_GetAnim`, and `MEMLOG 2 Anim3D_CreateTexture`.
