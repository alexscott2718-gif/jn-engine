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

## 2026-05-24 capture result

The XP v3 path has been exercised with a real capture:

- `instrument/proxy/ddraw.dll` deployed successfully to the XP game directory.
- `build/level1_v3_retry.omtc` is a 702 MB v3 stream from XP.
- The retry stream has all 256 `TEXTURE_PIXELS` payloads in-stream.
- `build/frame6533_v3_retry.omtc` is a 21 MB self-contained replay extracted
  from the retry capture.
- `build/frame6533_v3_retry.png` is the replay screenshot: Retroville with
  Jimmy centered, rendered from in-stream pixels only.

Replay command:

```bash
cd ~/jn-engine
export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib"
unset JN_REPLAY_TEX_MAP
JN_REPLAY=build/frame6533_v3_retry.omtc JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

The replay reported 3557 GL draws and 256 registered textures.

Important caveat: the attempted `mark 0xface1` command did not appear as a
`FRAME_MARK` record in the retry stream. Frame 6533 was chosen by byte-offset
timing near when the mark command was sent. For a future canonical recapture,
verify `scan_mark.py` before discarding the XP session.

## Deploy + recapture (when convenient)

Requires the XP machine + receiver up.

```bash
# 1. Deploy the new ddraw.dll to the JNBG install on XP.
cd ~/jn-engine
python3 instrument/deploy_xp.py                 # copies proxy/ddraw.dll across

# 2. Start a fresh receiver (Debian). Use an interactive terminal/TTY if you
#    intend to type control commands like `mark 0xface1`.
python3 instrument/receiver/receive.py serve --out build/level1_v3.omtc

# 3. Launch the game from the visible XP desktop/VNC. `deploy_xp.py --launch`
#    can start Neutron.exe headless/non-interactively, so prefer manual launch
#    when validating visuals. Walk into Level 1 enough that the texture set
#    the replay will use has all been
#    bound (a few seconds in-level is enough; TEXTURE_PIXELS is one-shot
#    per texture, lazy). Optionally mark a frame to pin a target:
#       mark 0xface1   # type into the live receiver's control prompt
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

If `scan_mark.py` finds no mark, use a lightweight frame/offset scan to pick a
candidate Level-1 frame after all 256 `TEXTURE_PIXELS` records have arrived,
then extract that frame with `extract_frame_capture.py`.

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
