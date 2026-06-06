# jn-engine — Agent Instructions (Claude + Codex)

Both Claude Code and Codex load this file when working in `~/jn-engine`. It is read
**in addition to** `~/CLAUDE.md` (which `~/AGENTS.md` symlinks to — machine role, services,
session-export workflow, JN operating rules). Keep cross-tool conventions in these shared
files, never in an agent's private memory.

## Read before touching code
1. `docs/PROJECT_HISTORY.md` — phase-by-phase what/why, current state.
2. `docs/ARCHITECTURE.md` — how the foundry (C/Python) and the game (Godot) fit together.
3. `docs/godot_bridge_plan.md` — active track. Current branch: `godot-bridge-phase0-1`.
   Decomp/native (C engine, capture replay, OMT/glTF foundry) feeds the Godot game; the
   Godot game is the primary artifact.
4. `docs/claude_code_failure_patterns.md` — distilled failure modes; the durable rules from
   it are also in `~/CLAUDE.md` § "JN Engine Operating Rules". Don't relitigate settled facts
   (matrix convention, `PROJ[3][3]=1`, capture-reprojection ceiling) without measured evidence.

## Shared memory — the anti-silo rule
There is **one source of truth, and it is version-controlled markdown**, not an agent's
private memory store (Claude's `~/.claude/.../memory/` is Claude-only; Codex can't read it,
and vice-versa).

- **Durable project facts/decisions/gotchas → append to `docs/PROJECT_HISTORY.md`**
  (or `docs/ARCHITECTURE.md` for structural facts). Commit them. That is how the other
  agent — and future you — inherit the knowledge.
- Machine/workflow facts that aren't jn-engine-specific → `~/CLAUDE.md` (shared via the
  AGENTS.md symlink).
- Treat each tool's built-in/auto memory as a **scratch cache**, not a record others rely on.
  If a fact matters beyond this session, it must land in a shared markdown file or it doesn't
  count as remembered.

## Capture/keyframe → Godot motion (the current question)
The keyframe approach (capture as visual oracle vs. native render) is being extended from
static frames to **motion** (HUD actions, camera pan, character animation, movement,
cutscenes). Direction per the godot_bridge_plan: export time-sampled timelines as **data
sidecars**, have Godot play them and validate frame-by-frame against captured source frames —
don't port replay logic into Godot. Record findings in PROJECT_HISTORY as this lands.

## Gotchas (carry forward)
- `pkill -f jnengine` kills your own shell (cmdline contains "jnengine") — use `pkill -x jnengine`.
- XP has no `certutil`; verify uploads by download-back SHA-1.
- Interactive game runs must be launched from the XP noVNC desktop, not `xp_client`/command-server
  (invisible window station → proxy never hits `DirectDrawCreateEx`).
- `rsync` is not installed on this box — use `cp -u`.
