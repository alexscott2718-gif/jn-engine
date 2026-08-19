# Audit tooling

Checks that verify *written claims* rather than ported behaviour. The 2026-08 audit
found the project had strong gates for code and none for prose; this is the missing
half.

## `spec_check.py` — the gate

Compares all 208 class specs in `docs/decomp/` against three independent sources the
project already generates:

| Source | What it contributes |
|---|---|
| `docs/gam_schema.md` | FourCC↔class map, shipped instance counts, harvested properties, dominant `ObjectTag` |
| `docs/_gam_classids.tsv` | the class-id registrar scan (`Scan_ClassIds.java`) |
| `src/game/entities.c` | the native vtable binding table |

Findings are tiered by evidence strength: **T1** registrar-level (binary-derived),
**T2** shipped-data (`.gam` ObjectTag), **T3** engine-code (`entities.c` binding).

```bash
python3 tools/audit/spec_check.py                  # CI mode: fail on anything new
python3 tools/audit/spec_check.py --all            # every finding, baseline included
python3 tools/audit/spec_check.py --json out.json  # machine-readable
python3 tools/audit/spec_check.py --update-baseline
```

Exit `0` clean · `1` new findings · `2` bad invocation.

Needs no binary, no build, and no network. It runs in `make check`.

### The baseline

`docs/audit/spec_check_baseline.json` records the disagreements that already existed
when the check landed, so it could go green immediately and ratchet from there. The
check fails only on findings *not* in that file.

**Fixing a spec means deleting its entry from the baseline.** The check also warns
about baseline entries that no longer reproduce, so a fix can't silently rot. Only run
`--update-baseline` for a change you can justify in the pull request.

## `pe.py` — dependency-free PE reader

COFF and optional headers, section table, Rich header, imports, exports. No `pefile`,
no network, stdlib only.

```bash
python3 tools/audit/pe.py assets/exe/jnbg/Neutron.exe
```

Needs `assets/exe/` populated — see `docs/GAME_FILES.md`. This is what established the
2001-09-30 link date, the MSVC 6.0 RTM provenance from the Rich header, and that the
three shipped `OMT2.dll` builds export the same 1,358 names in the same ordinal order.

## Reproducing the audit findings

`docs/audit/06-open-questions.md` is the write-up; `docs/audit/06-openq.jsonl` holds
44 provenance-tagged records, one JSON object per line. Every record cites a path and
a line, or a byte offset in a named binary with its SHA-256 in `docs/binaries.sha256`.

Claims tagged `CONFIRMED` were observed directly. Claims tagged `INFERRED` state their
own falsifier. If you re-run something and get a different answer, that is a finding —
file it.
