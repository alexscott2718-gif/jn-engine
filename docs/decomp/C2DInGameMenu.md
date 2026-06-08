# C2DInGameMenu

## Identity

| Item | Value |
|---|---|
| RTTI name | `C2DInGameMenu` |
| Base chain | `CLocalGameObject -> CGameObject -> OMediaClassStreamer` |
| Vftable(s) | `0048d414` |
| Ctor(s) | `CLocalGameObject` streamer construction path; allocates child canvas elements via `FUN_00478990(0xd0)` |
| Dtor(s) | scalar deleting destructor at vtable slot 0 |
| Ledger row | `docs/decomp_ledger.csv` |

`C2DInGameMenu` is the **in-game HUD / overlay** rendered on top of the 3D scene
during gameplay (distinct from `CMainMenu`, the front-end). It is the highest-method
class in the controller family — 17 owned methods of 316 walked slots. It owns the
numeric HUD counters (score / fuel / gadget counts), allocates `0xd0`-byte canvas
child elements for the readouts, draws them each frame, and handles the
death/restart transition. Visual ground truth: [`hud.c`](./hud.c) and the captured
frame-8881 HUD analysis in `docs/hud_chrome_digit_recapture.md`.

## Field Map

Offsets are from the primary `C2DInGameMenu` pointer (`this[N]` = slot arithmetic on
the incomplete struct).

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this[0x13f]` | int | `hud_counter_top_right` | `vfunc_00_312` `00406690` | Printed `%3.0d` at screen `(400, 0x8c)`. |
| `this[0x140]` | int | `hud_counter_top_left` | `vfunc_00_312` | Printed `%3.0d` at `(0x7d, 0x8c)`. |
| `this[0x141]` | int | `hud_counter_mid` | `vfunc_00_312` | Printed `%5.0d` at `(0x185, 0x10e)`. |
| `DAT_004f83c0` | int | `hud_score_or_fuel` | `vfunc_00_312` | Global counter printed `%6.0d` at `(0x1a9, 0x1b3)`. |

## Vtable Methods

316 slots walked; 17 owned (the `00402…`–`00407…` range). Characterized:

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 0 | `004029a0` | `ScalarDeletingDestructor` | Owned deleting destructor. | non-trivial |
| 8 | `00402b40` | `vfunc_00_008` | Inherited-slot override (init/attach). | raw block |
| 259 | `004072d0` | `ResetHud` | HUD reset slot. | raw block |
| 276 | `00402d70` | `vfunc_00_276` | HUD element setup. | raw block |
| 280 | `004033f0` | `vfunc_00_280` | HUD element setup / layout. | raw block |
| 302 | `004063e0` | `AllocHudElement` | Allocates a `0xd0`-byte `OMediaCanvasElement` child (`FUN_00478990(0xd0)`). | non-trivial |
| 306 | `00402b70` | `AllocHudElement2` | Second `0xd0`-byte canvas-child allocation path. | non-trivial |
| 312 | `00406690` | `DrawHud` | Per-frame HUD draw: prints `hud_counter_top_left/right` (`%3.0d`), `hud_counter_mid` (`%5.0d`), and `hud_score_or_fuel` (`%6.0d`) via `FUN_00468660`; on death/restart calls `FUN_00460e70("RestartLevel.tsk")` and inherited slot `0x114("level1f.gam")`. | non-trivial |
| 315 | `00407490` | `vfunc_00_315` | HUD teardown / mode switch. | raw block |

(Remaining owned slots `00404de0`, `00405fb0`, `004060d0`, `00406080`, `00406590`,
`00406650`, `00402f30`, `00403a20` are HUD element create/show/hide helpers.)

## Per-Frame Behavior

```c
C2DInGameMenu::DrawHud():               // vfunc_00_312 @ 00406690
    FUN_00468660(0x7d, 0x8c,  font, "%3.0d", hud_counter_top_left)    // this[0x140]
    FUN_00468660(400,  0x8c,  font, "%3.0d", hud_counter_top_right)   // this[0x13f]
    FUN_00468660(0x185,0x10e, font, "%5.0d", hud_counter_mid)         // this[0x141]
    FUN_00468660(0x1a9,0x1b3, font, "%6.0d", hud_score_or_fuel)       // DAT_004f83c0
    ...
    if (death/restart condition):
        FUN_00460e70("RestartLevel.tsk")            // reload respawn task
        global_level_loader->slot_0x114("level1f.gam")
```

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| HUD counter formats | `%3.0d`, `%5.0d`, `%6.0d` | `.rdata:004ec808`, `004ec800`, `004ec714` |
| Canvas child size | `0xd0` bytes | `FUN_00478990(0xd0)` in slots 302/306 |
| `RestartLevel.tsk` | respawn task on death | `.rdata:004ec7e4`; `DrawHud` |
| `level1f.gam` | level reload target on death path | `.rdata:004ec7d8`; `DrawHud` |
| Menu-system traces | `LoadMyMenu`, `UnloadMyMenu`, `Activating Item %d`, `Item IsActive:%d` | `.rdata:004ec620`+ (menu manager strings) |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| HUD layout | frame-8881 capture | `docs/hud_chrome_digit_recapture.md` | Visual ground truth for counter positions/digits. |
| HUD reimpl | `docs/decomp/hud.c` | repo | Native HUD rebuilt from the capture; counter wiring to `GameState`. |
| Task file | `RestartLevel.tsk` | executable reference | Death/respawn reload. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C2DInGameMenu` (`slots=316`, `owned_methods=17`);
decompiled `DrawHud` with literal screen coords / format strings / counter offsets;
cross-referenced against the native HUD rebuild (`hud.c`) and the frame-8881 capture.
Not runtime-validated.

Open questions:
- Map each `this[0x13f..0x141]` + `DAT_004f83c0` counter to its gameplay meaning
  (score, fuel, gadget count, lives) — the capture shows positions; the source values
  need the producer functions.
- Attribute the `LoadMyMenu`/`Activating Item` menu-manager strings precisely (this
  class vs. a shared menu manager vs. `CMenuElement`).
- Name the eight uncharacterized HUD-element helper slots.

## Notes

- Evidence: `DumpClass.java C2DInGameMenu /tmp/decomp_C2DInGameMenu.md`; HUD docs
  `docs/hud_chrome_digit_recapture.md`, `docs/decomp/hud.c`.
- This is the gameplay HUD overlay; `CMainMenu` is the separate front-end menu mode.
