# Next session — commit (a)/(b), then the M7b on-XP gate

Written 2026-05-22. Supersedes the "next session" framing in
`m7_followups_plan.md` for items (a) and (b), which are now **done** (this
session). Read `m7_followups_plan.md` for the full M7b test table — it is still
the authority on the live-run test criteria; this doc only updates state and
sequences the remaining work.

## ⚠️ UPDATE 2026-05-22 (run #2) — stale proxy found & replaced; M7b gate still OPEN

A first M7b on-XP run was attempted this session and **surfaced a blocker**:
the proxy that was deployed (`2d30e333…`, 237056 B) **did not act on any
control command.** Diagnosis (all verified):
- Capture worked end-to-end — proxy connected, ~4187 draws/frame at 640×480,
  textures streaming; the v1→v2 send path is fine.
- The receiver control path is fine — `help` over the FIFO re-printed the
  banner, so commands were encoded and `sendall`'d on the live socket.
- `protocol.py` ↔ `protocol.h` command values + the `{type u8,len u24}` framing
  match exactly; the proxy *source* (`capture.c`) has a correct, complete recv
  path (`omtc_poll_commands`→`omtc_cmd_push`→`omtc_cmd_drain` at frame_begin,
  flipping `g_capture_enabled`).
- Yet `stop` had **zero effect**: the `.omtc` kept growing 7.8 MB/2 s (same as
  running). ⇒ the deployed binary **predated the M7b recv code** (recv path
  written 2026-05-20; that binary was almost certainly the 05-19 M5 build —
  same byte-size by coincidence, different SHA-1 only looked like a timestamp).

**Fix applied this session:** rebuilt the proxy from current source
(`bash instrument/proxy/build.sh`) → **`ddraw.dll` sha1
`c5cefde8df2327ff8e73610a41b875110132e144`, 237056 B**, and **deployed it to the
XP game dir** (download-back SHA-1 verified MATCH). Game was already closed; no
proxy is running. **So next session: the *current* M7b proxy is already on XP —
skip the deploy, go straight to launch + command retest.** If in doubt,
re-verify the deployed `ddraw.dll` sha1 == `c5cefde8…` before trusting it.

**Two operational gotchas learned (fold into the run order below):**
1. **Run the receiver UNBUFFERED** — `PYTHONUNBUFFERED=1 python3 -u
   instrument/receiver/receive.py serve …`. Plain `python3` block-buffers stdout
   to the redirected log, so `[header]`/`[frame N]` lines never appear live
   (only the `flush=True` control banner does) — you fly blind. With `-u` the
   per-frame `draws=`/`textures=` lines stream live.
2. **`receive.py serve` exits on proxy disconnect** — it `accept()`s once, then
   the recv loop `break`s on EOF and the process returns. So quitting/relaunching
   the game (or a proxy crash) kills the receiver; **restart it before the user
   relaunches.** (Enhancement option: wrap the accept+serve in a re-accept loop.)
3. **Frame-line granularity is every 100 frames** — too coarse to *prove* a
   toggle quickly. To verify `stop`/`start` fast, measure `.omtc` byte-growth
   over a 2 s window (stat -c%s before/after); a working `stop` freezes growth.
4. Launch the game **only from the noVNC interactive session** — launching via
   `xp_client`/the command-server service puts the window on an invisible
   window station (process runs, no visible window, stalls before 3D init / the
   proxy never reaches `DirectDrawCreateEx`). Confirmed this session: the
   service-launched `Neutron.exe` (PID 4020) showed nothing and never connected;
   killed it, user launched from noVNC, proxy connected immediately.

**Resume point:** Step 2 below (the on-XP M7b exit gate), now with a known-good
proxy. Step 1 (commit) is N/A — `/home/scotty/jn-engine` is **not a git repo**
and there are no commits (user confirmed: do not attempt to commit). The (a)/(b)
smoke test *did* pass this session: `matched_diff.sh … --frame 6844` ran to
completion with no manual SIGINT (validates (a)) and section 2 read
**IDENTITY / no mirror**, identity 813.6 vs negate-X 1380.6 (validates (b)).

