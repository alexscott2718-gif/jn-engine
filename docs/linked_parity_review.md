# Linked-Parity Review (audit of the `linked` branch)

Audited 2026-07-02 against `docs/linked_parity_plan.md` (Linkage Certificate
L1–L5) and `docs/linkage_progress.md`. Scope: every `linked` row in
`docs/linkage_certificates.csv` (8), plus legitimacy of every `linked-blocked`
note (10). The specific risk hunted: a row marked `linked` whose oracle proves
nothing — circular (native compared against itself), hardcoded (oracle restates
the port), or green-under-mutation (no teeth).

Method, per `linked` row:

1. **Gate green** — `tools/check_linkage_certificates.py` exits 0
   (8 linked / 10 blocked) and `--selftest` passes (gate accepts a green
   oracle, rejects a failing one).
2. **Oracle integrity** — read `tools/linkage_oracles/<Class>.py` and its
   `*_dump.c` harness; confirmed each compiles the **real, unmodified native
   source** (`cc` over `src/...`) and diffs it against an **independent
   reference** (a separate Python parser, a decomp-doc golden table, or the
   shipped `.gam` corpus) — never against a value produced by the same code.
3. **Mutation test** — perturbed the native C (constant flip / comparison
   change / clamp-order swap), re-ran the oracle, required RED, reverted,
   required green again. A sed that silently failed to apply was detected via
   `git diff` and would have been reported as `SED_NOOP`, so every RED below is
   a real kill.
4. **L2/scope** — checked each class doc's "Native Linkage" section exists,
   carries the method-map, and that the aspect name honestly bounds what the
   oracle certifies (with the "Not covered" residue explicitly listed).

L5 non-regression, re-run this audit: `make` green, `tools/audit_faithfulness.py`
green (0 findings across all 35 levels), `make web` green (needs
`source ~/emsdk/emsdk_env.sh` first — the bare non-interactive shell lacks
`emcc`; not a regression), gate green.

## Per-row verdicts — `linked` (8 rows)

| class / aspect | oracle vs. independent reference? | mutation applied (native side) | oracle under mutation | verdict |
|---|---|---|---|---|
| `CTaskList` / tsk-deserialization | C `task_parse_file` vs `tools/tsk_parser.py` + doc golden table; streams synthesized from the measured LV1B format | spawn-Y read offset `pos+4` → `pos+8` in `task_loader.c` | **RED** (newgame case), green after revert | **LINKED** — with the L1 caveat below |
| `CLoadLevel` / gam-deserialization | C `gam_load` vs `tools/gam_parser.py` over all 35 shipped `.gam` (3299 objects, 97 LOAD rows) | per-object checksum skip `fseek(f,4,…)` → `5` in `gam_loader.c` | **RED** (framing desync at Level1 obj 1), green after revert | **LINKED** — strongest row; real-corpus framing check has broad teeth |
| `CTaskList` / set-task-state | C `task_set_entity_state` vs an in-oracle transcription of `FUN_0045f990`'s loop + doc golden table | `strcasecmp` → `strcmp` at the write-match line | **RED** (case-varied writes return 0), green after revert | **LINKED** |
| `C3DCutSceneCamera` / 3cam-camera-math | C `cutscene_3cam_dist`/`_place` vs formula transcribed from the decomp doc, over all 136 shipped 3CAM rows | (a) zoom sign flip; (b) floor/ceiling clamp-order swap | **RED both** — (b) killed by Level7 `podcam` (MinDist>MaxDist), exactly as the oracle claims real data would | **LINKED** — the clamp-order claim is not just asserted, it demonstrably has teeth |
| `C3DMultiCutSceneCamera` / 3mca-offset-table | C `cutscene_mca_local_offset` vs doc jump table, over all 906 in-range CameraTypeN entries | `140.0f` → `141.0f` (case-1 Y offset) | **RED** (first case-1 entry), green after revert | **LINKED** — 6 out-of-table entries honestly excluded, not silently certified |
| `CJimmyGame` / initgame-seed | C `game_flow_init_game` vs decomp constants, with zero-baseline + idempotence checks | `JIMMYGAME_DEFAULT_MISSION_VAL 100` → `101` | **RED**, green after revert | **LINKED** — thin aspect (two constants + reset semantics) but honestly named and scoped |
| `C3DPatrolPoint` / on-arrive | C `behavior_ai_find_patrol_point` + `gam_prop_f` vs graph built independently by `gam_parser.py`, all 742 shipped 3PAT waypoints | `strcasecmp` → `strcmp` in tag lookup | **RED** — killed by genuinely case-mismatched shipped edges (`Lib1`→`lib2`, `lib4`→`Lib1` in Level1) | **LINKED** — real data exercises the case-insensitivity claim, not a synthetic case |
| `C3DAITrigger` / dispatch-graph | C `aitrig_activate`/`aitrig_apply_story_progress` (real code via the `fire_tag` test hook) vs expectations derived from the recovered bodies | story-table `CARLOUT` target SCENE `0x4b` → `0x4c` | **RED** (`CARLOUT_hit`), green after revert | **LINKED** — dumper stubs only spawn-gating/physics/Goddard-AI, all outside the certified aspect; the mutated core is the real compiled code |

