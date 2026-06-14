# jn-engine — Architecture & Codebase Ground-Truth

*What is actually in the tree right now, how the pieces fit, and — critically —
which pieces are **current** vs. **historical dead-ends** you should not build on.*

**Read [`PROJECT_HISTORY.md`](./PROJECT_HISTORY.md) first** for *why* the code is
shaped this way. This document is the *what*. Where the two disagree, the code wins —
tell us and we'll fix the doc.

Legend used throughout:
- 🟢 **CURRENT** — live, load-bearing, safe to extend.
- 🟡 **SUPPORT** — tooling/validation, used occasionally but not in the hot path.
- 🔴 **HISTORICAL** — kept for reference/reproducibility; **do not build new work on
  it.** Usually superseded by something in the 🟢 list.

---

## 1. The 10,000-foot view

`jn-engine` is a clean-room C reimplementation of the **Open Media Toolkit 2.0**
engine that ran *Jimmy Neutron: Boy Genius* (2002), plus the toolchain that
**captures the original game's Direct3D 7 output on real Windows XP** to use as
ground truth. One codebase produces:

- a **native Linux** binary (`jnengine`, SDL2 + OpenGL),
- a **WebAssembly** build (Emscripten + WebGL2) — the public browser demo,
- and, from the *same* source, a **D3D7-replay** renderer and a **capture** build.

The same engine also runs the sequel, **JNvsJN** (different asset stack: Granny
`.grn` instead of OMT).

### The single most important mental model: three runtime modes

`src/game/main.c` is one process that behaves as one of three things depending on
environment:

| Mode | Trigger | What it does | Code |
|---|---|---|---|
| 🟢 **Native game** | default | Loads a `.gam` level, builds the entity world, runs the 60 Hz fixed-step game loop, renders via the GL renderer. This is the *product*. | `main.c` main loop |
| 🟢 **Replay** | `JN_REPLAY=<file.omtc>` | Skips game logic entirely; walks a captured D3D7 command stream and translates it to GL. The *fidelity oracle*. | `src/engine/replay.c` |
| 🟡 **Capture** | built with `make capture`, run with `JN_CAPTURE=<out.omtc>` | Native game loop **plus** emits its own draws in the OMTC wire format, so jn-engine output can be diffed against the original game's capture. | `src/engine/capture.c` |

Replay is gated *before* world setup (`main.c` ~L517); capture is woven into the
normal loop via no-op-able hooks (`capture_begin_frame` / `capture_draw` /
`capture_end_frame`).

---

## 2. Repo map

```
jn-engine/
├── src/
│   ├── engine/            🟢 platform + renderer + the two capture/replay paths
│   │   └── assets/        🟢 on-disk format loaders + asset cache
│   └── game/              🟢 entity model, game loop orchestration, gameplay
│       └── behaviors/     🟢 per-FourCC entity vtables
├── instrument/            the capture/ground-truth pipeline (mostly runs on/against XP)
│   ├── proxy/             🟢 ddraw.dll — D3D7 interception proxy (the breakthrough tool)
│   ├── granny_proxy/      🟢 granny.dll — same trick for JNvsJN's Granny meshes
│   ├── receiver/          🟢 receive.py — decodes the live capture stream to .omtc
│   └── diff/              🟡 native-vs-capture diff, static OMT reader (track0), extractors
├── tools/                 🟡 parsers, exporters, level builders, validators, deploy scripts
├── assets/                game data: source formats + derived .glb/.png + capture fixtures
├── web/                   🟢 WASM shell + built browser bundles (game-1 and jnvsjn)
├── docs/                  this folder — see §11 for which docs are live
├── build/                 untracked: large .omtc captures + generated inspect output
└── Makefile               🟢 the whole build/validation matrix
```

---

## 3. Engine layer — `src/engine/`

### 3.1 Platform & loop glue 🟢
- **`window.c`** — SDL2 window + GL context (`Window` struct: `sdl_win`, `width`,
  `height`, `should_quit`).
