# C3DPickupItem

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPickupItem` |
| Base chain | `CPickupType -> C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004ae538`, `004ae548`, `004ae998`, `004ae9ac` |
| Ctor(s) | FourCC factory/constructor at `004358b0` for `3PIC`; adjusted constructor helper at `00435ae0` |
| Dtor(s) | adjusted scalar deleting destructor at `00435ab0`; destructor thunks at `00436940`, `00436950`, `00436960`, `00436970` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the active `C3DSprite`/gameplay subobject pointer used by the property registrar, matching the existing `C3DSprite`, `C3DTriggerType`, and `CPickupType` specs. The outer object constructor stores several of the same fields at `outer + 0xc8 + offset`; raw vtable 3 helper `00436830` uses the adjusted pointer form, so its disassembly shows `+0x6d0`, `+0x6f0`, and `+0x6f4` for the primary fields `0x608`, `0x628`, and `0x62c`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; `.gam` `3PIC` | Canvas sprite size. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; `.gam` `3PIC`; raw slot `00436200` | Sprite/canvas index. `0x6a` is a special sentinel that skips normal visibility refresh. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; `.gam` `3PIC`; raw slot `00436200` | OMT sprite database, normally `sprites.omt`. |
| inherited `0x520` | char buffer/string | `ToggleObject` | `C3DTriggerType`; `.gam` `3PIC`; slot 16 | Optional object tag activated after pickup collection. |
| inherited `0x584` | int | `Toggle` | `C3DTriggerType`; `.gam` `3PIC`; slot 16 | State argument passed to `ToggleObject` and `ActivateObject` targets through vtable offset `0x428`. |
| inherited `0x588` | char buffer/string | `NextTrigger` | `C3DTriggerType`; `.gam` `3PIC`; slot 16 | Follow-up trigger tag fired after collection. |
| inherited `0x5ec` | int | `FadeType` | `C3DTriggerType`; `.gam` `3PIC` | Registered inherited trigger fade mode. |
| inherited `0x5f0` | float | `FadeTime` | `C3DTriggerType`; `.gam` `3PIC` | Registered inherited trigger fade duration. |
| inherited `0x5f4` | int | `PickupIndex` | `CPickupType`; `.gam` `3PIC`; slots 10/16/259/266 | Index into global pickup state table `DAT_004f8438`. |
| inherited `0x5f8` | int | `PIC_NUMBER` | `CPickupType`; `.gam` `3PIC`; slots 16/259 | Picture/inventory id awarded by this pickup; raw slot `00436200` may derive it from selected `SpriteIndex`. |
| inherited `0x5fc` | int | `RequiredLevel` | `CPickupType`; `.gam` `3PIC` | Registered progress gate. |
| inherited `0x600` | int | `ExactLevel` | `CPickupType`; `.gam` `3PIC` | Registered exact progress gate. |
| `0x604` | int | `SoundIndex` | property registration; slots 16/241 | Pickup or ambient sound id. |
| `0x608` | int | `NeedMoreSound` | property registration; vtable 3 slot 54 | Sound played when `RequiredPicNum`/`ReqPicNumAmount` cannot be satisfied. |
| `0x60c` | int | `TimesToTrigger` | property registration; slot 16 | Maximum number of scoring/sound trigger uses; `-1` means unlimited. |
| `0x610` | int | `trigger_count` | slot 16; constructor default | Runtime use count compared against `TimesToTrigger`. |
| `0x614` | int | `IsAmbient` | property registration; slots 16/241 | Nonzero turns the pickup sound into a persistent ambient sound path. |
| `0x618` | int/sound handle | `ambient_sound_handle` | slots 16/241; constructor default `-1` | Active ambient sound handle stopped/replaced by sound helpers. |
| `0x61c` | byte/bool | `ambient_started` | raw slot `00436550`; constructor default false | Guards one-time ambient sound startup. |
| `0x620` | int | `PointValue` | property registration; slot 16 | Score value awarded through `FUN_0042adc0`. |
| `0x624` | int | `PickedUpIndex` | property registration; slots 16/259 | Replacement sprite index used when the pickup state becomes `2`; `-1` means hide/mark collected. |
| `0x628` | int | `RequiredPicNum` | property registration; vtable 3 slot 54 | Required picture/inventory id; `-1` disables the requirement. |
| `0x62c` | int | `ReqPicNumAmount` | property registration; vtable 3 slot 54 | Required amount consumed when `RequiredPicNum` passes. |
| `0x630` | int | `InitallyActive` | property registration; raw slot `00436200` | Initial active state; spelling matches executable and `.gam`. |
| `0x634` | char buffer/string | `ActivateObject` | property registration; slot 16 | Optional object tag activated after collection. |
| `0x69c` | int | `PassThru` | property registration; constructor default `1` | Serialized pickup collision/pass-through flag; consumer not yet isolated. |
| `0x6a0` | int | `ShowArrow` | property registration; constructor default `1` | Serialized arrow/indicator flag; consumer not yet isolated. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `004358b0` | `CtorPickupItem3PIC` | Constructs the `CPickupType` base, installs all four adjusted vftables, registers class string `C3DPICKUPITEM()`, binds FourCC `3PIC`, initializes defaults (`SpriteIndex=8`, `SoundIndex=-1`, `TimesToTrigger=-1`, `RequiredPicNum=-1`, `ReqPicNumAmount=1`, `PassThru=1`, `ShowArrow=1`), and calls `InitObjectPickupItem`. | non-trivial |
| 7 | `00435b80` | `InitObjectPickupItem` | Runs `CPickupType::InitObject`, then registers `SoundIndex`, `NeedMoreSound`, `TimesToTrigger`, `Radius`, `IsAmbient`, `PointValue`, `PickedUpIndex`, `RequiredPicNum`, `ReqPicNumAmount`, `InitallyActive`, `ActivateObject`, `PassThru`, and `ShowArrow`. | non-trivial |
| 10 | `00435b20` | `ResetPickupItemVisibility` | Runs `CLocalGameObject` reset, sets state through slot `0x1b0`, then checks `DAT_004f8438[PickupIndex]`. If the pickup is uncollected and has a normal sprite, marks the adjusted canvas dirty/visible through the inherited slot at offset `0x58`. | non-trivial |
| 16 | `00435ce0` | `HandlePickupCollection` | Collision/activation entry. Only handles the global player object, with a special `C3DGODDARD`/`SEWERPART`/`NextTrigger` early-out. For valid pickup indices it gates on collected state and the required-picture predicate, updates `DAT_004f8438`, hides or swaps the sprite to `PickedUpIndex`, fires `ActivateObject`, `ToggleObject`, and `NextTrigger`, awards picture ids and score, and plays/stops pickup or ambient sounds. | non-trivial |
| 241 | `00436550` | `UpdateAmbientPickupSound` | Raw vtable target not named in Ghidra. Calls base update helper `00463090`, then if `IsAmbient` is set and `ambient_started` is false, starts/restarts the looping or positional sound for `SoundIndex` and stores the returned handle. | non-trivial |
| 259 | `00436200` | `PostLoadPickupItem` | Raw vtable target not named in Ghidra. Runs `CPickupType::LoadPickupSpriteAndState`, logs pickup index/position, applies pickup table state, optionally swaps to `PickedUpIndex`, applies `InitallyActive`, and contains a `SpriteIndex` switch that rewrites `PIC_NUMBER` for several sprite ids. | non-trivial |
| 266 | `004360b0` | `SetPickupItemState` | State setter. State `0` shows/enables the pickup and refreshes dependent state; state `1` clears `DAT_004f8438[PickupIndex]` when needed and refreshes the normal canvas path before calling a follow-up state slot. | non-trivial |
| vtable 3 slot 54 | `00436830` | `CheckRequiredPicAndConsume` | Adjusted-pointer helper used by the collection path. If `RequiredPicNum == -1`, returns true. Otherwise checks the picture/inventory bit/count, consumes `ReqPicNumAmount` on success, clears the picture flag when the count reaches zero, or plays `NeedMoreSound` and returns false. | non-trivial |

## Per-Frame Behavior

`C3DPickupItem` is mostly event-driven, but it owns an ambient-sound update slot in addition to pickup collection:

```c
C3DPickupItem::PostLoadPickupItem():
    CPickupType::LoadPickupSpriteAndState()
    if 0 <= PickupIndex < MAX_PICKUPS:
        if DAT_004f8438[PickupIndex] == 0:
            show_canvas_if_normal_sprite()
        elif DAT_004f8438[PickupIndex] == 2 and PickedUpIndex != -1:
            reload_canvas(SpriteDatabase, PickedUpIndex, SpriteSize)
    if InitallyActive == 0:
        set_state_inactive()
    maybe_derive_PIC_NUMBER_from_SpriteIndex()

