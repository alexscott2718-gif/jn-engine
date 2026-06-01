# Jimmy: Animation, Movement & Physics Upgrade Plan

*Drafted 2026-06-01. Goal: bring the player character (Jimmy) as close to 1:1
with the original JNBG as the source data allows — real animation, the
original's directional pose vocabulary, and capture-tuned movement/physics.*

## Current state (the gap)

- **Animation:** none. `player_anim.c` swaps between 5 **static** `.ASE` poses
  (idle/run/jump/fall/pickup); `ase_loader.c` reads only the **first** `*MESH`
  block and ignores `*MESH_ANIMATION` entirely. Jimmy is a rigid mannequin that
  pops between frozen poses.
- **Movement:** `behavior_player.c` sets velocity instantly from input (no
  accel/decel), faces travel direction, single jump impulse.
- **Physics:** `physics.c` — constant gravity, per-axis AABB resolution, ground
  plane fallback. Constants (`PHYSICS_GRAVITY`, `PLAYER_MOVE_SPEED`,
  `PLAYER_JUMP_VEL`) are guesses.

## Ground truth (what the original actually does)

### Animation = per-frame vertex morph (`*MESH_ANIMATION`)
Measured from `~/xp-jnbg-original/ASE/jim*.ase`. Each clip is a `GEOMOBJECT`
whose `*MESH_ANIMATION { }` holds **N full `*MESH` keyframes** of the *same*
426-vertex / 814-face mesh, each tagged `*TIMEVALUE t`. Topology (vertex count,
faces) is **constant across frames** → animation is linear per-vertex lerp
between consecutive keyframes (MD2-style morph). `*SCENE_FRAMESPEED` gives the
playback fps; `*SCENE_LASTFRAME` the frame count.

Per-frame `*MESH` block layout (same grammar `ase_loader` already parses for a
single mesh, just repeated):
```
*MESH_ANIMATION {
  *MESH { *TIMEVALUE 0   *MESH_NUMVERTEX 426 *MESH_NUMFACES 814
          *MESH_VERTEX_LIST{…} *MESH_FACE_LIST{…} *MESH_TVERTLIST{…} … }
  *MESH { *TIMEVALUE 480 … }   # next keyframe
  …
}
```

### The original's pose vocabulary (frame counts / fps, measured)
| Clip | frames | fps | role |
|---|---|---|---|
| `jimrun` | 5 | 10 | forward run (loop) |
| `jimleft` | 6 | 10 | strafe left (loop) |
| `jimright` | 6 | 10 | strafe right (loop) |
| `jimbackpedal` | 5 | 10 | move backward (loop) |
| `jimstop` | 2 | 10 | idle (loop/hold) |
| `jimjump` | 3 | 10 | jump (one-shot → hold last) |
| `jimfall` | 4 | 10 | fall (loop) |
| `jimpickup` | 4 | 10 | pickup (one-shot) |
| `jimladder` | 6 | 5 | ladder climb |
| `jimswing`/`jimscooter`/`jimfly`/`jimshoot`/`jimdrive` | 2 | 10 | contextual |
| `jimtalk`/`jimhit`/`jimsplat`/`jimscratch`/`jimheadshrink`/`jimbuttons` | 2–10 | 5–10 | scripted |

**Key 1:1 detail:** the original uses **separate directional clips**
(`jimleft`/`jimright`/`jimbackpedal`/`jimrun`) selected by movement direction
relative to facing — it does NOT just rotate a single run cycle.

### Physics — no decomp source
`OMT2.dll` Ghidra work covered rendering only; there are **no measured movement
constants**. Per decision, physics will be **tuned against the D3D7 capture**
(Jimmy's per-frame world transform in the `.omtc` stream) — speed, jump apex,
gravity, accel — not guessed. Capture-as-ground-truth, consistent with project
rules. (Animation, by contrast, *is* exactly recoverable from the ASE files.)

## Decisions (locked with user 2026-06-01)
1. **Full directional 1:1** animation (separate strafe/backpedal/run clips).
2. **Physics tuned vs D3D capture** (measure, don't guess).
3. **Plan-doc-first**, phase-by-phase with a visual QA gate each phase.

## Phases

### Phase 1 — Keyframe animation core  ← biggest visible win
- Extend `ase_loader.c` to parse `*MESH_ANIMATION` → N keyframes. New
  `AseAnim`/animated-model type holding per-frame vertex arrays (shared
  faces/UV/material — identical across frames) + `framespeed`, `frame_count`.
  Single-`*MESH` files load as a 1-frame clip (back-comaptible).
- New `anim_model.{c,h}` (or extend `AseModel`): GPU path uploads two frames +
  a lerp factor; vertex shader interpolates `mix(posA, posB, f)`. (Two morph
  VBOs + a uniform — cheap, WebGL2-friendly. Alternative: CPU lerp into one VBO
  per frame if shader morph is fussy under FULL_ES3 — decide at impl.)
- `player_anim.c`: play `jimrun` as a **real looping cycle** at its 10 fps.
- **Gate:** Jimmy visibly runs (legs cycle) in the demo.

### Phase 2 — Directional pose/state machine (1:1)
- Load the full clip set (run/left/right/backpedal/stop/jump/fall/pickup).
- State machine driven by movement direction **relative to facing/camera**:
  forward→`jimrun`, left→`jimleft`, right→`jimright`, back→`jimbackpedal`,
  still→`jimstop`. Air: rising→`jimjump`, falling→`jimfall`. `jimpickup` as a
  one-shot overriding locomotion (current pickup hook).
- Per-clip loop vs one-shot vs hold-last rules + clean transitions.
- **Gate:** strafing/backpedaling/jumping show the correct distinct clips.

### Phase 3 — Movement & physics feel (capture-tuned)
- Tool: extract Jimmy's per-frame world position from the Level-1 `.omtc`
  (the `CAPTURE_LEVEL1_JIMMY_MODEL` transform path already exists) → derive
  ground speed, jump apex height + airtime → solve gravity + jump velocity;
  estimate accel/decel from velocity ramp.
- Apply to `behavior_player.c` / `physics.c`: acceleration + friction (not
  instant velocity), matched jump arc, run speed. Keep analog joystick scaling.
- **Gate:** side-by-side native vs capture motion comparison.

### Phase 4 — Contextual states (Level-1 only)
- Only what Level 1 uses (ladder/swing/etc. if present). Defer the rest.

## Risks / notes
- **Shader morph under WebGL2/FULL_ES3:** two POSITION attributes + lerp uniform
  must round-trip through emcc. Fallback = CPU lerp. Validate in Phase 1.
- **Memory:** 426 verts × ~6 frames × few clips is tiny; no concern.
- **`tools/omt_mesh_export.py` / glTF:** Jimmy is a hand-authored ASE, not OMT —
  stays on the ASE path. This work does NOT touch the OMT→glTF pipeline.
- **VBO churn:** preload all clip frames once at init (like current poses);
  never per-frame allocations.