- **`input.c`** — keyboard + the *virtual* input API the WASM build drives from JS
  (`input_set_virtual_move/fly`, `input_press_virtual_jump`, `input_toggle_noclip`,
  `input_toggle_turbo`). These are the `EXPORTED_FUNCTIONS` in the web build.
- **`audio.c`** — SDL2_mixer; `audio_play(slot)`.

### 3.2 Renderer 🟢 — `renderer.c` / `renderer.h` (~1000 LOC, the heart)
A small immediate-ish GL 3.3 / WebGL2 renderer. Public surface (see `renderer.h`):
- **Camera:** `Camera` struct (pos/yaw/pitch/fov/near/far); `renderer_camera()`,
  `renderer_begin_frame()` computes view·proj. `renderer_set_camera_override(view,
  proj)` installs an explicit GL column-major matrix pair (M7a — used so the
  native-vs-capture validators render from the *exact* captured camera);
  `renderer_camera_override_active()` gates the follow cam and HUD.
- **Mesh draws:** `renderer_draw_model[_matrix][_anim][_euler]` — `texture_id=0`
  means "use the model's bound texture." Anim variants take `frame_a/frame_b/lerp`
  for ASE keyframe blending.
- **Sky:** `renderer_set_sky(top,bot)` gradient; `renderer_draw_cloud_dome(tex,spin)`
  (rotating cloud hemisphere) + `renderer_draw_sky_dome(model,spin)` (faithful
  `bluesky3` painted backdrop).
- **Billboards:** `renderer_draw_billboard(...)` camera-facing alpha-tested quad
  (trees, pickups, sprite objects); `renderer_set_billboard_uv_flip_y()` matches the
  capture's FVF152 vertical convention.
- **HUD:** `renderer_draw_sprite_2d` / `renderer_draw_screen_rect` — ortho, depth
  off, blended; called after the 3D scene.