C3DPickupItem::HandlePickupCollection(toucher):
    if toucher != global_player:
        return
    if !CheckRequiredPicAndConsume():
        return
    if PickupIndex > 0 and DAT_004f8438[PickupIndex] != 0:
        return
    update_DAT_004f8438_and_visibility_or_replacement_sprite()
    fire_tag(ActivateObject, Toggle)
    fire_tag(ToggleObject, Toggle)
    award_pic_number_and_score()
    fire_next_trigger(NextTrigger)
    play_or_stop_pickup_sound()

C3DPickupItem::UpdateAmbientPickupSound(dt):
    base_update(dt)
    if IsAmbient and !ambient_started and SoundIndex != -1:
        stop(ambient_sound_handle)
        ambient_sound_handle = play_ambient_sound(SoundIndex)
        ambient_started = true
```

The collection path also handles `PickupIndex == 0` as a special non-table pickup: it can award `PIC_NUMBER`, fire activation targets, and hide/disable itself without writing the normal pickup state table.

## Constants And Wiring

`C3DPickupItem` maps to placeable FourCC `3PIC` (`FUN_004358b0`). The current corpus has 383 `3PIC` instances.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | 383 samples including `"APPLEPIE"`, `"BUBBLEPICKUP"`, `"C3DPICKUPITEM"`, `"NEST2"` | Used by trigger lookup and logging. |
| `SpriteSize` | int (`6`) | inherited `0x4b4` | `20..400` | Canvas scale for the pickup sprite. |
| `SpriteDatabase` | str (`1`) | inherited `0x4bc` | all `"sprites.omt"` | Loaded by sprite canvas setup and replacement-sprite path. |
| `SpriteIndex` | int (`6`) | inherited `0x4b8` | `8..189` | Initial icon; raw slot `00436200` can also derive `PIC_NUMBER` from selected values. |
| `Toggle`, `ToggleObject`, `NextTrigger`, `FadeType`, `FadeTime` | inherited trigger group | inherited | `Toggle=-1..1`, `ToggleObject` examples `"applepie"`, `"applepie2"`, `"applepie3"`, `"c3dkitty"`, `NextTrigger` examples `"2space1"`, `"balloons"`, `"dino1cam"`, `"egg1"` | Activation and follow-up trigger wiring after collection. |
| `PickupIndex`, `PIC_NUMBER`, `RequiredLevel`, `ExactLevel` | inherited pickup group | inherited `0x5f4..0x600` | `PickupIndex=203..3812`, `PIC_NUMBER=-1..72`, `RequiredLevel=-1..470`, `ExactLevel=-1..170` | Global pickup state, picture award, and level/progress gate data. |
| `SoundIndex` | int (`6`) | `0x604` | `-1..248` | Collection sound or ambient sound source. |
| `NeedMoreSound` | int (`6`) | `0x608` | `-1..178` | Played when required picture count is missing. |
| `TimesToTrigger` | int (`6`) | `0x60c` | `-1..151` | Limits scoring/sound trigger repeats. |
| `Radius` | float (`3`) | inherited/outer `0x34` registration | `1..1000` | Registered collision/activation radius; exact inherited field owner still needs struct cleanup. |
| `IsAmbient` | int (`6`) | `0x614` | all `0` in current rows | Selects ambient sound behavior; no current `3PIC` row enables it. |
| `PointValue` | int (`6`) | `0x620` | `-1..1000` | Score award through `FUN_0042adc0`. |
| `PickedUpIndex` | int (`6`) | `0x624` | `-1..171` | Replacement sprite after collection state `2`; `-1` hides/marks collected. |
| `RequiredPicNum` | int (`6`) | `0x628` | `-1..27` | Picture/inventory requirement. |
| `ReqPicNumAmount` | int (`6`) | `0x62c` | `-1..3` | Amount consumed on requirement success. |
| `InitallyActive` | int (`6`) | `0x630` | `0..1` | Initial active state; spelling matches `.gam`. |
| `ActivateObject` | str (`1`) | `0x634` | examples `"anewflurp"`, `"cand"`, `"cbar"`, `"cjar"` | Optional object fired after collection. |
| `PassThru` | int (`6`) | `0x69c` | `-1..1` | Serialized pass-through behavior; consumer still open. |
| `ShowArrow` | int (`6`) | `0x6a0` | `-1..1` | Serialized arrow/indicator behavior; consumer still open. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | `.gam` `3PIC`; `SpriteDatabase` | Main pickup icon database. |
| sprite index | `SpriteIndex`, `PickedUpIndex` | `.gam` `3PIC`; raw slot `00436200` | Initial and replacement canvas ids. |
| pickup state table | `DAT_004f8438` | slots 10/16/259/266 | Shared collected/replacement state table indexed by `PickupIndex`. |
| picture/inventory service | `FUN_004038c0`, `FUN_00403950`, `FUN_00403910`, `FUN_004061b0`, `FUN_004061c0`, `FUN_004061d0` | slots 16 and vtable 3 slot 54 | Sets/clears picture flags, reads/updates counts, and records pickup-picture linkage. |
| sound service | `FUN_00458980`, `FUN_004589c0`, `FUN_00458a00`, `FUN_0047d890`, global handle `DAT_004eefd8` | slots 16/241/vtable 3 slot 54 | Plays one-shot, missing-requirement, and ambient sounds while avoiding overlap through the global handle. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions and names for raw vtable targets `00436200` and `00436550`, then re-run `DumpClass` so these methods appear with ownership and cleaner decompilation.
- Name the global player pointer at `DAT_005099e4` and confirm whether non-player pickups can ever call slot 16.
- Name the picture/inventory helpers around `FUN_004038c0` and `FUN_004061b0` with evidence from UI/inventory classes.
- Confirm the exact consumers of `PassThru` and `ShowArrow`; they are registered and defaulted here but not consumed by the owned methods examined.
- Resolve the inherited owner of the registered `Radius` field; current registration uses seed offset `0xd` but the consuming collision code is outside this class.

## Notes

- Evidence: `DumpClass.java C3DPickupItem /tmp/decomp_C3DPickupItem.md` (`slots=337`, `owned_methods=5`, `offsets=17`), plus `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` for raw slots `00436200`, `00436550`, and vtable 3 slot `00436830`.
- String-table evidence around `0x4f034c..0x4f03c8` names `ShowArrow`, `PassThru`, `InitallyActive`, `ReqPicNumAmount`, `RequiredPicNum`, `PickedUpIndex`, `PointValue`, `IsAmbient`, and `NeedMoreSound`.
- Preserve the misspelling `InitallyActive`; it appears in both the executable string and `.gam` schema.

## Native Linkage (linked-parity branch)

Aspect: **`collection`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`.
Oracle: `tools/linkage_oracles/C3DPickupItem.py`.

