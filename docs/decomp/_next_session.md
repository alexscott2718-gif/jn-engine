# Next Codex Session — Kickoff Prompt

> Paste this (or point Codex at this path) to resume the Neutron.exe decomp campaign.
> Living doc: update the "Current state" + "Your task" sections at each wave boundary so
> the next fresh session starts oriented. Source of truth remains the ledger +
> per-class docs + `PROJECT_HISTORY.md`.

---

You are resuming the full-tier decompilation of Neutron.exe (all 208 C* gameplay
classes → faithful spec markdown). This is a shared, committed campaign — read the
shared docs, don't rely on tool-private memory.

## Orient yourself first (read in this order)
1. `~/AGENTS.md`  (== `~/CLAUDE.md`; machine/workflow conventions, anti-silo policy)
2. `~/jn-engine/docs/codex_full_decomp_plan.md`  (the execution plan — phases, §1
   definition-of-done, §5 per-class inner loop, §0 .gam accelerator)
3. `~/jn-engine/docs/decomp/README.md`  (headless Ghidra invocation + status values)
4. `~/jn-engine/docs/decomp_ledger.csv`  (resumable state — 208 rows)
5. The Wave 1 reference specs already landed under `docs/decomp/`; match that
   depth/format.

## Current state (branch: decomp-campaign)
- Phase 0 DONE: RTTI analyzer run + Ghidra project saved (annotated as of commit
  `d63c6c0`), DumpHierarchy/DumpClass/DumpFunctions/ApplyRttiClassMarkup committed,
  `_hierarchy.md` (208 C* classes), ledger seeded, .gam backfill at 55/93 named FourCCs.
- Phase 1 DONE: all 25 base/framework classes have specs and ledger rows at
  `status=spec` / `owner=codex`. This batch is ready for Alex's spec-review gate.
- Latest pushed Wave 1 commits ended at `8340051` (`decomp(CGameType): spec game
  controller base`), after `C3DCursor`, `CViewPort`, and `CGameType`.
- Ledger regen is now SAFE: `python3 tools/decomp_ledger.py` preserves
  status/owner/confidence + manual notes on re-run (verified idempotent). Re-run it
  freely after you back-fill FourCC↔class names; it will NOT reset your progress.
- The Ghidra structs are still 4-byte seed structs only — `this[N].vftable` prints as
  `N*4` (see CGameObject spec note). Building real per-class structs (plan §0.3) as you
  go is the biggest readability win; do bases first so leaves inherit.

## Your task this session: start Wave 2 (player/friends/NPCs)
Start with `C3DPlayer`, then move through the player/friends/NPC family in ledger
order unless the DAG shows a better dependency order. For each placeable class, prefill
the property field map from `gam_schema.md` before reading code.

```
C3DPlayer
C3DJimmy, C3DNeutron, C3DRedNeutron
C3DFriends, C3DSheen, C3DCarl, C3DLibby, C3DCindy
C3DGoddard, C3DJudy, C3DHugh, C3DNick, C3DBenny, C3DKitty, C3DHumphrey
C3DPirate, C3DFleetCommander, C3DSumo, C3DAbductee
```

Known anchors: `C3DPlayer` inherits the already-specced `C3DFlyingObject` movement
base; prior scoping pinned the player per-frame integrator at `FUN_0041a140`, the
movement-base registrar at `FUN_00419f70`, and the vtable slot at `.rdata 0x49d398`.

## Inner loop (per class — see plan §5)
1. Set ledger `status=in_progress, owner=codex`.
2. If placeable (FourCC in `gam_schema.md`): pre-fill field-map/constants/wiring from
   the property table BEFORE reading code.
3. `DumpClass.java <ClassName>` → decompile vtable slots + field map; reconcile
   offsets against .gam registrar names. Resolve callees one level deep; document
   engine calls by their `OMedia*` name and move on (don't descend into OMT2.dll).
4. Write `docs/decomp/<ClassName>.md` from `_TEMPLATE.md`; link bases with `[[...]]`.
5. Set `status=spec` + confidence + open questions. Commit per class:
   `decomp(<Class>): spec base lifecycle`.
6. At wave end: append a 1-paragraph era entry to `PROJECT_HISTORY.md` and update
   ledger summary counts.

## Hard rules
- Commit ONLY decomp artifacts: `docs/decomp/**`, `docs/decomp_ledger.csv`, `tools/**`,
  and (at wave end) `PROJECT_HISTORY.md`. There is a large PRE-EXISTING dirty asset
  tree in the working copy — do NOT stage or touch it.
- Don't relitigate settled invariants (matrix convention, `PROJ[3][3]=1`, no X-mirror,
  `canvas_id=Canv+1`, DIFFUSE alpha 0, no fog) — see `PROJECT_HISTORY.md` §Invariants.
- Confirm every heuristic against the decompiled body, not just an offset/immediate
  scan (false-positive trap noted in plan §9).
- The ledger + per-class docs + `PROJECT_HISTORY.md` are the only source of truth.

## Definition of done for this session
Wave 2 classes handled this session have `docs/decomp/<Class>.md`, ledger rows at
`status=spec` with confidence + open questions, and per-class commits pushed on
`decomp-campaign`. If the whole wave closes, append a Wave 2 paragraph to
`PROJECT_HISTORY.md` and update this handoff again.
