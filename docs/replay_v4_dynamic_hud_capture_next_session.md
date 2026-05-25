# Replay v4 Dynamic HUD Capture Handoff

## Current state

The texture-refresh recapture fixed the stale world texture problems in
`build/frame_v4_refresh.omtc`:

- ground black/cyan striping resolved
- mixed day/space skybox resolved
- hedge gauge-digit bleed resolved
- water stale/black pages resolved

The remaining white rectangles were traced to dynamic HUD/gui counter elements,
not world texture corruption.

Replay-side mitigation is in `src/engine/replay.c`: when a draw binds a texture
id that has no captured texture record, replay now skips that draw by default
instead of drawing the 1x1 white fallback. Set `JN_REPLAY_SHOW_MISSING_TEX=1`
to restore the old missing-texture white fallback visualization.

Verification output:

- `build/frame_v4_refresh_hudfix.png`
- replay reported `3377 GL draws`, `274 registered textures`,
  `92 skipped missing-texture draws`
- near-white pixels dropped from `21432` in `build/frame_v4_refresh.png` to
  `506` in `build/frame_v4_refresh_hudfix.png`

This is a visual mitigation, not a complete HUD replay solution. The next step
is to capture those dynamic HUD texture payloads correctly.

## HUD diagnosis

The white HUD/counter rectangles were not caused by color key, alpha blending,
material/diffuse state, screen-space transforms, or texture-stage state.

They were caused by bound texture ids that appear in draw records but have no
corresponding captured texture metadata or pixels.

Visible counter/HUD cluster:

- draw range: mostly `3453` through `3469`
- state: alpha blend enabled, z disabled, clamp addressing, point filtering
- geometry: screen-space quads around `0..32`, `0..20`, with negative y values
- texture ids include:
  - `0x22dc60`
  - `0x3d71188`
  - `0x5c77398`
  - `0x3d51668`
  - `0x3d51c70`
  - `0x5c402d8`
  - `0x5c403d0`
  - `0x5c41008`
  - `0x5177f68`
  - `0x5177c68`
  - `0x5177d60`
  - `0x5177a58`
  - `0x5177a98`
  - `0x5373f30`
  - `0x5374028`

In `build/frame_v4_refresh.omtc`, those ids have `SET_TEXTURE` records but zero
`TEXTURE_DEF`, `TEXTURE_FORMAT`, `TEXTURE_PIXELS`, and `TEXTURE_COLORKEY`
records.

Other missing alpha-blended dynamic ids appear earlier around draws `3285`
through `3432`, including repeated ids such as:

- `0x3f817e8`
- `0x23df10`
- `0x23df50`
- `0x5c4f2c0`
- `0x4003310`
- `0x4003828`

## Focused capture/parser plan

1. Confirm whether the full refreshed capture has the missing HUD payloads.

   Scan `build/level1_v4_refresh.omtc`, not only the extracted frame, for the
   missing ids. Count `SET_TEXTURE`, `TEXTURE_DEF`, `TEXTURE_FORMAT`,
   `TEXTURE_PIXELS`, and `TEXTURE_COLORKEY`.

   If the full capture has only `SET_TEXTURE`, the proxy never emitted the
   texture payloads. If the full capture has payloads but
   `build/frame_v4_refresh.omtc` does not, fix `extract_frame_capture.py`.

2. Instrument proxy texture emission failure reasons.

   In `instrument/proxy/capture.c`, add env-gated logging for bound textures
   that do not emit pixels. Log:

   - texture id
   - dimensions
   - caps / surface desc flags
   - pixel format
   - lock result
   - whether the surface was already marked seen
   - whether `TEXTURE_DEF`, `TEXTURE_FORMAT`, `TEXTURE_PIXELS`, and
     `TEXTURE_COLORKEY` were emitted

