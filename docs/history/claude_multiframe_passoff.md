# Claude Code passoff: multi-frame world reproject

## Read this first

You are in `/home/scotty/jn-engine`.

The previous Claude session implemented the multi-frame capture-backed
Level 1 path, but got stuck on an empty render. Codex fixed that bug and
updated `docs/multiframe_world_reproject_handoff.md`. This file is the
short passoff for what happened, why the earlier debug loop failed, and
what to do next.

Do not revert unrelated dirty files. The worktree already contains many
generated `build/` screenshots, pycache files, and generated gallery docs.
Treat the working tree as authoritative.

## Current status

Multi-frame now passes:

```sh
PYTHONDONTWRITEBYTECODE=1 make capture-multiframe
```

Current metrics:

```text
spawn screenshot   MAE 44.229  RMS 58.170
far screenshot     MAE 41.789  RMS 52.850
far-vs-spawn diff  MAE 33.507  RMS 45.705
capture-backed multi-frame PASS
```

Baseline validators still pass:

```text
make capture-static       MAE 1.023  RMS 4.916    PASS
make capture-live-jimmy   existing measurements    PASS
make capture-live-hud     MAE 2.331  RMS 14.491    PASS
```

The high spawn MAE is expected for now. The current fixture unions baked
eye-space draws from multiple source frames without recovering each
frame's true VIEW matrix. It is useful as a smoke test that geometry
reaches the renderer and changes with movement, not as the final geometric
solution.

## Exact bug that blocked Claude

The empty render was caused by per-frame state reset in `src/game/main.c`.

Multi-frame startup enabled world rendering once:

```c
capture_scene_set_group_use_world(CAPTURE_SCENE_GROUP_STATIC_WORLD, 1);
```

But every render frame then called:

```c
capture_scene_set_group_world_offset(CAPTURE_SCENE_GROUP_STATIC_WORLD,
                                     0.0f, 0.0f, 0.0f);
```

That helper does two things: it sets the offset, and it disables world mode
when the offset is zero. That behavior is correct for the older
`JN_CAPTURE_BACKED_WORLD_PAN=1` path, but wrong for
`JN_CAPTURE_BACKED_MULTIFRAME=1`, where spawn has zero offset but still
must render through runtime `view*proj`.

Codex fixed this by restoring the explicit world-mode flag after the
zero-offset reset:

```c
if (capture_multiframe) {
    capture_scene_set_group_use_world(CAPTURE_SCENE_GROUP_STATIC_WORLD, 1);
}
```

See `src/game/main.c` near the render path after
`capture_scene_set_group_world_offset(..., 0,0,0)`.

## Why the previous debug loop missed it

The earlier session checked the binary data and loader path first:

- vertex stride,
- draw struct layout,
- texture table,
- `JNW1` magic,
- anchor-only fixtures,
- z=0 draw filtering,
- `scene_reproject.bin` copied through the multi-frame path.

Those were reasonable checks, but the missing diagnostic was the final
draw-site predicate in `capture_scene_render()`:

```c
int draw_use_world = (g_reproject || g_world_mode) && d->group < 32 &&
                     g_group_use_world[d->group];
```

The log line `loaded ... world-mode` only proved `g_world_mode == 1`.
It did not prove `g_group_use_world[0] == 1` at draw time. The bug was
that `g_group_use_world[0]` was being reset to zero every frame.

Also, the `scene_reproject.bin` cross-test was misleading:

- `scene_reproject.bin` has valid clip coordinates.
- `scene_world.bin` intentionally zeroes the clip slot.
- If world mode is accidentally disabled, `scene_reproject.bin` still
  renders through clip-space, while `scene_world.bin` disappears.

Transferable rule: when a render feature loads but draws nothing, inspect
the final draw predicate and shader-mode inputs before spending time on
binary layout. For this renderer, log the first few draws' group,
texture lookup result, visibility mask, `g_world_mode`,
`g_group_use_world[group]`, and `draw_use_world`.

