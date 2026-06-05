# jn-engine — next-week plan (2026-05-23 → next session)

Self-contained handoff. State of the pivot, what's blocking, what to do, in
priority order. Read this first if you're picking up cold.

## Where we left off (2026-05-22 / 2026-05-24 sessions)

**The faithful-engine pivot is proven end-to-end.** A `.omtc` replay engine
inside `jnengine` consumes the original game's captured D3D7 command stream
and renders Level 1 textured Retroville on Linux GL — no game logic, no scene
model, no guessed parameters. The pipeline is:

```
Original game (XP)
    │ D3D7 calls
    ▼
ddraw.dll proxy (instrument/proxy/)  ──TCP──▶  receiver  ──▶  level1.omtc (4 GB)
                                                                      │
                                                                      ▼
                                                  extract_frame_capture.py
                                                                      │
                                                                      ▼  (442 KB per frame)
                                                            jnengine JN_REPLAY=…
                                                                      │
                                                                      ▼
                                                          GL framebuffer
```

Key facts:
- **frame16565** of `build/level1_session.omtc` is the canonical marked frame
  (FRAME_MARK tag `0xface1`).
- **`build/frame16565.omtc`** = 442 KB self-contained replay file (v2,
  texture pixels not in stream → uses sidecar PNG path).
- **`build/frame16565_v3.omtc`** = 19.6 MB; same frame with `TEXTURE_PIXELS`
  records hand-injected from local PNGs (validates v3 plumbing).
- **`build/level1_v3_retry.omtc`** = 702 MB real XP v3 recapture, saved
  2026-05-24 with all 256 `TEXTURE_PIXELS` records in-stream.
- **`build/frame6533_v3_retry.omtc`** = 21 MB self-contained real v3 replay
  frame extracted from that capture; replay output
  **`build/frame6533_v3_retry.png`** renders Retroville/Jimmy using only
  in-stream texture pixels (no PNG sidecar).
- **`build/replay_texmap.{json,txt}`** = heuristic dim-based pairing
  `tex_id → PNG path`, 196/256 mapped.
- **The proxy is v3-ready and deployed on XP** as of 2026-05-24. Stock
  `ddraw_orig.dll` remains in the game directory for rollback.

### Run the replay (real v3 capture, no XP needed)
```bash
cd ~/jn-engine
export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib"
unset JN_CAPTURE JN_CAPTURE_CAMERA JN_REPLAY_TEX_MAP
export JN_REPLAY=build/frame6533_v3_retry.omtc JN_SCREENSHOT=1
xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

Expected: `screenshot.png` renders Retroville with Jimmy centered, 3557 GL
draws, 256 textures registered from in-stream `TEXTURE_PIXELS`.

### Run the older sidecar replay (textured, no XP needed)
```bash
cd ~/jn-engine
export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib"
unset JN_CAPTURE JN_CAPTURE_CAMERA
export JN_REPLAY=build/frame16565.omtc \
       JN_REPLAY_TEX_MAP=build/replay_texmap.txt JN_SCREENSHOT=1
xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine     # headless, screenshot
# or
DISPLAY=:0.0 ./jnengine                                # live on desktop
```

Single-frame output is textured Retroville (Jimmy + NPC + streets +
buildings + HUD). See `docs/replay_v0_findings.md` for the v0/v0.1/v0.2
narrative.

---

## Active gotchas (don't relearn these)

1. **Live-window-on-X compositor reads alpha → looks like silhouettes.**
   Fixed in commit `a99a574`: replay shader now forces
   `FragColor.a = 1.0`. Do not reintroduce texture/vertex alpha as framebuffer
   alpha unless the SDL/X compositor path is handled separately.
2. **D3D7 vertex DIFFUSE alpha is often 0.** Don't `discard` on it — that
   killed ~2/3 of geometry in the silhouette-debug session. Already fixed in
   the committed shader.
3. **D3D matrix conventions are equivalent two ways.** Col-major col-vector
   ≡ row-major row-vector applied as transpose. Don't keep relitigating —
   `replay.c`'s current `mat4_mul_col` + `transpose=GL_FALSE` works.
   Proof: probe in this session showed identical 6511/9856 on-screen counts
   under both. See `docs/replay_v0_findings.md`.
4. **Captured `PROJ[3][3]=1`** is the game's own w-buffer +1 offset
   projection (NOT a proxy artifact, confirmed by reading
   `instrument/proxy/com_wrappers.c:227` + `capture.c:598-602` —
   `memcpy(payload, D3DMATRIX, 64)` raw). Don't try to "fix" it.
5. **SHA-1 texture matching against local PNGs is impossible** (M5 caveat,
   re-confirmed by `instrument/diff/sha_hunt.py`: 0/256 across 9 byte
   orderings). The proxy's locked-surface bytes aren't byte-equivalent to
   decoded PNGs. The proper path is v3 pixel payloads; the 2026-05-24 retry
   capture proves all 256 payloads can be captured and replayed.
6. **`make capture` clean rule USED to nuke `web/`.** Already fixed
   (commit aeeb818) to remove only build outputs, keeping `web/shell.html`.
   If `make web` fails with "shell-file not found" again, that fix regressed.
7. **`pkill -f "jnengine"` kills your shell** because the bash cmdline
   contains "jnengine". Use `pkill -x jnengine` (exact name match).
8. **Receiver marks require an interactive receiver stdin.** Starting
   `receive.py serve` through a non-TTY exec leaves stdin at `/dev/null`, so
   typing `mark 0xface1` later is impossible and direct `/proc/.../fd`
   injection fails. Start it with a real TTY/stdin if the next capture needs a
   `FRAME_MARK`.

---

## Priority queue for next session

### P0 — `git push` what's local (no remote yet)
Repo was `git init`'d this session and has 11 commits but no remote. If you
want this off-machine, configure one:
```bash
cd ~/jn-engine
git remote add origin <url>
git push -u origin master
```
Otherwise the work is local-only.

### P1 — Alpha=1 shader fix
Done in commit `a99a574`. Headless replay still renders textured Retroville,
and the live-window compositor transparency bug is closed.

### P2 — Real XP v3 capture / pixel-payload replay
Done enough to validate the path. `instrument/proxy/ddraw.dll` was deployed
to XP, `build/level1_v3_retry.omtc` captured a real v3 session, and
`build/frame6533_v3_retry.omtc` replays with all 256 in-stream texture
payloads:
```bash
JN_REPLAY=build/frame6533_v3_retry.omtc JN_SCREENSHOT=1 \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

The intended `FRAME_MARK` did not land in the retry stream even though the
receiver accepted `mark 0xface1`; frame 6533 was chosen by byte-offset timing
near when the mark command was sent. For a future canonical capture, start the
receiver interactively and confirm `scan_mark.py` finds the tag before
discarding the XP session.

### P3 — Multi-frame streaming replay (doesn't need XP)
Today's replay is one frame, re-rendered each tick. Walk many/all frames in
sequence → the original game's gameplay playing back inside our renderer.
Steps:
- `extract_session_range.py <orig.omtc> --frames A,B --out range.omtc` —
  prelude + records for frames in `[A, B]`, similar to
  `extract_frame_capture.py` but with multiple `FRAME_BEGIN/END` boundaries.
- `replay.c`: split `replay_render_frame` into an outer streaming loop —
  parse records sequentially, on `FRAME_END` swap and advance to next
  `FRAME_BEGIN`. Add `JN_REPLAY_LOOP=1` for repeat-from-start.
- Demonstration target: render frames 16560–16570 in sequence (~10
  frames of the marked moment).
- Pure Linux, validates time-series fidelity.