3. Fix "seen before successful pixels".

   Check the current texture tracking path around lock failure. If a surface is
   marked seen after a failed lock or partial metadata emit, change the state
   model so it is only considered fully captured after successful
   `TEXTURE_DEF + TEXTURE_FORMAT + TEXTURE_PIXELS`.

   Failed locks should remain retryable on later binds. Add a small retry limit
   or env-gated logging throttle if needed.

4. Handle dynamic/update-after-first-seen HUD surfaces.

   HUD counters may use small renderable or system-memory surfaces whose pixels
   change after first sighting. Add a targeted dynamic texture refresh path for
   HUD-like draws:

   - small textures such as `16x16`, `32x32`, `4x16`, `16x4`, `4x4`
   - alpha-blended draws
   - z disabled
   - screen-space or HUD-sized quad bounds

   Prefer a faithful trigger: re-emit `TEXTURE_PIXELS` when a lightweight hash
   changes for such textures. Avoid broad replay heuristics.

5. Update extraction behavior if needed.

   In `instrument/diff/extract_frame_capture.py`, ensure the prelude keeps the
   latest texture metadata and pixel payload per texture id before the marked
   frame. If a texture's latest pixels arrive inside the target frame before its
   first draw, preserve that record before the draw.

6. Validate without XP recapture until proxy changes require it.

   Use existing artifacts for parser/extractor checks. Recapture from XP only
   after proxy capture behavior changes.

   Success criteria:

   - missing HUD ids appear in `textures.json`
   - skipped missing-texture draw count drops from `92` toward `0`
   - replay renders HUD digits/icons instead of white rectangles or skipped gaps
   - world texture fixes remain intact

## Useful commands

Build replay:

```sh
make
```

Replay current extracted frame:

```sh
env JN_REPLAY=build/frame_v4_refresh.omtc JN_SCREENSHOT=1 xvfb-run -a ./jnengine
cp screenshot.png build/frame_v4_refresh_hudfix.png
```

Show old missing-texture fallback visualization:

```sh
env JN_REPLAY=build/frame_v4_refresh.omtc JN_REPLAY_SHOW_MISSING_TEX=1 JN_SCREENSHOT=1 xvfb-run -a ./jnengine
```

Inspect v4 replay frame:

```sh
python3 instrument/diff/inspect_replay_v4.py \
  build/frame_v4_refresh.omtc \
  --out-dir build/replay_v4_refresh_inspect
```

Quick missing-id count script:

```sh
python3 - <<'PY'
from pathlib import Path
import struct

path = Path("build/frame_v4_refresh.omtc")
ids = {
    0x22dc60, 0x3d71188, 0x5c77398, 0x3d51668, 0x3d51c70,
    0x5c402d8, 0x5c403d0, 0x5c41008, 0x5177f68, 0x5177c68,
    0x5177d60, 0x5177a58, 0x5177a98, 0x5373f30, 0x5374028,
    0x3f817e8, 0x23df10, 0x23df50, 0x5c4f2c0, 0x4003310,
    0x4003828,
}
counts = {i: {"set": 0, "def": 0, "fmt": 0, "pix": 0, "ck": 0} for i in ids}
raw = path.read_bytes()
off = 14
while off + 4 <= len(raw):
    rt = raw[off]
    plen = (raw[off + 1] << 16) | struct.unpack_from("<H", raw, off + 2)[0]
    off += 4
    p = raw[off:off + plen]
    off += plen
    if rt == 5 and len(p) >= 5:
        tid = struct.unpack_from("<I", p, 1)[0]
        if tid in counts:
            counts[tid]["set"] += 1
    elif rt in (6, 14, 15, 16) and len(p) >= 4:
        tid = struct.unpack_from("<I", p, 0)[0]
        if tid in counts:
            counts[tid][{6: "def", 14: "pix", 15: "fmt", 16: "ck"}[rt]] += 1
for tid, c in sorted(counts.items()):
    print(hex(tid), c)
PY
```

## Fresh-session prompt

