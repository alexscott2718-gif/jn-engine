# C3DShrinkRay

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DShrinkRay` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b4340`, `004b4350`, `004b47a0`, `004b47dc`, `004b47f0` |
| Ctor(s) | constructor/factory block `0043fe20`; registers FourCC `3SHR` at `0043feeb` |
| Dtor(s) | scalar deleting destructor at `0043ff40`; cleanup helper `0043ff70`; adjusted destructor thunks at `00440250`, `00440260`, `00440270`, `00440280`, `00440290` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DShrinkRay` is a concrete animated 3D shrink-ray object. It binds `ray.ase`, loads three texture frames (`ray0000..ray0002.png`), and its per-frame override cycles the active texture slot every `0.1` seconds. The executable registers FourCC `3SHR`, but no `3SHR` objects appear in the current `.gam` corpus.

## Runtime-confirmed behavior (game owner, 2026-06-23)

Answers the open question below ("does any contact code use `C3DSHRINKRAY` for shrink effects"): **yes.** Fired at certain AI (Dino, Darwin fish, Humphrey, ...), the ray **shrinks** the target — which then plays `HISHRINK`, scales down, and becomes a small **moving pickup** the player can collect. Those targets derive from `C3DEnemy -> C3DPickupType -> C3DAI` and each registers a `HISHRINK` frame (`dinoshrink`/`darwinshrink`/`plantshrink`/Humphrey's `HISHRINK`). This class only animates the ray object itself; the shrink-on-contact → shrink → pickup transition lives in an unported hit/contact path, and `3SHR` has zero `.gam` placements, so the native port records this and defers the active mechanic. See `src/game/behaviors/behavior_creature.c` and `src/game/behaviors/behavior_humphrey.c`.

## Field Map

Offsets are byte offsets from the primary active `C3DShrinkRay` pointer used by slot-1 methods. Constructor writes are `0xc0` higher in outer allocation space.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4bc` | pointer/texture handle | `primary_texture_or_material` | init slot `00440000`; update `00440180` | Passed to inherited texture/material attach slot with the current frame index. |
| inherited `0x500`, `0x504`, `0x508` | pointer[3] | `ray_material_slots` | init slot `00440000` | Three material/texture records touched directly; each gets RGBA-like values `(1, 1, 1, 0.5)` at suboffsets `0x4c..0x58`. |
| `0x5fc` | uint16 | `texture_frame_index` | ctor `0043fe20`; update `00440180` | Current frame slot. Update increments it and wraps after frame `2`. |
| `0x600` | float | `texture_frame_timer` | init slot `00440000`; update `00440180` | Accumulates `dt`; when it reaches `0.1`, update advances the texture frame and resets the timer. |
| `0x604` | byte/bool | `lazy_init_latch` | ctor `0043fe20`; update `00440180` | Constructor clears it; first update sets it and calls slot 7 once before normal frame timing continues. |
| inherited active `0x8` / outer `0xc8` | byte/bool | `animated_flag_0x8` | ctor `0043fe20` | Constructor sets this inherited byte to `1`; exact inherited meaning unresolved. |
| outer `0x6cc` | subobject/tail | class streamer tail | constructor/destructor scaffolding | Tail cleanup/streamer allocation handled around construction and destruction; not gameplay tuning. |

## Vtable Methods

