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

### Physics — FOUND in Neutron.exe + Level1.gam (recoverable as data!)
*Correction (2026-06-01): the earlier "no decomp source" claim was wrong — the
physics is NOT in OMT2.dll (that's render only), it's in `Neutron.exe`'s
`C3DPlayer` class, and it is **data-driven** from the `.gam` files.*

**The exact constants** (parsed from `Level1.gam`'s C3DPlayer LOAD block — a
big-endian `<name><u32 type=3><u32 len=4><BE float>` key/value table):

| param | value | meaning |
|---|---|---|
| `MaxSpeed` | **1400** | top ground speed (units/s) — vs our guessed 600 |
| `AccelRate` | **400** | ground acceleration |
| `DecelRate` | **1** | deceleration (very low → long momentum coast) |
| `MaxHeight` | **1500** | height cap |
| `UpRate` | **650** | jump ascent rate |
| `DownRate` | **−650** | descent rate |
| `MaxVertVelocity` | **650** | vertical speed cap |
| `NewGravity` | **0** | NOT gravity-accel — see below |
| `AccelLean` / `DecelLean` | **±20** | body lean into accel/decel/turns |

Jimmy's spawn is also in this block: `Position (10540.46, 596.02, −4619.65)`,
`Rotation 0`.

**How the vertical model works** (disassembled `C3DPlayer` update @
`Neutron.exe:0x40ed40`, the routine that reads these fields):
- It is **NOT accelerating gravity**. It's a **scripted, phase-based vertical
  arc**. `[player+0x658]` holds a **jump phase 0–8** driving a 9-entry handler
  jump table (`0x40ef4c`); each phase applies a per-phase vertical rate derived
  from UpRate/DownRate and steps the phase on a timer (`+dt` accumulators at
  `+0x618` height, `+0x628` timer). The arc gate compares the height
  accumulator against `1.0 / MaxVertVelocity` (const `1.0` @ `0x48d924`).
- `NewGravity=0` is the **initial phase/state value**, not an acceleration — the
  `mov [player+0x630], 2..8` writes are the phase machine, which is why the
  `.gam` value is 0.
- Phases tie to the **HIJUMP→jimjump.ase / HIFALL→jimfall.ase** animation
  triggers (`Neutron.exe:0x422b7e`), so jump/fall *animation* is driven by the
  *physics phase* — animation and physics are coupled in the original.

**Param→field offsets** (from `C3DPlayer::InitObject` @ `0x419f..`, for the
.gam-parse implementation): UpRate→`+0x624`, DownRate→`+0x628`,
MaxVertVelocity→`+0x62c`, NewGravity/phase→`+0x630` (`+0x658` = live jump phase).

So physics becomes: **parse the real constants from `.gam`** (decided: runtime
parse) + **replicate the phase-based vertical arc**. The D3D capture is demoted
to a *validator* (does the arc match recorded motion?), not the source.
Fully reversing all 9 phase handlers byte-for-byte is Phase-3 work.

## Decisions (locked with user 2026-06-01)
1. **Full directional 1:1** animation (separate strafe/backpedal/run clips).
2. **Physics: parse the real `.gam` constants at runtime** (data-driven, like
   the original) + replicate the phase-based vertical arc. (Superseded the
   earlier "tune vs capture" plan once the constants were found in the binary;
   capture is now a validator.) Confirmed-the-model decision: **decompile
   `C3DPlayer` update first** — done above (phase-based arc, not gravity).
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

### Phase 3 — Movement & physics (data-driven, 1:1 with C3DPlayer)
**Status 2026-06-01: WHOLE TRACK DEFERRED / REVERTED.** The data-driven movement
(accel/decel ramp, .gam speed/jump, body lean) produced ice-skating + wrong
turn/speed feel; reverted `behavior_player.c` to the approved simple tank-turn
(instant velocity, MOVE_SPEED 600, TURN_RATE 2.8, JUMP_VEL 650) per user. The
`.gam` parse (`player_physics.{c,h}` + gam_loader) + `renderer_draw_model_anim_euler`
stay in the tree, dormant, ready to resume. Resume needs the C3DPlayer
phase-handler RE + capture validation (capture rig). Original 3a/3b/3c notes:
- 3a: `src/engine/player_physics.{c,h}` holds the C3DPlayer constants; `gam_loader.c`
  parses them from the `.gam` (verified: "player physics from .gam: MaxSpeed=1400
  Accel=400 Decel=1 Up=650 Down=-650 MaxVert=650 Lean=20/-20").
- 3b: `behavior_player.c` ramps a signed along-heading speed toward MaxSpeed*input
  by AccelRate / DecelRate (kept along the tank-turn heading). Rates applied per
  assumed 30 Hz sim tick (`PLAYER_SIM_HZ`, calibration knob). Jump uses UpRate.
- 3c PARTIAL: **body lean DONE** (2026-06-01) — AccelLean/DecelLean drive pitch
  (fwd/back) + roll (into turns) on the player, applied via new
  `renderer_draw_model_anim_euler` (yaw/pitch/roll); stored in e->rx/rz, smoothed.
  Sign/magnitude QA-tunable. STILL TODO: phase-based vertical jump arc (not
  accel-gravity; needs Neutron.exe phase-handler RE + capture validation, routed
  to the capture rig).

### Phase 4 — Contextual states (Level-1 only)  — DONE (2026-06-01)
- Added PA_SWING (jimswing) + PA_LADDER (jimladder) to the player anim system
  (player_anim.{c,h}, loop-classified, clips load). Level 1's only contextual
  prop is the swing (3SWN x2; no ladder FourCC -> PA_LADDER dormant).
- `behavior_player.c`: `player_near_swing()` scans the world for a 3SWN within
  SWING_RADIUS (400); when the player is otherwise idle near a swing it plays
  PA_SWING. Verified clips load + the idle->swing path renders.

- **Parse the C3DPlayer param block** from the `.gam` in `gam_loader.c`:
  MaxSpeed, AccelRate, DecelRate, MaxHeight, UpRate, DownRate, MaxVertVelocity,
  AccelLean, DecelLean → a `PlayerPhysics` struct on the player entity. (BE
  float key/value table; same parse already proven by the extraction script.)
- **Ground movement:** replace instant-velocity with accel toward MaxSpeed by
  AccelRate, decel by DecelRate (the low DecelRate=1 gives the original's long
  coast). Apply AccelLean as a visual body-lean.
- **Vertical:** replicate the phase-based jump arc (`Neutron.exe:0x40ed40`,
  phases 0–8) using UpRate/DownRate/MaxVertVelocity — NOT accel-gravity. Couple
  the jump phase to the jump/fall animation (HIJUMP/HIFALL → jimjump/jimfall),
  matching the original's physics↔anim coupling.
- **Validator:** compare the resulting arc/speed against Jimmy's per-frame world
  position in the Level-1 `.omtc` (the `CAPTURE_LEVEL1_JIMMY_MODEL` path exists)
  — confirmation, not the source.
- **Gate:** side-by-side native vs capture motion; speed/jump-apex within tol.
- *Open RE task:* fully decode the 9 phase handlers (`0x40eddc`..`0x40eed8`) for
  exact per-phase rates; current understanding is the arc shape + the constants.

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
