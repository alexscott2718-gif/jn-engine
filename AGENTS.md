# jn-engine — Agent Instructions (Claude + Codex)

Both Claude Code and Codex load this file when working in `~/jn-engine`. It is read
**in addition to** `~/CLAUDE.md` (which `~/AGENTS.md` symlinks to — machine role, services,
session-export workflow, JN operating rules). Keep cross-tool conventions in these shared
files, never in an agent's private memory.

## Mission (as of 2026-06-22)
**Native Linux port.** The decomp **spec** campaign is finished (208/208 classes) and the
C engine in `src/` is **the product** — not a foundry for something else. **Godot was retired**
(`docs/godot_bridge_plan.md` is superseded; do not start work against it). Branch: `native-port`.
Waves N1–N5 have landed; the current frontier is **behavior coverage** (enemies/NPCs/vehicles
still largely inert — visual coverage is effectively complete).

## Read before touching code
1. `docs/decomp/_next_session.md` — the **live per-session handoff** (current state + your task).
2. `docs/native_port_plan.md` — **the active execution plan** (§1 contract, §3 validation, §4 waves).
3. `docs/PROJECT_HISTORY.md` — phase-by-phase what/why; `docs/ARCHITECTURE.md` — engine state + invariants.
4. `docs/asset_catalog.md` + the **Asset Catalog** (`tools/build_asset_catalog.py`,
   `exentt.com/JN-assets/catalog/`) — the resolution/usage map for any asset question: which FourCC
   draws what, its texture truth, which levels use it, and whether it has a native vtable yet.
5. `docs/claude_code_failure_patterns.md` — distilled failure modes; the durable rules from
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

## The current frontier — behavior coverage (the active question)
Visual coverage is effectively complete (the Asset Catalog shows **0 used-in-level FourCCs draw a
placeholder box**); the gap is **runtime behavior**. Of the ~93 FourCCs placed in levels, only ~32
have a native vtable — enemies, friends/NPCs, and most vehicles are still static. Port them faithfully
from `docs/decomp/<Class>.md` onto the existing N1–N4 bases (`behavior_ai.c`, `behavior_projectile.c`,
`behavior_vehicle.c`, …); the next concrete target is in `docs/decomp/_next_session.md` (remaining
enemy roster + NPCs). Use the Asset Catalog's behavior column to pick targets by instance count ×
level reach. The captured `.omtc` D3D7 stream remains the **validator** for motion, not a runtime dep.
Record findings in PROJECT_HISTORY as waves land.

## Gotchas (carry forward)
- `pkill -f jnengine` kills your own shell (cmdline contains "jnengine") — use `pkill -x jnengine`.
- XP has no `certutil`; verify uploads by download-back SHA-1.
- Interactive game runs must be launched from the XP noVNC desktop, not `xp_client`/command-server
  (invisible window station → proxy never hits `DirectDrawCreateEx`).
- `rsync` is not installed on this box — use `cp -u`.
