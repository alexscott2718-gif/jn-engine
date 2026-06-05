# JNvsJN — Missing Gameplay Systems: Plan + Source Map

Status: 2026-06-03. Scope: items, HUD, inventory/tools, vehicles. Where each
lives in the original game's data/code, and how to bring it into jn-engine.

## Where the original logic + assets live

| Thing | Location |
| --- | --- |
| Game logic / entity classes | `Neutron2.exe` (`.../Program_Executable_Files/`), software path `Neutron2SW.exe` |
| 3D render / D3D7 | `OMT2.dll` (same as JNBG game-1; Ghidra notes in `docs/ghidra_notes.md`) |
| Skeletal actors / tools | `granny.dll` + `grn/*.grn` (see `docs/jnvsjn_grn_probe_findings.md`) |
| HUD / icon art | `omt/icons.omt`, `omt/permanenticons.omt`; `png/bars.png`, `png/heart.png`, `png/toolchest.png` |
| Entity placement + props | `gam/*.gam` (per-level), `omt/<level>.omt` (geometry) |

### FourCC → class decoder (KEY ARTIFACT)

The exe contains RTTI/ctor strings of the form `<4-char-prefix>C3D<ClassName>`
where the **prefix reversed is the FourCC**. Extracted the full map (~90 classes)
from `Neutron2.exe`:

```
strings -n 7 Neutron2.exe | grep -oE '[A-Za-z0-9]{4}C3D[A-Za-z]+' | sort -u \
  | awk '{p=substr($0,1,4);c=substr($0,5);cmd="echo "p"|rev";cmd|getline r;close(cmd);print r" -> "c}'
```

This is the authoritative entity table — drives BOTH this plan and the broader
`entity_visual.c` coverage work. Relevant rows for the four systems below:

- **Items:** `3PIC`→C3DPickupItem, `3GEM`→C3DGem, `3TRO`→C3DTrophy,
  `3BAL`→C3DBalloon, `3CHK`→C3DCheckPoint
- **HUD:** `3RAD`→C3DRadar, `3MEO`→C3DMenuObject, plus `C2DInGameMenu` (2D layer)
- **Tools/weapons:** `3PRO`→C3DProjectile; tools are GRN-held (`watergun1/2.grn`,
  `megaburpgun.grn`, `burpgunhead.grn`); gam tags `WATERGUN1`, `pickupwatergun`,
  `PICKUPGLASSES`, `tools04`, `gunhelp1/2`
- **Vehicles:** `3VEH`(base C3DVehicle), `3JEE`→C3DJeep, `3HOV`→C3DHoverCycle,
  `3SUB`→C3DSub, `3NCA`/`3NC2`→C3DNeuCar, `3ROC`→C3DRocketShip,
  `3LUN`→C3DLunarLander, `3RCK`→C3DRocket

## What the engine has today

- `gamestate` tracks `items_total` / `items_collected` only.
- `behavior_item.c`: pickup = proximity trigger that increments the counter; no
  visual, no inventory slot, no tool grant.
- **No HUD / 2D overlay at all** (no ortho pass, no icon draw).
- No inventory, no held tools, no vehicles (vehicle FourCCs draw placeholder
  boxes or nothing).
- Renderer is 3D-only (`renderer_draw_model`, `renderer_draw_billboard`).

---

## Plan by system (incremental, lowest-effort-first)

### 1. HUD / 2D overlay  ← do FIRST (unblocks items + inventory display)

The engine needs a 2D screen-space layer. None exists yet.

- **Add** `renderer_draw_sprite_2d(tex, x, y, w, h, ...)` — an orthographic pass
  drawn after the 3D scene (reuse the billboard shader with an ortho matrix; the
  PNG-embedded-texture path already exists in `gltf_loader`/`tex_loader`).
- **Source the art:** extract `icons.omt` + `permanenticons.omt` canvases with
  the toolkit (`omt-extract`/canvas path) → PNG atlas; `bars.png` (health/status
  bars), `heart.png` (lives), `toolchest.png` (inventory button).
- **First HUD:** gem/item counter (we already have `items_collected/total`) +
  health bar. Wire into the main loop's post-3D draw.
- `C3DRadar` (`3RAD`, 557 instances across levels) is a HUD minimap element —
  later; needs entity positions projected to a 2D disc.
