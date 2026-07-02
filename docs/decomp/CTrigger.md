# CTrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `CTrigger` |
| Base chain | `C3DLight -> OMediaLight -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d6a1c`, `004d6a2c`, `004d6e7c`, `004d6e90` |
| Ctor(s) | installs the four `CTrigger` vftables over the `C3DLight` construction path |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `0047de30` (destroys the `OMediaClassStreamer` at `this+0x173`) |
| Ledger row | `docs/decomp_ledger.csv` |

`CTrigger` is the engine's **proximity trigger** — an invisible spherical volume that
fires an *enter* action when a watched object crosses inside its radius and an *exit*
action when it leaves. It derives from `C3DLight` (so it reuses the light's
world-position + radius representation as the trigger volume; nothing is drawn).
It maintains a list of watched targets, each with a latched inside/outside flag, and
dispatches through two vtable action slots. This is the low-level mechanism that the
data-driven activation graph (`ActivateObject*` / `NextTrigger` / `AITarget` in
`.gam`, and `CTriggerTimer`) is built on top of.

## Field Map

Offsets are from the primary `CTrigger` pointer (`this[N]` is slot arithmetic on the
incomplete struct; the named byte offsets follow Ghidra's printout).

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this[0x139]` | float | `trigger_radius` | `vfunc_01_241` | Distance threshold. A target is "inside" when `dist(target, trigger) < trigger_radius`. Compared with `<=` against the Euclidean distance. |
| `this[0x13b]` | pointer | `watched_list_head` | `vfunc_01_241` | Head/sentinel of a circular linked list of watched-target nodes. Update walks `node = *node` until it returns to the head. |
| `this[0x17c]` | pointer | `register_list` | `vfunc_03_045` | The list the registrar (slot 45) splices new watch nodes into (the same list `watched_list_head` iterates). |
| `this[0x17d]` | int | `watched_count` | `vfunc_03_045` | Incremented each time a target is registered. |
| `this+0x173` | subobject | `class_streamer` | `vfunc_02_002` | `OMediaClassStreamer` tail destroyed by the deleting destructor. |

### Watch-node layout (from the update + registrar)

Each node is an 8-byte payload `{ u8 inside_flag; void* target }` carried by a 12-byte
list cell `{ next; prev; payload }`:

| Node field | Meaning |
|---|---|
| `payload[0]` (`*pcVar1`) | Latched state: `0` = currently outside, `1` = currently inside. |
| `payload[4]` (`pcVar1+4`) | Pointer to the watched target object; its world position is read via the target's slot `0x310`. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 21 | `0047e060` | `Reset21` | Thin pass-through to `CGameObject::vfunc_00_021`. | trivial |
| vtable 1 slot 241 | `0047dfa0` | `UpdateTrigger` | Per-frame proximity test over every watched node; fires enter/exit actions on state change. | non-trivial |
| vtable 2 slot 2 | `0047de30` | `ScalarDeletingDestructor` | Destroys `class_streamer`, frees the adjusted allocation. | non-trivial |
| vtable 3 slot 45 | `0047df30` | `RegisterTarget` | Allocates an `{inside_flag=0, target}` node, splices it into the watched list, increments `watched_count`. | non-trivial |

### Per-frame behavior

```c
CTrigger::UpdateTrigger(dt):                 // vfunc_01_241 @ 0047dfa0
    for node in circular_list(watched_list_head):     // this[0x13b]
        payload = node.payload                          // {inside_flag, target}
        my_pos     = this->get_world_position()         // slot 0x310 -> local_20
        target_pos = payload.target->get_world_position()  // target slot 0x310
        dist = length(my_pos - target_pos)
        if dist >= trigger_radius:                       // this[0x139]
            if payload.inside_flag != 0:                 // was inside -> now outside
                this->slot_0x58(payload.target)          // EXIT action
                payload.inside_flag = 0
        else:                                            // dist < radius
            if payload.inside_flag == 0:                 // was outside -> now inside
                this->slot_0x54(payload.target)          // ENTER action
                payload.inside_flag = 1
```

```c
CTrigger::RegisterTarget(target):            // vfunc_03_045 @ 0047df30
    node = alloc(8); node.inside_flag = 0; node.target = target
    splice node into register_list (this[0x17c])
    watched_count++                          // this[0x17d]
```

The enter/exit edges are **debounced by `inside_flag`**, so each action fires exactly
once per crossing. The actual enter/exit effects are the overridable vtable slots
`0x54` (enter) and `0x58` (exit) — subclasses / the activation wiring bind these to
"activate object", "play sound", "start cutscene", etc.

## Constants And Wiring

`CTrigger` registers no own `.gam` properties in an owned `InitObject` (it inherits
`C3DLight`'s position/radius properties — the trigger volume is the light's radius).
The targets it watches and the actions on slots `0x54`/`0x58` are wired at runtime by
the level/activation graph, not stored on the trigger itself.

| Item | Source | Notes |
|---|---|---|
| `trigger_radius` | inherited `C3DLight` radius | The proximity threshold. |
| enter action | vtable slot `0x54` | Fired once when a target enters. |
| exit action | vtable slot `0x58` | Fired once when a target leaves. |
| `CTriggerTimer` | sibling spec | Timer-driven variant in the same family. |

## Assets

None. `CTrigger` is a non-visual volume (inherits the light representation; nothing is
drawn).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CTrigger` (`slots=328`, `owned_methods=4`); the
proximity test, the latched enter/exit edges, and the linked-list registrar are read
directly from the decompiled `UpdateTrigger`/`RegisterTarget`. Not runtime-validated.

Open questions:
- Resolve the concrete bindings of action slots `0x54` (enter) and `0x58` (exit) for
  the shipped trigger subclasses / `.gam` activation rows.
- Confirm `this[0x139]`/`this[0x13b]` against the inherited `C3DLight`/`OMediaLight`
  field layout once those base structs are applied.
- Map `RegisterTarget`'s caller — who populates the watched-target list (player? all
  game objects? a specific `AITarget`?).

