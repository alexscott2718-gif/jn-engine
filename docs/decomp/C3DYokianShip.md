# C3DYokianShip

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokianShip` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c1370`, `004c1380`, `004c17d0`, `004c180c`, `004c1820` |
| Ctor(s) | constructor/factory block `FUN_0044b7d0`; registers FourCC `3YSH` at `0044b896` |
| Dtor(s) | adjusted scalar deleting destructor at `0044bae0`; cleanup/vtable reset helper at `0044bb10` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokianShip` is the concrete `3YSH` placeable Yokian ship AI object. It inherits normal `C3DAI` patrol/targeting and animated-object transform behavior, loads the `yokianship.omt` database, and optionally creates a `C3DTractorBeam` plus a `C3DAbductee` helper pair that it moves and toggles during update. `3YSH` is also registered by `C3DYokianShield`, but current `.gam` rows are ship-tagged AI rows and should be treated as this class.

## Field Map

Offsets below are byte offsets from the outer `C3DYokianShip` allocation pointer unless marked active. The raw update slot is entered with the active AI pointer, so active/outer aliases are listed for helper fields.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3YSH`; cleanup `0044bb10`; raw update `0044bbc0` | Rows use `"none"` and `"scene"`; update also checks the global `"SCENE"` task/state value for gating. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3YSH` | Current rows use `100.0..2500.0`; consumed by inherited AI target/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3YSH` | Current rows include `"SHIP1PT"`, `"SHIP2PT"`, `"SHIP3PT"`, `"SHIP6PT"`, and `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3YSH` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3YSH` | Current rows use `90`. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0044b7d0`; `.gam` `3YSH` | Constructor default is `3`; all current rows also use `3`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3YSH` | Current rows use `1500.0`. |
| `0x568` | pointer/handle | `yokianship_omt_database` | ctor `0044b7d0` | Result of `FUN_0046a910("yokianship.omt")`, then consumed by the OMT/shape binding path. |
| `0x634` | byte/bool | `ship_runtime_flag_0` | ctor `0044b7d0` | Cleared after OMT load; exact use not found in ship-owned raw blocks. |
| active `0x604` / outer `0x6c4` | float | `ship_motion_tuning_700` | ctor `0044b7d0` | Constructor writes `700.0`; exact inherited C3DAI/animated meaning still unresolved. |
| active `0x608` / outer `0x6c8` | int | `ship_mode_3` | ctor `0044b7d0` | Constructor writes `3`; likely inherited AI/animation mode. |
| `0x7d0`, `0x7f8`, `0x820`, `0x848`, `0x870`, `0x898` | char buffers/strings | `ship_animation_names` | ctor `0044b7d0` | Constructor copies `"none"` into six animation/name slots. |
| `0x8d4` | float | `ship_blend_or_scale_0_3` | ctor `0044b7d0` | Constructor writes `0.3`; not directly consumed by the ship raw update because that update sees the active pointer. |
| active `0x8d4` / outer `0x994` | pointer | `tractor_beam_child` | ctor `0044b7d0`; raw update `0044bbc0` | Optional adjusted `C3DTractorBeam` child. Not allocated on `LV1D` or `LV1F`. |
| active `0x8d8` / outer `0x998` | byte/bool | `child_visibility_phase` | ctor `0044b7d0`; raw update `0044bbc0` | Cleared by constructor; toggled when `child_phase_timer` expires. Drives helper show/hide alternation. |
| active `0x8dc` / outer `0x99c` | float | `child_phase_timer` | ctor `0044b7d0`; raw update `0044bbc0` | Cleared by constructor; reset to `5.0` when the helper visibility phase toggles. |
| active `0x8e0` / outer `0x9a0` | pointer | `abductee_child` | ctor `0044b7d0`; raw update `0044bbc0` | Optional adjusted `C3DAbductee` child paired with the tractor beam. Not allocated on `LV1D` or `LV1F`. |

No ship-specific serialized fields beyond inherited common/AI properties were observed.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044b7d0` | `CtorYokianShip3YSH` | Constructs `C3DAI`, installs five adjusted ship vtables, registers `C3DYOKIANSHIP` strings and FourCC `3YSH`, seeds animation/default AI fields, applies inherited tuning constants, loads `yokianship.omt`, creates tractor-beam/abductee children outside `LV1D`/`LV1F`, and clears helper phase state. | non-trivial |
| 7 | `00407ee0` | `C3DAI::InitObjectAI` | Inherited AI init/property registration. | inherited |
| 8 | `0044bbb0` | `UnInitYokianShipThunk` | Raw thunk to inherited animated uninit at `0040e670`. | raw thunk |
| 16 | `0040a3c0` | `C3DAI` collision/AI reaction slot | Inherited AI behavior. | inherited |
| 241 | `0044bbc0` | `UpdateYokianShipHelpers` | Raw update slot. Runs inherited AI/animated update, maintains helper transforms, skips helper logic on `LV1D`/`LV1F` or when level/task gates fail, decrements the helper phase timer, and alternates visibility of `tractor_beam_child` and `abductee_child`. | raw block |
| 259 | `00409480` | `C3DAI::PostLoadAI` | Inherited AI post-load. | inherited |
| 260 | `0040a6b0` | `C3DAI` post-load/update helper | Inherited AI behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated` level/visibility gate | Inherited animated behavior. | inherited |
| vtable 3 slot 2 | `0044bae0` | scalar deleting destructor | Runs the ship cleanup/vtable reset helper, destroys the adjusted streamer/string subobject at outer `0x9a8`, and frees the allocation when requested. | non-trivial |

## Runtime Behavior

```c
C3DYokianShip::CtorYokianShip3YSH():
    C3DAI::Ctor()
    install_ship_vtables()
    register_strings("C3DYOKIANSHIP()", "C3DYOKIANSHIP")
    C3DAI::InitObjectAI()
    C3DObject::setup()
    register_fourcc("3YSH")

    ship_motion_tuning_700 = 700.0f
    ship_mode_3 = 3
    AIState = 3
    copy "none" into six animation/name slots
    apply inherited 100.0 tuning to two adjusted slots
    ship_blend_or_scale_0_3 = 0.3f

    yokianship_omt_database = lookup_omt("yokianship.omt")
    bind_omt_shape(yokianship_omt_database)
    show(true)

    tractor_beam_child = NULL
    abductee_child = NULL
    if current_level not in {"LV1D", "LV1F"}:
        tractor_beam_child = new C3DTractorBeam(1)->active
        hide/disable tractor_beam_child
        abductee_child = new C3DAbductee(1)->active

    child_visibility_phase = false
    child_phase_timer = 0.0f
