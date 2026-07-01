# Linked-Parity Plan (branch: `linked`)

Written 2026-07-01. This branch has one job: push as many vtable-parity rows as
possible from `approximated`/`must-link` to **`linked`** (native behavior provably
follows the recovered decompiled body) **using only static/headless evidence — no
gameplay, no QA**. When the statically-linkable set is exhausted, merge back into
`native-port` and resume the reimplementation there, where the `linked-blocked`
residual (feel/art/audio/location) gets its by-eye/by-ear closure.

Assumed operator: **Sonnet 5** does most of the per-class work; a stronger model or
a human handles the hard decompilation recovery flagged below.

> Read alongside: `docs/vtable_parity_plan.md` (domains + must-link criteria),
> `docs/vtable_linkage_audit.md` (current status per class), `AGENTS.md`
> (anti-silo rule), `docs/native_port_plan.md` (§5 discipline, §N5 game-flow).

---

## 1. What "linked" means with no gameplay/QA — the Linkage Certificate

The normal parity plan proves `linked` largely through gameplay/QA (by-eye
cutscene review, by-ear audio, capture-with-input traces, playthroughs). Strip
those away and `linked` becomes a purely static standard. A row is `linked` only
with all five (the **Linkage Certificate**):

- **L1 — Body recovered.** `docs/decomp/<Class>.md` carries the *recovered
  function body* (pseudocode + field/offset semantics) for every must-link method,
  not just a field map. No body -> recover it first (Ghidra + `Neutron.exe`).
- **L2 — 1:1 transcription.** The native entry point mirrors that control flow:
  same branch structure, same accumulate/clamp order, same constants, same field
  semantics. Documented as a method-map table
  (`decomp addr -> native fn -> any deliberate deviation + why`).
- **L3 — Headless oracle passes.** A `tools/linkage_oracles/<Class>.py` feeds real
  shipped data (or decomp-derived vectors) through both a decomp-derived reference
  and the native path and asserts equality — **byte-exact** (parsers),
  **epsilon-exact** (deterministic math), or **distribution-exact** (enumerations).
  No possible oracle => the row is `linked-blocked`, not `linked`.
- **L4 — No hand-tuned magic.** Every constant traces to a decomp address or a
  shipped-data field. This is the specific guard against the "ice-skating" failure,
  which came from tuning an approximation instead of transcribing a body.
- **L5 — Non-regression.** `make`, `make web`, and `audit_faithfulness.py` (0/35 —
  headless, not QA) stay green.

The certificate is enforced by machine (see §5): no green oracle, no `linked`.

## 2. The honest target

True 100% is impossible here: rows whose only ground truth is visual/aural cannot
be closed without gameplay/QA. The goal is **100% of the statically-linkable
subset**, with the residual explicitly bucketed as **`linked-blocked`** and handed
back to `native-port`.

Note the inversion from the native-branch instinct: player free-roam *feel* is
exactly what this branch *cannot* close (its only faithful validator is
by-eye/capture-with-input). But the player *logic* (accel/decel accumulate->clamp
state machine, mode dispatch, anim-ended transitions) **is** linkable via a
headless input-trace oracle. "Logic linked" and "feel confirmed" split apart; this
branch owns the former.

### Scope buckets

| Domain | Statically linkable here | Returns to native-port |
|---|---|---|
| Progression / objectives (the 43-spec game-flow layer) | High — task tables, level-load, checkpoint state (`tsk_parser.py` proves the model) | — |
| Triggers / story sequencing | High — deterministic dispatch; enter/exit latch + NextTrigger cascade | — |
| Inventory / items / gadgets | High — pickup state tables, consume/score/NextTrigger | — |
| Camera / cutscene | High — scripted, deterministic, capture-reproducible math | final by-eye tuning |
| AI / pathing | High — patrol/seek/arrival/facing math | Cindy location (needs evidence) |
| Vehicles | Med-High — integrators, steering, AI paths (numeric) | rider-insertion poses (visual) |
| Menus / UI flow | Med — state-graph / input->transition logic | "looks right" layout |
| Player movement | Logic only — state-field oracle | free-roam **feel** |
| Animation / actor pose (53 approx) | Dispatch logic only — which anim on which event | pose/texture correctness, Goddard UV |

The day-one `linked-blocked` carve-out is seeded in
`docs/linkage_certificates.csv`: `C3DPlayer` (free-roam feel), `C3DGoddard`
(texture/UV), `C3DCindy` (location/pathing), `C3DSoundEffect` (by-ear mix),
`C3DCarl` (vehicle rider pose).

## 3. Branch model

