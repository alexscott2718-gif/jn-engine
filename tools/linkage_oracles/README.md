# Linkage oracles

A **linkage oracle** is the headless faithfulness proof that lets a `(class,
aspect)` row in `docs/linkage_certificates.csv` reach status `linked` on the
`linked` branch. It is the L3 requirement of the Linkage Certificate (see
`docs/linked_parity_plan.md`).

## Contract

An oracle is a standalone Python script in this directory that:

1. builds inputs from **shipped data** (`assets/`, `.gam`/`.tsk` tables, catalog
   JSON) or embeds **decomp-derived test vectors**;
2. computes the **expected** result from the recovered decompiled body;
3. computes the **actual** result the native/ported logic produces;
4. asserts they match: **byte-exact** (parsers/serializers), **epsilon-exact**
   (deterministic float math), or **distribution-exact** (enumerations/tables);
5. prints one `PASS ...` line and exits **0**; on mismatch prints a diff and
   exits **non-zero**.

Hard rules:

- **No display, no audio, no gameplay, no QA.** Only static reproduction.
- **No hand-tuned magic** (L4): every constant traces to a decomp address or a
  shipped-data field. An oracle that just restates a tuned constant proves nothing.
- If the behavior's only ground truth is visual/aural (feel, texture, mix,
  location), it **cannot** have an oracle -> it is `linked-blocked`, not `linked`.

## Running

The gate runs every oracle referenced by a `linked` manifest row:

```bash
python3 tools/check_linkage_certificates.py            # validate + scoreboard
python3 tools/check_linkage_certificates.py --selftest # prove the gate catches a bad oracle
```

`tools/build_vtable_parity_report.py` calls the gate at the end, so the audit
cannot be regenerated while any `linked` claim is uncertified.

## Template

Copy `example_oracle.py` and replace the two stand-in functions with (a) the
value transcribed from the decompiled body and (b) the value the ported path
computes. Name it after the class it certifies, e.g. `C3DCutSceneCamera.py`.
