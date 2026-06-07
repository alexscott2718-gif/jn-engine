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
5. The three reference specs already landed: `docs/decomp/{CGfx,CGameObject,
   CLocalGameObject}.md`  (match this depth/format)

## Current state (branch: decomp-campaign)
- Phase 0 DONE: RTTI analyzer run + Ghidra project saved (annotated as of commit
  `d63c6c0`), DumpHierarchy/DumpClass/DumpFunctions/ApplyRttiClassMarkup committed,
  `_hierarchy.md` (208 C* classes), ledger seeded, .gam backfill at 55/93 named FourCCs.
- Phase 1 STARTED: 3 of 25 base classes specced (CGfx, CGameObject, CLocalGameObject),
  all `status=spec` / `confidence=Medium` in the ledger.
- Ledger regen is now SAFE: `python3 tools/decomp_ledger.py` preserves
  status/owner/confidence + manual notes on re-run (verified idempotent). Re-run it
  freely after you back-fill FourCC↔class names; it will NOT reset your progress.
- The Ghidra structs are still 4-byte seed structs only — `this[N].vftable` prints as
  `N*4` (see CGameObject spec note). Building real per-class structs (plan §0.3) as you
  go is the biggest readability win; do bases first so leaves inherit.

## Your task this session: finish Phase 1 (the 22 remaining base/framework classes)
Bases unblock every leaf, so do them strictly bases→derived. Remaining set (confirm
the exact DAG against `_hierarchy.md` before ordering):

```
C3DObject  ← do FIRST (root 3D base; almost everything inherits it)
then: C3DAnimated, C3DSprite (the two mid-level bases)
then: C3DOmtObj, C3DAI, C3DAIOmtObj, C3DVehicle, C3DFlyingObject, C3DEnemy
then sprite chain: C3DSpriteType, C3DPermanentSprite, C3DTriggerType, CPickupType,
      C3DAnimatedSprite, C3DPickupItem, C3DPickupType
then: C3DTrigger, C3DProjectile, C3DCamera, C3DCursor, CViewPort, CGameType
```

Note `C3DFlyingObject` = the movement base (registers MaxSpeed/UpRate/DownRate/
NewGravity/lean that C3DPlayer inherits) — spec it carefully; Wave 2's player work
depends on it.

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
The 22 remaining Phase-1 base classes have `docs/decomp/<Class>.md` at the depth of the
existing three, their ledger rows at `status=spec` with confidence + open questions, a
`PROJECT_HISTORY.md` era paragraph for Phase 1, and per-class commits on
`decomp-campaign`. This batch is then ready for Alex's spec-review gate.