## State to resume from

**Done this session (uncommitted, Debian-side):**
- **(a)** `capture_should_exit()` — demo now breaks its main loop after the
  `JN_CAPTURE_FRAMES` budget instead of hanging. Files: `src/engine/capture.c`
  (`g_done` flag + accessor), `src/engine/capture.h` (decl + no-op stub),
  `src/game/main.c` (break after `capture_end_frame()`). Both `make` and
  `make capture` build clean.
- **(b)** `diff.py` mirroring fix — the GL→D3D view-space normalization now
  negates **both X and Z** (was Z only), so section 2 reports IDENTITY / no
  mirror, agreeing with `extract_camera.py`'s rigorous world-space verdict.
  Verified on the frame-6844 dry-run artefacts: identity 813.6 vs negate-X
  1380.6 (exact swap of the pre-fix numbers); all other report sections
  byte-identical. File: `instrument/diff/diff.py` only.
  - **Phase 12 mirroring answer is now settled: the level is NOT X-mirrored.**
    The earlier "possible X-flip" gap is closed — it was a diff.py artefact.

**Deployed on XP (left in place, safe):**
- Game dir `C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius\`.
- `ddraw.dll` = **v2 proxy** (sha1 `2d30e33354e81cd556f4668bb00e33cff128645b`,
  237056 bytes), `ddraw_orig.dll` = stock 2004 ddraw. The v1 `.bak` was
  deleted (no longer needed). Proxy retries `<DEBIAN_HOST>:7070` on a background
  thread; render thread never blocks → fine to launch the game with no
  receiver.

**Environment gotchas discovered this session:**
- `xp_client.py` is at `~/xp-command-server/`, **not** `~/jn-engine/`. Token
  `<XP_TOKEN_REDACTED>`, XP command server `<XP_HOST>:9999`.
- XP has **no `certutil`** — verify an upload by `xp_client.download()` of the
  deployed file and comparing SHA-1 locally (round-trip).
- `tools/vnccap.py` is **capture-only** (grabs one framebuffer → PNG; no RFB
  PointerEvent/KeyEvent). It cannot pilot the game.
- `receive.py serve` reads control commands (`mark`/`stop`/`start`/`cam`/
  `camclear`/`redump`/`kill`) from **stdin** on a thread. To drive it
  programmatically, launch it with stdin from a FIFO and hold the FIFO open
  with a background writer, then `echo "<cmd>" > fifo`.
- Avoid recursive `dir /b /s C:\` over the command server — full-disk scans on
  XP are very slow and time out the client.
- Compound `copy A B & del A` in cmd runs the `del` even if the `copy` fails
  (locked file). Use separate `exec` calls when a delete is conditional on a
  copy succeeding.

## Step 1 — Commit (a) + (b)  [was deferred item (c)]

Single commit on a branch (repo wasn't a git repo at `/home/scotty`; the
jn-engine repo is at `~/jn-engine` — confirm `git status` there first).

Touch: `src/engine/capture.c`, `src/engine/capture.h`, `src/game/main.c`,
`instrument/diff/diff.py`. Suggested message:

```
Phase12 M7 follow-ups: clean capture exit + diff.py X-handedness fix

(a) capture_should_exit() lets the demo break its main loop after the
    JN_CAPTURE_FRAMES budget instead of hanging the matched_diff driver.
(b) diff.py GL->D3D view-space fold now negates X as well as Z, so the
    object-set mirroring test agrees with extract_camera (NO mirror).
    Settles the Phase 12 mirroring question: level is not X-mirrored.
