# CTrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `CTrigger` |
| FourCC | `TRIG` (resolved target 5: factory `FUN_0047dcf0` installs the four `CTrigger` vftables and registers class id `'TRIG'` at `0047de03`; `docs/_gam_classids.tsv` row `GIRT`; 1 shipped instance whose `ObjectTag "CTRIGGER"` is the factory default tag @ `004f6c38`) |
| Base chain | `C3DLight -> OMediaLight -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d6a1c`, `004d6a2c`, `004d6e7c`, `004d6e90` |
| Ctor(s) | `FUN_0047dcf0` — installs the four `CTrigger` vftables over the `C3DLight` construction path, sets default tag `"CTRIGGER"` via `CGameObject::SetObjectTagLike`, allocates the watched-list sentinel, defaults `trigger_radius = 10.0f`, and registers class id `'TRIG'` via `C3DLight::vfunc_03_043` |
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

Byte offsets pinned by the constructor (target 5); the slot-arithmetic prints
in the method bodies are the same fields viewed from different subobject
pointers (`vfunc_01_*` methods take the vftable-1 pointer at base `+0x104`;
`vfunc_03_045` and the ctor use the primary base).

| Offset (base) | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x5e8` | float | `trigger_radius` | ctor `47ddb9`; `vfunc_01_241` (`this[0x139]` slot-1-relative) | Distance threshold; ctor default `10.0f`. A target is "inside" when `dist(target, trigger) < trigger_radius`. **Not** the registered `LightRange` property (slot-1 `+0x4b4`); no `.gam` property writes this field, so the shipped `TRIG` row's `LightRange 1e+04` never reaches it. |
| `0x5ec` | byte | `heap_allocated_flag` | ctor `47dd52` | Ctor parameter; gates the deleting destructor's free. |
| `0x5f0` | pointer | `watched_list_head` | ctor `47dd65`; `vfunc_01_241` (`this[0x13b]` slot-1-relative); `vfunc_03_045` (`this[0x17c]` base-relative) | Self-linked 12-byte sentinel of the circular watched-target list. The update's iteration list and the registrar's splice list are this same field. |
| `0x5f4` | int | `watched_count` | ctor; `vfunc_03_045` (`this[0x17d]`) | Incremented per registered target. |
| `0x5fc` | subobject | `class_streamer` | ctor (`in_ECX + 0x17f`); `vfunc_02_002` (`this+0x173` from the `+0x30` subobject) | `OMediaClassStreamer` tail destroyed by the deleting destructor. |

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
| `trigger_radius` | owned float at base `+0x5e8`, ctor default `10.0f` | The proximity threshold. Distinct from the registered `LightRange` (`+0x4b4`); no property writes it. |
| enter action | vtable slot `0x54` | Fired once when a target enters. |
| exit action | vtable slot `0x58` | Fired once when a target leaves. |
| `CTriggerTimer` | subclass (`TRIT`, unplaced) | Overrides slot 21 (enter) to arm/reset a dt-accumulating timer and slot 241 to accumulate; the only static caller of `UpdateTrigger` (`UpdateTriggerTimer_0047e240`). Confirms slot 21/`0x54` = enter and slot 22/`0x58` = exit. |

## Assets

None. `CTrigger` is a non-visual volume (inherits the light representation; nothing is
drawn).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CTrigger` (`slots=328`, `owned_methods=4`); the
proximity test, the latched enter/exit edges, and the linked-list registrar are read
directly from the decompiled `UpdateTrigger`/`RegisterTarget`. Not runtime-validated.

Open questions:
- Resolved (target 5): slot 21/`0x54` is the enter action and slot 22/`0x58` the
  exit action — proven by `CTriggerTimer`'s slot-21 arm/reset override
  (`TriggerTimerEnterArmOrReset_0047e270`).
- Resolved (target 5): `this[0x139]`/`this[0x13b]` are owned `CTrigger` fields at
  base `+0x5e8`/`+0x5f0`, initialized by the ctor — not inherited light fields.
- Map `RegisterTarget`'s caller — `0047df30` has **no static code callers**
  (vtable DATA refs only), so watchers are registered purely through virtual
  dispatch (slot 45 = vtable `+0xb4`), still unmapped. In particular nothing
  statically registers a watcher for the single placed `TRIG`.

## Notes

- Evidence: `DumpClass.java CTrigger /tmp/dumps2/decomp_CTrigger.md`.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). The
  proximity-volume + latched-edge pattern is the primitive under the wider
  trigger/activation graph; see `CTriggerTimer`, `C3DAITrigger`, and the
  `ActivateObject*`/`NextTrigger` `.gam` wiring documented in `docs/gam_schema.md`.

## Native Linkage (linked-parity branch)

Aspect: **`enter-exit-latch`** — status `linked-blocked`.
Certificate: `docs/linkage_certificates.csv`.

Re-dispositioned 2026-07-02 (Ghidra recovery plan target 5), superseding the
earlier three-way-conflation note. The identity questions are now settled:

- **`TRIG` = `CTrigger` itself.** The `TRIG` factory `FUN_0047dcf0` is the
  `CTrigger` constructor (installs all four `CTrigger` vftables, default tag
  `"CTRIGGER"`, class id `'TRIG'` @ `0047de03`); `docs/gam_schema.md` is
  updated. The single shipped `TRIG` row's `ObjectTag "CTRIGGER"` is the
  factory default tag, and its `LightRange 1e+04` lands in the registered
  light field (`+0x4b4`), **not** in `trigger_radius` (`+0x5e8`, ctor default
  `10.0f`, unregistered).
- **L1 is complete** for the whole family: the proximity watched-list latch
  (this doc), the constructor, and the `CTriggerTimer` subclass
  (`TRIT`, zero shipped instances — ctor `0047e0e0`, slot-241 dt-accumulator
  `0047e240`, slot-21 enter arm/reset `0047e270`). Evidence:
  `docs/decomp/evidence/triggertype_trigger_target5.md`.
- **No watcher wiring exists in the shipped data**: `RegisterTarget`
  (`0047df30`) has no static code caller (virtual slot-45 dispatch only, still
  unmapped), so the one placed `TRIG` has no statically-provable watched
  targets.

The row stays `linked-blocked` because **L2 fails by design**: native `TRIG`
(`src/game/behaviors/behavior_trig.c`, `vt_trig`) is a deliberate one-shot log
stub (a `user_flag` latch around a `printf`), fired by the native engine's own
trigger dispatch (`src/engine/physics.c`: player-AABB overlap re-fires
`on_trigger` every overlapping frame — no exit event, no per-target watched
list, no sphere radius, no debounced enter/exit edges). There is no native
implementation of the recovered watched-list/latched-edge mechanism, and no
fidelity claim to certify. Porting it (sphere-radius volumes, per-watcher
inside latches, exit dispatch) is engine behavior work for native-port, not
certification. `C3DTrigger`/`3TRI`'s inert native cascade
(`behavior_prop.c`, "fully none (inert)") is unchanged from the earlier note.

### Not covered / open

- A future native-port pass that implements the watched-list sphere latch
  1:1 (enter/exit edges included) would open a genuine `linked` aspect here;
  the recovered body is now fully specified for that port.
- `C3DTriggerType::RunTriggerTypeNextTarget` is now fully recovered (target 5)
  and dispositioned separately as
  `C3DTriggerType`/`nexttrigger-camera-retarget` (`linked-blocked`: native has
  no global camera/player-target record).
