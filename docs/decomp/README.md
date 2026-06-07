# Neutron.exe Gameplay Decomp Specs

This directory is the durable output for the full-tier decomp campaign. Each gameplay class gets one spec markdown file using `_TEMPLATE.md`; `docs/decomp_ledger.csv` is the resumable state machine for the campaign.

## Phase 0 Artifacts

- `_hierarchy.md`: RTTI-derived inheritance DAG for 208 non-OMedia gameplay classes.
- `_ghidra_markup.md`: saved Ghidra class namespace/vftable/thiscall markup summary.
- `../decomp_ledger.csv`: one row per gameplay class with wave, status, and notes.
- `../gam_schema.md`: `.gam` property accelerator for the 93 placeable FourCC object types.

## Regeneration

Run from repo root:

```bash
JAVA_HOME=/home/scotty/jdk21 PATH=/home/scotty/jdk21/bin:$PATH \
  /home/scotty/ghidra/support/analyzeHeadless /home/scotty/ghidra-projects JN_decomp \
  -process Neutron.exe -noanalysis -readOnly \
  -scriptPath "/home/scotty/jn-engine/tools/ghidra;/home/scotty/ghidra-scripts" \
  -postScript DumpHierarchy.java /home/scotty/jn-engine/docs/decomp/_hierarchy.md

JAVA_HOME=/home/scotty/jdk21 PATH=/home/scotty/jdk21/bin:$PATH \
  /home/scotty/ghidra/support/analyzeHeadless /home/scotty/ghidra-projects JN_decomp \
  -process Neutron.exe -noanalysis -readOnly \
  -scriptPath "/home/scotty/jn-engine/tools/ghidra;/home/scotty/ghidra-scripts" \
  -postScript Scan_ClassIds.java

cp /tmp/classids.txt docs/_gam_classids.tsv
python3 tools/gam_schema.py
python3 tools/decomp_ledger.py
```

## Status Values

Use only: `todo`, `in_progress`, `spec`, `ported(optional)`, `validated`.

Set `validated` only after Alex or a runtime/visual check has confirmed behavior where that validation pays off. Spec review is the normal gate at wave boundaries.

## Phase 0 Caveats

The Microsoft RTTI analyzer was run and the Ghidra project was saved, but this Ghidra install did not materialize a useful `/ClassDataTypes` tree for `Neutron.exe`. The campaign therefore uses custom RTTI scripts:

- `DumpHierarchy.java` parses MSVC x86 RTTI directly.
- `ApplyRttiClassMarkup.java` seeds class namespaces, vftable labels, minimal class structs, `__thiscall`, and typed `this` parameters.

Seed structs currently contain only the primary `vftable` pointer. Per-class work still has to fill concrete fields from `.gam` registrars and decompiler offset access.

## Placeable Accelerator

`docs/gam_schema.md` currently resolves 55 of 93 placeable FourCC rows to RTTI class names, represented as 55 `placeable:<FourCC>` tags across 54 ledger class rows. The remaining rows still have pinned `InitObject fn` addresses and must stay unresolved until a class owner is proven from decompiled bodies or stronger Ghidra markup. Ledger rows are tagged only when the mapping resolves to an RTTI class.