## Notes

- Evidence: `DumpClass.java CTrigger /tmp/dumps2/decomp_CTrigger.md`.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). The
  proximity-volume + latched-edge pattern is the primitive under the wider
  trigger/activation graph; see `CTriggerTimer`, `C3DAITrigger`, and the
  `ActivateObject*`/`NextTrigger` `.gam` wiring documented in `docs/gam_schema.md`.

## Native Linkage (linked-parity branch)

Aspect: **`enter-exit-latch`** — status `linked-blocked`.
Certificate: `docs/linkage_certificates.csv`.

Investigated 2026-07-02 (linked-parity pass). This worklist row's title
conflates three distinct decompiled classes, and none of them has a faithful,
testable native counterpart this pass:

- **`CTrigger` itself** (this doc) is the engine's proximity-volume primitive
  (watched-target linked list, latched `inside_flag`, debounced enter/exit
  dispatch to vtable slots `0x54`/`0x58`) — but it is **not a placeable
  FourCC** (no `Identity` FourCC field in this doc, no `docs/gam_schema.md`
  row). It's an internal mechanism other trigger-family classes build on, and
  no native file implements *this* linked-list/latch structure directly —
  `src/game/behaviors/behavior_trig.c` (below) is a different, simpler
  mechanism for a different, concrete FourCC.
- **`C3DTriggerType`** (`docs/decomp/C3DTriggerType.md`) is a shared base for
  `C3DAITrigger`/`C3DCutSceneCamera`/`C3DMultiCutSceneCamera`/etc. — also not
  itself placeable, and its one owned runtime method
  (`RunTriggerTypeNextTarget`) is explicitly flagged in that doc as "still
  raw decompiler output" (a global-record camera-targeting computation with
  unresolved trig-table semantics) — too poor an L1 to certify against.
- **`C3DTrigger`** (`3TRI`, `docs/decomp/C3DTrigger.md`) is the class with
  the actual placeable FourCC and a fully decompiled `ActivateTrigger`
  cascade (`ActivateBy` gate, `TimesToTrigger` limit, `NextTrigger`
  resolution, `ActivateObject0..4` state-gated cascade, sound playback) — but
  the native port (`src/game/behaviors/behavior_prop.c`, `vt_prop`) is
  **explicitly and completely inert** for this FourCC: "its activate-object
  cascade / NextTrigger dispatch is the project-wide deferred scripted-trigger
  system... fully 'none' (inert)." Zero ported logic to diff against.

Separately, the `TRIG` FourCC (`src/game/behaviors/behavior_trig.c`,
`vt_trig`) — the file the worklist row actually named — implements neither of
the above: a one-shot latch (`user_flag`) with no enter/exit distinction, no
`ActivateBy` gate, no `NextTrigger` cascade, and no sound. Its *own* RTTI
class name is still unresolved in `docs/gam_schema.md` ("`TRIG` | — (name
pending Phase 0)"), so there is no recovered decompiled body for `TRIG`
itself to certify `behavior_trig.c` against either.

Every reading of this row therefore lands on "no faithful native counterpart
to certify this pass" — either no native implementation of the documented
mechanism exists (`CTrigger`), the recovered body is too raw to trust
(`C3DTriggerType`), the native port explicitly declines to implement the
recovered body (`C3DTrigger`/`3TRI`), or the concrete class has no recovered
body at all (`TRIG`). Recorded `linked-blocked` rather than force a fit.

### Not covered / open

- A future pass that (a) resolves `TRIG`'s RTTI class and recovers its body,
  or (b) ports `C3DTrigger`/`3TRI`'s already-decompiled `ActivateTrigger`
  cascade for real, could open a genuine `linked` aspect here.
- `C3DTriggerType::RunTriggerTypeNextTarget` needs a cleaner Ghidra pass
  before it's worth linkage work at all.
