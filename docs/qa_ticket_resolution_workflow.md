# QA Ticket Resolution Workflow

This is the shared handoff contract for community QA tickets. It exists so Claude
Code and Codex handle tickets the same way: same intake assumptions, same fix
philosophy, same verification gates, and same public resolution-log page format.

## Intake

1. Start from the reporter's exported QA payload: markdown table plus fenced JSON
   from the in-game QA annotate tool.
2. Read the current context before changing code:
   - `docs/PROJECT_HISTORY.md`
   - `docs/ARCHITECTURE.md`
   - the latest `docs/qa/sandmanfan-*/index.html` pages
   - relevant `docs/decomp/C3D*.md` class specs for each FourCC/class
3. Treat report coordinates, FourCC, tag, asset path, sprite index, and camera
   position as evidence. Do not reduce the ticket to a name-match row edit until
   the authored data and class spec have been checked.

## Fix Philosophy

Prefer engine rules and authored data over per-instance curation.

- If a defect repeats across instances, find the load/resolve/render rule that is
  wrong and fix it once.
- If the `.gam` authors a mesh, sprite, texture, visibility flag, or OMT shape
  index, assume that is the first source of truth until measured evidence says
  otherwise.
- If a fix creates a new invariant, encode it in `tools/audit_faithfulness.py`
  where possible.
- Keep hard-won conventions intact: `.gam` rotations import as
  `rx, ry = +deg*pi/180`, `rz = -deg*pi/180`; JNBG `sprites.omt` authored
  canvases beat visible guessed defaults; `C3DOmtObj` uses
  `OmtDatabase`/`OmtIndex`; OMT entity-bound shapes use raw-origin GLBs.

## Resolution Log Page

Every completed community ticket gets a tracked static page under:

```text
docs/qa/<reporter>-YYYY-MM-DD[/index.html]
docs/qa/<reporter>-YYYY-MM-DD/img/*
```

If there are multiple same-day tickets, append a letter suffix matching the
existing convention (`2026-06-11b`, `2026-06-12b`). Mirror the same directory to:

```text
/var/www/jn-engine/qa/<same-folder>/
```

The page should match the existing resolution-log format:

- dark monospace page, max-width wrapper, ticket table, category legend
- headline: `QA Ticket Resolution Log #N` plus `X / X RESOLVED`
- subline: reporter, date, levels, fixing agent, live demo link, previous tickets
- one card per report or shared root cause
- each card includes the original quote, before/after screenshots, evidence trail,
  and explicit fix statement
- grouped cards are preferred when one root cause resolves multiple reports
- process notes / collateral repairs / verification summary at the end
- footer with source/live/archive context

Use the existing pages as templates:

- `docs/qa/sandmanfan-2026-06-11/index.html`
- `docs/qa/sandmanfan-2026-06-11b/index.html`
- `docs/qa/sandmanfan-2026-06-12/index.html`
- `docs/qa/sandmanfan-2026-06-12b/index.html`

Keep author language truthful. If Codex resolves a ticket, say Codex; if Claude
does, say Claude. Do not copy the "authored by Claude" line into a Codex page.

## Evidence Images

Before/after images must use comparable cameras.

- Prefer the reporter's exported camera when available.
- Use `tools/qa_shot.sh` or the existing `JN_NATIVE_LEVEL1_CAMERA` /
  `JN_DEMO_SPAWN_XYZ` screenshot paths for aimed native shots.
- For honest before shots, temporarily build/revert only the relevant fix or use
  a preserved deprecated build. Do not hand-edit screenshots.
- Use cache-safe filenames when replacing deployed images; Cloudflare may keep old
  image URLs for hours.

## Verification Gates

Before calling a ticket complete:

```bash
make
python3 tools/audit_faithfulness.py
source ~/emsdk/emsdk_env.sh
make web
./tools/deploy_wasm.sh
python3 tools/qa_web_verify.py
```

For narrow non-renderer/docs-only changes, state clearly which gates were skipped
and why. For visual/QA tickets, the faithfulness sweep and web QA harness should
pass unless there is a deliberate waiver with a written reason.

After deployment, check that the tracked page and deployed page match:

```bash
diff -qr docs/qa/<ticket-folder> /var/www/jn-engine/qa/<ticket-folder>
```

## Commit/Handoff

- Commit the code, generated/tracked assets, docs page, and verification tooling
  changes together when they are one ticket resolution.
- The commit message should list the root-cause rules, not just the symptoms.
- Push `decomp-campaign` when the ticket is complete and deployed.
- Update `docs/PROJECT_HISTORY.md` when the ticket creates a new invariant,
  changes a settled convention, adds a sweep rule, or supersedes an earlier note.
- Keep this workflow doc current when the page format or verification gates change.
