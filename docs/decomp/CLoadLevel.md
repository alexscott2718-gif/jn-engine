# CLoadLevel

## Identity

| Item | Value |
|---|---|
| RTTI name | `CLoadLevel` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d024c`, `004d025c`, `004d06ac`, `004d06c0` |
| Ctor(s) | installs the `CLoadLevel` vftables; `InitObject` registers FourCC `LOAD` (`0x4c4f4144`) |
| Dtor(s) | inherited `C3DSprite` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`CLoadLevel` is the **level-transition portal** — the placeable `LOAD` object that,
when the player enters its radius (and any task/level prerequisites are met), fades the
screen and loads another level at a named start point. It derives from `C3DSprite`
(a 2D canvas/sprite element) — the portal can carry a sprite/marker. It is the
data-driven counterpart to the menu's hard-coded `NewGame.tsk` route: the *in-world*
way the game moves between the 35 `.gam` levels. (FourCC `LOAD`; see
`docs/_gam_classids.tsv` `LOAD -> C3DLoadLevel/CLoadLevel`.)

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`) via the `vftable+0x3fc` registrar; types
are `.gam` type ids (`1=string 3=float 6=int`). Strings resolved from `Neutron.exe`.

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x148` | string | `LevelName` | Target level `.gam` to load (e.g. `level1d.gam`). |
| `0x15c` | string | `StartPoint` | Named spawn/start-point tag in the target level. |
| `0x170` | string | `RequiredTask` | Task that must be satisfied before the portal will fire. |
| `0x184` | int | `RequiredLevel` | Minimum level/progress gate. |
| `0x186` | int | `ExactLevel` | Exact level-match gate (alternative to `RequiredLevel`). |
| `0x0d` | float | `Radius` | Proximity radius the player must enter to trigger the load. |
| `0x185` | int | `SoundIndex` | Sound played on activation. |
| `0x187` | int | `FadeType` | Screen-fade style used during the transition. |
| `0x188` | float | `FadeTime` | Fade duration. |

`InitObject` also calls the shape subobject (`this[-0x32]`) slot `0xc0` with
`0x4c4f4144` = the FourCC **`LOAD`** (little-endian `'LOAD'`), registering the class id.

See `docs/gam_schema.md` for the per-level `LOAD` rows and the actual
`LevelName`/`StartPoint` values used across the game's level graph.

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00457da0` | `InitObject` | Registers the 9 `.gam` properties above + FourCC `LOAD`. | non-trivial |
| vtable 3 slot 53 | `00458370` | `ActivateLoad` | Fires the transition: hides the portal (slot `0xd8`) and calls the global game object (`*DAT_00509980`) slot `0x100` with the request block at `this+0x17a`, handing off the `LevelName`/`StartPoint`/fade request to the level loader. | non-trivial |

### Activation behavior

```c
CLoadLevel::ActivateLoad():                  // vfunc_03_053 @ 00458370
    reset request block at this+0x18e
    this->hide()                             // slot 0xd8 (C3DSprite hide)
    global_game = *DAT_00509980
    global_game->slot_0x100(&this->load_request)   // this+0x17a -> begin level load
```

The proximity/prerequisite test (player within `Radius`, `RequiredTask`/`RequiredLevel`
satisfied) is performed by the inherited update / collision path; `ActivateLoad` is the
commit step that hands the `{LevelName, StartPoint, FadeType, FadeTime}` request to the
global game controller (`CGameType`/`CJimmyGame` at `DAT_00509980`) which performs the
actual `.gam`/`.tsk` swap.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| FourCC | `LOAD` (`0x4c4f4144`) | `InitObject` shape slot `0xc0` immediate |
| Level graph | `LevelName` → target `.gam`, `StartPoint` → spawn tag | `.gam` `LOAD` rows; `docs/gam_schema.md` |
| Gate | `RequiredTask` / `RequiredLevel` / `ExactLevel` | progress prerequisites checked before firing |
| Loader handoff | `*DAT_00509980` slot `0x100` | the current game-mode object performs the swap |

## Assets

| Kind | Name | Notes |
|---|---|---|
| Sprite/marker | inherited `C3DSprite` canvas | Portal may carry a 2D marker; no own ASE/PNG registered in `InitObject`. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CLoadLevel` (`slots=338`, `owned_methods=2`,
`offsets=11`); all 9 properties + the `LOAD` FourCC + the loader-handoff call are read
directly from the decompiled `InitObject`/`ActivateLoad`. Not runtime-validated.

Open questions:
- Decode the exact request-block layout at `this+0x17a` / `this+0x18e` passed to the
  loader slot `0x100`.
- Confirm where the `Radius` proximity + `RequiredTask`/`RequiredLevel` gate is
  evaluated (inherited collision/update slot) and how it calls `ActivateLoad`.
- Tie `FadeType` values to the fade implementation.

## Notes

- Evidence: `DumpClass.java CLoadLevel /tmp/dumps2/decomp_CLoadLevel.md`; FourCC string
  `'LOAD'` and the `level1d.gam` reference confirmed via PE string resolution.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). This is
  the in-world level-graph edge; the menu's `NewGame.tsk`/VR routes (see
  [`CMainMenu.md`](./CMainMenu.md)) are the front-end equivalent.
