# Replay v3 Next Steps: Texture Alpha, Pixel Format, and Color Key

## Current state

The faithful replay path now consumes in-stream `TEXTURE_PIXELS` from v3 captures.
The known good Level 1 artifacts are:

- `build/level1_v3_retry.omtc`
- `build/frame6533_v3_retry.omtc`
- `build/frame6533_v3_retry.png`

The replay alpha/blend first pass is intentionally worth keeping:

- `src/engine/replay.c` restores meaningful fragment alpha for alpha-blended
  draws instead of forcing every fragment to `alpha=1`.
- RGB blending follows captured D3D `D3DRS_SRCBLEND` / `D3DRS_DESTBLEND`.
- The final framebuffer stays opaque via `glBlendFuncSeparate(..., GL_ZERO,
  GL_ONE)` for destination alpha.
- `src/engine/glad.c` / `src/engine/glad.h` now expose `glBlendFuncSeparate`
  minimally, following the existing local GL loader pattern.

Verification command used:

```sh
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_REPLAY=build/frame6533_v3_retry.omtc \
  JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

Observed verification result:

- Build succeeded with only the existing SDL_mixer archive warning.
- Replay reported `3557` GL draws and `256` registered textures.
- Output `screenshot.png` remained fully opaque: alpha range `(255, 255)`.
- White foliage/tree-top rectangular quads did not materially improve.
- The ground still looked like the wrong blue texture rather than grass.

## Working conclusion

The remaining artifact is probably not an asset lookup problem and probably not
plain GL output-alpha loss. The v3 capture has raw texture pixels, but the
protocol currently carries only `bpp`, not enough DirectDraw surface semantics.

The next likely missing pieces are:

- Full `DDPIXELFORMAT` metadata, especially flags and RGB/A masks.
- DirectDraw color key metadata from `SetColorKey` / `GetColorKey` paths.
- Replay-side conversion of captured pixels using those masks and keyed
  transparency rules before upload.

Do not add broad replay heuristics for white pixels or foliage rectangles. The
goal is still faithful replay from captured D3D/DDraw state.

## Suggested next session prompt

You are working in `/home/scotty/jn-engine` on DebianXPGateway.

Goal: extend the faithful v3 replay capture protocol so texture replay has
enough metadata to fix the white rectangular foliage/tree-top quads and the
incorrect-looking ground texture in the real Level 1 replay. This is the
faithful replay path, not the imitation engine.

Start by reading:

- `docs/replay_v3_next_steps.md`
- `docs/replay_v3_deploy.md`
- `src/engine/replay.c`
- `instrument/proxy/protocol.h`
- `instrument/proxy/com_wrappers.c`
- `instrument/proxy/capture.c`

Current artifacts:

- `build/level1_v3_retry.omtc`
- `build/frame6533_v3_retry.omtc`
- `build/frame6533_v3_retry.png`

Known state:

- v3 replay uses in-stream `TEXTURE_PIXELS`.
- `build/frame6533_v3_retry.omtc` has `3557` draws and `256`
  `TEXTURE_PIXELS`.
- Captured frame textures are all 32 bpp.
- Many textures contain alpha-zero pixels.
- Draw blend combos include:
  - `alphablend=0, srcblend=2, destblend=1` for 3329 draws
  - `alphablend=1, srcblend=5, destblend=6` for 197 draws
  - `alphablend=1, srcblend=5, destblend=2` for 28 draws
  - `alphablend=1, srcblend=9, destblend=1` for 3 draws
- No `ALPHATEST` records were seen in the extracted frame.
- Current proxy stores only `bpp`, not full `DDPIXELFORMAT` masks or color key
  data.
- `SetColorKey` wrappers exist, but protocol/capture does not currently emit
  color key metadata.
- A first replay alpha/blend pass was implemented and verified, but it did not
  fix the white boxes or ground.

Implement next pass:

1. Add protocol records or extend texture metadata to capture enough
   `DDPIXELFORMAT` data for replay:
   - `dwFlags`
   - `dwRGBBitCount`
   - `dwRBitMask`
   - `dwGBitMask`
   - `dwBBitMask`
   - `dwRGBAlphaBitMask`
2. Capture DirectDraw color key metadata from `SetColorKey` and any relevant
   `GetColorKey` / surface state paths:
   - surface id / texture id association
   - key flags
   - low/high color values
3. Preserve backward compatibility with existing v1/v2/v3 captures where
   feasible. Prefer adding a new protocol version over silently changing the
   meaning of existing records.
4. Update receiver parsing/extraction if protocol changes require it.
5. Update replay texture upload/conversion:
   - interpret captured 32 bpp pixels using masks instead of assuming BGRA
     whenever metadata is present
   - apply color key transparency only when captured surface metadata says it is
     active
   - keep framebuffer alpha opaque
6. Build locally.
7. Render existing `build/frame6533_v3_retry.omtc` to confirm backward
   compatibility. It may still show artifacts because it lacks new metadata.
8. Do not contact XP or recapture until the user explicitly approves it.
9. After approval, recapture XP with the new protocol and compare against the
   old `build/frame6533_v3_retry.png`.

Constraints:

- Do not touch nginx/systemd/router/git remote.
- Do not contact XP unless explicitly asked.
- Do not remove or revert untracked/generated files.
- Use `apply_patch` for manual file edits.
- Use `pkill -x jnengine` only if killing the program is needed.
- Never use `pkill -f`.

Expected deliverable:

- Scoped protocol/capture/replay changes for pixel format and color key
  metadata.
- Local build verification.
- Replay verification on the existing v3 frame for backward compatibility.
- A clear note that XP recapture is required before judging the visual fix.
