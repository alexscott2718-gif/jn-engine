# Fresh-session prompt - C3DAnimated event-animation dispatch port

Paste the block below into a fresh coding-agent session started on the gateway in
`~/jn-engine`.

---

You are on the `linked` branch of `~/jn-engine` on the gateway machine. Your job is
to activate the `C3DAnimated` event-animation dispatch port plan and make the first
implementation slice without broadening scope.

Read first, in this order:

1. `AGENTS.md` if present, plus `docs/PROJECT_HISTORY.md`.
2. `docs/c3danimated_dispatch_port_plan.md`.
3. `docs/decomp/C3DAnimated.md`.
4. `docs/decomp/evidence/c3danimated_target7.md`.
5. `docs/linkage_progress.md` and the `C3DAnimated` rows in
   `docs/linkage_certificates.csv`.
6. `docs/linked_parity_plan.md` for the L1-L5 certificate rules.
7. Native files: `src/game/behaviors/behavior_cutscene.c`,
   `src/game/behaviors/behavior_base.c`, `src/game/player_anim.c`,
   `src/game/player_anim.h`, and `src/game/entity_visual.c`.

Context:

- The branch currently has 14 oracle-verified `linked` rows and 15
  `linked-blocked` rows.
- `C3DAnimated / event-animation-dispatch` is blocked because native currently uses
  static actor-pose aliases and separate Jimmy animation logic, not the original
  OMedia animation-record dispatcher.
- Target 7 recovered the L1 bodies. The blocker is now native-port work, not lack
  of evidence.

Goal for this session:

1. Confirm the worktree state and run the baseline gates if they have not just run:

   ```bash
   cd ~/jn-engine
   git status --short --branch
   python3 tools/build_vtable_parity_report.py
   make
   python3 tools/audit_faithfulness.py
   ```

2. Do Phase 1 from `docs/c3danimated_dispatch_port_plan.md`: update
   `docs/decomp/C3DAnimated.md` with a concrete `## Native Linkage` map for the
   dispatch port.

3. Start Phase 2 only if the linkage map is clear: add the narrow dispatch module
   boundary (`src/game/animated_dispatch.h/.c`) and a synthetic oracle skeleton at
   `tools/linkage_oracles/C3DAnimated_dispatch.py`.

4. Do not flip `docs/linkage_certificates.csv` to `linked` yet unless the oracle is
   complete, green, non-self-comparing, and exercises the real native path.

Hard boundaries:

- Do not attempt full vertex morph visual fidelity in this slice.
- Do not rewrite player movement.
- Do not start the Jimmy gadget/menu port.
- Do not certify `ase_loader.c` as original Neutron.exe behavior.
- Do not mix generated catalog refreshes or gate-date churn into the behavior commit.
- If the worktree has unrelated user changes, preserve them.

Implementation discipline:

- Keep commits small: docs linkage map first, dispatch/oracle second, runtime wiring
  third.
- Run `make` and `python3 tools/audit_faithfulness.py` after C changes.
- Run existing cutscene camera oracles if `behavior_cutscene.c` changes.
- Record meaningful progress in `docs/PROJECT_HISTORY.md`.

Definition of done for the first useful slice:

- `C3DAnimated.md` explains the original-to-native dispatch map and deliberate
  deviations.
- A small native dispatch module exists or its exact API is documented if you stop
  before code.
- The oracle plan/skeleton exists and makes clear what will be tested.
- The branch remains buildable and the faithfulness audit stays at 0 findings.
- Any remaining blockers are specific and written into the docs, not left in private
  session memory.