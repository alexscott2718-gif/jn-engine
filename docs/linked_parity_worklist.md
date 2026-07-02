# Linked-Parity Worklist

The ordered queue for the `linked` branch. Pick the top unchecked row, certify it
(the recipe below), commit, push, repeat. Metric lives in `docs/linkage_progress.md`;
contract in `docs/linked_parity_plan.md` (Linkage Certificate L1-L5).

## Per-row recipe (the loop)

1. **Read** the decomp doc + the native file + the reference (parser/formula).
2. **Recover (L1)** — if the decomp doc lacks the recovered body for the methods you
   touch, deepen it (Ghidra on the listed vtable addrs). *Flag genuinely hard
   boundary-repair rows instead of guessing.*
3. **Transcribe map (L2)** — add a `## Native Linkage` section to the decomp doc: a
   `decomp -> native` method/field map + any deliberate deviation. (See
   `docs/decomp/CTaskList.md` for the worked example.)
4. **Oracle (L3)** — write `tools/linkage_oracles/<Class>.py` using one pattern:
   - **P1 byte-exact** (best; use when the native parse/logic unit is libc-only
     compilable): synthesize inputs from the format/decomp, compile the native unit
     into a headless dumper (see `ctasklist_dump.c`), and diff native vs the Python
     reference **byte-identically**. This is the CTaskList pattern.
   - **P2 reproduce-vs-ground-truth** (when the native code is engine-entangled):
     transcribe the decompiled formula/table in the oracle, run it over **all shipped
     rows** (real `.gam`/catalog data), and assert the result matches the measured
     ground truth (distribution-exact / value-exact) AND that the native source
     carries the same constants (extract/compile a small snippet, or assert the
     shared constant). No hand-tuned magic (L4).
   - If no headless oracle is possible, the row is **`linked-blocked`**, not `linked`.
5. **Certify** — add the `(class,aspect,domain,linked,oracle,linkage_doc,)` row to
   `docs/linkage_certificates.csv`.
6. **Gate (L5)** — `python3 tools/build_vtable_parity_report.py` (runs the gate),
   then `make` + `python3 tools/audit_faithfulness.py` (0/35) if you touched C.
7. **Record** — regenerated `docs/linkage_progress.md` + a line in `PROJECT_HISTORY.md`.
8. **Commit one row, push.** Small commits; the branch stays rebased on `native-port`.

## Queue

### Tier A — byte-exact format oracles (P1; strongest, fastest)
- [x] **CTaskList / tsk-deserialization** — `task_loader.c` <-> `tsk_parser.py`. DONE
  (5b7a14a) — the worked example.
- [x] **CLoadLevel / gam-deserialization** -- DONE (this commit). — `src/engine/assets/gam_loader.c` <->
  `tools/gam_parser.py` (`parse_gam`). Check if the record/pstring parse is libc-only
  compilable (P1); else transcribe (P2). `docs/decomp/CLoadLevel.md`, `gam_schema.py`.
- [~] **C3DAnimated / ase-deserialization** -- FLAGGED, not linkable here (2026-07-02).
  `.ase` is this project's own OMT->ASE exporter output (PROJECT_HISTORY.md Era 2), not
  a Neutron.exe format -- no decompiled body exists to certify `ase_loader.c` against
  (L1 unsatisfiable); the real original mesh format is OMT/3DSP (already decoded,
  `docs/omt_3dsp_format.md`). Recorded `linked-blocked` in `docs/linkage_certificates.csv`
  with the full reasoning; do not retry as a byte-exact row.
- [x] **CTaskList / set-task-state** -- DONE (this commit). — `task_set_entity_state` <-> `FUN_0045f990`
  (write EXISTING entries only, never append). Small P1 (task_loader.c is libc-only).
  `docs/decomp/CTaskList.md`.

### Tier B — deterministic math / dispatch (P2; reproduce over shipped data)
- [x] **C3DCutSceneCamera / 3cam-camera-math** -- DONE (this commit; scoped to dist formula + CameraType precedence, see doc). — `behavior_cutscene.c` <-> Neutron.exe
  `00415f90`. Reproduce static/orbit/dolly placement + `dist=clamp(InitialDist-
  ZoomSpeed*t,Min,Max)` for **all shipped 3CAM rows**; assert finite/sane + the known
  distribution (VFC {1:121,0:9,3:6}, CT {0:95,2:16,3:15,1:10}). `C3DCutSceneCamera.md`.
- [x] **C3DMultiCutSceneCamera / 3mca-offset-table** -- DONE (this commit; local-offset table only, see doc). — reproduce the `CameraTypeN`
  target-local offset table over shipped 3MCA rows. `C3DMultiCutSceneCamera.md`.
- [x] **CJimmyGame / initgame-seed** -- DONE (this commit; seed only, win-bridge + death flow not decompiled, see doc). — `game_flow.c` <-> `InitGame` (lives=5,
  mission_value=100, level-clear -> `level_objective_met`). Reproduce seed + win-bridge
  transitions. `docs/decomp/CJimmyGame.md`.
- [ ] **C3DStartPoint / spawn + C3DCheckPoint / progress** — `00442740` / `00414410`
  <-> `behavior_checkpoint.c`. `docs/decomp/C3DStartPoint.md`, `C3DCheckPoint.md`.
- [ ] **C3DPatrolPoint / on-arrive + AI patrol** — `00434ea0` <-> `behavior_ai.c`.
  Patrol arrival/next-select/facing math over shipped PatrolPoint chains.
  `docs/decomp/C3DPatrolPoint.md`. (Defer Cindy — `linked-blocked`.)
- [ ] **C3DAITrigger / dispatch-graph** — `behavior_ai_trigger.c`. Deterministic
  hide/show/AINewPos/AINewRotY/AIPatrol/ToggleObject/NextTrigger mutation of a
  synthetic target. `docs/decomp/C3DAITrigger.md`.
- [ ] **C3DPickupItem / collection** — `00435ce0/00436200/00436830` <->
  `behavior_pickup.c`. Consume required-picture, score, NextTrigger. `C3DPickupItem.md`.
- [ ] **CTrigger / C3DTriggerType / enter-exit-latch** — `00447400..` <->
  `behavior_trig.c`. Enter/exit latch + NextTrigger cascade trace. `CTrigger.md`.

### Tier C — logic-only (feel/visual stays linked-blocked)
- [ ] **C3DPlayer / movement-logic** — state-field oracle: drive a synthetic input
  trace, assert `current_speed`/`walk_speed`/`player_mode`/anim-phase evolve per the
  decompiled body (`00437940`, `00438bc0`, `00439900`). Feel = `linked-blocked`.
- [ ] **CMainMenu / C2DInGameMenu / menu-state-graph** — input -> screen/selection
  transitions. Layout "looks right" stays for native-port.
- [ ] **C3DAnimated dispatch (batch the 53 approx animation rows)** — which anim on
  which event; pose/texture correctness stays `linked-blocked` (Goddard UV etc.).

## Already `linked-blocked` (do not attempt here)
`C3DPlayer` free-roam feel, `C3DGoddard` texture/UV, `C3DCindy` location,
`C3DSoundEffect` by-ear mix, `C3DCarl` rider pose. See `docs/linkage_certificates.csv`.

## Stop / handoff
When Tier A+B are certified (or a row needs decomp evidence you can't recover
statically), stop and hand to the Fable-5 review pass
(`docs/linked_parity_review_prompt.md`).
