# Replay v4 Visual Followups

Fresh-session plan written after the first successful Level 1 v4 recapture.

## Baseline

- Canonical v4 stream: `build/level1_v4.omtc`
- Valid Level 1 mark: `0xbeef`
- Mark location: frame `20741`, seq `71264`
- Extracted replay frame: `build/frame_v4.omtc`
- Current replay screenshot: `build/frame_v4.png`
- Follow-up recapture handoff: `docs/replay_v4_recapture_next_session.md`

The v4 capture is a major improvement over the v3 replay. The white rectangle
seen in the screenshot is expected building geometry, not an alpha/keying
failure. Texture pixels, texture format records, and color-key records are all
present in the extracted frame.

Known remaining visual issues:

- Skybox texture content is wrong: some day skybox is visible, but space skybox
  content also appears.
- Ground texture content is wrong, especially the out-of-bounds blue/black
  striping.
- Stream water remains wrong or incomplete.
- Some object textures are stale or replaced, for example hedges showing
  neutron gauge counter digits.
- Tree trunks may still need texture/material verification.

## Follow-up Status From 2026-05-25

Material handling was a real replay-side bug. The captured lit passes had
`LIGHTING=1`, black vertex diffuse, and white material emissive. Replay now
uses captured material color for lighting-enabled draws instead of multiplying
those passes by black vertex diffuse. This made ground and sky content visible.

Replay also gained zero-alpha discard for captured alpha textures, which removed
magenta halos around giant number textures and improved cutout artifacts.

The remaining ground, skybox, water, and hedge-number artifacts appear to be
stale texture pixel capture rather than purely replay material state. The proxy
previously captured texture pixels only on first `SetTexture`; it did not
re-dump already-known texture surfaces after content mutations. The proxy now
refreshes already-known texture surfaces after:

- `IDirect3DDevice7::Load`
- `IDirectDrawSurface7::Blt`
- `IDirectDrawSurface7::BltFast`
- `IDirectDrawSurface7::Unlock`

A new recapture is required because `build/frame_v4.omtc` already contains the
old stale first-sighting texture pixels.

## Verification Already Completed

The extracted frame contains:

- `256` `TEXTURE_DEF` records
- `256` `TEXTURE_PIXELS` records
- `256` `TEXTURE_FORMAT` records
- `1024` `TEXTURE_COLORKEY` records

All texture formats in the extracted frame reported the same 32-bit ARGB masks:

- R: `0x00ff0000`
- G: `0x0000ff00`
- B: `0x000000ff`
- A: `0xff000000`

All captured color-key records had `active=0`. Treat that as data from the
capture, not as proof of a bug by itself.

Replay command used:

```sh
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_REPLAY=build/frame_v4.omtc \
  JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

Replay result:

- `3503` GL draws
- `256` registered textures
- `screenshot.png` written at `1280x720`

## Plan

### 1. Preserve The Baseline

Keep `build/level1_v4.omtc`, `build/frame_v4.omtc`, and `build/frame_v4.png`
as the comparison baseline. Do not overwrite `build/frame_v4.png` when running
new replays; save new outputs with descriptive names.

### 2. Classify Problem Draws

Completed with `instrument/diff/inspect_replay_v4.py`. The tool dumps, per
draw:

- draw index
- bound texture IDs per stage
- render states
- texture stage states
- material
- transform presence
- vertex bounds
- diffuse color range

Use geometry and state to group likely draws:

- Ground: large low-elevation geometry with broad X/Z coverage.
- Water: low-elevation flat strips, likely translucent or multi-pass.
- Skybox: very large or far geometry, unusual depth/write/cull state.
- Tree trunks: vertical narrow geometry with bark-like texture or diffuse.

### 3. Extract Texture Contact Sheets

Completed with `instrument/diff/inspect_replay_v4.py`. It dumps captured
textures from a replay frame as PNGs using the v4 masks and generates metadata
plus a contact sheet with:

- texture ID
- dimensions
- format masks
- SHA-1
- alpha range
- dominant colors
- color-key records

Use this to identify likely sky, ground, bark, and water textures.

Current v4 inspection output lives under `build/replay_v4_inspect_fast2`.

### 4. Debug Ground Textures

Find the draws that produce the blue/black striped ground. Determine whether
the bound texture pixels are wrong or replay state is wrong.

Inspect these texture stage and sampler-related states around the suspect
draws:

- color op / color arg1 / color arg2
- alpha op / alpha args
- texture coordinate index
- address U/V
- min/mag/mip filters
- texture transform flags

Likely failure classes are missing texture wrapping/clamping, missing texture
coordinate transform handling, or incomplete multi-texture combine behavior.

### 5. Debug Missing Stream Water

Determine whether water is:

- drawn as textured geometry,
- drawn as translucent geometry,
- drawn as a multi-pass blend,
- drawn through a DirectDraw blit/overlay path,
- or missing from capture entirely.

Search frame draws for water candidates:

- flat geometry at stream elevation
- blue/cyan diffuse colors
- non-opaque vertex alpha
- alpha blending enabled
- water-like texture IDs

Inspect these render states:

- `D3DRENDERSTATE_ALPHABLENDENABLE`
- `D3DRENDERSTATE_SRCBLEND`
- `D3DRENDERSTATE_DESTBLEND`
- `D3DRENDERSTATE_ALPHATESTENABLE`
- `D3DRENDERSTATE_ZWRITEENABLE`
- `D3DRENDERSTATE_TEXTUREFACTOR`
- fog states

If water draws exist but are invisible, fix the replay alpha/blend/state
behavior. If water draws do not exist, extend proxy logging around missing draw
paths, blits, or surface operations.

### 6. Debug Skybox

Identify skybox draws and bound textures. Confirm whether skybox texture pixels
exist in the capture.

Check render state around candidate draws:

- z-enable
- z-write
- culling
- lighting
- fog
- texture addressing

If skybox draws are absent, inspect whether the proxy misses a draw path such
as indexed draws, strided draws, blits, clears, or background surface updates.
If draws are present but render incorrectly, focus on replay state handling.

### 7. Debug Tree Trunks

Identify trunk draws by geometry and texture IDs. Compare captured texture
pixels against expected bark colors.

Check whether the wrong color comes from:

- texture decode
- diffuse vertex color modulation
- material state
- lighting state
- texture-stage combine state

### 8. Add Temporary Replay Debug Toggles

Completed in `src/engine/replay.c`. Temporary environment-gated diagnostics:

- `JN_REPLAY_ONLY_TEX`
- `JN_REPLAY_DRAW_START`
- `JN_REPLAY_DRAW_END`
- `JN_REPLAY_HIGHLIGHT_TEX`
- `JN_REPLAY_DISABLE_BLEND`
- `JN_REPLAY_FLAT_GROUPS`

Do not turn these into broad replay heuristics. The final replay path should
remain faithful to captured D3D/DDraw state.

### 9. Fix Order

Recommended priority:

1. Ground texture, because it dominates the frame and likely exposes texture
   stage/addressing bugs.
2. Water, because it may require blend or alpha state support.
3. Skybox, because it may expose missing draw/state coverage.
4. Tree trunks, likely texture modulation, lighting, or material behavior.

After each fix, replay `build/frame_v4.omtc` and save a named output, for
example:

- `build/frame_v4_ground_fix.png`
- `build/frame_v4_water_fix.png`
- `build/frame_v4_skybox_fix.png`
- `build/frame_v4_trunks_fix.png`

## Acceptance Criteria

- Ground no longer shows blue/black striping and matches expected terrain/road
  surfaces.
- Stream water is visible in the correct location with plausible
  transparency/color.
- Skybox renders instead of black or missing background.
- Tree trunks render with expected bark coloring.
- Jimmy, buildings, UI, and already-improved environment textures do not
  regress.
