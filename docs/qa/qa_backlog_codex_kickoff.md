# Codex Kickoff — Resume the QA Backlog Campaign (Session 2)

> Paste everything below the line into a fresh Codex session running in `/home/scotty/jn-engine`.
> (Codex loads `~/AGENTS.md`, which is the symlink to the shared project instructions.)

---

You are continuing a multi-session **QA ticket resolution campaign** in `/home/scotty/jn-engine`,
branch **`native-port`**. A previous session (Claude) closed 11 of ~24 reports; your job is to
continue from the living handoff and drive the rest to done, then finalize (deploy + tracked pages).

This is a **faithful reimplementation**: the decomp specs (`docs/decomp/<Class>.md`) and authored
`.gam`/OMT data are ground truth. Do not invent "engine-tuned" behavior. **Verify the visible/playable
result (pixels), not just that a function ran.**

## Read first, in this order
1. `docs/qa/qa_backlog_campaign_handoff.md` — **the source of truth for this campaign.** Full 24-report
   ledger with per-item status, root-cause notes, the audio-system investigation, the "Resume here
   (Session 2)" attack order, and the Finalize checklist. Keep its Status column updated as you work.
2. `docs/qa_ticket_resolution_workflow.md` — the shared contract (intake, fix philosophy, resolution-log
   page format, verification gates).
3. `~/AGENTS.md` — machine/workflow conventions (build toolchain, deploy, anti-silo policy).
4. Per-item: the matching `docs/decomp/C3D*.md` spec before changing any entity's behavior/visual.

## What's already done (committed on `native-port`, do not redo)
3SPH→invisible; foot-anchor 3HUG/3CIN; 3FLE→fleet commander; enemy PROJ→missile; l1 tree billboard
gated to the level1 family (l3c tree04 now uses its glb texture); red-neutron size floor + bigger
pulse. Sweep is **0 findings**. **#12 apple-pie is WONTFIX-as-bug** (authored FruitbowlEmty; the pie is
a deferred fruit-fill mechanic). The native fixes are committed but **not yet web-deployed and have no
`docs/qa/<reporter>` pages** — that's the Finalize step.

## Your task
Work the **"Resume here (Session 2)"** list in the handoff, tractable→hard:
#14 3JIM l1 lab-facing (ORI; ⚠️ re-aim `qa_web_verify.py`'s sky probe if you move spawn yaw) →
#15/#6 buttons (orientation + flash + float) → #16/#17 l1b 3ARR/3PIC → #5/#4 boat & Cindy paths →
#13 house02 floor → Group I audio (#7,#18–#24; see the handoff's audio notes — `C3DSoundEffect`
halts on exit and `C3DMusicTrigger` replaces, so the l3a stacking + l1a shrink-ray-as-music are
elsewhere, notes inside). Then **Finalize**.

## How to work (commands)
- Native build: `make` (zig toolchain; expect exit 0). Kill stray game: `pkill -x jnengine`.
- Aimed screenshot from a reporter's camera (this is how to verify visually):
  `bash tools/qa_shot.sh OUT.png EX EY EZ CX CY CZ <level> <dist>` (uses the report's entity+cam JSON;
  renders headless via xvfb). Read the PNG back to confirm pixels.
- Runtime entity probe: `JN_DUMP_ENTITY_TAG=<tag|type> JN_SCREENSHOT=1 JN_SCREENSHOT_PATH=/tmp/x.png \
  JN_SCREENSHOT_WARMUP_TICKS=N xvfb-run -a ./jnengine --level <lvl>` (prints pos/state). Audit dump:
  prefix `JN_AUDIT=1`.
- Regression gate: `python3 tools/audit_faithfulness.py` (must stay 0 findings).
- Commit per fix with a root-cause message; you may push `native-port` as you go. Use **your own**
  commit trailer convention (do **not** copy Claude's Co-Authored-By/Session trailers).

## Finalize (run once, when the batch is ready — see handoff for the full checklist)
`make` → `audit_faithfulness.py` (0) → `source ~/emsdk/emsdk_env.sh && ./tools/deploy_wasm.sh`
(387 MB, **outward-facing/public** — the user pre-authorized deploying these QA fixes, but confirm
before the first public deploy of the batch) → `python3 tools/qa_web_verify.py` (16/16). Then write
**four** tracked resolution pages — `docs/qa/sandmanfan-2026-06-24/`, `docs/qa/awefan-2026-06-14/`,
`docs/qa/awefan-2026-06-14b/`, `docs/qa/lu9-2026-06-14/` — copying the dark-mono template from
`docs/qa/sandmanfan-2026-06-12b/index.html`, with before/after shots; mirror each to
`/var/www/jn-engine/qa/<folder>/` and `diff -qr`. Update `docs/PROJECT_HISTORY.md` with the new
invariants. **In the resolution pages and history, attribute the work to Codex** (and credit the
earlier session's fixes as already-landed) — keep authorship truthful per the workflow.

## Constraints / gotchas
- Accept-all-edits autonomy between checkpoints; **visual QA (screenshots) is the gate**. Show the
  user before/after pixels at checkpoints.
- Keep settled conventions: `.gam` rotations `rx,ry=+deg·π/180`, `rz=−deg·π/180`; authored
  `sprites.omt` canvases beat guessed defaults; `C3DOmtObj` uses `OmtDatabase`/`OmtIndex`.
- Proprietary assets/captures aren't in git. The XP machine (original game over VNC) is available if a
  fix needs an original-vs-port comparison (e.g. the red-neutron size floor `170` is a chosen value to
  confirm).
- Don't relitigate the WONTFIX (#12) without new evidence.
