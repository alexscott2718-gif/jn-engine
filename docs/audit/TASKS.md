# Open work from the 2026-08 audit

Each task below is self-contained: what's wrong, where, how to verify you fixed it,
and what "done" means. Claim one in the Discord or via the contributor MCP
(`claim_task`) so two people don't land the same fix.

**Legend** — `[disc]` needs `assets/exe/` populated (see `docs/GAME_FILES.md`);
everything else needs only a clone.

Evidence for every claim: `docs/audit/06-open-questions.md`, with 44 tagged records in
`docs/audit/06-openq.jsonl`. Published write-up: <https://exentt.com/jn/audit.html>.

---

## P0 — documents that mislead every new reader

### A-01 · Remove the retracted 94% statistic — **DONE 2026-08-21**
**Good first task.** No binaries, no build.

> Both lines now state the real figure (1 of 70 high-tier cases), say the 94% was
> retracted, and link `track0_static_reader_findings.md`. They also carry the
> retraction's own explanation of *why* the two disagree -- the capture oracle's
> per-triangle UV vote cross-attributes textures between meshes sharing UV triples,
> so the oracle is the unreliable side -- because a reader who only learns the number
> is smaller is likely to conclude the static reader is the broken one. The audit's
> proposed minimal patch is below and this follows it.

Two documents still print a figure that was retracted in May. Re-running the validator
shipped in the same commit gives **1/70**, not 94%. Both files are on the "read these
first" path, and both still carry it on `origin/master`.

- `docs/ARCHITECTURE.md:290` — "validates it against the capture oracle (~94%)"
- `docs/PROJECT_HISTORY.md:257` — "reproduces the capture oracle ~**94%**"
- Retraction and true figures: `docs/track0_static_reader_findings.md:14-19`

**Done when:** neither file asserts 94%, both point at the corrected document, and
`grep -rn '94%' docs/` returns only the retraction itself.

### A-02 · Qualify the `canvas_id = Canv + 1` invariant — **DONE 2026-08-21**
**Good first task.**

> Both invariant lists now name the path: `+1` on the heuristic-scanned `0MF2` path,
> direct from the OMT header's `3DSh` chunk table, with the side-by-side table in
> `omt_rendering_breakthrough.md` §6 cited at both sites.

The rule holds for the *heuristic-scanned* `0MF2` path only. The OMT header chunk
table yields the canvas id directly. Both invariant lists state it unconditionally —
inside a section that tells readers not to re-examine it.

- `docs/ARCHITECTURE.md:420`, `docs/PROJECT_HISTORY.md:1246`
- Both paths tabulated side by side at `docs/omt_rendering_breakthrough.md:203`

**Done when:** both invariants name which parsing path they apply to.

---

## P1 — specs that assert what the data denies

### B-01 · Fix the FourCC resolver, then regenerate
The single highest-leverage fix: it closes 17 spec defects at the source.

`tools/gen_placeable_specs.py:225-241` `fourcc_for()` resolves in three branches and
each leaks:

1. `CLS2FCC[cls]` is a **case-sensitive** lookup built only from `gam_schema.md`. The
   schema stores `C3DEYE`; the key is `C3DEye`. **14 classes** differ by case alone.
2. A regex over decompiled text, which only fires when the decompiler printed an
   immediate.
3. **Nearest-preceding registrar within `0x800`** — pure address proximity, no identity
   check. This is what gave `C3DDarwinFish` the id of `C3DDino`, and `C3DSparrow` the
   id of *level 5*.

And `_load_registrars()` (`:208-220`) reads only columns 1 and 3 of
`docs/_gam_classids.tsv`. The **class-name column is never read**, though it spells out
`C3DCorona()`, `C3DGRILL`, `C3DPASSCARD`, `C3DSmokePuff()` in plain text.

**Suggested shape:** match case-insensitively; read the TSV's class column; make
branch 3 verify that the registrar actually installs one of the class's vtables, or
drop it and emit "unresolved" honestly.

**Done when:** `python3 tools/audit/spec_check.py` reports fewer findings, the baseline
is trimmed by the same amount, and no regenerated spec claims a `LEV\d` id.

### B-02 · Re-run validation on 29 specs
Each states *"No registered `.gam` properties to cross-check"* while `gam_schema.md`
holds harvested properties for its FourCC — **627 property rows across 617 shipped
instances**, including `C3DTree` (169 instances) and `C3DRock` (99).

The narrow reading may be true — the class may register none of its own in
`InitObject`. But the sentence claims no cross-check material existed, and that is
false. List them with:

```bash
python3 tools/audit/spec_check.py --all | grep NOPROPS_BUT_HARVESTED
```

**Done when:** each spec either cross-checks against the harvested set, or says
precisely which parent registers the properties.

### B-03 · `[disc]` Confirm the remaining unresolved FourCCs against the binary
Seven classes are named only by the shipped `.gam` `ObjectTag`, not by a registrar
scan row. Confirm each by disassembling its registrar and matching installed vtables
against the spec's own `Vftable(s)` row — the method used for the other 16.

`C3DFireStrato` (`3FLA`) is the known-hard one: its registrar does not write the
spec's vtables within +0x1200 of the pinned address.

