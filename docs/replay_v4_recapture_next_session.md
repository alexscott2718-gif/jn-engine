# Replay v4 Texture Refresh Recapture

Fresh-session handoff for the next Level 1 recapture after the replay visual
follow-up work on 2026-05-25.

## Goal

Recapture the marked Level 1 frame with the rebuilt proxy and verify whether
texture refresh hooks fix stale texture content:

- wrong/out-of-bounds ground texture,
- mixed day/space skybox texture content,
- hedge surfaces showing neutron gauge counter digits,
- stream/water surfaces using black or stale texture pages.

The existing `build/frame_v4.omtc` cannot prove or disprove this fix because it
already contains stale first-sighting texture pixels. A new capture is required.

## What Changed Before This Handoff

Replay-side diagnostics and partial fixes:

- `src/engine/replay.c` now has temporary environment-gated debug controls:
  - `JN_REPLAY_DRAW_START`
  - `JN_REPLAY_DRAW_END`
  - `JN_REPLAY_ONLY_TEX`
  - `JN_REPLAY_HIGHLIGHT_TEX`
  - `JN_REPLAY_DISABLE_BLEND`
  - `JN_REPLAY_FLAT_GROUPS`
- Replay now handles captured `SET_MATERIAL` enough to avoid black lit
  material passes. The important observed pattern was `LIGHTING=1`, black
  vertex diffuse, and white material emissive.
- Replay now discards fully-zero-alpha texels from captured alpha textures.
- `instrument/diff/inspect_replay_v4.py` dumps draw state and captured texture
  contact sheets from a self-contained replay frame.

Proxy-side recapture fix:

- `instrument/proxy/com_wrappers.c` now has a shared texture surface dump
  helper.
- `instrument/proxy/capture.c` exposes `omtc_texture_is_known`.
- `instrument/proxy/gen_wrappers.py` wires post-call refresh hooks for:
  - `IDirect3DDevice7::Load`
  - `IDirectDrawSurface7::Blt`
  - `IDirectDrawSurface7::BltFast`
  - `IDirectDrawSurface7::Unlock`
- `instrument/proxy/com_wrappers_gen.inc` was regenerated.
- `instrument/proxy/build.sh` rebuilt and verified `instrument/proxy/ddraw.dll`
  as XP-safe.

## Baseline Artifacts

Keep these unchanged as the stale-pixel baseline:

- `build/level1_v4.omtc`
- `build/frame_v4.omtc`
- `build/frame_v4.png`

Useful comparison screenshots produced during this investigation:

- `build/frame_v4_material_lighting.png`
- `build/frame_v4_alpha_cutout.png`
- `build/frame_v4_texture_refresh_proxy_smoke.png`

Useful inspection output:

- `build/replay_v4_inspect_fast2/draws.csv`
- `build/replay_v4_inspect_fast2/draws.json`
- `build/replay_v4_inspect_fast2/textures.html`
- `build/replay_v4_inspect_fast2/texture_contact_sheet.png`

These build outputs are diagnostic artifacts, not required source inputs.

## Preflight

Build the Linux replay engine:

```sh
make
```

Build the XP proxy:

```sh
cd instrument/proxy
./build.sh
```

Expected proxy build result:

- `ddraw.dll` is PE32.
- Export table has 22 total exports.
- `DirectDrawCreateEx @11` and `DirectDrawEnumerateA @12` are implemented.
- 20 exports forward to `ddraw_orig`.
- Imports are only `KERNEL32.dll`, `USER32.dll`, and `WS2_32.dll`.
- No UCRT dependency.

## Recapture Steps

Use the same working Level 1 mark flow as the v4 baseline:

- valid mark tag: `0xbeef`
- previous mark location: frame `20741`, seq `71264`
- receiver command sequence should still use capture start, mark frame, and
  texture redump as needed.

Deploy the rebuilt proxy DLL to the XP game directory, replacing the old proxy
`ddraw.dll` and preserving the real DirectDraw DLL as `ddraw_orig.dll`.

Start the receiver:

```sh
python3 instrument/receiver/receive.py serve
```

Run the game on XP, reach Level 1, and send the same mark command when the
camera is at the target comparison view. Save the new full stream with a name
that does not overwrite v4, for example:

```text
build/level1_v4_refresh.omtc
```

Extract the marked frame into a new self-contained replay frame:

```sh
python3 instrument/diff/scan_mark.py build/level1_v4_refresh.omtc
python3 instrument/diff/extract_frame_capture.py \
  build/level1_v4_refresh.omtc \
  --frame <marked-frame-index> \
  --out build/frame_v4_refresh.omtc
```

## Verification Commands

Replay the new frame:

```sh
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_REPLAY=build/frame_v4_refresh.omtc \
  JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
cp screenshot.png build/frame_v4_refresh.png
```

Inspect the new frame:

```sh
python3 instrument/diff/inspect_replay_v4.py \
  build/frame_v4_refresh.omtc \
  --out-dir build/replay_v4_refresh_inspect
```

Compare these against the stale v4 inspection:

- texture IDs used by ground draws,
- texture IDs used by skybox-like draws,
- textures used by hedge/tree draws,
- water-like draw texture pixels and alpha ranges,
- count of repeated `TEXTURE_PIXELS` records.

The refreshed capture may contain more than 256 `TEXTURE_PIXELS` records
because the proxy now re-emits pixels for known surfaces after mutation calls.
That is expected. The extractor stores the latest pixel payload per texture ID
in its prelude.

## Expected Outcome

Pass conditions for the new capture:

- Ground no longer shows the out-of-bounds black/cyan striping.
- Day skybox does not include space skybox texture content.
- Hedges do not show neutron gauge counter digits.
- Water texture pixels are not all-black stale pages.
- The replay still issues roughly the same draw count as v4 and still registers
  the expected texture population.

If these artifacts persist after recapture, the next suspects are:

- a texture mutation path not yet hooked (`Lock` without `Unlock`, DC updates,
  palette changes, overlay/update paths),
- texture ID reuse requiring generation IDs instead of pointer-only IDs,
- missing texture stage combine or texture transform state.
