# Next session — true frame capture of Level 1

Written 2026-05-22.

## Why this exists / what "true capture" means

VNC screen-scraping of the live XP game is **unfixable past a point** (see
CLAUDE.md "VNC artifacts"): the async full-screen poll races DirectDraw's
double-buffer `Flip()`, giving flashing black boxes and a character that
flickers invisible↔visible. That is a *capture* artifact, not the game.

The **D3D7 proxy `ddraw.dll`** sidesteps it entirely: it taps the real render
stream at `DrawPrimitive` and streams it to a Debian receiver over TCP. No
screen read, no flip race — these are the *true* frames the game submitted.
This plan captures a clean `.omtc` of **Level 1 gameplay** that way.

This is a focused capture run. It reuses the M7b rig from
`m7b_onxp_run_plan.md` (read that for the full toggle/nudge test table — not
repeated here). Goal here is narrower: **get one clean, marked Level-1
`.omtc`.**

## Navigation path (user pilots via noVNC)

The game opens like this — the user drives it through the menus/cutscene on the
noVNC session; `vnccap.py` cannot inject input:

1. **Init cutscene** plays on launch.
2. Opens to the **Lab level** (hub).
3. From the Lab, proceed into **Level 1** gameplay.

`mark` the stream the instant Level-1 *gameplay* (not the loading screen)
begins, so the diff later lands on a representative frame.

## Pre-flight (Debian + XP)

1. **Deployed proxy already verified (2026-05-22).** Round-tripped the XP
   `ddraw.dll` and hashed locally:
   **`e275442179abf5f4ccad030ef71812337b87f1f5`** (237056 B). This is
   **byte-identical to the current local build** `instrument/proxy/ddraw.dll`
   (built 2026-05-22 04:39) — i.e. newer than *both* previously-documented
   hashes (`2d30e333…`, `c5cefde8…`), which are now obsolete. Since this build
   includes the M7b recv path in `capture.c`, the deployed proxy **is current
   and M7b-capable. No redeploy needed.**

   Next session, just re-confirm it's still `e275442179…` (the game dir is
   untouched, so it should be) before trusting capture/control:
   ```bash
   # XP has no certutil — round-trip the file and hash locally.
   python3 - <<'PY'
   import sys; sys.path.insert(0, '/home/scotty/xp-command-server')
   from xp_client import XpClient
   with XpClient('<XP_HOST>', 9999, '<XP_TOKEN_REDACTED>') as c:
       c.download(r'C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius\ddraw.dll',
                  '/tmp/xp_ddraw.dll')
   PY
   sha1sum /tmp/xp_ddraw.dll   # expect e275442179abf5f4ccad030ef71812337b87f1f5
   ```
   If it ever differs, rebuild + redeploy: `bash instrument/proxy/build.sh`,
   then `xp_client.upload()` the new `ddraw.dll` and re-verify the round-trip.
   (Live command-response is still proven during the run via the `stop`
   byte-growth check — hash match only proves the binary, not the link.)

2. **Port 7070 must be free:** `ss -tlnp | grep 7070` → empty.

3. **Game must be closed** on XP before launch (a stale instance holds the
   proxy socket / window station).

## Run order

1. **Debian — start receiver with a FIFO control channel, UNBUFFERED.**
   (Plain `python3` block-buffers stdout to the log → you fly blind. The `-u`
   is mandatory to see live `draws=`/`textures=` frame lines.)
   ```bash
   cd /home/scotty/jn-engine
   mkfifo /tmp/lvl1_ctrl 2>/dev/null
   sleep infinity > /tmp/lvl1_ctrl &        # holds FIFO open (no EOF)
   PYTHONUNBUFFERED=1 python3 -u instrument/receiver/receive.py serve \
       --out build/level1_session.omtc < /tmp/lvl1_ctrl > /tmp/lvl1_serve.log 2>&1 &
   ```
   Issue commands with `echo "<cmd>" > /tmp/lvl1_ctrl`.
   ⚠️ `receive.py serve` **exits on proxy disconnect** (single `accept()`, recv
   loop breaks on EOF). If the game quits/crashes/relaunches, **restart the
   receiver before the user relaunches** the game.

2. **XP — user launches the game from the noVNC session ONLY.**
   `https://<DEBIAN_HOST>:4401` (LAN) or `https://<EXTERNAL_HOST>:8401`.
   Do **not** launch via `xp_client`/the command-server service — that puts the
   window on an invisible window station; the process runs but never reaches
   `DirectDrawCreateEx`, so the proxy never connects (confirmed in run #2).
   - Watch `/tmp/lvl1_serve.log` for
     `[serve] proxy connected from <XP_HOST>` + per-frame `draws=NNNN` lines.

3. **User pilots:** init cutscene → Lab level → into Level 1 gameplay.

4. **Capture:**
   - The moment Level-1 gameplay starts: `echo "mark 0xL1" > /tmp/lvl1_ctrl`
     (pick a memorable tag; record the exact value used).
   - Play **~1 min of representative Level 1** — walk the level, face the
     terrain/water/ground that Phase 12 cares about (ground texturing, slopes,
     the stream/water, lighting).
   - `echo "stop" > /tmp/lvl1_ctrl` to freeze capture; confirm `.omtc`
     byte-growth halts (`stat -c%s build/level1_session.omtc` before/after a 2 s
     window — frame-line logging is only every 100 frames, too coarse to prove a
     toggle quickly).
   - Quit the game cleanly.

5. **Verify the capture is good** before calling it done:
   - `.omtc` is non-trivial in size and stopped growing after `stop`.
   - The `0xL1` mark is present — decode/scan the `.omtc` for the mark record
     so Step (next) can target that frame.
   - Header + TEXTURE_DEFs present (if missing textures, `redump` re-emits them;
     re-`mark` after).

6. **Save** `build/level1_session.omtc` as the canonical Level-1 v2 source.

## Then (next planning target, not this run)

Feed the marked frame to the diff to get the **real** Phase 12 five-gap report
(this replaces the frame-6844 *dry-run* numbers):
```bash
./instrument/diff/matched_diff.sh build/level1_session.omtc --frame <0xL1 frame>
```
Sign off mirroring (already settled: NO mirror), lighting, ground texture,
terrain topography, water — against measured Level-1 numbers. After sign-off,
the Phase 12 engine fixes (lighting/ground/terrain/water) become the next plan.

## Gotchas (folded from m7b run #2)

- **`wmic`/`typeperf` hang the freeSSHd persistent shell** (today's lesson).
  Recover: `sudo systemctl restart xp-daemon.service`. Use `reg`/`sc`/`dir`/
  `type` only; read XP CPU off Task Manager in the noVNC session.
- `xp_client.py` lives at `~/xp-command-server/` (not `~/jn-engine/`). Token
  `<XP_TOKEN_REDACTED>`, command server `<XP_HOST>:9999`.
- XP has **no `certutil`** — verify uploads by download-back SHA-1.
- `tools/vnccap.py` is **capture-only** (no input injection) — user must pilot.
- Proxy retries `<DEBIAN_HOST>:7070` on a background thread and never blocks the
  render thread → safe to have the game running with no receiver.
- Abort/restore: if the game crashes with the proxy, kill it and restore stock
  ddraw with `copy /Y ddraw_orig.dll ddraw.dll` in the game dir.
