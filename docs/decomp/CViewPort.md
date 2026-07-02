# CViewPort

## Identity

| Item | Value |
|---|---|
| RTTI name | `CViewPort` |
| Base chain | `OMediaViewPort -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaSupervisor` |
| Vftable(s) | `004d748c`, `004d749c`, `004d74b0`, `004d74c4` |
| Ctor(s) | constructor at `0047e310` |
| Dtor(s) | adjusted scalar deleting destructor at `0047e3e0`; cleanup helper at `0047e410` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`CViewPort` is a runtime viewport/frame-step wrapper around `OMediaViewPort`. It is not a `.gam` placeable class and does not bind a FourCC.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x11c` | subobject | `class_streamer` | ctor `0047e310`; dtor `0047e3e0` | Optional `OMediaClassStreamer` subobject constructed when the constructor flag is non-zero and destroyed by the scalar deleting destructor. |
| static `0x696018..0x69601f` | qword | `last_counter` | init `0047e470`; delta helper `0047e490` | Last high-resolution counter sample used by the viewport timing helper. |
| static `0x696020..0x696027` | qword | `counter_frequency` | ctor registration; delta helper `0047e490` | High-resolution counter frequency; elapsed seconds are `counter_delta / counter_frequency`. |
| static `0x696028` | uint32 | `frame_count` | frame wrapper `0047e4f0` | Incremented after each viewport frame. |
| static `0x696030` | float | `last_delta_seconds` | init `0047e470`; frame wrapper `0047e4f0` | Current frame delta. The invalid-delta trace prints `LASTDELTA=%f`. |
| static `0x696034` | uint32 | `render_port_counter` | slot `0047e450` | Incremented by a small viewport/render-port wrapper after forwarding to an OMedia import. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0047e310` | `CtorViewPort` | Optionally constructs the class-streamer subobject at primary `0x11c`, constructs the inherited `OMediaViewPort`, installs the four `CViewPort` vftables, registers the `.?AVCViewPort@@` RTTI/class record, and initializes viewport timing globals. | non-trivial |
| vtable 2 slot 2 | `0047e3e0` | `ScalarDeletingDestructor` | Adjusts from the `OMediaViewPort` subobject back to the primary pointer, calls `CleanupViewPort`, destroys `class_streamer`, and frees the primary allocation when the delete flag is set. | non-trivial |
| cleanup | `0047e410` | `CleanupViewPort` | Reinstalls the `CViewPort` vftables/displacement entry, then tail-calls `OMediaViewPort::~OMediaViewPort`. | non-trivial |
| vtable 3 slot 15 | `0047e450` | `ForwardAndCountRenderPort` | Forwards its argument through imported OMedia render-port logic, then increments static `render_port_counter`. | thin wrapper |
| helper | `0047e470` | `InitViewPortTiming` | Samples the high-resolution counter into `last_counter` and clears `last_delta_seconds`. | non-trivial |
| vtable 3 slot 46 | `0047e4f0` | `FrameStepAndRender` | Reads the frame delta through an inherited virtual call, rejects deltas outside `(0.0, 1.0)`, drives gameplay update hooks with the delta, refreshes a camera/view matrix from globals `DAT_00509a38` and `DAT_00509a50`, calls inherited viewport rendering, runs post-frame cleanup, and increments `frame_count`. | non-trivial |
| vtable 3 slot 53 | `0047e490` | `ComputeCounterDeltaSeconds` | Samples the high-resolution counter, computes elapsed seconds from `counter_frequency`, and advances `last_counter` only when the computed delta is non-zero. | non-trivial |

## Per-Frame Behavior

```c
CViewPort::FrameStepAndRender(arg):
    dt = inherited_get_delta()
    last_delta_seconds = dt
    if dt <= 0.0 or dt >= 1.0:
        log("LASTDELTA=%f", dt)
    else:
        if update_gate_allows_gameplay() and DAT_00509980 != null and DAT_00509980[0x8c] == 0:
            update_gameplay_subsystems(dt)
        update_timers(dt)
        update_input_or_world_lists()
        if DAT_00509a38 != null:
            rebuild_view_matrix_from_camera_globals()  // L1 recovered: see evidence/camera_record_layout.md
        update_render_or_scene(dt)
    OMediaViewPort::render_or_step(arg)
    post_frame_cleanup()
    frame_count++
```

This class is the bridge between OMedia's viewport dispatch and the game's per-frame update ordering. `C3DVehicle` can also allocate an `OMediaViewPort` directly for vehicle camera handling, but `CViewPort` is the named game-level viewport wrapper with frame timing and update orchestration.

## Constants And Wiring

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| valid delta lower bound | float | `0x48d914` | `0.0` | `FrameStepAndRender` rejects `dt <= 0.0`. |
| valid delta upper bound | float | `0x48d924` | `1.0` | `FrameStepAndRender` rejects `dt >= 1.0`. |
| invalid delta format | string | `0x4f6cb0` | `LASTDELTA=%f` | Logged when the inherited delta is outside the valid range. |
| RTTI/class string | string | `0x4f6ca0` | `.?AVCViewPort@@` | Constructor registration record. |
| camera/view globals | pointers | `DAT_00509a38`, `DAT_00509a50` | nullable | `FrameStepAndRender` builds a matrix block at `DAT_00509a38 + 0x7c` from camera angle fields near `DAT_00509a50 + 0x50`. |

## Assets

`CViewPort` does not name or load meshes, canvases, sounds, or textures. It coordinates frame timing and render/update dispatch for other systems.

## Confidence

Confidence: Medium

Validation: Static Ghidra dump + local `objdump` disassembly + string/constant cross-check only; not runtime-validated.

Open questions:
- Name the imported OMedia calls behind IAT entries `0x48d0b8`, `0x48d0bc`, `0x48d24c`, `0x48d300`, and `0x48d304`.
- Confirm whether `render_port_counter` at `0x696034` is diagnostics-only or feeds later timing logic.
- Resolve the exact owners of the gameplay update hooks called from `FrameStepAndRender` (`00479bc0`, `00462a20`, `004693a0`, `0047d8f0`, `00477db0`, and `00468600`).
- ~~Name the camera matrix structure copied into DAT_00509a38 + 0x7c.~~ **DONE 2026-07-02**: 68-byte {float m[16]; u32 tag} (tags: 1 identity, 4 translation, 5 rotation, 6 composed); the full view build is L1 in docs/decomp/evidence/camera_record_layout.md — RotY(-angle_y)·RotX(-angle_x)·RotZ(-angle_z), zero translation, copied as 17 dwords.

## Notes

- Evidence: `DumpClass.java CViewPort /tmp/decomp_CViewPort.md` (`slots=65`, `owned_methods=1`, `offsets=1`).
- Additional evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `0047e310..0047e6fc` and helper cluster `0047e700..0047e91f`.
- Ghidra currently recognizes only the adjusted destructor as owned by `CViewPort`; the constructor and render-step wrappers are documented from vtable slots and confirmed code bytes.