```

```c
C3DYokianShip::UpdateYokianShipHelpers(dt):
    C3DAI::Update(dt)

    compute ship transform anchors through inherited OMedia slots
    if current_level in {"LV1D", "LV1F"}:
        return
    if RequiredLevel gate fails against current SCENE state:
        return
    if tractor_beam_child == NULL:
        return

    copy ship transform/orientation to tractor_beam_child
    copy offset transform/orientation to abductee_child

    child_phase_timer -= dt
    if child_visibility_phase:
        animate abductee offset while phase is active

    if child_phase_timer < 0.0f:
        child_phase_timer = 5.0f
        child_visibility_phase = !child_visibility_phase
        if child_visibility_phase:
            hide tractor_beam_child
            hide abductee_child
        else:
            show tractor_beam_child
            show abductee_child
```

The helper update uses inherited OMedia transform slots heavily. The pseudocode above preserves the state transitions and child ownership without over-naming the still-raw matrix/vector helpers.

## Constants And Wiring

### `.gam` Placeable Properties

`3YSH` appears eleven times across the level `.gam` files. These rows are `C3DYokianShip` rows despite the duplicate `3YSH` shield helper registrar.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DYOKIANSHIP"`, `"SHIP1"`, `"sh2"`, `"yokship1"`, ... | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861492040` | FourCC/object id value for `3YSH`. |
| `PositionX` | float | inherited | `-3340..19100` | Base placement transform. |
| `PositionY` | float | inherited | `2030..26600` | Base placement transform. |
| `PositionZ` | float | inherited | `-21700..80500` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..270` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"none"`, `"scene"` | Raw update checks global `"SCENE"` state when helper gates are active. |
| `Debug` | int | inherited | `0` | Base debug flag; no ship-owned branch found. |
| `RequiredLevel` | int | inherited | `-1..380` | Inherited/ship raw update gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `HasCollision` | int | inherited | `-1..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Ship-specific code does not consume it directly. |
| `PatrolPoint` | str | inherited `0x648` | `"SHIP1PT"`, `"SHIP2PT"`, `"SHIP3PT"`, `"SHIP6PT"`, ... | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `100..2500` | Compared by inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `90` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited active `0x87c` | `3` | Constructor and current rows both seed state `3`. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3YSH` | Concrete placeable class id for Yokian Ship. | ctor `0044b7d0`; `push 0x33595348` at `0044b896` |
| `C3DYOKIANSHIP` | Concrete object/type string. | string `.data:004f08e0`; constructor string path |
| `C3DYOKIANSHIP()` | Concrete class string. | string `.data:004f08d0`; constructor string path |
| `yokianship.omt` | OMT database for the ship model. | string `.data:004f18d0`; ctor lookup at `0044b969` |
| `C3DTractorBeam` | Optional helper child class. | string `.data:004f18c0`; ctor allocates `004462c0` outside `LV1D`/`LV1F` |
| `C3DAbductee` | Optional helper child class. | string `.data:004f18b4`; ctor allocates `004076e0` outside `LV1D`/`LV1F` |
| `LV1D`, `LV1F` | Levels where helper children are not allocated/updated. | current-level comparisons at `0044b9c4` and `0044bc49` |
| `none` | Default animation/name string copied into six slots. | string `.data:004eca6c`; ctor string copies |
| `SCENE` | Task/state gate string used by raw update. | string `.data:004ed220`; raw update |
| `700.0` | Constructor tuning value at active `0x604`. | immediate `0x442f0000`; ctor `0044b8b5` |
| `100.0` | Two inherited tuning calls during construction. | immediate `0x42c80000`; ctor `0044b92f`, `0044b93b` |
| `0.3` | Constructor tuning value at outer `0x8d4`. | immediate `0x3e99999a`; ctor `0044b952` |
| `5.0` | Helper phase timer reset. | immediate `0x40a00000`; raw update |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `yokianship.omt` | ctor `0044b7d0`; parsed metadata `assets/parsed/yokianship/yokianship.json` | Original source path in metadata is `/home/scotty/xp-jnbg-original/omt/yokianship.omt`. |
| texture/canvas | `a-1ship` | parsed `yokianship.omt` image index `0` | `32x32` 16-bit image. |
| texture/canvas | `a-0ship` | parsed `yokianship.omt` image index `1` | `32x32` 16-bit image. |
| texture/canvas | `yokianship` | parsed `yokianship.omt` image index `2` | `256x256` 16-bit ship texture. |
| helper class | `C3DTractorBeam` | ctor `0044b7d0` | Spawned helper child on most levels; separate wave 6 spec pending. |
| helper class | `C3DAbductee` | ctor `0044b7d0`; `docs/decomp/C3DAbductee.md` | Spawned helper child on most levels. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, local `yokianship.omt` metadata, shield duplicate cross-check, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw ship targets `0044bbc0`, `0044bae0`, and `0044bb10`.
- Name the inherited transform/matrix slots used by `UpdateYokianShipHelpers`.
- Runtime-check why `LV1D` and `LV1F` disable tractor-beam/abductee child allocation.
- Confirm the exact semantics of constructor tuning values `700.0`, `100.0`, and `0.3`.
- Determine whether `child_visibility_phase` true means visually hidden or active after the inherited show/hide slot names are finalized.

## Notes

- Evidence: `DumpClass.java C3DYokianShip /tmp/decomp_C3DYokianShip.md` (`slots=391`, `owned_methods=0`, `offsets=0`), local objdump window over `0044b7d0..0044bee0`, string scans around `004f08d0` and `004f18b0`, parsed `yokianship.omt` metadata, `.gam` schema for `3YSH`, and `C3DYokianShield` duplicate registrar cross-check.
- `3YSH -> C3DYokianShip` was backfilled as the `.gam`-facing class in `docs/_gam_classids.tsv`, and `tools/gam_schema.py` now carries an explicit duplicate-FourCC override so regenerated schema rows display the ship rather than the runtime shield helper.
