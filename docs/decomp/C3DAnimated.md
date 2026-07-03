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

The original side is now L1-backed, and the native port target is the recovered
OMedia animation-record dispatcher, not the existing cutscene pose table. The
port-in-progress is tracked in `docs/c3danimated_dispatch_port_plan.md`; until
runtime wiring lands, the certificate stays `linked-blocked`.

### OMedia runtime resolution

The embedded `OMediaAnim` begins at adjusted `+0x90`. The decisive source is the
OMT 2.5 tree at
`~/omt-src/open-media-toolkit-master/sources/OMTClasses/World/Anim/OMediaAnim.{h,cpp}`,
with `omt_WMilliSec` confirmed as `float` in `OMediaWorldUnits.h:40-41`.

| Adjusted offset / slot | OMT name | Native port meaning |
|---:|---|---|
| `+0x94` | `anim_def` | Applied animation definition / clip pointer. |
| `+0x98` | `current_frame` | Current frame cursor; native uses `cur_frame`, with `-1` for no frame. |
| `+0x9c` | `next_frame` | OMedia interpolation state; out of scope for dispatch certification. |
| `+0xa0` | `current_sequence` | Sequence index; collapsed to one native clip sequence. |
| `+0xa4` | `updatecount` | OMedia internal update counter; not needed by the event dispatch. |
| `+0xa8` | `pause_count` | Pause gate for frame advance. |
| `+0xac` | `play_timebased` | Always forced true by `SetAnim3DByName` / slot 57. |
| `+0xad` | `play_loop` | Completion gate: loops never fire slot 65. |
| `+0xae` | `play_reverse` | Not set by recovered paths; intentionally not ported. |
| `+0xaf` | `play_started` | Timebase priming flag. |
| `+0xb0` | `play_pingpong` | Not set by recovered paths; intentionally not ported. |
| `+0xb4` | `current_frame_tbcount` | Remaining ms for the current frame. |
| vtable `+0xc` | `set_anim_def` | Clip application; resets frame/sequence only when the def changes. |
| vtable `+0x10` | `setcurrentsequence(long, bool restart)` | Sequence select; second arg is the recovered loop/restart flag. |
| vtable `+0x1c` | `getcurrentframe_pos()` | Returns `0` when there is no def/current frame. |
| vtable `+0x24` | `setplay_timebased(bool)` | Forces time-based playback and resets timer phase. |

Consequences for the native port:

1. The last-frame hook is gated by `play_loop`, not by pause. Pause stops
   `OMediaAnim::update_logic` frame advance, but a paused one-shot already at
   its last frame still re-fires slot 65 every update.
2. There is no completion latch. Non-looping clips hold at the last frame;
   `C3DPlayer::OnPlayerAnimEnded` consumes the repeated event by switching
   animation state.
3. `SetAnim3DByName(name, loop_flag)` zeroes `anim_clock` after the ready flags
   pass, even on a lookup miss. Re-selecting the same record preserves the
   frame cursor but resets timer phase; selecting a different record resets the
   frame cursor through the def-change path.

### Lookup-key composition

Earlier prose implied appending a shape suffix. The L1 copy idioms correct that:
the mode-selected global is copied first, then the caller name is appended into
an 80-byte stack key. Mode `0` uses `DAT_004ed3dc`, mode `1` uses
`DAT_004ed3e0`, and all other mode words use `DAT_004f81a8`. The global data
contents are unrecovered, so native exposes them as settable key strings.

For modes `0` and `1`, the visible/current name buffer is pre-copied from the
caller name before lookup, even on a miss. For other modes, the name buffer is
updated only after a successful record lookup.

### Method map

| Original method | Native target | Notes |
|---|---|---|
| `InitAnim3DDatabase` | `animated_dispatch_init_entity` | Collapses OMedia DB creation, builder registration, and default-shape seeding to a lazy per-entity dispatch state with ready flags set. |
| `CreateAnim3DRecord` | `animated_dispatch_create_record` | Stores a native clip pointer instead of importing an `A3dm`; `clip == NULL` preserves the failed-import record shape. |
| `FindAnim3DRecordByName` | `animated_dispatch_find_record` | Case-insensitive linked-list lookup under the two ready flags. |
| `SetAnim3DByName` | `animated_dispatch_set_by_name` | Ready guards, clock reset, key composition, record lookup, sequence select, and clip application. |
| `SelectAnim3DRecordIndex` | `animated_dispatch_select_index` | Writes current index, loop flag, timebase phase reset, and frame-count refresh. |
| `GetCurrentAnim3DRecord` / `GetCurrentAnim3DObject` | `animated_dispatch_current_record` / `animated_dispatch_current_clip` | Read-only accessors for the current record and clip. |
| `UpdateAnimated` dispatch slice | `animated_dispatch_update` | Only the animation-time walk and completion hook; `PickupLink`, `Update3DObject`, and `CanMove` stay with existing native behavior paths. |
| vtable-4 slot 65 base | `NULL` hook | Base no-op at `00472970` becomes a null callback. |
| `C3DPlayer` slot 65 | `animated_dispatch_set_anim_ended_hook` | Future wiring target for the player return-to-STOP/FENCE/LADDER/SPLAT/HIT behavior. |
| `SetAnim3DPaused` | `animated_dispatch_set_paused` | Edge-guarded pause count mirror; unpause resets `play_started`. |
| `ApplyAnimatedEnabledState` / `ApplyAnimatedCollisionVisibleState` | existing `behavior_base.c` gates | Not part of the dispatch module scope. |

### Native state carrier

`Entity` gets one lazily allocated pointer, `struct AnimatedDispatch
*anim_dispatch`; the module owns allocation/freeing. The state mirrors the
original offsets needed by the recovered dispatch path:

| Native field | Original evidence | Purpose |
|---|---:|---|
| `records`, `record_count` | record list / creation count | Linked animation records and id assignment. |
| `AnimatedRecord::name[64]` | record name buffer | Case-insensitive lookup key target. |
| `AnimatedRecord::next` | `+0x40` | Tail-appended linked list. |
| `AnimatedRecord::seq_id` | `+0x44` | Creation id / sequence id. |
| `AnimatedRecord::frame_count` | `+0x48` | Cached completion threshold. |
| `AnimatedRecord::clip` | `+0x4c` | Native stand-in for the imported DB object/def. |
| `ready_a`, `ready_b` | `+0x634/+0x635` | Lookup/select/update guard bytes. |
| `shape_mode` | adjusted `+0x580` | Selects the key lead string and name-buffer side effect. |
| `clock` | adjusted `+0x584` | Per-dispatch animation clock. |
| `current_index` | adjusted `+0x588` | Current record/sequence id. |
| `current_clip` | adjusted `+0x58c` | Applied native clip pointer. |
| `current` | adjusted `+0x62c` | Current animation record. |
| `current_name[64]` | `+0x17a` region | Visible/current name buffer. |
| `play_loop` | adjusted `+0xad` | Last-frame hook gate. |
| `paused` | adjusted `+0x654` mirror / `pause_count` | Stops frame advance only. |
| `play_started`, `tb_count_ms`, `cur_frame` | OMedia timebase fields | Time-based frame walk and phase reset. |
| `on_anim_ended`, `anim_ended_fires` | vtable-4 slot 65 | Hook callback and oracle-visible fire count. |

### Deliberate deviations

1. Native clips are treated as single-sequence defs. This collapses OMedia's
   sequence-vector indirection, positional mapping, and refresh-vs-clamp
   interplay. The shipped-data pass still needs to validate that exported
   `A3dm` defs are single-sequence for this dispatch surface.
2. No OMedia database is ported. The native record stores an `AnimatedClip`
   pointer that represents the imported `A3dm` timing/frame-count data.
3. Frame advance is merged into the dispatch update. The original timebase uses
   float milliseconds (`omt_WMilliSec`), so there is no integer-truncation gap.
   `play_reverse` and `play_pingpong` are not set by recovered paths.
4. Caller buffers are not mutated. The original truncates the caller's record
   name buffer in place on success; native keeps truncation copy-side only.
5. The 80-byte lookup key is bounded with `snprintf`/explicit copies instead of
   preserving a possible original stack overrun.
6. MEMLOG and inherited trace calls are intentionally dropped.

### Scope and certificate status

This port covers dispatch, update, and completion-event logic only. Visual
fidelity, actor mesh selection, and player movement remain on their existing
native-by-eye tracks, and the separate `C3DAnimated`/`ase-deserialization`
certificate remains `linked-blocked` for the self-comparison reason documented
in `docs/linkage_certificates.csv`.

Historical native gap note: current native `behavior_cutscene.c` dispatches
cutscene actor poses through a static `ACTOR_ANIMS[]` alias table; Jimmy maps
aliases to the separate `PlayerAnim` enum and `player_anim.c` advances hardcoded
ASE clips. Native `behavior_animsprite.c` is the separate `C3DAnimatedSprite` /
`3ANI` billboard frame animator, not this OMedia morph-animation record path.
An oracle over those existing native systems would certify a different design,
so the row stays blocked until the recovered dispatcher is wired and certified.

## Confidence

Confidence: Medium-high

Validation: Static Ghidra recovery plus PE vtable probe; not runtime-validated.

Open questions:
- Apply full `C3DAnimated`/OMedia morph structs so adjusted vtable-4 field indexes can be mapped to absolute offsets without colliding with primary property offsets.
- Identify constructor(s) and the helper at `FUN_0040d300` used by the deleting destructor.
- Name the inherited setters called by slots `0x58` and `0x118`; slots `0x110`/`0x11c` are partially resolved through the shape-selection path, but slot `+0x10c` remains unnamed.
- Confirm the target lookup semantics behind `PickupLink` and `FUN_00474070`.
- Define the material fields at offsets `0x30`, `0x34`, `0x38`, and `0x4c..0x58`.
- Validate the single-sequence-per-`A3dm` assumption against shipped exported animation defs.
- Locate the writer for the loader-ready bytes at adjusted `+0x634/+0x635` (`InitAnim3DDatabase` writes a nearby byte at adjusted `+0x570`).
- Recover the slot-48 (`+0xc0`) setter body; current evidence assumes it is the `set_anim_def` application path.

## Notes

- Evidence: `DumpClass.java C3DAnimated /tmp/decomp_C3DAnimated.md` (`slots=368`, `owned_methods=18`, `offsets=10`).
- Target 7 evidence: `docs/decomp/evidence/c3danimated_target7.md`.
- OMT source evidence: `~/omt-src/open-media-toolkit-master/sources/OMTClasses/World/Anim/OMediaAnim.h`, `OMediaAnim.cpp`, `OMediaWorldUnits.h:40-41`, and `OMediaClassStreamer.h:49-55`.
- Extra raw vtable targets `0040e1f0`, `0040d9e0`, `0040da30`, `0040dab0`, `0040db10`, `0040df80`, `0040e3e0`, and `0040d350` are now function-defined in the Ghidra project and documented above.
- String evidence from `/home/scotty/xp-jnbg-original/Neutron.exe`: `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, `PickupLink`, `MEMLOG Anim3D_CreateAnim`, `Anim3D_GetAnim`, and `MEMLOG 2 Anim3D_CreateTexture`.
