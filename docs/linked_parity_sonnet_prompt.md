# Sonnet-5 kickoff prompt — linked-parity campaign

> Paste the block below into a fresh Sonnet 5 session working in `~/jn-engine`
> (reach the box via the gateway SSH). Recommended reasoning effort: **high**
> (drop to medium for the Tier-A byte-exact rows that mirror CTaskList).

---

You are on the `linked` branch of `~/jn-engine`. Your job: move vtable-parity rows to
**`linked`** — native behavior *provably* follows the recovered decompiled body —
using **only static/headless evidence. No gameplay, no QA, no by-eye, no by-ear.**

**Orient (read in this order):**
1. `AGENTS.md` — conventions + the anti-silo rule (durable facts go in committed
   markdown, never private memory).
2. `docs/linked_parity_plan.md` — the contract: the Linkage Certificate **L1-L5** and
   the honest target (100% of the *statically-linkable* subset; the rest is
   `linked-blocked` and returns to `native-port`).
3. `docs/linked_parity_worklist.md` — the ordered queue + the per-row recipe and the
   two oracle patterns (P1 byte-exact, P2 reproduce-vs-ground-truth).
4. **The worked example** — commit `5b7a14a`: `docs/decomp/CTaskList.md` (the
   `## Native Linkage` section), `tools/linkage_oracles/CTaskList.py`, and
   `ctasklist_dump.c`. Copy this shape.

**The loop (one row per commit):**
- Take the top unchecked worklist row. Follow the recipe: Recover (L1) -> Transcribe
  map (L2) -> Oracle (L3) -> add the `docs/linkage_certificates.csv` row -> gate (L5).
- **A `linked` row MUST have a green oracle that diffs the native path against a
  reference or measured ground truth. No oracle -> `linked-blocked`, not `linked`.**
  Never mark `linked` off a self-comparing or answer-hardcoded oracle (L4).
- If a row needs Ghidra body-recovery you can't do cleanly (e.g. raw function-boundary
  repair like the C3DJimmy cluster), **stop and flag it** rather than guessing.

**Gates every commit (all headless):**
```
python3 tools/check_linkage_certificates.py            # the gate; --selftest to sanity-check
python3 tools/build_vtable_parity_report.py            # regenerates the audit, re-runs the gate
make && python3 tools/audit_faithfulness.py            # only if you touched C; expect 0/35
```
Record each landing in `PROJECT_HISTORY.md`; commit one row; `git push`.

**Do NOT:** touch the 5 `linked-blocked` rows; run the game/QA to "confirm"; relitigate
settled invariants; add scene-specific overrides.

**Stop when** Tier A + Tier B are certified (or you hit a recover-blocked row). Leave a
one-paragraph handoff in `PROJECT_HISTORY.md` and hand off to the Fable-5 review pass
(`docs/linked_parity_review_prompt.md`).