## Files added or modified by the multi-frame work

New:

- `tools/extract_world_keyframes.py`
- `tools/build_multiframe_world_fixture.py`
- `tools/validate_capture_backed_multiframe.py`
- `assets/capture/level1_hudfix/keyframes.json`
- `assets/capture/level1_hudfix/scene_world.bin`
- `assets/capture/level1_hudfix/scene_world_summary.json`
- `docs/multiframe_world_reproject_handoff.md`
- this file

Modified:

- `Makefile`
- `src/engine/capture_scene.c`
- `src/engine/capture_scene.h`
- `src/game/main.c`

Do not commit `.omtc` captures or `build/` PNGs unless explicitly asked.

## Next task: per-frame VIEW recovery

The remaining problem is geometric, not loader/render plumbing.

JNBG does not emit `SET_TRANSFORM(VIEW)`. It bakes the camera into every
per-draw WORLD matrix:

```text
WORLD_baked = MODEL_true . VIEW
```

The current `scene_world.bin` stores per-frame post-WORLD coordinates,
which are effectively eye-space for each source frame. Rendering all
keyframes through one projection means projecting source-frame A eye-space
as if it were source-frame B. This can look passable for nearby frames, but
it is not a coherent shared world.

Use `instrument/diff/extract_camera.py` as the source of truth for VIEW
recovery. It already documents and implements the camera solve for one
frame. Refactor it into callable pieces, then apply it per keyframe in
`tools/build_multiframe_world_fixture.py`.

Recommended implementation plan:

1. Refactor `instrument/diff/extract_camera.py` so the core camera solve can
   be called as a function for a frame, without only printing CLI output.
2. For each selected keyframe, solve/recover its VIEW or equivalent inverse
   transform.
3. Convert each frame's baked eye-space/world slot into a shared frame:
   multiply by `inverse(VIEW_frame)` before writing vertices.
4. Keep the current `JNW1` file layout unless there is a clear reason to
   bump it.
5. Rebuild:

   ```sh
   make capture-world-fixture
   ```

6. Validate:

   ```sh
   make
   PYTHONDONTWRITEBYTECODE=1 make capture-static
   PYTHONDONTWRITEBYTECODE=1 make capture-live-jimmy
   PYTHONDONTWRITEBYTECODE=1 make capture-live-hud
   PYTHONDONTWRITEBYTECODE=1 make capture-multiframe
   ```

7. Inspect `build/capture_backed_multiframe_spawn.png` and
   `build/capture_backed_multiframe_far_move.png`. The goal is not just a
   lower MAE; the far view should reveal coherent newly visible geometry in
   the correct relative position.

## After VIEW recovery

Once true shared world coordinates are available:

1. Tighten keyframe selection. The current centroid-of-WORLD-translations
   fingerprint is a workaround. With real camera positions, select by camera
   pose/coverage instead.
2. Add per-draw or per-geometry deduplication. The current fixture stores
   repeated static geometry from every keyframe.
3. Tighten the multi-frame validator thresholds based on the new real
   metrics. Keep `far-vs-spawn` as the primary "new geometry appears"
   signal, but the spawn threshold should become more meaningful after the
   shared-world fix.

## Commands to start from a fresh Claude session

```sh
cd /home/scotty/jn-engine
git status --short
sed -n '1,260p' docs/claude_multiframe_passoff.md
sed -n '1,320p' docs/multiframe_world_reproject_handoff.md
sed -n '1,220p' instrument/diff/extract_camera.py
sed -n '1,340p' tools/build_multiframe_world_fixture.py
sed -n '840,920p' src/game/main.c
PYTHONDONTWRITEBYTECODE=1 make capture-multiframe
```

If debugging render disappearance again, add a temporary log inside
`capture_scene_render()` for the first 10 draws:

```text
i, tex_id, texture_found, group, group_mask, g_reproject, g_world_mode,
g_group_use_world[group], draw_use_world
```

Remove temporary logs before wrapping up.