- `linked` is cut off `native-port` and kept **rebased** on it. Work is additive
  refactors of `src/game/behaviors/*`, `docs/decomp/*`, and `tools/linkage_oracles/*`.
- At close, merge `linked -> native-port`. The branches compose: `linked` proves
  logic faithfulness statically; `native-port` then runs the by-eye/by-ear gates on
  the now-faithful logic and closes the `linked-blocked` residual.

## 4. Workflow (tuned for Sonnet 5)

A loop of small, cold-start-safe, one-class(-cluster)-per-task units. Each task is
self-contained (point Sonnet at the class doc + native file + an exemplar + this
plan). Two explicit phases per class — **separating them is what prevents
hand-guessing**:

1. **Recover.** Deepen `docs/decomp/<Class>.md` from `spec` to full body for the
   must-link methods (Ghidra pass on the listed vtable addresses). Output:
   pseudocode + method-map. *This is where a stronger model or human review pays
   off most* — e.g. the `C3DJimmy` cluster's "repair raw function boundaries" is
   genuinely hard; **flag those** rather than letting Sonnet guess.
2. **Transcribe + prove.** Port the body 1:1, write the L3 oracle, fill the linkage
   table. Iterate against the gate script (deterministic) until green.

Discipline:

- **Batch near-identical rows.** Don't hand-port the 40 `CLevel*Game` leaves or the
  53 shared-base animation rows — table-drive and special-case the outliers
  (`CLevel01FGame` death/restart, ...), per `native_port_plan.md` §N5.
- **Parallelism.** Independent domains (progression, triggers, inventory, cutscene)
  can run as concurrent Sonnet tasks (different behavior files); serialize within a
  domain.
- **Every port ships an oracle or is marked `linked-blocked`.** No oracle, no
  `linked`. Non-negotiable.
- **One source of truth.** Per class: small commit, regenerate the report + append
  to `PROJECT_HISTORY.md`. No agent-private memory (AGENTS.md anti-silo rule).

## 5. Tooling — the un-fakeable metric

- `docs/linkage_certificates.csv` — the manifest; one row per `(class, aspect)` at
  `linked` or `linked-blocked`. Authoritative for this branch.
- `tools/linkage_oracles/` — the L3 proofs (`README.md` = contract,
  `example_oracle.py` = template).
- `tools/check_linkage_certificates.py` — the **gate**. A `linked` row must have an
  existing `linkage_doc` and an `oracle` that runs green; a `linked-blocked` row
  must have a `note`. Emits `docs/linkage_progress.md` (the scoreboard). `--selftest`
  proves the gate accepts a green oracle and rejects a failing one.
- `tools/build_vtable_parity_report.py` calls the gate at the end of `main()`, so
  the audit **cannot** be regenerated while any `linked` claim is uncertified.

North-star metric: **`linked` (oracle-verified) count** in `linkage_progress.md`.
Do not track raw vtable %.

## 6. Gate stack (all headless — no gameplay/QA)

```
make  ->  audit_faithfulness.py (0/35)  ->  make web
      ->  python3 tools/check_linkage_certificates.py   (gate)
      ->  python3 tools/build_vtable_parity_report.py    (regenerates audit; re-runs gate)
```

`qa_web_verify.py` (16/16) is kept only as a **build-integrity** check, not as QA
validation.

## 7. Wave order (by static-verifiability, highest first)

1. **Metric tooling** — the oracle-gated gate + `linked-blocked` status. *(this commit)*
2. **Progression / game-flow** (the 43-spec layer) — parsers + task/checkpoint/
   level-load state; strongest oracles, biggest structural payoff.
3. **Triggers** — deterministic dispatch graph; unblocks cutscene/AI/audio linkage.
4. **Inventory / items / gadgets** — pickup state + consume/score/NextTrigger.
5. **Cutscene camera** — extend the already-proven distribution/math oracle approach.
6. **AI / pathing** (defer Cindy) and **Vehicles** (defer rider poses).
7. **Menus** (state-graph logic) and **Animation dispatch logic** (batch the 53).
8. **Player logic** — state-field oracle (feel deferred to native).
9. Sweep the `approximated` tail; anything left is `linked-blocked -> native`.

## 8. Exit criteria

- 100% of the statically-linkable subset at `linked`, each with a green oracle the
  gate enforces.
- Every remaining row explicitly `linked-blocked` with a one-line note naming the
  gameplay/eye/ear evidence it needs.
- `PROJECT_HISTORY.md` updated; `linked` merged into `native-port`; a short handoff
  listing the `linked-blocked` set as the native branch's resume-point.