`DumpClass` decompiled slot 7 only. Slot 241 is a direct vtable target in the ShrinkRay address range and is included from local disassembly.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0043fe20` | `CtorShrinkRay3SHR` | Constructs `C3DAnimated`, installs ShrinkRay vtables, sets runtime type `C3DSHRINKRAY`, registers the misspelled constructor string `C3DShringRay()`, runs `InitObjectShrinkRay`, applies inherited scalar `0.3`, registers `3SHR`, clears inherited update/render toggles, initializes texture frame state, and sets one inherited animated byte. | non-trivial |
| 7 | `00440000` | `InitObjectShrinkRay` | Runs `C3DAnimated::InitObject`, initializes the adjusted 3D object, registers `HIRAY -> ray.ase`, loads texture slots `0..2` from `ray0000..ray0002.png`, applies material mode `(slot, 6, 1)` for each frame, attaches frame `0`, sets three material records to `(1, 1, 1, 0.5)`, applies inherited scalars `0.5` and `40.0`, selects `HIRAY`, clears `texture_frame_timer`, and finalizes. | non-trivial |
| 8 | `0040e670` | `C3DAnimated::UnInitObjectAnimated` | Inherited animated uninit. | inherited |
| 10 | `004587f0` | `CLocalGameObject` reset/base slot | Inherited base behavior. | inherited |
| 241 | `00440180` | `UpdateShrinkRayTexture` | Raw update slot. Runs `C3DAnimated::UpdateAnimated(dt)`, lazily calls slot 7 once if `lazy_init_latch` is clear, accumulates `texture_frame_timer`, and every `0.1` seconds advances `texture_frame_index`, reapplies scalar `0.5`, attaches the current texture frame, and resets the timer. | raw block |
| 242 | `0040d3a0` | `C3DAnimated` inherited slot | Inherited animated behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated` level/visibility gate | Inherited animated behavior. | inherited |
| vtable 3 slot 2 | `0043ff40` | scalar deleting destructor | Adjusts from the secondary subobject pointer, calls cleanup helper `0043ff70`, destroys the tail subobject at outer `0x6cc`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0043ff70` | `CleanupShrinkRay` | Reinstalls ShrinkRay vtables, logs `Deleting the shrinkray`, and calls the `C3DAnimated` cleanup helper at `0040d300`. | non-trivial |

## Runtime Behavior

```c
C3DShrinkRay::CtorShrinkRay3SHR():
    C3DAnimated::Ctor()
    install_shrinkray_vtables()
    set_runtime_type("C3DSHRINKRAY")
    register_class_string("C3DShringRay()") // executable typo
    InitObjectShrinkRay()
    inherited_scalar_0x114(0.3)
    register_fourcc("3SHR")
    clear inherited update/render toggles
    texture_frame_index = 0
    lazy_init_latch = false
    inherited_active_byte_0x8 = true
```

```c
C3DShrinkRay::InitObjectShrinkRay():
    C3DAnimated::InitObject()
    inherited_init_adjusted_shape()
    register_anim("HIRAY", "ray.ase")
    for slot, png in [(0, "ray0000.png"), (1, "ray0001.png"), (2, "ray0002.png")]:
        load_texture(png, slot)
        set_material_mode(slot, 6, 1)
    attach_texture(primary_texture_or_material, 0)
    for rec in ray_material_slots[0..2]:
        rec.color_or_rgba = (1.0, 1.0, 1.0, 0.5)
    inherited_scalar_0x114(0.5)
    inherited_scalar_0x110(40.0)
    select_animation("HIRAY")
    texture_frame_timer = 0.0f
```

```c
C3DShrinkRay::UpdateShrinkRayTexture(dt):
    C3DAnimated::UpdateAnimated(dt)
    if !lazy_init_latch:
        lazy_init_latch = true
        InitObjectShrinkRay()

    texture_frame_timer += dt
    if texture_frame_timer < 0.1:
        return

    texture_frame_index += 1
    inherited_scalar_0x114(0.5)
    if texture_frame_index > 2:
        texture_frame_index = 0
    attach_texture(primary_texture_or_material, texture_frame_index)
    texture_frame_timer = 0.0f