Model: GPT-5 Codex
Effort: high

We are in `/home/scotty/jn-engine`. Continue replay v4 dynamic HUD capture work.

Read first:

- `docs/replay_v4_dynamic_hud_capture_next_session.md`
- `docs/replay_v4_recapture_next_session.md`
- `src/engine/replay.c`
- `instrument/proxy/capture.c`
- `instrument/diff/extract_frame_capture.py`
- `instrument/diff/inspect_replay_v4.py`

Context:

- Texture-refresh recapture succeeded and fixed the stale world texture issues.
- Current artifacts:
  - `build/level1_v4_refresh.omtc`
  - `build/frame_v4_refresh.omtc`
  - `build/frame_v4_refresh.png`
  - `build/frame_v4_refresh_hudfix.png`
  - `build/replay_v4_refresh_inspect/`
- Marked frame:
  - tag `0xbeef`
  - frame index `11413`
  - seq `23033`
- Current replay-side mitigation:
  - `src/engine/replay.c` skips draws whose bound texture id has no captured
    texture record.
  - `JN_REPLAY_SHOW_MISSING_TEX=1` restores the old white fallback debug view.
  - replay now reports skipped missing-texture draws.
- Verified mitigation:
  - `build/frame_v4_refresh_hudfix.png`
  - replay reported `3377 GL draws`, `274 registered textures`,
    `92 skipped missing-texture draws`
  - white HUD rectangles are gone/materially improved
  - world texture fixes remain intact

Task:

Create and execute a focused plan to capture the dynamic HUD/gui textures in the
new proxy/parser path. Do not start by recapturing from XP. First inspect the
existing full capture and extracted frame to determine whether missing HUD ids
are absent from the full capture or lost during extraction.

Known missing HUD/counter texture ids include:

- `0x22dc60`
- `0x3d71188`
- `0x5c77398`
- `0x3d51668`
- `0x3d51c70`
- `0x5c402d8`
- `0x5c403d0`
- `0x5c41008`
- `0x5177f68`
- `0x5177c68`
- `0x5177d60`
- `0x5177a58`
- `0x5177a98`
- `0x5373f30`
- `0x5374028`

Also inspect repeated earlier missing alpha-blended dynamic ids around draws
`3285` through `3432`, including:

- `0x3f817e8`
- `0x23df10`
- `0x23df50`
- `0x5c4f2c0`
- `0x4003310`
- `0x4003828`

Determine whether the next fix belongs in:

- `instrument/proxy/capture.c` because the proxy never emits payloads for these
  surfaces
- `instrument/diff/extract_frame_capture.py` because payloads exist in the full
  stream but are not included in the self-contained frame
- `instrument/diff/inspect_replay_v4.py` only if inspection is hiding useful
  metadata

Likely first proxy-side hypothesis:

- A texture surface may be marked seen before a successful pixel lock/dump, so
  dynamic HUD textures become non-retryable after a failed or partial first
  sighting.

Implementation constraints:

- Keep changes narrowly scoped.
- Prefer faithful capture/parser fixes over replay heuristics.
- Add env-gated debug logging only if useful.
- Do not recapture from XP unless the local scan proves proxy changes are
  needed and cannot be validated with existing artifacts.

Verification:

1. Scan `build/level1_v4_refresh.omtc` and `build/frame_v4_refresh.omtc` for
   missing HUD ids and summarize counts by record type.
2. If extractor-only, fix extraction and regenerate a self-contained frame from
   `build/level1_v4_refresh.omtc`.
3. If proxy-side, implement the smallest capture fix and document the exact
   recapture command sequence needed next.
4. Re-run replay under Xvfb when a new or regenerated frame is available:

```sh
env JN_REPLAY=<new-frame.omtc> JN_SCREENSHOT=1 xvfb-run -a ./jnengine
```

5. Save the screenshot with a descriptive name under `build/`.
6. Confirm dynamic HUD textures render or, if recapture is still required,
   confirm the local evidence proving why.

