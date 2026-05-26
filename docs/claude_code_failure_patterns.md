# Claude Code failure patterns for JN engine work

Written 2026-05-26 after auditing Claude Code session exports and existing
handoff docs. Purpose: preserve the repeated errors and operational traps that
have cost time on the JNBG native engine / capture-backed renderer work, then
turn them into concrete instructions for future Claude sessions.

## Source logs reviewed

- `ObsidianVault/06 Personal/Claude Code Sessions/session_2026-05-16_1256_bf525efd.md`
  and `bf525efd-f7ed-4562-94d2-fa20ab39dbe0.jsonl`: file-transfer and console
  command problems, freeSSHd limitations, and the decision to build the XP
  command server.
- `docs/m7b_onxp_run_plan.md`: stale proxy deployment, receiver control,
  launch-context, and XP command gotchas from the M7b live-run attempts.
- `docs/level1_capture_plan.md`: current true-frame capture runbook and
  noVNC/receiver requirements.
- `docs/omt2_instrumentation_plan.md`: corrected `ddraw.dll` proxy export
  assumptions, pass-through gates, receiver memory problem, and runtime
  capture architecture.
- `docs/claude_multiframe_passoff.md` and
  `docs/multiframe_world_reproject_handoff.md`: the multi-frame empty-render
  bug, why the previous debug path was misleading, and the later hybrid pivot.
- `docs/next_week_plan.md`: active replay/capture gotchas that should not be
  relearned.
- `/home/scotty/CLAUDE.md`: consolidated XP VNC, XP daemon, command server, and
  project gotchas.

## Recurring failure patterns

### 1. Repeating known conclusions instead of trusting project handoffs

Several later sessions re-opened questions already settled by previous docs:
matrix convention, `PROJ[3][3] = 1`, no X mirror after the diff fix, and the
limit of capture reprojection as a playable Level 1 backbone. This wastes time
and risks undoing correct work.

Instruction: before changing renderer, capture, proxy, or XP workflow code, read
the relevant current handoff first. If a doc says "do not relitigate" or "pivot",
treat that as a constraint unless new measured evidence directly contradicts it.

### 2. Fixing the wrong layer before checking the final predicate

The multi-frame renderer loaded valid data but drew nothing. Claude spent time
checking vertex stride, binary layout, texture tables, magic values, z filters,
and cross-tests. The actual bug was in the final draw predicate: per-frame
`capture_scene_set_group_world_offset(..., 0, 0, 0)` disabled
`g_group_use_world[STATIC_WORLD]`, so `draw_use_world` became false.

Instruction: when render data loads but disappears, instrument the draw-site
decision before binary spelunking. For `capture_scene_render()`, log group,
texture lookup, visibility, `g_world_mode`, `g_group_use_world[group]`, and
`draw_use_world` for the first few draws.

### 3. Making proxy assumptions from static imports only

The first `ddraw.dll` proxy plan assumed only `DirectDrawCreateEx` and
`DirectDrawEnumerateA` mattered because `OMT2.dll` imported only those symbols.
That crashed the game. Runtime-loaded `d3dim700.dll` resolves ddraw internal
exports against the proxy, so the proxy must export all 22 real ddraw symbols
at exact ordinals and forward the 20 unimplemented ones.

Instruction: for XP DLL proxy work, static imports are not enough. Verify the
full runtime export/import surface and always run a pure pass-through gate
before adding logging or behavioral changes. Check `objdump -p` before deploy.

### 4. Trusting deployed binaries by size or date

An M7b run used a stale deployed proxy. File size and a plausible SHA/date story
misled the session, while `stop` commands had no effect. Round-trip SHA checks
later proved the deployed binary was not the expected current build.

Instruction: every XP proxy deployment must be verified by download-back SHA-1.
Do not trust file size, timestamp, or "same bytes by coincidence". XP has no
`certutil`; hash the downloaded copy on Debian.

### 5. Launching the game from the wrong Windows session

Launching `Neutron.exe` via `xp_client` or the command-server service puts the
process on an invisible window station. The process can exist, but no visible
window appears and the proxy may never reach `DirectDrawCreateEx`.

Instruction: if human-visible gameplay, menu navigation, or D3D proxy capture is
needed, the game must be launched from the XP noVNC desktop session. Use
`xp_client` for file transfer, verification, killing stale processes, and logs,
not for starting interactive game runs.

### 6. Treating `vnccap.py` as an input tool