**Done when:** each is either binary-confirmed or documented as genuinely unresolvable.

---

## P2 — explanations that are wrong

### C-01 · Correct two invariant explanations
Both were checked against the engine's own LGPL source. The *observations* stand; the
*reasons* printed next to them do not.

- **"w-buffer"** — `PROJ[3][3]=1` is real and must not be repaired, but the depth mode
  is a plain z-buffer. `ZENABLE` is only ever set to `D3DZB_TRUE`/`D3DZB_FALSE`, never
  `D3DZB_USEW`, and nothing in the toolkit source mentions w-buffering.
- **`CULLMODE=NONE`** — attributed to `OMediaPipeline` software-culling every polygon.
  That pipeline is used by the *software* renderer only; the DirectX renderer never
  touches it, so it cannot explain a D3D7 capture. Culling is initialised **on**
  (`D3DCULL_CW`) and driven by the two-sided flag plus an unconditional
  `disable_faceculling()` in the canvas-element render path.

**Done when:** both explanations are corrected without weakening the rules themselves.
No ported behaviour changes — do not "fix" any rendering code for this.

### C-02 · Flag the `3TAR` duplicate FourCC — **DONE 2026-08-21**

> Both halves. `gam_schema.md` now carries a `3TAR` duplicate-registrar caveat in the
> same shape as the `3YSH` one, naming both sites and saying the shipped rows are the
> moving target's. The decision half was already made: the engine binds `3TAR` to
> `vt_moving_target` ("3TAR: bind the moving target, not the shadow"), and
> `docs/decomp/C3DShadow.md` -- which still described itself as placed 22 times with
> 25 harvested properties, all of them the moving target's -- now claims none of the
> instances. Worth noting for whoever reads the baseline: `spec_check` fires on the
> spec that *disagrees* with the registrar scan, and the scan agrees with `C3DShadow`,
> so the flagged row is `C3DMovingTarget` -- the class that is right.
`3TAR` has two registrars — the same shape `docs/gam_schema.md:32` already flags a
caveat for on `3YSH`, and only on `3YSH`. Its 22 shipped instances are tagged
`C3DMOVINGTARGET`, while `src/game/entities.c:98` binds `3TAR` to `vt_shadow`
("Shadow sprite decor").

**Done when:** the schema carries the same caveat it carries for `3YSH`, and someone
has decided which class the shipped instances should actually drive.

---

## P3 — facts to correct

### D-01 · Say which year is meant
Three documents say the game is from 2002 without distinguishing build from release.
The binaries were linked **2001-09-30**; the stray "2003" elsewhere is the toolkit's
open-source release, not the game. A PE header dates a build, not a street date — so
"2002" may well be right for the release; it just needs to say which it means.

`README.md:4` · `docs/ARCHITECTURE.md:21` · `docs/PROJECT_HISTORY.md:16`

### D-02 · Drop a phantom class from the registrar scan — **needs a decision first**
`docs/_gam_classids.tsv:141` assigns `3TRO` to a class `C3DTrophy` that does not exist
in `docs/decomp/_hierarchy.md`. The column is documented as
"class *or nearby string*", and here it caught a nearby string. The real owner is
`C3DVRTrophy`, confirmed against the binary.

> **Not done, deliberately (2026-08-21).** `_gam_classids.tsv` is raw
> `Scan_ClassIds.java` output, and its column header says `class_or_nearby_string`.
> Hand-editing one row to remove a nearby string it really did find makes the file
> *less* faithful to the scan while making it look more authoritative, and a
> regeneration would silently undo it. The same shape appears at least twice more --
> `3NEU` catches `C3DSprite()` and `3TAR` catches `C3DShadow()` -- so the general fix
> is either to teach the scan an identity check or to consume the column as the
> heuristic it is, which is what `spec_check`'s baseline notes already do for all
> three. Someone should pick one; editing this row alone is the option that helps
> least.

---

## Needs a decision, not a patch

- **The `0x140` struct mismatch.** A note claims a 2.5.0 SDK drop-in "fails on a struct
  size mismatch at offset `0x140`", naming no class and no build log. All 180
  name-parseable imports resolve against the 2.5.0 source, the API genuinely drifted,
  and the game does bake 100 distinct object sizes into 313 allocation sites — but
  `0x140` is not among them. Settling it needs the original build error text.
- **Krabby Patty.** The local Ghidra project is one unrecorded hour with no link to the
  maintained effort. Connect it or retire it.

---

## Ground rules

Everything here follows the project's existing discipline, which the audit found to be
the part that works:

- **A green oracle or it isn't linked.** Behaviour claims go through the
  linkage-certificate gate. `linked-blocked` is a first-class, respectable outcome.
- **Decompiler output is a hypothesis; the raw disassembly is the evidence.** This rule
  was paid for — a committed evidence document once assigned the wrong expressions
  because the decompiler dropped a stack argument.
- **Say "we don't know."** The ground-truth queue exists for exactly that, and the
  audit's highest praise went to the sessions that refused to certify without evidence.
- **Cite file and line.** Every claim in `docs/audit/` resolves to a path and a line or
  a byte offset. Match that.
