# CMenuElement

## Identity

| Item | Value |
|---|---|
| RTTI name | `CMenuElement` |
| Base chain | `OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> MagicHatTarget -> CGameObject` |
| Vftable(s) | `004d11ec`, `004d11fc`, `004d161c`, `004d1630`, `004d1640` |
| Ctor(s) | `OMediaCanvasElement` streamer construction path |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `0045e6a0` (`this+0x13d` streamer tail) |
| Ledger row | `docs/decomp_ledger.csv` |

`CMenuElement` is a single **clickable menu item** — a 2D canvas sprite with
rollover/activation behavior, owned by [`CMainMenu`](./CMainMenu.md). Unlike the menu
controllers it derives from `OMediaCanvasElement` (the engine's 2D canvas object),
giving it sprite/position/hit-test machinery; it adds the menu interaction: when the
item is in its inactive/rollover state it shows the mouse cursor and forwards to a
target slot, then runs the inherited canvas `update_logic`.

## Field Map

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this[0xc]` | subobject | `canvas_base` | `vfunc_04_015` `0045e650` | Adjustment to the `OMediaCanvasElement` subobject pointer (`this + 0xc`) used when dispatching to the target. |
| `this[0x16]` | pointer | `parent_or_input` | `vfunc_04_015` | Object queried (slot `0x38` → slot `0x14`) for the current active/hit state; also provides the target slot `[0x23]+0xc`. |
| `this+0x13d` | subobject | `class_streamer` | `vfunc_02_002` `0045e6a0` | `OMediaClassStreamer` tail destroyed by the deleting destructor. |

## Vtable Methods

321 slots walked; 2 owned.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 4 slot 15 | `0045e650` | `UpdateItemLogic` | Queries `parent_or_input` for active/hit state; if **not** active, dispatches to the target slot (`parent_or_input[0x23]+0xc`) with the adjusted `this+0xc` and calls `OMediaMouseCursor::show()`; always finishes with inherited `OMediaCanvasElement::update_logic(this, dt)`. The per-frame rollover/click handler. | non-trivial |
| vtable 2 slot 2 | `0045e6a0` | `ScalarDeletingDestructor` | Owned deleting destructor; destroys the streamer tail at `this+0x13d`. | non-trivial |

## Per-Frame Behavior

```c
CMenuElement::UpdateItemLogic(dt):      // vfunc_04_015 @ 0045e650
    state = parent_or_input->slot_0x38()->slot_0x14()   // active/hit query
    if state == 0:                                       // inactive / rollover
        target = (this != NULL) ? this + 0xc : NULL
        parent_or_input->slot_(0x23+0xc)(target)         // notify menu controller
        OMediaMouseCursor::show()
    OMediaCanvasElement::update_logic(this, dt)          // inherited canvas update
```

## Constants And Wiring

| Item | Source | Notes |
|---|---|---|
| Rollover cursor | `OMediaMouseCursor::show()` | Cursor shown while item is in the inactive/rollover branch. |
| Target dispatch | `parent_or_input[0x23]+0xc` | Activation routed back to the owning menu controller (`CMainMenu`). |
| Item layout (position / sprite / level route) | `CMainMenu` + executable routing table | JNBG drives items from the executable menu table, not a `menu.dat` (sequel-only asset — not used). |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| Canvas sprite | per-item icon | `OMediaCanvasElement` | The item's drawn sprite; specific canvas id set by the owning menu. |

## Confidence

Confidence: Low-Medium

Validation: Ghidra `DumpClass.java CMenuElement` (`slots=321`, `owned_methods=2`);
decompiled `UpdateItemLogic` and destructor; target 4 includes the raw body in
`docs/decomp/evidence/menu_manager_target4.md`. Base chain confirms
`OMediaCanvasElement` 2D-canvas lineage. The boolean polarity of the active/hit
query and the exact target slot semantics are named conservatively. Not
runtime-validated.

Open questions:
- Confirm the active/hit polarity (is state `0` "rollover" or "not hovered"?) and the
  meaning of target slot `0x23+0xc` on the menu controller.
- Identify where item position/size/sprite/level-route are assigned (the producer in
  `CMainMenu` or the streamer load).
- Resolve the `MagicHatTarget` base's role in the menu interaction.

## Notes

- Evidence: `DumpClass.java CMenuElement /tmp/decomp_CMenuElement.md`.
- Owned by `CMainMenu`; sibling concept to HUD elements in `C2DInGameMenu` but built
  on the generic `OMediaCanvasElement` rather than the HUD overlay class.

## Native Linkage (linked-parity branch)

Aspect: **`update-item-logic`** -- status `linked-blocked` (target 4, 2026-07-02).

`UpdateItemLogic` is body-backed: it queries the parent/input object through
slots `0x38` and `0x14`; when that query returns `0`, it dispatches the
adjusted canvas subobject (`this+0xc`) through `parent_or_input[0x23]+0xc`,
shows the mouse cursor, and then runs inherited canvas update logic.

The native menu has no `CMenuElement` canvas object, mouse cursor path, or
target-slot dispatch; `src/game/menu.c` is a keyboard-list stand-in scoped to
the routing-table certificate. A green oracle would require porting this
canvas item behavior first.

That port cannot be isolated from the missing `CMainMenu` table records: the
snapshot explicitly leaves the `state == 0` hit/rollover polarity and the
`parent_or_input[0x23]+0xc` target semantics conservative, and it does not
recover the requested activation-sound caller. Keep this aspect blocked until
the owning canvas protocol and table contents are recovered; do not substitute
keyboard selection state for those fields.