Opened 2026-08-20 by the picture-flag economy port
(`docs/picture_flag_wiring_plan.md`, phases 2–4). The 2026-07-02 investigation
below is preserved because it explains the shape of the gap that was closed.

### What is ported

`src/game/behaviors/behavior_item.c`'s `vt_item` (`item_on_trigger` /
`item_on_spawn`) over `behavior_pickup_core.c` and the `gamestate` picture
store now follows `HandlePickupCollection` (`00435ce0`) in its recovered order:

1. `CheckRequiredPicAndConsume` (vtable 3 slot 54, `00436830`) — consume
   `ReqPicNumAmount` of `RequiredPicNum`, or play `NeedMoreSound` and refuse.
   It runs **before** the collected-state check, as the recovered body does.
2. the `DAT_004f8438` collected-state test and write, keyed on
   `(level, PickupIndex)` — 22 indices collide across levels, so an
   index-keyed table would be silently wrong.
3. `ActivateObject` then `ToggleObject`, each through the target's state slot
   (the original's vtable offset `0x428`) carrying the authored `Toggle`.
4. the `PIC_NUMBER` and `PointValue` award.
5. `NextTrigger`, as a trigger-chain forward.
6. the pickup sound.

`SetPickupItemState` (`004360b0`) states 0 and 1 are ported as the pickup
family's state slot, and the load-time half of `PostLoadPickupItem`
(`00436200`) — already-collected, and `InitallyActive` — runs at spawn. Those
two together are what make the twelve authored vending-machine pairs
(`cmach`/`cand`, `fmach`/`flurp`, `mdiam`/`diam`, `gdish`/`refill`,
`piggy1`/`piggy2`, `cjar`/`coins2`) behave as an exchange: the machine's gate
consumes, its `Toggle=1` write reveals the product, and the product's write
re-arms the machine. Each cycle is picture-negative, which is why the consume
reading of `RequiredPicNum` is the only one that terminates.

### How it is proven

`tools/linkage_oracles/C3DPickupItem.py` compiles the real, unmodified
behaviour and drives it over **all 383 shipped `3PIC` rows** in the 35 shipped
levels, diffing four things per row against expectations computed from the
recovered bodies and that row's own authored properties: the post-spawn load
gate, the full ordered event sequence on a funded collection, the refusal path
one unit short (including that nothing is partially consumed), and the
gate-before-collected-check ordering (re-touching a collected row still takes
the currency). `--selftest` mutation-tests the oracle against three defects it
must catch: swapping the gate and the collected-state check, moving the award
ahead of the side-effect dispatch, and consuming on a refusal.

Building it found two real defects: `gam_loader.c` mapped a property named
`Points`, which no shipped level authors — the field is `PointValue` (`0x620`)
— so no pickup in the native port had ever awarded score; and the award tested
truthiness, so every row authoring the format's `-1` unset scored minus one
point.

### Not covered

- `PickedUpIndex`'s replacement-sprite swap on pickup state 2 — native hides.
- `TimesToTrigger` / `trigger_count` repeat limiting — native latches once-only.
- `IsAmbient` / `UpdateAmbientPickupSound` (slot 241); no shipped row enables it.
- `PassThru`, `ShowArrow` — registered and defaulted here, but the decomp does
  not isolate their consumers either.
- The state slot on every class except the pickup family. `ActivateObject` /
  `ToggleObject` targets that are not `3PIC` (`3RCK`, `3OMT`, `3HYD`, `3SWN`,
  `3KIT`) and **all** `NextTrigger` targets (`3CAM` 19, `3MCA` 20, `3AIT` 4)
  resolve and then find no native body. The oracle records that outcome rather
  than counting it as success.
- The sound *mix*; only its position in the sequence is certified.
- Whether the original also *hides* an `InitallyActive=0` pickup. The recovered
  slot-266 body describes states 0 and 1, both of which show, and never
  describes the inactive state. Asserting invisibility changed the `level1`
  golden, so the claim was withdrawn and needs capture evidence.
- The `3FIS`/`3GIR`/`3DIN` creature leaf, which shares `CPickupType` and
  authors `PIC_NUMBER`, is a different FourCC on a different vtable and is not
  part of this aspect. Its award path exists but nothing collects it: the
  shrink transition that turns those creatures into pickups is undecompiled.

### Investigated 2026-07-02 (the gap this closed)

The worklist row named `behavior_pickup.c`, but that file implements
`C3DBaseballPickup`/`C3DBubblePickup`/`C3DHelmet`/`C3DMetalPickup`
(`3BPU`/`3BUP`/`3HEL`/`3MEP`) — different classes entirely, none of them placed
in the shipped `.gam` corpus. The real native counterpart to `3PIC` is
`behavior_item.c`'s `vt_item`, which at the time granted a "tool" by
case-insensitive substring match on `ObjectTag` against a hardcoded table, with
no `RequiredPicNum` gate, no `PickupIndex` state table, no
`ActivateObject`/`ToggleObject`/`NextTrigger` dispatch, no `NeedMoreSound`, and
`PIC_NUMBER` special-cased only for the baseball. That inventory model is still
present and still load-bearing — the watergun, jetpack and keys gate real
progression — but it is now additive to the ported picture economy rather than
a replacement for it.
