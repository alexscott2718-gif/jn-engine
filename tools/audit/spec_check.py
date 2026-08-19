#!/usr/bin/env python3
"""Cross-check every class spec in docs/decomp/ against the project's own
generated data.

The 2026-08 audit found that nothing in the pipeline compared a spec's Identity
block to the data the project generates from the binary and the shipped levels.
17 of 208 specs disagreed with it. This is the check that was missing.

Three independent sources are used:

  docs/gam_schema.md       FourCC-class map, instance counts, harvested properties,
                           and the dominant ObjectTag of each shipped type
  docs/_gam_classids.tsv   the class-id registrar scan (Scan_ClassIds.java)
  src/game/entities.c      the native vtable binding table

Findings are tiered by how strong the contradicting evidence is:

  T1  registrar-level  a binary-derived source names the class for a FourCC
  T2  shipped-data     the .gam instance table's dominant ObjectTag names the class
  T3  engine-code      entities.c binds the FourCC to a native vtable

Exit status
  0  no findings outside the baseline
  1  new findings (not in docs/audit/spec_check_baseline.json)
  2  bad invocation / missing inputs

Run ``--update-baseline`` after deliberately changing a spec to re-record the
accepted set. Removing an entry from the baseline is how a fix gets locked in.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

NONSPEC = {
    "README.md", "_TEMPLATE.md", "_ghidra_markup.md", "_hierarchy.md",
    "_next_session.md", "_next_session_collision.md", "_scene_sequencer.md",
}

# Unhedged "not placed" assertions only. The generated template's
# "(inherits its parent's property set, or is created at runtime rather than
# placed)" is a DISJUNCTION, not an assertion, and is deliberately not matched.
HARD_NOTPLACED = [
    re.compile(r"does not appear in the 35 parsed `?\.gam`? files"),
    re.compile(r"zero shipped `?\.gam`? instances"),
    re.compile(r"no shipped `?\.gam`? instances"),
]

LEVEL_ID = re.compile(r"^LEV\d$")


def norm(s: str | None) -> str:
    return re.sub(r"[^A-Z0-9]", "", (s or "").upper())


def lines_of(p: Path) -> list[str]:
    return p.read_text(encoding="utf-8", errors="replace").split("\n")


# ---------------------------------------------------------------- sources
def parse_gam_schema(p: Path):
    classmap, instances, props = {}, {}, {}
    sec, cur = None, None
    for i, ln in enumerate(lines_of(p), 1):
        if ln.startswith("## FourCC"):
            sec = "classmap"; continue
        if ln.startswith("## Object types"):
            sec = "inst"; continue
        if ln.startswith("## Per-type property detail"):
            sec = "props"; continue
        if ln.startswith("## "):
            sec = None; continue
        if sec == "classmap":
            m = re.match(r"^\|\s*`([^`]{4})`\s*\|\s*(.*?)\s*\|\s*`?(\w*)`?\s*\|\s*(\d+)\s*\|\s*$", ln)
            if m:
                cls = m.group(2).strip()
                if cls.startswith("—") or "pending" in cls:
                    cls = None
                classmap[m.group(1)] = {"class": cls, "fn": m.group(3),
                                        "sites": int(m.group(4)), "line": i}
        elif sec == "inst":
            m = re.match(r"^\|\s*`([^`]{4})`\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(.*?)\s*\|\s*$", ln)
            if m:
                instances[m.group(1)] = {"n": int(m.group(2)), "nprops": int(m.group(3)),
                                         "tag": m.group(5).strip(), "line": i}
        elif sec == "props":
            m = re.match(r"^###\s+`([^`]{4})`\s+—\s+(\d+)\s+instances", ln)
            if m:
                cur = m.group(1)
                props.setdefault(cur, {"line": i, "props": {}})
                continue
            if cur:
                m = re.match(r"^\|\s*(✓|✗)\s*\|\s*`([^`]+)`\s*\|", ln)
                if m:
                    props[cur]["props"][m.group(2)] = True
    return classmap, instances, props


def parse_classids(p: Path):
    rows = []
    for i, ln in enumerate(lines_of(p), 1):
        if i == 1 or not ln.strip():
            continue
        f = [x.strip() for x in re.split(r"\s{2,}|\t", ln.rstrip())]
        if len(f) < 3:
            continue
        rows.append({"imm": f[0], "fourcc": f[1], "site": f[2],
                     "fn": f[3] if len(f) > 3 else "",
                     "class": f[4] if len(f) > 4 else "", "line": i})
    return rows


def parse_entities(p: Path):
    e = {}
    for i, ln in enumerate(lines_of(p), 1):
        m = re.match(r'^\s*\{\s*"([A-Za-z0-9]{4})"\s*,\s*"([^"]*)"\s*,\s*&(\w+)\s*\}', ln)
        if m:
            e[m.group(1)] = {"desc": m.group(2), "vt": m.group(3), "line": i}
    return e


def parse_spec(path: Path):
    s = {"class": path.stem, "fourcc": None, "fourcc_line": None,
         "unresolved": False, "notplaced": False, "notplaced_line": None,
         "noprops": False, "noprops_line": None, "validation_noprops": False}
    sec = None
    for i, ln in enumerate(lines_of(path), 1):
        if ln.startswith("## "):
            sec = ln[3:].strip(); continue
        if sec == "Identity":
            m = re.match(r"^\|\s*FourCC\s*\|\s*(.*?)\s*\|?\s*$", ln)
            if m and s["fourcc_line"] is None:
                v = m.group(1).rstrip("|").strip()
                s["fourcc_line"] = i
                if "not resolved" in v:
                    s["unresolved"] = True
                mm = re.search(r"`([A-Za-z0-9]{4})`", v)
                if mm:
                    s["fourcc"] = mm.group(1)
            m = re.match(r"^\|\s*Ctor\(s\)\s*\|\s*(.*?)\s*\|?\s*$", ln)
            if m and s["fourcc"] is None:
                mm = re.search(r"(?:FourCC|class id|class-id immediate)\s+`([A-Za-z0-9]{4})`"
                               r"(?:/`([A-Za-z0-9]{4})`)?", m.group(1))
                if mm:
                    s["fourcc"] = mm.group(2) or mm.group(1)
                    s["fourcc_line"] = i
        if sec and sec.startswith("Field Map") and "No own `.gam` properties registered" in ln:
            s["noprops"] = True
            s["noprops_line"] = i
        if sec == "Validation" and "No registered `.gam` properties to cross-check" in ln:
            s["validation_noprops"] = True
        if not s["notplaced"]:
            for pat in HARD_NOTPLACED:
                if pat.search(ln):
                    s["notplaced"] = True
                    s["notplaced_line"] = i
                    break
    return s


# ---------------------------------------------------------------- the check
def run(repo: Path):
    gs = repo / "docs" / "gam_schema.md"
    ci = repo / "docs" / "_gam_classids.tsv"
    ec = repo / "src" / "game" / "entities.c"
    dd = repo / "docs" / "decomp"
    for p in (gs, ci, ec, dd):
        if not p.exists():
            print("missing input: %s" % p, file=sys.stderr)
            raise SystemExit(2)

    classmap, instances, props = parse_gam_schema(gs)
    cids = parse_classids(ci)
    ents = parse_entities(ec)

    t1 = defaultdict(list)
    for cc, d in classmap.items():
        if d["class"]:
            t1[norm(d["class"])].append(("docs/gam_schema.md", cc, d["line"]))
    for r in cids:
        if r["class"]:
            t1[norm(r["class"])].append(("docs/_gam_classids.tsv", r["fourcc"], r["line"]))
    t2 = defaultdict(list)
    for cc, d in instances.items():
        if d["tag"]:
            t2[norm(d["tag"])].append(("docs/gam_schema.md", cc, d["line"]))

    # FourCCs that belong to a *level* class, never to a placeable object
    level_ids = {r["fourcc"] for r in cids if LEVEL_ID.match(r["fourcc"])}
    level_ids |= {r["imm"] for r in cids if LEVEL_ID.match(r["fourcc"])}
    # FourCC -> the named class the registrar scan assigns it to
    owner = {}
    for r in cids:
        if r["class"]:
            owner.setdefault(r["fourcc"], set()).add(r["class"].rstrip("()"))

    specs = [parse_spec(p) for p in sorted(dd.glob("*.md")) if p.name not in NONSPEC]
    findings = []

    def add(cls, code, tier, line, desc, ref):
        findings.append({"class": cls, "code": code, "tier": tier,
                         "spec": "docs/decomp/%s.md" % cls, "line": line,
                         "desc": desc, "evidence": ref})

    for s in specs:
        k = norm(s["class"])
        e1 = t1.get(k, [])
        cc1 = sorted({c for _, c, _ in e1})
        e2 = [x for x in t2.get(k, []) if x[1] not in set(cc1)]
        cc2 = sorted({c for _, c, _ in e2})
        eff = s["fourcc"] or (cc1[0] if len(cc1) == 1 else (cc2[0] if len(cc2) == 1 else None))
        ref1 = "; ".join("%s:%d->%s" % (f, l, c) for f, c, l in e1)
        ref2 = "; ".join("%s:%d->%s" % (f, l, c) for f, c, l in e2)

        if s["unresolved"] and e1:
            add(s["class"], "FOURCC_UNRESOLVED_BUT_KNOWN", "T1", s["fourcc_line"],
                "Identity says the FourCC is '(not resolved)'; a binary-derived source "
                "resolves it to %s" % cc1, ref1)
        elif s["unresolved"] and e2:
            add(s["class"], "FOURCC_UNRESOLVED_BUT_SHIPPED", "T2", s["fourcc_line"],
                "Identity says '(not resolved)'; the shipped .gam corpus carries this class "
                "name as the dominant ObjectTag of %s" % cc2, ref2)

        if s["fourcc"] and s["fourcc"] in level_ids:
            add(s["class"], "FOURCC_IS_A_LEVEL_ID", "T1", s["fourcc_line"],
                "Identity states `%s`, which is a level class id, not an object FourCC"
                % s["fourcc"], "docs/_gam_classids.tsv")

        if s["fourcc"] and cc1 and s["fourcc"] not in cc1:
            add(s["class"], "FOURCC_MISMATCH", "T1", s["fourcc_line"],
                "Identity states `%s`; registrar-level sources say %s" % (s["fourcc"], cc1), ref1)
        elif s["fourcc"] and not cc1 and cc2 and s["fourcc"] not in cc2:
            add(s["class"], "FOURCC_CONTRADICTS_SHIPPED", "T2", s["fourcc_line"],
                "Identity states `%s`; the shipped corpus tags this class on %s"
                % (s["fourcc"], cc2), ref2)

        if s["fourcc"]:
            other = owner.get(s["fourcc"], set())
            other = {o for o in other if norm(o) != k}
            if other and not (set(cc1) & {s["fourcc"]}):
                add(s["class"], "FOURCC_OWNED_BY_ANOTHER_CLASS", "T1", s["fourcc_line"],
                    "Identity states `%s`, which the registrar scan assigns to %s"
                    % (s["fourcc"], sorted(other)), "docs/_gam_classids.tsv")

        if s["notplaced"] and eff and eff in instances and instances[eff]["n"] > 0:
            add(s["class"], "NOTPLACED_BUT_SHIPPED", "T1", s["notplaced_line"],
                "Spec asserts no shipped .gam presence; gam_schema records %d instances of `%s`"
                % (instances[eff]["n"], eff), "docs/gam_schema.md:%d" % instances[eff]["line"])

        if (s["unresolved"] or s["notplaced"]) and eff and eff in ents:
            add(s["class"], "NATIVE_VTABLE_BOUND", "T3", s["fourcc_line"],
                "Spec declares the FourCC unresolved / not placed, yet the engine binds `%s` "
                "to `%s`" % (eff, ents[eff]["vt"]),
                "src/game/entities.c:%d" % ents[eff]["line"])

        if (s["noprops"] or s["validation_noprops"]) and eff and props.get(eff, {}).get("props"):
            add(s["class"], "NOPROPS_BUT_HARVESTED", "T1",
                s["noprops_line"] or s["fourcc_line"],
                "Spec states there are no .gam properties to cross-check; gam_schema harvests "
                "%d for `%s` across %s instances"
                % (len(props[eff]["props"]), eff, instances.get(eff, {}).get("n", "?")),
                "docs/gam_schema.md:%d" % props[eff]["line"])

    return specs, findings


def key_of(f):
    return "%s::%s" % (f["class"], f["code"])


def main() -> int:
    here = Path(__file__).resolve()
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo", type=Path, default=here.parent.parent.parent)
    ap.add_argument("--baseline", type=Path, default=None)
    ap.add_argument("--json", type=Path, help="write the full finding set here")
    ap.add_argument("--update-baseline", action="store_true")
    ap.add_argument("--all", action="store_true", help="list every finding, baseline included")
    args = ap.parse_args()

    repo = args.repo.resolve()
    baseline_path = args.baseline or (repo / "docs" / "audit" / "spec_check_baseline.json")

    specs, findings = run(repo)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(findings, indent=1), encoding="utf-8")

    if args.update_baseline:
        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "_comment": "Known spec/data disagreements accepted at the time of recording. "
                        "spec_check.py fails on anything NOT listed here. Remove an entry "
                        "when the underlying spec is fixed, to lock the fix in.",
            "accepted": sorted({key_of(f) for f in findings}),
        }
        baseline_path.write_text(json.dumps(payload, indent=1) + "\n", encoding="utf-8")
        print("baseline updated: %d accepted finding(s) -> %s"
              % (len(payload["accepted"]), baseline_path))
        return 0

    accepted = set()
    if baseline_path.is_file():
        accepted = set(json.loads(baseline_path.read_text(encoding="utf-8")).get("accepted", []))

    new = [f for f in findings if key_of(f) not in accepted]
    stale = sorted(accepted - {key_of(f) for f in findings})

    print("spec-check: %d specs, %d finding(s), %d accepted by baseline, %d new"
          % (len(specs), len(findings), len(findings) - len(new), len(new)))
    if findings:
        counts = Counter(f["code"] for f in findings)
        for code, n in sorted(counts.items()):
            print("   %-34s %d" % (code, n))

    show = findings if args.all else new
    if show:
        print()
        for f in sorted(show, key=lambda x: (x["tier"], x["class"])):
            print("  [%s] %s:%s  %s" % (f["tier"], f["spec"], f["line"] or "?", f["code"]))
            print("        %s" % f["desc"])
            print("        evidence: %s" % f["evidence"])

    if stale:
        print()
        print("  %d baseline entr(y/ies) no longer reproduce - remove them from %s:"
              % (len(stale), baseline_path.name))
        for k in stale:
            print("        %s" % k)

    if new:
        print()
        print("FAIL: %d finding(s) not in the baseline." % len(new))
        print("      Fix the spec, or run --update-baseline if the change is deliberate.")
        return 1
    print("\nOK: no spec disagrees with the generated data outside the recorded baseline.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