### P4 — Render-state coverage polish
`replay.c` honors `LIGHTING / ZENABLE / ZWRITEENABLE / ALPHABLENDENABLE`,
and commit `3dc2f59` added stage-0 texture address/filter coverage for
`SET_TEXSTAGESTATE` (`ADDRESS`, `ADDRESSU/V`, `MINFILTER`, `MAGFILTER`,
`MIPFILTER`). Remaining render-state parity work: alpha test, fog, cull mode,
blend factors, and any texture-stage combine states that prove visible in
real captures.

### P5 — Simulation layer plan (the other half of "faithful")
The faithful architecture is `simulation → D3D7 commands → renderer`.
Renderer side is now real. Simulation side currently lives in the imitation
engine (`src/game/*`) — entities, behaviors, physics, GAM loading.
Sketch a doc — `docs/sim_layer_port.md` — for how to drive the renderer
from the imitation engine's emitted draws (replacing the current direct GL
calls with `omtc_*` emissions, then re-using `replay.c` to consume them).
That makes the engine SELF-faithful (same render path live as offline).

### P6 — Web build of the replay
The WASM shell (`web/shell.html`) currently loads the imitation engine.
Build a `--target=web` variant that loads `JN_REPLAY` from a fetched
`.omtc` and renders in browser via WebGL2 / `-sFULL_ES3`. Pixel-faithful
JNBG Level 1 in a browser tab. (Likely needs `glClipControl` removal —
already done in the shader Z remap; check WebGL caveats around `GL_BGRA`
upload — may need a CPU swap to RGBA in the v3 path for WebGL2.)

---

## Key files (quick reference)

| Path | Purpose |
|---|---|
| `src/engine/replay.c` + `.h` | The replay engine itself |
| `instrument/proxy/protocol.h` | Wire protocol (v3) — shared with receiver |
| `instrument/proxy/capture.c` | Proxy emitter (v3 TEXTURE_PIXELS emission lives here) |
| `instrument/proxy/ddraw.dll` | Built v3 proxy DLL — ready to deploy |
| `instrument/receiver/protocol.py` | Python mirror of protocol (v3) |
| `instrument/receiver/receive.py` | TCP receiver — raw-stream-save works for v3 unchanged |
| `instrument/diff/extract_frame_capture.py` | 4 GB capture → small per-frame .omtc (v3-aware) |
| `instrument/diff/build_replay_texmap.py` | Heuristic dim-based PNG↔tex_id sidecar |
| `instrument/diff/sha_hunt.py` | Tried exact SHA-1 PNG matching — 0/256 (negative result, kept as documentation) |
| `instrument/diff/inject_pixels_v3.py` | Bootstrap v3 .omtc from v2 + sidecar (no XP needed; validates v3 path) |
| `instrument/diff/cache_orig_frame.py` | One-shot stream → per-draw JSON cache (legacy, for the failed texture matcher) |
| `instrument/diff/match_textures.py` | Geometry-based SHA→PNG matcher — failed (kept for record) |
| `docs/faithful_engine_rethink.md` | The architecture rethink doc (READ FIRST if cold) |
| `docs/replay_v0_findings.md` | Replay implementation journal (v0/v0.1/v0.2) |
| `docs/replay_v3_deploy.md` | XP redeploy procedure for pixel-exact replay |
| `docs/phase12_faithfulness_audit.md` | Pre-pivot audit (still valid context) |
| `docs/phase12_canon_baseline.md` | Pre-pivot Phase-12 measurements (still valid context) |

## What to do if XP is on and you only have 30 minutes
If the goal is a cleaner canonical capture, run the v3 receiver interactively,
launch from the visible XP desktop, enter Level 1, issue `mark 0xface1`, quit,
and immediately verify with `scan_mark.py`. The DLL is already deployed; only
redeploy if `instrument/proxy/ddraw.dll` changes.

## What to do if XP is off
P3 (multi-frame streaming) is now the highest-value pure-Linux work, using
`build/level1_v3_retry.omtc` or a sliced range around frame 6533 as the test
input. Or jump to P5 / P6 if the architecture-level planning energy is there.