- `C2DInGameMenu` is the pause/menu — out of scope for first pass.
- **Effort: medium.** This is the keystone; items/inventory render through it.

### 2. Items (pickups)  ← extends current system

- Give `3PIC`/`3GEM` a **visible mesh** (reuse: `3GEM`→`assets/glb/grn/gemred.glb`
  etc. with GAM `Red/Green/Blue` tint; `3PIC` by tag → existing ASE/OMT props).
- Read GAM props the loader already passes through: `PickupIndex`, `PickupLink`,
  `PIC_NUMBER`, `Points`, `GlowAndScale`, `Radius`, `InitiallyVisible`.
- On collect: increment a **typed** counter (gems vs tools vs points), play the
  pickup pose (already exists), hide the entity. Show via the HUD from §1.
- Note exe string `ERROR: OVER max pickup items in C3DPickupItem` ⇒ there is a
  fixed-size pickup/inventory array; mirror that as the inventory model (§3).
- **Effort: low–medium** once §1 exists.

### 3. Inventory / tools (held items)

Tools are GRN-skinned held meshes (watergun, burpgun, glasses). The hard part is
GRN skinning (deferred); the *inventory model* is independent and can ship first.

- **Inventory state** in `gamestate`: a small fixed array of owned tools/keys
  (mirrors the exe's pickup array), current selected tool.
- **Acquire:** GAM tags `pickupwatergun`/`WATERGUN1`/`PICKUPGLASSES`/`tools04`
  on `3PIC` grant a tool slot.
- **Display:** tool icons from `icons.omt` in the HUD (§1); `toolchest.png` as
  the inventory affordance.
- **Use/equip:** map the GRN tool mesh to the player's hand bone — BLOCKED on
  GRN skeleton/skinning decode (`docs/jnvsjn_grn_probe_findings.md`, deferred).
  Interim: show the tool as a static mesh attached at an approximate offset, or
  icon-only.
- **Projectiles** (`3PRO`→C3DProjectile): water/burp shots — a simple spawned
  moving entity + collision; can come after equip.
- **Effort: low (state+icons), high (skinned held mesh + projectiles).**

### 4. Vehicles

Base `C3DVehicle` with subclasses Jeep/HoverCycle/Sub/NeuCar/RocketShip/
LunarLander. Counts are low (3JEE×2, 3HOV×2, 3SUB×4, 3ROC×40) so coverage is
cheap; ridability is the work.

- **Visual first:** point each vehicle FourCC at a mesh (OMT/ASE twin if present,
  else GRN: `nummeyscooterbase.grn`, `carbase.grn`; `3ROC`→`rocket.ASE`). Removes
  placeholder boxes immediately.
- **Ride mechanic:** enter-trigger → reparent player control to the vehicle
  entity, swap movement params (vehicles have own speed/turn; some GAM rows carry
  `AIHover`/`CanHover`/`carbeep`). Exit returns control to Jimmy.
- Each subclass differs (hover = airborne, sub = underwater, rocket = on-rails);
  implement Jeep (simplest ground vehicle) first as the template.
- **Effort: medium** for one ridable template, then per-vehicle tuning.

---

## Recommended order

1. **HUD 2D layer** (§1) — keystone; also makes item/gem counts visible, a quick
   visible win.
2. **Items visible + typed counters** (§2) — small, leans on §1.
3. **Inventory state + tool icons** (§3, non-skinned parts).
4. **Vehicle visuals → Jeep ride template** (§4).
5. Deferred/hard: GRN skinning (held tools, character anim), projectiles, radar,
   in-game menu.

## Cross-cutting prerequisite

Extract `icons.omt` + `permanenticons.omt` canvas atlases to PNG (toolkit canvas
path) — needed by §1/§2/§3. Verify counts: both are ~25 KB OMT files; treat as
icon sheets, not 3D.

## Open questions to resolve during implementation

- Exact HUD layout/positions: recover from a real screenshot of JNvsJN running
  (XP/VNC) or from `C2DInGameMenu`/icon canvas metadata.
- The pickup-array size (the "OVER max" cap) — find via Ghidra on
  `C3DPickupItem::` in Neutron2.exe to size the inventory exactly.
- Whether HUD draw is in Neutron2.exe or OMT2.dll's 2D path (decompile to confirm
  the icon-draw call site).
