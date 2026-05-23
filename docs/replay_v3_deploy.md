# Deploying the v3 proxy (pixel-faithful replay)

The v3 OMTC protocol adds `TEXTURE_PIXELS` records — the proxy now emits the
raw locked-surface bytes once per texture, alongside the existing SHA-only
`TEXTURE_DEF`. The replay engine consumes them and uploads pixel-exact
textures (replacing the heuristic PNG sidecar). All code lands; this doc is
the deploy + recapture procedure (requires XP coordination).

## What changed
- `instrument/proxy/protocol.h` — bumped to `OMTC_VERSION = 3`; new record
  `OMTC_RECORD_TYPE_TEXTURE_PIXELS = 14` with header
  `{tex_id u32, w u16, h u16, bpp u32, pixel_bytes u32}` + packed payload.
- `instrument/proxy/capture.c` — `omtc_register_texture` now emits a packed
  `TEXTURE_PIXELS` record after the `TEXTURE_DEF` (one-shot per texture,
  capped at 1 MB per texture). Already rebuilt + verified XP-safe.
- `instrument/receiver/protocol.py` — mirrors v3 (`TexturePixels` NamedTuple,
  version bump). Receiver saves raw bytes regardless of decoder coverage.
- `instrument/diff/extract_frame_capture.py` — passes the latest
  `TEXTURE_PIXELS` per texture into the per-frame self-contained .omtc.
- `src/engine/replay.c` — new `REC_TEXTURE_PIXELS` handler uploads pixels as
  a GL texture (D3D A8R8G8B8 → `GL_BGRA / GL_UNSIGNED_BYTE`, R8G8B8 → BGR),
  overriding the sidecar PNG fallback when pixels are present.

## Deploy + recapture (when convenient)

Requires the XP machine + receiver up.

```bash
# 1. Deploy the new ddraw.dll to the JNBG install on XP.
cd ~/jn-engine
python3 instrument/deploy_xp.py                 # copies proxy/ddraw.dll across

# 2. Start a fresh receiver (Debian).
python3 instrument/receiver/receive.py serve --out build/level1_v3.omtc

# 3. Launch the game (use --launch on deploy_xp.py or via VNC). Walk into
#    Level 1 enough that the texture set the replay will use has all been
#    bound (a few seconds in-level is enough; TEXTURE_PIXELS is one-shot
#    per texture, lazy). Optionally mark a frame to pin a target:
#       python3 instrument/receiver/receive.py mark 0xface1   # in a second terminal
#    then exit.

# 4. Pin the marked frame index F, then extract a self-contained replay:
python3 instrument/diff/scan_mark.py build/level1_v3.omtc 0xface1
python3 instrument/diff/extract_frame_capture.py build/level1_v3.omtc \
        --frame F --out build/frame_v3.omtc

# 5. Replay with pixel-exact textures (no sidecar needed):
JN_REPLAY=build/frame_v3.omtc JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

The replay engine still honors `JN_REPLAY_TEX_MAP` as a fallback for
textures not in the v3 stream (e.g. partial captures), but a complete v3
capture renders entirely from in-stream pixels.

## Expected result

`screenshot.png` will be the original Level-1 frame rendered with the
original's exact textures — pixel-faithful, no parameter tuning, no PNG
matching, no per-mesh material RE. That validates the rethink end to end.

## Rollback

If anything misbehaves on XP:
```bash
python3 instrument/deploy_xp.py --revert    # restore stock ddraw.dll
```
Or arm the kill-switch:
```bash
python3 instrument/deploy_xp.py --disable   # proxy becomes pure pass-through
```

## Storage / bandwidth

A full Level-1 texture set (~256 textures, mostly 32 bpp) totals ≤ 64 MB of
pixel payload — added once per fresh capture. The existing 4 GB stream is
geometry/state; adding texture pixels is a rounding error.

## Backward compatibility

- The receiver accepts v1/v2/v3 streams (range gate unchanged).
- The replay engine handles streams with OR without `TEXTURE_PIXELS`;
  textures without pixel data fall back to the sidecar PNG or to white.
- The existing 4 GB `build/level1_session.omtc` (v2) keeps working with the
  PNG-sidecar path.