```

Smoke test before committing: re-run the M7c dry run
(`./instrument/diff/matched_diff.sh instrument/m5_session.omtc --frame 6844
--out-dir ./build/m7c`) — it should now complete **without a manual SIGINT**
(that is (a)'s acceptance) and section 2 should read IDENTITY (that is (b)'s).

## Step 2 — M7b on-XP exit gate  ⛔ (collaboration, user at the console)

The blocker that stopped this last session: piloting the game to level-1 and
reading camera-nudge **direction** both need eyes + input on the live XP
screen, and `vnccap.py` can't inject input. **Resolution: the user pilots via
the XP noVNC browser session** (`https://<DEBIAN_HOST>:4401` LAN, or
`https://<EXTERNAL_HOST>:8401`), which has full keyboard+mouse, while Claude
runs the receiver + control channel. No new tooling required.

Run order:
1. **Debian:** `ss -tlnp | grep 7070` is empty. Start the receiver with a FIFO
   control channel:
   ```bash
   mkfifo /tmp/m7b_ctrl
   sleep infinity > /tmp/m7b_ctrl &        # holds the FIFO open (no EOF)
   python3 instrument/receiver/receive.py serve \
       --out build/m7b_session.omtc < /tmp/m7b_ctrl > /tmp/m7b_serve.log 2>&1 &
   ```
   Issue commands with `echo "mark 0xCAFE" > /tmp/m7b_ctrl`, etc.
2. **XP (user, via noVNC):** launch the game from the game dir and play into
   level-1 gameplay. (Claude can launch it via `xp_client` but the user must
   drive the menus.) Watch `/tmp/m7b_serve.log` for
   `[serve] proxy connected from <XP_HOST>` + `draws=NNNN` frame lines.
3. Run the test table from `m7_followups_plan.md` §(d):
   - **No-eyes tests (Claude verifies from the stream/log):** `stop`/`start`
     toggles the per-frame `draws=` lines; `mark 0xCAFE` tags the next frame
     (confirm by decoding the saved `.omtc`); `redump` re-emits TEXTURE_DEFs;
     `kill` stops capture; FPS holds ~140 (proxy logs an M3 SUMMARY every 300
     frames to `C:\omtc.log` — `xp_client.exec(['type C:\\omtc.log'])`).
   - **Eyes tests (user reads direction off noVNC):** `cam` translation nudge
     (`cam 1 0 0 0  0 1 0 0  0 0 1 0  100 0 0 1`) and yaw nudge — confirm the
     camera moves and **which way**. That direction fixes the sign convention
     (plan §10.1: forward `D·WORLD` by default; decide whether `D` needs
     pre-inversion). `camclear` snaps back.
   - Abort path: `camclear` → identity `cam`; if the game crashes, restore
     stock via `copy /Y ddraw_orig.dll ddraw.dll` (after killing the game).
4. **Capture the real session:** once toggles/mark/nudge/fps pass, `mark` a
   known tag, play ~1 min of representative level-1, `stop`, quit the game.
   Save `build/m7b_session.omtc` as the real v2 source.

## Step 3 — Real M7c report (the Phase 12 exit ⛔)

`./instrument/diff/matched_diff.sh build/m7b_session.omtc --frame <marked>`
→ the **real** five-gap report (camera-matched, not the dry-run proxy frame).
Sign off the five Phase 12 gaps against measured numbers:

| Gap | Dry-run signal (frame 6844, indicative) |
|---|---|
| Mirroring | **none** (settled — see (b)) |
| Lighting | original AMBIENT `0x333333`, LIGHTING OFF, 0 lights; demo over-brightens |
| Ground texture | original 21/21 textured, demo 2/8 |
| Terrain topography | original Y-span ~11.6× the demo's |
| Water | demo binds zero water-type textures |

After sign-off, Phase 12 implementation (the actual engine fixes for
lighting/ground/terrain/water) becomes the next planning target.

## Optional / if the collaboration path is unwanted

Extend `tools/vnccap.py` with RFB `PointerEvent` (msg type 5) + `KeyEvent`
(type 4) to script menu navigation and autonomous nudge-direction capture
(diff two framebuffers to measure shift). More work and fragile against a 2001
game's menus — only worth it if hands-off runs are needed repeatedly.
