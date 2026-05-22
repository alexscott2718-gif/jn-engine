# M7 follow-ups — plan (next session)

Three follow-ups surfaced by the M7c dry run (2026-05-20). Not committed.
Pick them up next session in this order — (a) is a tiny demo-side ergonomics
fix that makes (d) less painful; (b) is a debug exercise on diff.py; (d) is
the user gate for the M7b on-XP run.

Skipped: (c) commit. User decision — defer until after at least (a) lands.

## State to resume from

- M7b/M7c code complete (this session). Proxy DLL rebuilt clean at
  `instrument/proxy/ddraw.dll`, v2 protocol, command channel, FRAME_MARK,
  camera rewrite hook all wired. Verified with synthetic v1+v2 fixtures and
  full M7c dry-run pipeline.
- `instrument/diff/extract_camera.py` got a version-range update during the
  dry run (not part of the §10 file list — discovered live; harmless).
- Dry-run artefacts kept at `~/jn-engine/build/m7c/`:
  - `camera.cam` (frame 6844 descriptor)
  - `demo.omtc` (137 KB, 358 draws)
  - `demo.omtc.tex` (sidecar)
- First M7c report saved in this session's notes — key facts:
  - **Mirroring contradiction**: `extract_camera` says NO mirror (28/8
    inliers, world-space); `diff.py` says negate-X best fit (813 vs 1380,
    view-space). Almost certainly a view-space X-handedness artefact in
    `diff.py`, not real mirroring. Resolved in (b).
  - **Lighting** (sec 4): original AMBIENT `0xff333333`, LIGHTING OFF, 0
    lights. Demo over-brightens (in-shader directional 0.577,0.577,0.577).
  - **Terrain Y-span** original 70220 vs demo 6051 (~11.6×).
  - **Ground texture**: original 21/21 textured, demo 2/8.
  - **Water**: demo binds zero water-type textures.

## (a) Demo exits on JN_CAPTURE_FRAMES

**Problem.** `capture_shutdown()` closes the file when the frame budget is
hit (`src/engine/capture.c:283`), but the game's main loop keeps running.
The M7c driver hangs at step 3 until `jnengine` is SIGINT'd manually. We
worked around it by killing PID and letting the driver continue to step 4 —
fine ad-hoc, fragile for the real workflow.

**Design.**
1. `capture.h`: add `int capture_should_exit(void);` query.
2. `capture.c`: add a static `g_done` flag; set it inside the
   `--g_frames_left <= 0` branch right after `capture_shutdown()`.
   `capture_should_exit` returns `g_done`. Header-only no-op when
   `-DJN_CAPTURE` is not set (return 0).
3. `src/game/main.c`: after the per-frame `capture_end_frame()` call,
   check `capture_should_exit()` and break the main loop. Existing shutdown
   path runs as normal (input subsystem cleanup, etc.).

**Why a flag query instead of `exit(0)` in capture.c:** the game has a
shutdown path (`Input subsystem destroyed`, SDL teardown). Skipping it can
leak the window or the GL context on some drivers, and it precludes any
post-capture cleanup work we might add later.

**Test.** Re-run `matched_diff.sh` against `m5_session.omtc --frame 6844`
end-to-end with no SIGINT; driver completes cleanly.

**Files.** `src/engine/capture.c`, `src/engine/capture.h`, `src/game/main.c`.

## (b) Resolve mirroring contradiction in diff.py

**Problem.** `extract_camera.py` (world-space registration vs `level1.omt`
placements) says NO mirroring. `diff.py` (view-space cloud NN under per-axis
negation) says best fit is `negate-X`. They disagree on the rigorous Phase
12 mirroring question. extract_camera's method is the more rigorous one —
direct registration against ground-truth placements — so the bug is almost
certainly in `diff.py`'s view-space fold, not in the world-space solve.

**Hypothesis.** D3D7 (original) is left-handed; GL (demo) is right-handed.
`diff.py` docstring says it "normalizes the GL↔D3D Z handedness" when
folding the demo through `VIEW·WORLD`. It says nothing about X — and that's
the axis where the bogus best-fit appears. Most likely the demo's
view-space cloud needs an X sign flip (or, equivalently, the VIEW row
needs a column negation) to land in the same handedness as the original's
baked WORLD.

