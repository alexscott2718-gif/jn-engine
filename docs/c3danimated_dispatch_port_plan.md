# C3DAnimated Event-Animation Dispatch Port Plan

Drafted 2026-07-03 for the `linked` branch after the linkage campaign reached
14 `linked` / 15 `linked-blocked`.

## Goal

Port the original-shaped `C3DAnimated` event-to-animation dispatch into the native
engine so animation requests are selected, advanced, and completed through a
certifiable runtime path rather than static alias tables.

The first success target is logic fidelity:

- `SetAnim3DByName`-style lookup chooses the same animation record the recovered
  body would choose for a given object/alias request.
- `UpdateAnimated`-style advancement reaches last-frame state correctly.
- The base completion hook is no-op, while player-specific completion behavior has
  a native equivalent for the recovered Jimmy cases.
- A headless oracle proves dispatch/update/completion behavior. Visual pose quality
  and by-eye animation fidelity are follow-up work, not the first certificate.

## Current Evidence

Read these before editing code:

1. `docs/decomp/C3DAnimated.md`
2. `docs/decomp/evidence/c3danimated_target7.md`
3. `docs/linkage_progress.md`
4. `docs/linkage_certificates.csv` rows for `C3DAnimated`
5. `docs/linked_parity_plan.md` for the L1-L5 certificate contract
6. `src/game/behaviors/behavior_cutscene.c`
7. `src/game/behaviors/behavior_base.c`
8. `src/game/player_anim.c` and `src/game/player_anim.h`
9. `src/game/entity_visual.c` for how active visual aliases are consumed today
10. `docs/campaign_actor_animation_catalog.md` and
    `docs/campaign_actor_animation_catalog.json` for shipped actor alias coverage

Recovered L1 facts from target 7:

- `UpdateAnimated` is at `0040e050`.
- `SetAnim3DByName` is at `0040dd90`.
- Animation record creation/lookup/selection lives around
  `0040d4a0`, `0040d9e0`, `0040da30`, `0040dab0`, `0040db10`, and `0040df80`.
- DB initialization is at `0040e270`.
- Pause/enabled-state helpers are around `0040d350`, `0040e1f0`, and `0040e3e0`.
- Last-frame completion calls vtable-4 slot 65.
- Base `C3DAnimated` slot 65 is shared no-op/thunk `00472970`.
- `C3DPlayer` overrides slot 65 at `0043a900` for Jimmy completion cleanup.

## Non-Goals For The First Port

Do not combine these into the first certificate:

- Full vertex morph interpolation correctness.
- By-eye actor animation quality.
- Goddard texture/UV correctness.
- Replacing the approved native tank-turn movement.
- Full `C3DJimmy` gadget dispatch.
- Full menu/controller work.
- Certifying `ase_loader.c` against Neutron.exe. The `.ase` files are this
  project's exported/intermediate format, not the original binary format.

## Phase 0: Preflight

1. Confirm branch and cleanliness:

   ```bash
   cd ~/jn-engine
   git status --short --branch
   ```

2. If only generated date churn exists from a gate rerun, either restore it or
   intentionally commit it before port work. Do not mix gate-date churn with the
   dispatch port.

3. Run the baseline gates:

   ```bash
   python3 tools/build_vtable_parity_report.py
   make
   python3 tools/audit_faithfulness.py
   ```

4. Record the baseline result in the session notes. Do not edit certificates yet.

## Phase 1: Write The Native Linkage Map

Update `docs/decomp/C3DAnimated.md` before code changes.

Add or extend the `## Native Linkage` section with:

- Original function -> native target mapping.
- Native fields/state that will carry active animation alias, clip id, frame/time,
  loop/one-shot behavior, paused/enabled state, and completion latch.
- Deliberate deviations from OMedia DB internals.
- Explicit statement that this pass certifies dispatch/update/completion logic,
  not visual interpolation fidelity.

Expected mapping shape:

| Original | Native port target | Notes |
|---|---|---|
| `SetAnim3DByName` | `animated_dispatch_set_by_name` | Name/alias lookup and active clip selection. |
| `UpdateAnimated` | `animated_dispatch_update` plus existing animated base gates | Advance active clip, detect last frame. |
| Base slot 65 | default completion callback | No-op. |
| `C3DPlayer::OnPlayerAnimEnded` | player completion callback or explicit class hook | Cleanup for `FENCE`, `LADDER`, `SPLAT`, `HIT`-class cases. |

This commit may be docs-only if useful.

## Phase 2: Add A Small Dispatch Module

Prefer a narrow module rather than scattering the port through cutscene/player code:

- `src/game/animated_dispatch.h`
- `src/game/animated_dispatch.c`

Minimum API sketch:

```c
typedef enum AnimatedDispatchResult {
    ANIM_DISPATCH_NOT_FOUND = 0,
    ANIM_DISPATCH_SELECTED = 1,
    ANIM_DISPATCH_UNCHANGED = 2,
} AnimatedDispatchResult;

void animated_dispatch_init_entity(Entity *e);
AnimatedDispatchResult animated_dispatch_set_by_name(Entity *e, const char *alias);
void animated_dispatch_update(Entity *e, float dt);
const char *animated_dispatch_active_alias(const Entity *e);
int animated_dispatch_completed(const Entity *e);
```

Keep the data structure small and compatible with the existing `Entity` model. If
`Entity` lacks enough fields, add one compact state struct in the least invasive
place. Do not create per-class special cases until the generic path exists.

## Phase 3: Implement Lookup/Selection Semantics

Implement the native equivalent of `SetAnim3DByName` first.

Required behavior:

- Normalize or compare aliases in the same case-insensitive style used elsewhere
  in the port when original evidence allows it.
- Select the active record/alias for the entity.
- Reset frame/time state on a real selection change.
- Preserve current state on failed lookup unless the recovered body proves a clear
  failure side effect.
- Expose enough state for an oracle to inspect selected alias, frame/time, loop
  mode, and completion latch.

Initial data source options, in preference order:

1. Existing per-entity animation aliases already parsed from `.gam` / docs-backed
   class init data.
2. Generated campaign actor catalog alias tables.
3. A small static native table only when backed by `docs/decomp` class asset rows.

Do not invent aliases from filenames without a doc-backed mapping.

## Phase 4: Implement Update And Completion

Implement `UpdateAnimated`-shaped behavior:

- Respect existing animated visibility/level gates from `behavior_base.c`.
- Advance active clip time/frame when enabled and not paused.
- Detect last-frame reach for one-shot clips.
- Call the completion hook once per completion event.
- Base hook is no-op.
- Player hook handles the recovered Jimmy completion cases only where native state
  has a real counterpart.

If exact frame counts/fps are not available for every actor yet, the first oracle
can use synthetic records. Shipped-data coverage can come next.

## Phase 5: Wire One Runtime Caller First

Start with cutscene target animation requests in `behavior_cutscene.c`.

Reasoning:

- It is already a known approximation point.
- It is less likely to fight the approved player movement design.
- It creates visible payoff later without requiring full gadget/menu work.

Then consider these follow-ups, one at a time:

1. Generic animated entity update in `behavior_base.c`.
2. AI trigger `AIAnim` / `ActivateByAnim` dispatch in `behavior_ai_trigger.c`.
3. Player/Jimmy special completion hook integration.
4. Actor catalog preview capture driven by the engine path.

## Phase 6: Oracle Before Certificate

Add a headless oracle:

- `tools/linkage_oracles/C3DAnimated_dispatch.py`

First oracle scope:

- Compile the real native dispatch module or a small dumper that includes it.
- Feed synthetic animation records that cover case differences, missing aliases,
  selection changes, repeated same selection, looping clips, one-shot clips, pause
  or disabled state if ported, and completion hook calls.
- Compare native results to a Python reference transcribed from the recovered L1
  contract.
- Include mutation checks where practical: wrong alias comparison, missing reset,
  or skipped completion should turn the oracle red.

Second oracle/data pass:

- Walk shipped actor/cutscene aliases from catalog data.
- Report missing aliases separately from logic failures.
- Do not fail visual pose fidelity in this oracle.

Only update `docs/linkage_certificates.csv` from `linked-blocked` to `linked` once
there is a green, non-self-comparing oracle that tests the real native path.

## Phase 7: Verification Gates

Run after each C change:

```bash
python3 tools/build_vtable_parity_report.py
make
python3 tools/audit_faithfulness.py
python3 tools/linkage_oracles/C3DAnimated_dispatch.py
```

If cutscene runtime wiring changed, also run the relevant existing camera/cutscene
oracles to guard against regressions:

```bash
python3 tools/linkage_oracles/C3DCutSceneCamera.py
python3 tools/linkage_oracles/C3DMultiCutSceneCamera.py
```

## Recommended Commit Split

1. Plan/prompt docs.
2. `C3DAnimated` Native Linkage doc update.
3. Dispatch module plus synthetic oracle, no runtime wiring.
4. Cutscene animation requests routed through dispatch.
5. Completion hook path.
6. Shipped-data alias coverage and missing-alias report.
7. Certificate flip, if and only if the oracle satisfies L1-L5.

## Risks And Guardrails

- Avoid turning this into an animation renderer rewrite. Dispatch first.
- Do not certify `.ase` loader behavior as original binary behavior.
- Do not collapse player movement decisions into this port.
- Do not hardcode answers in the oracle; it must exercise the native path.
- Keep by-eye actor quality as a later native-port validation track.
- Keep generated catalog refreshes separate from behavior commits.

## Immediate Next Action

Start with Phase 1. The first implementation session should update the native
linkage map, define the dispatch module boundary, and produce the synthetic oracle
before broad runtime rewiring.