`tools/vnccap.py` captures a framebuffer only. It cannot inject keyboard or
mouse events. Attempts to automate game piloting through it led to bad plans.

Instruction: assume noVNC/user pilots the game unless a real input-injection
tool has been implemented and tested. `vnccap.py` is for screenshots only.

### 7. Receiver/control-channel fragility

`receive.py serve` accepts one connection and exits on proxy disconnect. Running
it with buffered stdout hides live frame lines. Starting it without usable stdin
makes `mark`/`stop` commands impossible later. Frame logs every 100 frames are
too coarse to prove `stop` quickly.

Instruction: start receivers with a real stdin or FIFO, `PYTHONUNBUFFERED=1`,
and `python3 -u`. Restart the receiver before each relaunch. Verify `stop` by
measuring `.omtc` byte growth over a short window, not by waiting for frame log
granularity.

### 8. Overusing fragile XP shell paths

freeSSHd hangs or crashes under load. `wmic` and `typeperf` can hang the
persistent shell. Recursive `dir /b /s C:\` is too slow. Compound XP commands
like `copy A B & del A` run the delete even if copy fails.

Instruction: prefer the XP command server and `xp_client.py` for command/file
work. Use `reg`, `sc`, `dir`, and `type` for XP shell checks. Avoid `wmic` and
`typeperf`. Use separate command-server exec calls when a later destructive
step depends on an earlier copy succeeding.

### 9. Blocking the game render thread or retaining too much receiver state

The proxy capture architecture only works if the game render thread enqueues
and never blocks on socket I/O. A receiver bug retained every decoded frame and
hit multi-GB RSS/swap thrash even though the proxy ring was bounded.

Instruction: keep proxy I/O on a send thread with bounded rings and whole-frame
drops. Keep receiver parsing incremental. Never add an unbounded frame list for
large `.omtc` sessions, and never read multi-GB captures wholly into memory.

### 10. Misreading alpha and D3D7 capture details

The replay/debug path hit multiple repeat traps: X compositor live-window alpha
made good output look like silhouettes; D3D7 vertex DIFFUSE alpha is often zero
and should not cause discard; local PNG SHA matching cannot identify raw locked
surface pixels; the captured projection matrix is the game's real w-buffer style
projection, not proxy corruption.

Instruction: keep replay alpha forced opaque for live windows unless compositor
handling is intentionally changed. Do not discard on vertex DIFFUSE alpha. Do
not try to match proxy raw surface SHA-1s against local PNG files. Do not
"repair" captured projection values without direct evidence.

### 11. Continuing a known dead-end target

The multi-frame capture-reproject path was useful evidence but is not the
playable Level 1 target. Free movement needs native runtime world/camera/
projection/visibility, so the active backbone is the hybrid/native Level 1 path.

Instruction: do not polish multi-frame far screenshots as the product path.
Use those artifacts as reconstruction evidence, then work on the native/hybrid
runtime world described by the current pivot docs.

## Draft instruction block for future Claude sessions

Add a condensed version of this to future JN fresh-session prompts or
`CLAUDE.md`:

```text
JN engine rules:
- Start by reading the current handoff docs for the exact subsystem. Do not
  re-open settled facts without new measured evidence.
- For render bugs where data loads but nothing appears, instrument the final
  draw predicate and shader mode before investigating binary layout.
- For XP proxy DLL work, verify exports with objdump, run pure pass-through
  first, and deploy only after a download-back SHA-1 match.
- Launch JNBG interactively only from the XP noVNC desktop. xp_client is for
  transfer, logs, process cleanup, and verification, not gameplay launch.
- Start receive.py with a FIFO or real stdin plus PYTHONUNBUFFERED=1 python3 -u.
  Restart it after every game disconnect. Verify stop/start by file byte growth.
- Treat vnccap.py as screenshot-only. It cannot pilot the game.
- Avoid freeSSHd-heavy workflows. Prefer xp_client.py; avoid wmic/typeperf;
  avoid recursive full-disk dir scans; do conditional XP file ops in separate
  commands.
- Never block the game render thread on network I/O. Never retain all frames or
  read multi-GB .omtc files into memory.
- Preserve known D3D7/replay facts: forced opaque live-window alpha, no discard
  on DIFFUSE alpha, captured PROJ[3][3]=1 is real, PNG SHA matching is not a
  texture identity solution.
- Multi-frame capture reprojection is reference evidence, not the playable
  Level 1 product path. Use the hybrid/native Level 1 pivot for new work.
```