```

## Constants And Wiring

`C3DShrinkRay` registers FourCC `3SHR`, but `assets/gam/*.gam` currently contain no `3SHR` objects. There are therefore no serialized per-instance ShrinkRay properties in `docs/gam_schema.md`; the known tuning below comes from constructor/init/update code.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3SHR` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0043fe20`; backfilled in `docs/_gam_classids.tsv` as `C3DShrinkRay()`. |
| `texture_frame_index` | uint16 | `0x5fc` | `0..2` | Advanced by slot 241 and passed into inherited texture attach slot. |
| `texture_frame_timer` | float | `0x600` | `0..0.1` | Drives texture-frame cadence. |
| `lazy_init_latch` | byte/bool | `0x604` | `0` then `1` | Gates one extra slot-7 call on first update. |
| frame threshold | double | `.rdata:0049cfb8` | `0.1` | Update advances texture frame when timer reaches this value. |
| inherited scalar | float | inherited slot `0x114` | constructor `0.3`; init/update `0.5` | Exact inherited meaning unresolved; called during setup and frame update. |
| inherited scalar | float | inherited slot `0x110` | init `40.0` | Scale/radius-like inherited setter; exact inherited name still open. |
| material mode tuple | ints | n/a | `(slot, 6, 1)` for slots `0..2` | Applied after loading each frame texture. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DSHRINKRAY` | Concrete object/type string set by constructor. | string `.data:004f0b2c`; constructor `0043fe20` |
| `C3DShringRay()` | Misspelled constructor/class string used by executable. | string `.data:004f0b1c`; constructor `0043fe20` |
| `C3DShrinkRay::InitObject()` | Slot-7 trace string. | string `.data:004f09fc`; init slot `00440000` |
| `HIRAY` | Animation/shape alias and selected animation. | string `.data:004ee33c`; init slot `00440000` |
| `Deleting the shrinkray` | Destructor/cleanup log string. | string `.data:004f0b3c`; cleanup helper `0043ff70` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `ray.ase` | init slot `00440000`; local asset `assets/ase/ray.ASE` | Bound to `HIRAY`. |
| texture frame | `ray0000.png` | init slot `00440000`; local asset `assets/png/ray0000.png` | Texture slot/frame `0`. |
| texture frame | `ray0001.png` | init slot `00440000`; local asset `assets/png/ray0001.png` | Texture slot/frame `1`. |
| texture frame | `ray0002.png` | init slot `00440000`; local asset `assets/png/ray0002.png` | Texture slot/frame `2`. |
| unused nearby asset | `ray0003.png` | asset scan only | Present on disk but no direct reference found in `C3DShrinkRay` constructor/init/update. |
| GLB conversion | `assets/glb/ase/ray.glb` | asset scan only | Derived local asset, not referenced by executable. |
| sound candidate | `shrinkray.wav` | asset scan only | Sound exists, but this class body does not call a confirmed sound/effect id. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, string/asset scans, timer constant decode, and class-id scan backfill only; not runtime-validated.

Open questions:
- Name inherited material/texture slots at vtable offsets `0xd8`, `0xe0`, `0xf0`, `0xf4`, `0xfc`, `0x110`, and `0x114`.
- Confirm why the constructor clears `lazy_init_latch` after calling `InitObjectShrinkRay`, causing first update to call slot 7 again.
- Runtime-check whether `ray0003.png` is truly unused or selected by another class/script path.
- Identify whether any collision/contact code elsewhere uses `C3DSHRINKRAY` for shrink effects; this class only animates the ray object itself.

## Notes

- Evidence: `DumpClass.java C3DShrinkRay /tmp/decomp_C3DShrinkRay.md` (`slots=368`, `owned_methods=1`, `offsets=0`), local objdump window `0043fe20..004402a0`, string scans around `004f09fc..004f0b78`, asset-file checks for `ray` files, and `.gam` byte scans showing no `3SHR` rows.
- `docs/_gam_classids.tsv` was backfilled for `3SHR -> C3DShrinkRay()` from RTTI/string/vtable evidence, then `python3 tools/gam_schema.py` was rerun. `docs/gam_schema.md` did not change because the current level corpus has no `3SHR` instances.