**No row was green under mutation. No circular or hardcoded oracle was found.**
Every dump harness compiles the actual `src/` translation units; every reference
is computed by a different artifact (Python parser, doc-transcribed formula, or
golden table), and the golden constants trace to decomp addresses or shipped
data (L4 holds).

### Caveats worth recording (none block the verdicts)

1. **`CTaskList`/tsk-deserialization has the weakest L1 chain.** The doc admits
   the LV1B format was *measured from the real `NewGame.tsk`* (2026-06-08), not
   recovered from a decompiled Neutron.exe parse body, and `task_loader.c`'s own
   header says it is "a port of tools/tsk_parser.py" — so the oracle's two sides
   share ancestry, and its test streams are synthesized by the oracle itself. The
   ground truth anchoring the row is the measured real file plus the doc golden
   table (SCENE=30 etc.), which the doc defends (the class is a 2-slot trivial
   streamer; the persistent format *is* the behavior). Acceptable, but this row's
   "independence" is file-measurement-based, not decompilation-based; a future
   pass could pin the streamer vtable slot (doc's own open question).
2. **`CLoadLevel`** applies harness default constants (`-1`/`0.0`/`"none"`) on
   both sides for the 6 LOAD rows missing optional props — this deliberately does
   not certify that *native default values* match the original's defaults for
   unauthored properties (out of the aspect's scope; fine, but worth knowing).
3. **Orbit/dolly camera world-position** is excluded from both cutscene rows
   because `transform_local` (vtable `+0x384`) is still undecompiled — the docs
   and oracles say so explicitly. Honest scoping, and the right call; do not
   read "3cam-camera-math linked" as "cutscene camera fully linked".

## Per-row verdicts — `linked-blocked` (10 rows)

Spot checks confirmed each note's factual claims against the actual sources
(`behavior_checkpoint.c` feel comment, `behavior_prop.c` "fully none (inert)"
3TRI comment, `behavior_item.c` substring tool-grant design, `place_player`
static in the 2,480-line `main.c`, `C3DTriggerType.md` "still raw decompiler
output" flag).

| class / aspect | note legitimate? | statically provable but parked? |
|---|---|---|
| `C3DPlayer` / free-roam-feel | Yes — feel is by-eye by definition; plan seeds this carve-out | No — but note the plan itself says the movement *logic* is linkable via an input-trace oracle; that row simply hasn't been attempted yet (it's wave 8, not parked) |
| `C3DGoddard` / texture-uv | Yes — art fidelity, no headless oracle | No |
| `C3DCindy` / location-pathing | Yes — needs capture/original evidence; "do not guess" is correct | No |
| `C3DSoundEffect` / by-ear-mix | Yes — by-ear; dispatch logic correctly noted as separately linkable | No |
| `C3DCarl` / vehicle-rider-pose | Yes — visual pose | No |
| `C3DAnimated` / ase-deserialization | Yes — the self-comparison exclusion (ase_loader vs ase_parser both read the project's *own* re-export format) is exactly the circularity this plan forbids; L1 genuinely unsatisfiable | No |
| `C3DStartPoint` / spawn | Yes, and unusually honest — but **borderline**: `place_player` is faithful and *statically provable in principle*; the blockage is harness cost (static fn inside monolithic `main.c`), not missing evidence. This is the one blocked row that a modest extraction refactor would convert to `linked`. Flagged as the top candidate if the branch gets another pass. | **Borderline** (parked on cost, not on evidence class — note says so itself) |
| `C3DCheckPoint` / progress | Yes — native is a deliberate simplification (verified comment); no fidelity claim exists to certify. Real porting work, out of certification scope | No |
| `C3DPickupItem` / collection | Yes — worklist row named the wrong file; real 3PIC counterpart (`behavior_item.c`) is a deliberate different design (verified) | No |
| `CTrigger` / enter-exit-latch | Yes — three-way conflation correctly untangled; 3TRI native is inert (verified comment), C3DTriggerType L1 is raw (verified doc flag), TRIG RTTI unresolved | No — porting C3DTrigger's cascade is behavior work, not certification |

## Summary

- **8/8 `linked` rows are truly linked**: gate green, selftest green, oracle
  design non-circular, and every row goes RED when the native side is perturbed
  (9 mutations total, 9 kills, all reverted clean; `src/` clean after).
- **10/10 `linked-blocked` notes are legitimate**; one (`C3DStartPoint`/spawn)
  is blocked on harness cost rather than evidence class and is the first thing
  to reopen if the branch gets more static-linkage budget.
- L5 non-regression holds: `make` green, `audit_faithfulness.py` 0 findings /
  35 levels, `make web` green (with emsdk env sourced), gate green.
- Nothing in this audit changed any code; the mutations were transient and
  reverted, verified via `git status --porcelain src/` (empty).

Residual risks accepted: the CTaskList format-measurement caveat above, and the
general limit that one mutation per row proves the oracle is not inert, not
that it kills *every* possible deviation.

## Addendum (2026-07-02, same day): C3DStartPoint/spawn converted to `linked`

The audit's one borderline blocked row was closed by doing the extraction its
note proposed: `place_player` moved verbatim to `src/game/spawn.c` (+
`spawn.h`), unblocking a real L3 oracle
(`tools/linkage_oracles/C3DStartPoint.py`: all 100 shipped STRT rows across
the 35 levels, 270 requests incl. case-varied, `@default` and miss cases).
Audited to the same standard as the table above: four native-side mutations
(case-sensitivity, wrong teleport axis, dropped match guard, hardcoded
MusicIndex) all go RED. One probe mutation -- the `gam_prop_i` MusicIndex
fallback `-1` -- stays green because no shipped STRT omits MusicIndex; that
fallback is explicitly NOT certified and is documented in the doc's "Not
covered" list, consistent with the corpus-scoped honesty the other rows use.
Scoreboard is now **9 linked (oracle-verified) / 9 linked-blocked**.

## Addendum 2 (2026-07-02, same day): C3DPlayer movement-logic investigated

The blocked-table row above repeats the plan's claim that the player movement
LOGIC "is linkable via an input-trace oracle... it's wave 8, not parked."
Investigated same day: that claim is withdrawn. The movement bodies are not
function-defined in Ghidra (L1: no recovered accumulate->clamp body exists),
and the native player is the approved deliberate tank-turn simplification
(L2: no fidelity claim to certify; the dormant movement_base.c ramp is the
tuned "ice-skating" approximation the plan's L4 cites). The row stays
`linked-blocked` with a corrected note; the real path (Ghidra recovery of
five entry points -> 1:1 port replacing approved movement -> oracle) is
recorded in docs/decomp/C3DPlayer.md's Native Linkage section.
