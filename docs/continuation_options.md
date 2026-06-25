# Continuation Options — where the jn-engine native port can go next

> **Purpose:** a decision menu for the next session (or contributor). Written 2026-06-25,
> right after the QA backlog campaign shipped and the motion/path follow-up moved it to 19/24 closed.
> It enumerates
> the open work tracks with their effort / impact / prerequisites / entry points so you can
> *pick* rather than re-derive. This is a pointer doc — the authoritative state still lives in
> [`PROJECT_HISTORY.md`](./PROJECT_HISTORY.md), [`native_port_plan.md`](./native_port_plan.md),
> [`decomp/_next_session.md`](./decomp/_next_session.md), and the QA living handoff
> [`qa/qa_backlog_campaign_handoff.md`](./qa/qa_backlog_campaign_handoff.md).

## State at a glance (2026-06-25)
- **Branch:** `native-port`.
- **Behavior coverage:** 93 / 93 used-in-level FourCCs have a native vtable. The former
  `3ROK`/`3SPR`/`3DAI` resolver/positioning tail is explicitly native-inert, preserving
  its hidden/non-solid resolver output.
- **Story machine:** the SCENE sequencer (both writers) + the C3DGoddard companion slice are in.
- **Web build:** live at `exentt.com/jn-engine` (`jnengine.aef9117b.js`, assets `63737d29`), Campaign toggle reachable, `qa_web_verify.py` 16/16.
- **QA backlog:** 19/24 fixed, 2 WONTFIX-as-faithful; remaining open work is Group I audio (#18–24).
- **Gates that must stay green every change:** `make` → `python3 tools/audit_faithfulness.py`
  (0 findings / 35 levels) → `make web` → `python3 tools/qa_web_verify.py` (16/16) → `./tools/deploy_wasm.sh`.

## The options (each is a self-contained track)

| # | Track | Effort | Impact | Headless-verifiable? |
|---|-------|--------|--------|----------------------|
| A | Finish the QA backlog tail (motion/path #4,#5) | Done | Motion/path reports closed | Yes |
| B | Group I audio (#18–24) + audio-faithfulness pass | High | Closes reports + new subsystem | **No — needs by-ear/desktop** |
| C | Close the behavior lens to 93/93 (`3ROK`/`3SPR`/`3DAI`) | Done | Breadth milestone complete | Yes |
| D | Deferred gameplay mechanics (shrink-ray, fruit-fill, …) | High | Real gameplay, not just visuals | Partly |
| E | Campaign playthrough hardening (end-to-end) | High | The game becomes *completable* | Partly |
| F | Motion/path fidelity (boat water + Cindy path) | Done | Float/stray-class reports closed | Yes |
| G | Collision containment (open-shell houses, walls) | Med | Stops out-of-bounds / see-inside | Yes |
| H | Contributor / capture-rig tasks (asset truth) | Med | Exact sizes/textures, offloadable | N/A (tooling) |

---

### A. Finish the QA backlog tail — motion/path (#4, #5) — DONE 2026-06-25
- **#5 l1 boat (3SAI) is closed:** `3SAI` now visually anchors to the `ncwater*` surface while the
  authored gameplay Y stays intact. The prior "off-river" concern was corrected by auditing actual
  `ncwater` mesh bounds; the whole `BOAT2 -> boat7` chain is inside the river mesh.
- **#4 l3d Cindy (3CIN) is closed:** grounding was already fixed, and `vt_cindy` now honors terminal
  patrol routes (`c1 -> c2 -> none`) instead of synthesizing a return loop. Cindy routes that should
  loop already encode it explicitly (`CINDY7 -> CINDY1`).

### B. Group I audio (#18–24) + an audio-faithfulness pass
The remaining audio reports, plus the broader gap they expose.
- **#22 l3a ride-track stacking** (looping sound never stops, stacks on re-entry): 3SOU is confirmed
  clean (halts on exit), so the emitter is unidentified — needs an l3a runtime audio trace.
- **#23 l1a shrink-ray-as-music:** the level1a start object likely authors a wrong/voice music DB, or
  the music-handle resolution falls back to an SFX bank — instrument the resolved `music_database` +
  `MusicIndex` in `main.c`.
- **#18–21 l1c furniture proximity sounds** (piano/TV/radio/toilet/bed): these are *room glb* placements
  with no proximity emitter — either author/synthesize positional `C3DSoundEffect`-style emitters at the
  props, or find the rows the original places. Largest/most speculative.
- **#24 l1 RocketPad voice line.**
- **⚠️ Hard constraint:** the headless **xvfb rig has no audio device**, so none of these can be
  verified *by ear* in CI — they need a desktop session (noVNC) for confirmation. The door-loop fix (#7)
  is the template: you can verify the *state machine / resolved db+index* headlessly, but not the sound.
  Entry points: `audio.c`, `behavior_soundfx.c`, `behavior_music.c`, the level start-object music wiring
  in `main.c`.

### C. Close the behavior lens to 93/93 — DONE 2026-06-25
The Asset Catalog's last three used-in-level FourCCs without a clean native vtable were
**resolver/positioning gaps, not behavior gaps**: `3ROK` (origin pool), `3SPR` (no serialized canvas),
`3DAI` (origin dummy). They now route to `vt_resolver_inert`, which keeps them hidden/non-solid
instead of inventing placement, canvas defaults, or AI motion. `behavior_todo.md` now reports
93 used FourCCs, 93 with native vtables, 0 missing.

### D. Deferred gameplay mechanics
Several mechanics were faithfully **deferred** (the port records the fact + gates the visual, but the
active mechanic isn't live). Turning these on is where the project shifts from "renders the levels" to
"plays like the game":
- **Shrink-ray active transition:** the ray turns Dino/Darwin/Humphrey AI into small *moving pickups*
  (game-owner ground truth); the classes already carry a pickup base, the transition is deferred.
- **Fruit-fill / "Apple Pie%"** (the real story behind QA #12): the fruit-bowl→pie mechanic.
- **Goddard** fetch/scripted-control is in; further companion behaviors (per `C3DGoddard.md`) remain.
- Each needs its decomp spec read first and a subsystem (effects/timers/inventory) that may not be ported.

### E. Campaign playthrough hardening
The SCENE story machine + Goddard landed, and the web Campaign toggle exists. The next structural step is
making a campaign **completable end-to-end**: level-transition correctness across all 22+ levels, mission
completion conditions, checkpoints, and the cutscene/trigger chains that gate progress. This is the
highest-impact "is it a game yet?" track. Entry points: `game_flow.c`, `task_loader.c`,
`behavior_ai_trigger.c`, `behavior_load.c`, `behavior_checkpoint.c`, `behavior_cutscene.c`.

### F. Motion/path fidelity (boat water + Cindy path) — DONE 2026-06-25
The water-surface query is in place for `3SAI`, the level1 boat route has been audited inside the
river bounds, and Cindy now follows authored patrol termination. General patrol cleanup can still
happen opportunistically, but the QA motion/path tail is closed.

### G. Collision containment
The #13 investigation surfaced this: background houses are open-bottomed facades (faithful), but the
free camera / player can see inside because there's no wall collision keeping them out. A containment
pass (wall collision on facades, level-perimeter blockers) would stop out-of-bounds viewing without
inventing geometry. Lower priority; verify it doesn't change collision faithfulness. Entry points:
`collision.c`, `behavior_prop.c`, the `BLOCK*`/`Blocking*` collider conventions in `main.c`.

### H. Contributor / capture-rig tasks
Offloadable, self-contained data-truth work that doesn't block the engine:
- The **object-capture rig** (`docs/CONTRIBUTOR_object_capture_plan.md`): camera no-clip + pose-queue to
  capture D3D7 maps sector-by-sector for exact tree sizes / textures.
- **Asset Catalog** coverage upkeep (`tools/build_asset_catalog.py`, live at `exentt.com/JN-assets/catalog/`).

---

## Recommended ordering (opinion, not a mandate)
1. **B (audio)** — the only QA backlog category left; batch it for a session where a **desktop/noVNC** is available for by-ear checks. Until
   then only the resolution-path instrumentation is verifiable.
2. **E (campaign playthrough)** — the biggest "is it a game" jump; do it once the actors are all present.
3. **D / G / H** — opportunistic; D when a mechanic is specifically requested, G/H as polish.

> If you want to clear the QA board: **B audio** is next, preferably in a desktop/noVNC session.