Final response should summarize changed files, commands run, what was proven,
and remaining risks.

## Execution update - 2026-05-25

Existing artifact scan proved this is proxy-side, not extractor-side:

- `build/frame_v4_refresh.omtc`: listed HUD ids have `SET_TEXTURE` records but
  zero `TEXTURE_DEF`, `TEXTURE_FORMAT`, `TEXTURE_PIXELS`, and
  `TEXTURE_COLORKEY`.
- `build/level1_v4_refresh.omtc`: same listed ids appear in thousands of
  `SET_TEXTURE` records, still with zero texture payload records.
- Full stream totals from the scan: `1,767,719 SET_TEXTURE`, `331 TEXTURE_DEF`,
  `331 TEXTURE_FORMAT`, `331 TEXTURE_PIXELS`, `1,324 TEXTURE_COLORKEY`.

Proxy changes made:

- `instrument/proxy/capture.c`: texture table capacity raised from 256 to 1024.
- `instrument/proxy/capture.c`: surfaces are no longer marked known by
  `omtc_texture_is_new`; they become known only after
  `TEXTURE_DEF + TEXTURE_FORMAT + TEXTURE_PIXELS` are successfully emitted.
- `instrument/proxy/capture.c`: mutation redumps are SHA-1 gated, so unchanged
  surfaces do not re-emit duplicate payloads.
- `instrument/proxy/com_wrappers.c`: `SetTexture` retries uncaptured surfaces on
  later binds after lock/payload failure.
- `instrument/proxy/com_wrappers.c`: added env-gated `OMTC_TEXTURE_DEBUG`
  logging for texture lock result, dimensions, caps, surface-desc flags, pixel
  format flags, bpp, previous known state, and emitted/skipped result.

Build/deploy status:

- `make` was clean.
- `instrument/proxy/build.sh` rebuilt and verified `ddraw.dll` as PE32,
  XP-safe, with only `KERNEL32.dll`, `USER32.dll`, and `WS2_32.dll` imports.
- The rebuilt proxy was deployed to the XP game directory by
  `python3 instrument/deploy_xp.py --help` because that script ignores unknown
  options and performs its default deploy path.

Playable-demo status:

- This work has updated capture/replay instrumentation only, not the
  user-playable demo experience.
- Last material playable demo source update was `aeeb818` at
  `2026-05-22 18:50:16 -0400`
  (`phase12 D1/D2: render only textured geometry (faithfulness)`).
- Packaged web demo artifacts under `web/` were last built around
  `2026-05-22 18:51 -0400`.
- The newer native `jnengine` binary timestamp on `2026-05-25` reflects
  replay/capture work, not playable-demo gameplay or HUD integration.
- If the HUD recapture succeeds, a separate playable-demo pass is still needed:
  extract/name recovered HUD glyphs/icons, add a first-class demo HUD renderer,
  wire it to demo game state, and validate against the fixed original capture.

Next required validation is a fresh capture from XP, because existing captures
do not contain the missing HUD payloads:

```sh
python3 instrument/receiver/receive.py serve --out build/level1_v4_hudfix.omtc
```

Launch the XP game with the deployed proxy, reach the Level 1 comparison view,
then send:

```text
redump
mark 0xbeef
```

After the session closes:

```sh
python3 instrument/diff/scan_mark.py build/level1_v4_hudfix.omtc
python3 instrument/diff/extract_frame_capture.py \
  build/level1_v4_hudfix.omtc \
  --frame <marked-frame-index> \
  --out build/frame_v4_hudfix.omtc
python3 instrument/diff/inspect_replay_v4.py \
  build/frame_v4_hudfix.omtc \
  --out-dir build/replay_v4_hudfix_inspect
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_REPLAY=build/frame_v4_hudfix.omtc \
  JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
cp screenshot.png build/frame_v4_hudfix.png
```
