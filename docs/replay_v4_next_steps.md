# Replay v4 Next Steps: Pixel Format and Color Key Validation

## Current state

The faithful replay capture protocol has been extended beyond v3 texture
pixels. v4 now carries enough DirectDraw texture metadata for replay to decode
captured surface bytes more faithfully:

- `instrument/proxy/protocol.h` bumps `OMTC_VERSION` to `4`.
- New record `TEXTURE_FORMAT` carries DDPIXELFORMAT fields:
  - `dwFlags`
  - `dwRGBBitCount`
  - `dwRBitMask`
  - `dwGBitMask`
  - `dwBBitMask`
  - `dwRGBAlphaBitMask`
- New record `TEXTURE_COLORKEY` carries DirectDraw color-key state:
  - texture/surface id
  - DDCKEY flags
  - low/high key values
  - active/cleared state
- `SetColorKey` and `GetColorKey` wrappers now capture color-key state after
  successful real DirectDraw calls.
- `SetTexture` probes the surface's color-key slots and emits current metadata
  before texture pixels when the texture is first registered.
- `instrument/receiver/protocol.py` and `receive.py` accept and decode v4
  metadata while still accepting v1-v3 streams.
- `instrument/diff/extract_frame_capture.py` preserves v4 texture metadata in
  self-contained frame captures.
- `src/engine/replay.c` uses masks for pixel conversion when v4 metadata is
  present, applies active source color keys during texture upload, and keeps
  the old v3 BGRA/BGR fallback for older captures.

## Verification completed

Local build:

```sh
make
```

Result:

- Build succeeded.
- Only the existing SDL_mixer archive warning was reported.

Proxy build:

```sh
cd instrument/proxy
./build.sh
```

Result:

- `ddraw.dll` rebuilt.
- Export table verification passed.
- Imported DLLs remained XP-safe: `KERNEL32.dll`, `USER32.dll`, `WS2_32.dll`.
- No UCRT dependency.

Python parser checks:

```sh
python3 -m py_compile \
  instrument/receiver/protocol.py \
  instrument/receiver/receive.py \
  instrument/diff/extract_frame_capture.py \
  instrument/diff/inject_pixels_v3.py
```

Result: passed.

Existing v3 backward-compatibility replay:

```sh
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_REPLAY=build/frame6533_v3_retry.omtc \
  JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

Result:

- Replay loaded `build/frame6533_v3_retry.omtc`.
- Replay issued `3557` GL draws.
- Replay registered `256` textures.
- `screenshot.png` was written at `1280x720`.
- Output alpha range remained `(255, 255)`.

Extractor backward-compatibility:

```sh
python3 instrument/diff/extract_frame_capture.py \
  build/frame6533_v3_retry.omtc \
  --frame 0 \
  --out /tmp/frame6533_v3_reextract.omtc
```

Result:

- Re-extracted the v3 frame successfully.
- Prelude contained `256` `TEXTURE_DEF` records and `256` `TEXTURE_PIXELS`
  records.
- Prelude contained `0` v4 texture-format records and `0` v4 color-key records,
  as expected for the old capture.

## Important limitation

The existing known-good captures are v3. They do not contain `TEXTURE_FORMAT`
or `TEXTURE_COLORKEY` records, so they can only verify backward compatibility.
They cannot prove whether the white foliage/tree-top rectangles or the
incorrect-looking ground texture are fixed.

A fresh XP capture with the rebuilt v4 proxy is required before judging the
visual result.

## Next steps

1. Deploy the rebuilt proxy to XP only when explicitly approved.

   ```sh
   python3 instrument/deploy_xp.py
   ```

2. Start a fresh receiver on Debian before launching the game.

   ```sh
   python3 instrument/receiver/receive.py serve --out build/level1_v4.omtc
   ```

3. Launch the game on XP and reach the Level 1 scene used by the previous
   frame-6533 comparison.

4. Mark a frame from the receiver control prompt if possible.

   ```text
   mark 0xface1
   ```

5. Stop capture after enough Level 1 texture bindings have occurred.

6. Confirm v4 metadata exists in the capture.

   Use the receiver file parser or a small record-count scan to confirm:

   - `TEXTURE_PIXELS` count is still near the expected texture count.
   - `TEXTURE_FORMAT` records are present.
   - `TEXTURE_COLORKEY` records are present or explicitly absent based on
     DirectDraw state.

7. If a mark was captured, locate the frame.

   ```sh
   python3 instrument/diff/scan_mark.py build/level1_v4.omtc 0xface1
   ```

8. Extract a self-contained v4 frame.

   ```sh
   python3 instrument/diff/extract_frame_capture.py build/level1_v4.omtc \
     --frame F \
     --out build/frame_v4.omtc
   ```

9. Replay the v4 frame.

   ```sh
   env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
     JN_REPLAY=build/frame_v4.omtc \
     JN_SCREENSHOT=1 \
     xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
   ```

10. Compare the new `screenshot.png` against:

    - `build/frame6533_v3_retry.png`
    - any old `screenshot.png` from the v3 replay

## What to inspect after recapture

- Whether white rectangular foliage/tree-top quads are gone.
- Whether the ground uses the expected grass/terrain texture instead of the
  blue-looking texture.
- Whether any formerly opaque texture regions became incorrectly transparent.
- Whether color-key records are emitted for the textures involved in foliage
  and alpha-cutout surfaces.
- Whether any texture reports unexpected masks, such as zero RGB masks or an
  alpha mask without `DDPF_ALPHAPIXELS`.

## Constraints

- Do not contact XP or recapture without explicit approval.
- Do not touch nginx, systemd, router config, or git remotes.
- Do not remove generated or untracked files.
- Do not use broad replay heuristics for white pixels or foliage rectangles.
- Keep the replay path faithful to captured D3D/DDraw state.