**Investigation steps.**
1. In `diff.py`, find where the demo's WORLD matrices are folded into
   view-space (search for `view_baked`, `VIEW·WORLD`, the Z normalization).
2. Print the demo and original cloud means + a few sample points side by
   side at the chosen frame. If the original is at `x` and the demo is at
   `-x` (within tolerance) for the same object, that confirms the X flip.
3. Apply the X normalization at the same code site as the Z normalization
   (probably negate the X column of the fold matrix, or negate `x` on the
   folded demo points). Pick the spot that mirrors the existing Z fix.
4. Re-run `matched_diff.sh --report-only --keep-camera --skip-make
   --skip-demo`. Confirm:
   - Section 2: identity is now the smallest NN distance, not negate-X.
   - All other sections (camera, textures, lighting, terrain) unchanged.
5. If identity still loses to negate-X after the fix, fall back to: dump a
   handful of identifiable objects (the house at a known placement, the
   rocket, etc.) and trace their position through both pipelines by hand
   to find where the sign flips.

**Acceptance.** `diff.py`'s section 2 agrees with `extract_camera`'s
verdict — "NO mirroring" — and the inlier counts make sense (identity
clearly winning).

**Files.** `instrument/diff/diff.py` only (don't touch extract_camera —
that's the source of truth).

## (d) M7b on-XP exit gate

**Goal.** The user-gated M7b exit from §10.1: prove the bidirectional
control channel works on a live game. Specifically:

1. Receiver-issued `mark <tag>` produces a `FRAME_MARK(tag)` on the next
   captured frame.
2. `stop` then `start` toggles capture cleanly (proxy stops emitting,
   resumes).
3. `cam <16 floats>` with a tiny `D` visibly nudges the game's camera in
   `vnccap.py`.
4. Frame rate stays at the M1 ~140 fps baseline (proxy logs M3 summary
   every 300 frames — read it off `C:\omtc.log`).

### Pre-flight checks (Debian, before touching XP)

- Confirm proxy binary is the v2 build: `objdump -p
  instrument/proxy/ddraw.dll | grep -E "Export RVA|DLL Name"`. 22 exports;
  imports KERNEL32 / USER32 / WS2_32.
- Confirm proxy SHA-1: `sha1sum instrument/proxy/ddraw.dll`. Compare after
  upload.
- Confirm Debian listener can bind 7070: `ss -tlnp | grep 7070` returns
  nothing.

### Deploy

```python
# from Debian, ~/jn-engine
from xp_client import XpClient
TOKEN = '<XP_TOKEN_REDACTED>'
with XpClient('<XP_HOST>', 9999, TOKEN) as c:
    print(c.ping(), 'ms')
    # Stop the game if it's running (no-op if not):
    c.exec(['taskkill /F /IM Neutron.exe 2>nul || ver >nul'])
    # Back up the current ddraw.dll, deploy the v2 build:
    c.exec(['copy /Y "C:\\JNBG\\ddraw.dll" "C:\\JNBG\\ddraw.dll.bak"'])
    c.upload('instrument/proxy/ddraw.dll', 'C:\\JNBG\\ddraw.dll')
```

(Game dir path TBD if not `C:\JNBG\` — check `~/xp-jnbg-original/`
location memory + previous M5 deploy notes.)

Verify on XP: `c.exec(['certutil -hashfile C:\\JNBG\\ddraw.dll SHA1'])`.
Must match the Debian SHA-1.

### Live run

1. On Debian: `python3 instrument/receiver/receive.py serve --out
   build/m7b_session.omtc`. The control CLI prints its help on connect.
2. On XP via `xp_client`: launch the game.
   `c.exec(['start "" "C:\\JNBG\\Neutron.exe"'])`. Optional: start
   `vnccap.py` capture in parallel to record the visual.
3. Wait for the level-1 menu → start gameplay → wait for the receiver to
   print `[serve] proxy connected from <XP_HOST>` and the first few
   `[frame N] ... draws=NNNN` lines.

### Tests (from the receiver's stdin)

| Test | Command | Pass criterion |
|---|---|---|
| Capture toggle | `stop` then `start` | After `stop` the per-frame `draws=...` line stops printing (proxy emits no more capture). After `start` it resumes. Game keeps rendering throughout (just not captured). |
| Frame mark | `mark 0xCAFE` | Within ~200 ms (poll cadence), the next captured frame's `Frame` carries `mark = 0xCAFE`. Confirm by tailing `build/m7b_session.omtc` after the run with `receive.py --file` and a tiny dump that prints `frame.mark` for any tagged frame. |
| Texture redump | `redump` | A burst of `TEXTURE_DEF` records re-appears on every still-bound texture in the next captured frame. Visible as a spike in `textures=...` in the live print (count holds steady at the global total, but new defs go out). |
| Camera nudge — translation | `cam 1 0 0 0  0 1 0 0  0 0 1 0  100 0 0 1` (row-major; tx=100 in the WORLD row-vector convention) | `vnccap.py` shows the camera shifted; the *direction* of the shift is what we read off to fix the sign convention (plan §10.1). |
| Camera clear | `camclear` | View snaps back to game-driven. |
| Camera nudge — yaw | `cam 0.9962 0 -0.0872 0  0 1 0 0  0.0872 0 0.9962 0  0 0 0 1` (≈5° Y-rotation, row-major) | `vnccap.py` shows a small yaw nudge; confirm direction. Tiny rotation — the game's own frustum culling caps how far we can nudge (plan §10.1 caveat). |
| Kill switch | `kill` | Proxy stops emitting capture entirely (no more frame lines printed). Game keeps rendering. |
| FPS stable | (passive) | The proxy logs an M3 SUMMARY every 300 frames to `C:\omtc.log`. After ~5 min of play, pull the log: `c.exec(['type C:\\JNBG\\omtc.log'])`. Look for ~140 fps lines. Any sustained drop = abort. |

### Abort path

- Camera clip / weird artefacts: `camclear`, then `cam` with identity.
- Proxy crashes the game: kill the game, `c.exec(['copy /Y
  C:\\JNBG\\ddraw.dll.bak C:\\JNBG\\ddraw.dll'])`. Sample `C:\omtc.log` for
  the crash spot before restoring.
- Frame rate tanks: send `kill`, then `stop`. If the game itself slowed
  (not just the capture), suspect the camera rewrite path; restore the bak.

### Capture for M7c real run

Once the toggle / mark / nudge / fps tests pass:
1. Issue `mark 0xC07C` (matches `extract_camera --frame K` discoverability).
2. Let the game run ~1 minute of representative level-1 gameplay.
3. `stop` — closes the capture cleanly.
4. Quit the game (`taskkill`).
5. Copy `build/m7b_session.omtc` aside as the real-deal v2 source for the
   M7c follow-up — then re-run `matched_diff.sh build/m7b_session.omtc
   --frame K`. That's the **real** Phase 12 gap report. (Where K is the
   frame whose mark you set above; receive.py logs the mark on decode.)

### Open questions to resolve on the spot

- Sign convention of the camera rewrite. Plan §10.1: forward `D · WORLD_baked`
  by default; observation from the translation test tells us whether
  `extract_camera.py` needs an inverse, and whether the receiver should
  pre-invert `D` before sending. Decide based on what direction the camera
  actually moves with `tx=100`.
- Whether `redump` actually re-emits texture defs in practice or whether the
  `g_tex_count = 0` reset races with an in-flight FRAME_END rollback. If
  the M5 overflow-rollback path is exercised the first frame after redump,
  textures might re-emit twice — harmless, but worth noting.

## Order of operations next session

1. Land (a). Tiny PR; re-run M7c dry run as the smoke test.
2. Land (b). Re-run M7c dry run with `--report-only`; check section 2.
3. Stage the M7b run when ready — separate session is fine, but a clean
   (a)+(b) base means the M7b artefacts are immediately usable.
4. After M7b passes, the *real* M7c run: `matched_diff.sh
   build/m7b_session.omtc --frame <marked>`. **That** is the Phase 12 exit
   ⛔ — sign-off on the five-gap numbers.
