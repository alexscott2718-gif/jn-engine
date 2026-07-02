# Fable-5 review prompt — linked-parity progress audit

> Run this AFTER a Sonnet-5 linking session, in `~/jn-engine` on the `linked` branch.
> Recommended reasoning effort: **medium** (this is verification + mutation-testing,
> not derivation). The point of the review is to catch **over-claimed `linked` rows**:
> oracles that prove nothing.

---

You are auditing the `linked` branch's linkage progress. The risk you exist to catch:
a row marked `linked` whose oracle is circular, hardcoded, or doesn't actually diff the
native path against independent ground truth. Read `docs/linked_parity_plan.md`
(Certificate L1-L5) and `docs/linkage_progress.md` first.

**For every `linked` row in `docs/linkage_certificates.csv`:**
1. **Gate is green:** `python3 tools/check_linkage_certificates.py` passes, and
   `--selftest` passes.
2. **Oracle integrity (the core check):** read `tools/linkage_oracles/<Class>.py`.
   Confirm it compares the **native path** (compiled C dumper, or the native source's
   own constants/formula) against an **independent** reference (a Python reference
   parser, a decomp-transcribed formula, or measured shipped-data ground truth) — NOT
   the value against itself, and NOT an answer baked into the oracle (L4).
3. **Mutation test (do this — it's the teeth):** temporarily perturb the native side
   (flip a constant in the C loader, or a byte in the synthesized input) and re-run the
   oracle; confirm it goes **red**. Revert. An oracle that stays green under mutation is
   worthless — report the row as NOT truly linked.
4. **L2 map accuracy:** the decomp doc's `## Native Linkage` section maps real
   `decomp -> native` functions; deliberate deviations are called out.
5. **Scope honesty:** the `aspect` matches what the oracle actually proves (e.g.
   "deserialization" must not silently claim wiring/consumption that isn't tested).

**Also check:** every `linked-blocked` row has a legitimate note (genuinely needs
gameplay/eye/ear), and no statically-provable row was parked as blocked to dodge work.

**Output:** write `docs/linked_parity_review.md` — a per-row verdict table
(`row | oracle-pattern | mutation-test | verdict{OK / weak / over-claimed} | note`),
a summary count, and a short list of rows to fix or downgrade. Do **not** fix code
yourself; report. Commit the review doc and push.