- **Faithfulness switches** (each measured, see history Era 3/5):
  `renderer_set_scene_tint`, `renderer_set_hide_untextured_groups` (skip
  collision-only / unresolved canvas slots so they don't draw as gray slabs),
  `renderer_set_alpha_cutout` (Phase 4 shader `discard`), `renderer_set_color_key`
  (punch baked sky-blue out of the `2D_Trees` boundary walls).

### 3.3 Replay 🟢 — `replay.c` / `replay.h` (~1100 LOC)
The faithful path. `replay_active()` checks `JN_REPLAY`; `replay_init()` loads a
**single-frame self-contained `.omtc`** (produced by
`instrument/diff/extract_frame_capture.py`) and sets up GL; `replay_render_frame()`
re-renders it. Translates **D3D7 → GL**: column-major/column-vector matrices, FVF
`0x152` vertices (pos·normal·diffuse·uv, 36 B), render states, and the D3D Z-range
`[0,1] → [-1,1]` remap in-shader. Textures come either from an in-stream
`TEXTURE_PIXELS` payload (v3+) or a PNG sidecar map. **Invariant:** forces opaque
fragment alpha (D3D DIFFUSE alpha is commonly 0 → silhouettes on X compositors).

### 3.4 Capture 🟡 — `capture.c` / `capture.h`
Demo-side emitter. Compiled in **only** under `-DJN_CAPTURE` (else every entry point
is a no-op macro, so call sites in `renderer.c` / `main.c` need no `#ifdef`). Emits
the **same OMTC wire protocol** as the proxy (`instrument/proxy/protocol.h`) so the
two engines produce directly comparable `.omtc`. Transposes jn-engine's GL
column-major matrices to the protocol's D3D row-major layout. Knobs: `JN_CAPTURE`
(path), `JN_CAPTURE_FRAMES`, `JN_CAPTURE_CAMERA`.

### 3.5 World, physics, ground 🟢
- **`world.c` / `world.h`** — the `Entity` struct (FourCC `type`, `tag`, position/
  velocity/AABB, GRN animation filename slots, sprite index, vtable ptr, model ptr,
  intrusive `next`) and the `World` (entity linked list + `WorldPlacement[]` static
  geometry + a **safety floor** AABB). `world_query_segment()` is the ray test used
  by camera collision. `world_box_*` is the shared placeholder-box VAO.
- **`physics.c`** — gravity, AABB collision, trigger overlap, `physics_step(world,
  dt)`.
- **`player_physics.c`** — data-driven player movement constants parsed from
  `Level1.gam`'s `C3DPlayer` (MaxSpeed/Accel/Decel/UpRate…). (History Era 9.)
- **`ground.c`** — the **safety floor** tile (`ground_init`/`ground_draw`). Note: this
  is a *synthetic* fallback floor, **not** game geometry — the "green slab" bug in
  Era 8 was this tile. Real terrain comes from OMT placements.

### 3.6 Measured-constant headers 🟢 (generated, do not hand-edit)
- **`canon_data.h`** — Phase-12 measured ground footprint/topography + `CANON_GROUND_TEXTURE`.
- **`phase1_sky_tint.h`** — keyframe-8881 sky + scene tint.
- **`phase4_capture_state.h`** — measured alpha/blend/fog render state.

These are emitted by `tools/` samplers (`sample_phase1_sky_tint.py`, etc.) and the
`instrument/diff/gen_canon_header.py` generator. Treat as build artifacts.

### 3.7 Vendored libraries 🔵 (third-party, don't modify)
`glad.c/.h` (GL loader, native only — filtered out of the web build), `cgltf.h`
(glTF parsing), `stb_image.h` (PNG decode).

---

## 4. Asset loaders — `src/engine/assets/`

This is the data path: **on-disk game formats → in-memory models/textures → cache.**

| File | Role | Status |
|---|---|---|
| `gam_loader.c` | Parses `Level*.gam` → spawns `Entity`s (FourCC, tag, properties, GRN/ASE/PNG refs, sprite index). The level *is* a `.gam`. | 🟢 |
| `gltf_loader.c` | Loads `.glb` static meshes via cgltf. **The current mesh path.** | 🟢 |
| `ase_loader.c` | Loads Autodesk `.ASE` meshes (multi-material, canvas-resolved UVs). | 🟡 legacy meshes + Jimmy poses still ASE; static level geometry moved to glTF |
| `tex_loader.c` | PNG load (stb_image) + texture cache; `tex_cache_resolve_bmp` maps a baked `.bmp` ref to the real PNG. | 🟢 |
| `asset_cache.c` | Model + texture cache with **per-level lifecycle**: `asset_cache_begin_level()`, `asset_cache_purge_stale()` across level swaps, `asset_cache_destroy_all()`. | 🟢 |
| `placement_loader.c` | Loads `*_placements.txt` → `WorldPlacement[]` (the OMT static-geometry layout for a level). | 🟢 |
| `billboard_overrides.c` | Loads `*_billboard_overrides.txt` — capture-measured billboard sizes/textures (only ~12 trees were in frame 8881; the rest fall back to a measured median). | 🟡 |

**Asset flow today (🟢 path):**
```
Level1.gam ──gam_loader──> entities ──┐
level1.omt ──(tools/omt→glb export)──> assets/glb/omt/*.glb ──gltf_loader──┐
                                                                            ├─> entity_visual resolver ─> renderer
PNG textures (OMT canvas-resolved) ──tex_loader──────────────────────────┘
```
The 🔴 dead-end it replaced: per-mesh **texture-override sidecars** + the OMT→ASE
exporter feeding raw `.ASE` for static geometry. Retired in Era 8 (glTF) — do not
reintroduce override lists.

---

## 5. Game layer — `src/game/`

### 5.1 Orchestration — `main.c` 🟢
Owns init order (window → renderer → audio → capture → input), the replay
early-exit, level (re)loading + **level swap** draining (button→door world actions
queue a swap via `gamestate`; the loop rebuilds the world outside the entity walk),
the fixed-timestep loop, and the **entity draw-dispatch chain** (this order matters):

1. **Player** → dedicated `player_anim` blend path.
2. **`3ASE`** (generic per-object mesh named by the GAM, JNvsJN) → `assets/ase/jnvsjn/`.
3. **Authored door mesh** (`3SWN`/`3SCD` with `ASEFile`/`PNGFile`) → mixed-case
   original asset lookup, preferring `assets/glb/ase/` twins and applying the
   authored PNG over either mesh source.
4. **Sprite-indexed** (`3PIC`/`3SRO`/`3SPR`/`3ANI` with `sprite_index>0`) → 2D
   sprite billboard from `assets/parsed/sprites/`.
5. **Pre-bound vtable model** (`e->model`).
6. **Resolver** (`entity_visual_resolve`) → sprite billboard / mesh / invisible.
7. **Fallback** → colored placeholder box.

Then **static OMT placements** are drawn (with the tree trunk-mesh + camera-facing
canopy-billboard special case), then the **HUD**.

It's also the home of the many **`JN_*` test/QA env knobs** — see §10.

### 5.2 Entity model & resolver 🟢
- **`entities.c`** — `entity_update()` dispatches to the vtable; `entity_bind_vtables()`
  binds each entity's behavior by FourCC after load and runs `on_spawn`.
- **`entity_visual.c`** — **the resolver** (history Eras 6–7). Tiered lookup:
  1. per-`(FourCC, tag)` override (`TAG_TABLE`),
  2. GRN-stem → glb asset table (`GRN_ASSET_TABLE`, JNvsJN props — prefers a textured
     OMT/glTF twin over the untextured GRN extraction),
  3. JNBG `C3DOmtObj` authored `OmtDatabase`/`OmtIndex` → raw-origin OMT glb shape,
  4. per-FourCC default (`TYPE_TABLE`), with a JNBG exception: an authored
     `sprites.omt` canvas beats a *visible* type default for C3DSprite-family rows
     (`3TRE` cones/trees, `3TAR` targets, `3CHK` flags),
  5. per-instance sprite database fallback,
  6. FourCC marked invisible.
  No match → caller draws the placeholder box. This is where you wire a new
  game object to its mesh/sprite.

### 5.3 Camera, state, HUD, animation 🟢
- **`camera.c`** — `FollowCam` (tank-turn follow, free-look offset, collision via
  `world_query_segment`).
- **`gamestate.c`** — items/objectives, tool inventory, **level-swap requests**,
  respawn, per-level reset. The bridge between behaviors and `main.c`.
- **`hud.c`** — items counter, tool icons, level-clear banner (drawn via
  `renderer_draw_sprite_2d`).
- **`player_anim.c`** — Jimmy pose set + `player_anim_sample()` (frame blend).

### 5.4 Behaviors — `src/game/behaviors/` 🟢
Per-FourCC vtables (`EntityVTable`: `on_spawn`/`on_update`/`on_trigger` + flags
`PHYSICS`/`SOLID`/`TRIGGER`/`PLAYER`). One file each: `behavior_player`,
`behavior_door`, `behavior_button`, `behavior_checkpoint`, `behavior_item`,
`behavior_load` (level transitions), `behavior_movplat` / `behavior_plat` (moving
platforms), `behavior_trig` (triggers), `behavior_default`. This is the gameplay
layer that grew with the JNvsJN playthrough work (history Era 10).

---

## 6. Instrumentation pipeline — `instrument/`

The ground-truth machinery. **The core technique of the whole project** (history
Eras 3–4, 10): intercept the original game's graphics calls on real XP, stream them
out, and use them as truth.

### 6.1 The proxy — `instrument/proxy/` 🟢
A drop-in **`ddraw.dll`** that sits between the original game and Windows. It
captures the live D3D7 command stream and connects *out* over TCP to the receiver
(retries every 1 s, **never blocks the game's render thread** — bounded ring buffer
with whole-frame drops, safe to leave deployed with no receiver). The stock
pass-through DLL is kept as `ddraw_orig.dll`. Build via `build.sh`; exports are
generated/forwarded (`gen_def.py`, `fix_forwarders.py`, `gen_wrappers.py`).

> **Hard-won rule:** verify every XP redeploy by **download-back SHA-1** (XP has no
> `certutil`); forward **all** unimplemented exports at exact ordinals; run pure
> pass-through before adding behavior. See `CLAUDE.md` operating rules.

### 6.2 The wire protocol — `instrument/proxy/protocol.h` 🟢
Shared C/Python schema, little-endian, **OMTC** magic, **versions 1–4**:
- **v1** — base record stream (frames, transforms, textures, render/stage state,
  lights, materials, `DRAW_PRIMITIVE`). (`m5_session.omtc`, 621 MB.)
- **v2** — `FRAME_MARK` + receiver→proxy command channel (`OMTC_CMD_*`: camera delta,
  mark frame, capture start/stop, redump textures, killswitch).
- **v3** — `TEXTURE_PIXELS`: one-shot raw locked-surface bytes per texture →
  **pixel-exact replay with no PNG matching.**
- **v4** — `TEXTURE_FORMAT` (DDPIXELFORMAT masks) + `TEXTURE_COLORKEY` (DirectDraw
  color key), to decode packed surface bits + alpha faithfully.

Note the FVF expectation: `0x152` (XYZ·NORMAL·DIFFUSE·TEX1). OMT2 only ever calls
`DrawPrimitive` (not indexed).

### 6.3 The receiver — `instrument/receiver/receive.py` 🟢
`serve` mode listens for the proxy's outbound connection, decodes the stream into
frames, and saves the raw `.omtc`. `--file` re-parses a saved session offline.
Resolves texture SHA-1s against the original PNG set when possible (raw surface
memory rarely hashes identically, so unmatched textures get stable synthetic names).

> **Operating rule:** start with `PYTHONUNBUFFERED=1 python3 -u`, drive `mark`/`stop`
> via a FIFO/real stdin, and **restart after every game disconnect** (it exits on
> proxy disconnect). Verify `stop` by `.omtc` byte-growth, not log granularity.

### 6.4 The diff / analysis tools — `instrument/diff/` 🟡
The reconciliation toolbox behind history Eras 5 & 7:
- **`extract_frame_capture.py`** — reduce a multi-GB capture to one self-contained
  frame (feeds replay).
- **`diff.py`** — compare jn-engine capture vs original capture.
- **`track0_*.py`** — the **static OMT reader** (Era 7 breakthrough): builds a static
  mesh→canvas map, validates it against the capture oracle (~94%), resolves textures
  deterministically. **This is the current texture-resolution source of truth.**
- **`extract_texture_groundtruth.py`**, `match_textures.py`, `build_replay_texmap.py`,
  `inject_pixels_v3.py` (validate v3 locally with PNG-injected pixels),
  `extract_camera.py` / `extract_canon.py` / `gen_canon_header.py`.

### 6.5 Granny proxy — `instrument/granny_proxy/` 🟢
The same proxy trick re-applied to **JNvsJN**, which uses **Granny 3D** (`granny.dll`)
instead of OMT. Dumps `.grn` mesh/skin data; `grnmesh_to_glb.py` / `grnmesh_to_obj.py`
/ `grntex_to_png.py` convert it. Type-tree/skinning + UVs are the deferred frontier.

---

## 7. Tools — `tools/` 🟡

Standalone Python (and a couple of shell) utilities. Grouped by job:
- **Format parsers:** `ase_parser.py`, `gam_parser.py`/`gam_probe.py`,
  `omt_parser.py`, `tsk_parser.py`, `grn_mesh.py`/`grn_probe.py`.
- **Exporters:** `omt_mesh_export.py` (OMT→ASE, 🔴 legacy), `grn_to_glb.py`.
  *(OMT→glTF export lives with the gltf plan; see `docs/gltf_export_plan.md`.)*
- **Level building:** `build_native_level1_map.py`, `build_native_keyframe_cameras.py`,
  `build_all_levels.sh`, `build_hybrid_level1_manifest.py` (🔴 hybrid path).
- **Native-vs-capture validation:** `diff_native_capture_keyframe.py`,
  `validate_native_keyframe_alignment.py`, `validate_native_level1_map.py`,
  `build_native_capture_side_by_side.py`, `solve_keyframe_views.py`.
- **Capture-backed validators:** `validate_capture_backed_{static,live_jimmy,live_hud,multiframe}.py`,
  `validate_replay_fixture.py`.
- **Galleries / deploy:** `gen_asset_galleries.py`, `deploy_wasm.sh`,
  `deploy_jnvsjn_web.sh`, `stage_jnvsjn_web.sh`, `deploy_grn_catalog.sh`.
- **`vnccap.py`** — screenshot-only capture of the XP session (**cannot inject
  input** — a human pilots the game via the XP noVNC desktop).
- **`tools/contrib_awefan/`** — the self-contained proof-of-concept handed to
  external contributors (`poc_level1.py`, `render_hydrant.py`, `omt_parse.py`).

---

## 8. Assets — `assets/`

Source formats and everything derived from them:
- `gam/` — `Level*.gam` (level definitions). `omt/` — OMT meshes. `ase/` — ASE
  meshes (incl. `ase/jnvsjn/`). `glb/` — **current** exported meshes (`glb/omt/`,
  `glb/grn/`, `glb/grn_capture/`, `glb/sky/`). `png/`, `textures/`, `hud/` —
  textures. `parsed/sprites/` — sprite-index PNGs. `audio/`.
- `native/` — capture-measured overrides (`*_billboard_overrides.txt`,
  `keyframe_cameras/`, `tree_bark.png`, `billboard_textures/`).
- `capture/` — committed replay-fixture manifests (the large `.omtc` themselves live
  untracked under `build/`).
- `hybrid/`, `meshes/`, `levels/`, `exe/` — mixed; `hybrid/` is 🔴 (superseded path).

---

## 9. Build & run — the Makefile

Toolchain: **`zig cc`** cross-target, vendored **static SDL2/SDL2_mixer** under
`~/sdl2`, X11 from a local toolchain. Key targets:

| Target | Produces |
|---|---|
| `make` | 🟢 `jnengine` native binary |
| `make capture` | 🟡 `jnengine` with `-DJN_CAPTURE` (clean rebuild; run with `JN_CAPTURE=<out.omtc>`) |
| `make web` | 🟢 `web/jnengine.{html,js,wasm,data}` (Emscripten, `FULL_ES3`, WebGL2, `ASYNCIFY`, preloads `assets/`) |
| `make web-jnvsjn` | 🟢 JNvsJN browser bundle (`web/jnvsjn/`, preloads the staged JNvsJN asset tree) |
| `make native-level1` / `…-keyframes` / `diff-native-capture` / `native-vs-capture-8881-review` | 🟡 the native-vs-capture validation chain |
| `make replay-hudfix` / `capture-{static,live-jimmy,live-hud,multiframe}` | 🟡 fixture/capture validators |

> **`make clean` keeps `web/shell.html`** on purpose (a tracked source file) — see the
> Makefile comment. **`pkill -f jnengine` kills your own shell** (cmdline contains
> "jnengine"); use `pkill -x jnengine`.

**Run the native game:** `./jnengine --level level1` (default level is `level1`).
Controls: W/S forward-back, A/D turn, Space jump, Shift run, R respawn, LMB free-look,
Esc quit. Path roots are overridable via `JN_GAM_ROOT`, `JN_PLACEMENTS_ROOT`,
`JN_NATIVE_ROOT`, `JN_PLB_ROOT`.

---

## 10. Environment-variable reference (contributor-facing)

The native binary is heavily env-driven (mostly QA/demo helpers in `main.c`):

**Modes:** `JN_REPLAY` (replay a `.omtc`), `JN_CAPTURE` + `JN_CAPTURE_FRAMES` +
`JN_CAPTURE_CAMERA` (capture build), `JN_SCREENSHOT` + `JN_SCREENSHOT_PATH` +
`JN_SCREENSHOT_WARMUP_TICKS` (headless screenshot).
**Camera:** `JN_NATIVE_LEVEL1_CAMERA` / `JN_NATIVE_LEVEL1_KEYFRAME` (install a matched
capture camera).
**Spawn/demo:** `JN_DEMO_SPAWN`, `JN_DEMO_SPAWN_XYZ="x,y,z"`, `JN_DEMO_AUTORUN`,
`JN_DEMO_MOVE_X/Y`, `JN_DEMO_JUMP[_TICK]`, `JN_TEST_SWAP="level:spawn"`.
**Visual debug:** `JN_DISABLE_HUD`, `JN_DISABLE_TREE_BILLBOARDS`, `JN_DEBUG_DRAW`.
**Object spawners (QA):** `JN_TEST_CHARS`, `JN_TEST_SPRITES`, `JN_TEST_PICKUPS`,
`JN_TEST_GEMS`, `JN_TEST_COV`, `JN_TEST_TOOLS`, `JN_TEST_CLEAR`.

---

## 11. Which docs are live

`docs/` accumulated ~85 files written on the fly. Anchor on these; treat the rest as
phase history (cross-referenced from `PROJECT_HISTORY.md`):

- 🟢 **`PROJECT_HISTORY.md`** (this doc's companion) — the narrative + invariants.
- 🟢 **`omt_rendering_breakthrough.md`** + **`omt_3dsp_format.md`** — current texture/
  mesh resolution truth.
- 🟢 **`gltf_export_plan.md`** — the current asset format path.
- 🟢 **`ghidra_notes.md`** — the OMT2.dll RE map (render entry point etc.).
- 🟢 **`qa_ticket_resolution_workflow.md`** — required workflow for turning
  in-game QA exports into fixes, verification, deploys, and public
  `docs/qa/<ticket>/` resolution-log pages.
- 🟢 **`CONTRIBUTOR_object_capture_plan.md`** / **`CONTRIBUTING_AWEFAN.md`** — on-ramps.
- 🟢 **`claude_code_failure_patterns.md`** — the operating rules (also in `CLAUDE.md`).
- 🔴 **`phase12_*`**, **`replay_v0/v3/v4_*`**, **`native_vs_capture_8881_*`**,
  **`multiframe_world_reproject_*`**, **`hybrid_level1_*`** — *reference evidence* from
  superseded or research tracks. Real and correct *as history*; not the product path.

---

## 12. The invariants you must not break

Repeated from `PROJECT_HISTORY.md` because they bite at the code level:

1. Matrix convention is **column-major / column-vector**; captured **`PROJ[3][3]=1`**
   is the real w-buffer projection — don't "repair" it.
2. The engine does **zero UV flips**; flips happen at *export* (3DSP is DX-convention).
   No X-mirror after the diff fix.
3. **`canvas_id = Canv + 1`**; the static OMT reader (`track0`) is texture truth,
   capture is validator.
4. **D3D7 DIFFUSE alpha is commonly 0** — never `discard` on vertex alpha; force
   opaque fragment alpha for the live window.
5. The capture has **no D3D fog** — don't add fog to fix edges.
6. Some FourCCs (`3NEU`/`3LEA`/`3CON`/`3BAL`/`3RED`) are **billboard sprites, not
   meshes** — their absence from `level1.omt` is expected.
7. Never block the render thread on socket I/O; never read a multi-GB `.omtc` wholly
   into memory (parsing is incremental).

---

*Living document. When you add a subsystem or retire a dead end, update the status
tags here and the matching era in `PROJECT_HISTORY.md`.*
