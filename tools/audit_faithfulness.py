#!/usr/bin/env python3
"""audit_faithfulness.py — sweep every level through the engine's REAL
resolution code (JN_AUDIT=1) and assert the invariants the community QA
tickets established. This is the "fix classes, not instances" instrument:
instead of waiting for a reporter to find each broken object, every entity
and placement in every level is checked against the rules each ticket paid
for (docs/PROJECT_HISTORY.md Era 13).

Rules enforced (violation -> nonzero exit unless waived):
  box                a FourCC resolved to nothing -> placeholder box
                     (2026-06-09 placeholder-box campaign: target is zero)
  mesh-missing       a resolution names a file that doesn't load
  sprite-unresolved  an authored sprite ref has no extracted image
  untextured-mesh    a *visible* mesh draw with zero resolved textures —
                     renders invisible or as a dark slab (the school
                     fire-door bug, ticket #3 follow-up)
  stub-mesh          a visible mesh with < 12 verts — a degenerate OMT->ASE
                     export stub bound as a visual (firedoor.ASE class)
  block-visible      a BLOCK*-prefixed collision placement that drew
                     (ticket #3 follow-up: "all Block meshes are invisible")

Known-accepted findings live in tools/audit_waivers.json — each entry must
carry a reason. The audit fails on NEW findings only, so it can gate changes
while the existing backlog is burned down deliberately.

Usage:
  python3 tools/audit_faithfulness.py                # all JNBG levels
  python3 tools/audit_faithfulness.py level1 level2  # subset
  python3 tools/audit_faithfulness.py --update-waivers   # re-baseline
Report: build/audit_faithfulness.json
"""
import json
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WAIVERS = os.path.join(REPO, "tools", "audit_waivers.json")
REPORT = os.path.join(REPO, "build", "audit_faithfulness.json")
LINE_RE = re.compile(
    r"\[audit\] cat=(?P<cat>\S+) name=(?P<name>\S+) tag=(?P<tag>\S*) "
    r"kind=(?P<kind>\S+) path=(?P<path>\S+) verts=(?P<verts>-?\d+) "
    r"textured=(?P<textured>\d)")

# Draw decisions that put pixels on screen (the rest are intentional skips).
VISIBLE_KINDS = {"mesh", "mesh-prebound", "sprite", "tree-billboard", "player"}


def discover_levels():
    names = []
    for f in sorted(os.listdir(os.path.join(REPO, "assets", "gam"))):
        if f.lower().endswith(".gam"):
            names.append(os.path.splitext(f)[0].lower())
    return names


def run_level(level):
    env = dict(os.environ)
    env.update({
        "JN_AUDIT": "1",
        "JN_SCREENSHOT": "1",                       # reuses its exit-after-warmup
        "JN_SCREENSHOT_PATH": "/tmp/jn_audit_shot.png",
        "JN_SCREENSHOT_WARMUP_TICKS": "2",
        "LD_LIBRARY_PATH": os.path.expanduser(
            "~/toolchain/usr/lib/x86_64-linux-gnu") + ":" +
            os.path.expanduser("~/sdl2/lib"),
    })
    try:
        proc = subprocess.run(
            ["xvfb-run", "-a", "./jnengine", "--level", level],
            cwd=REPO, env=env, capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired as ex:
        return None, f"timeout: {ex}"
    rows = {}
    for line in (proc.stderr or "").splitlines():
        m = LINE_RE.match(line.strip())
        if not m:
            continue
        d = m.groupdict()
        d["verts"] = int(d["verts"])
        d["textured"] = int(d["textured"])
        # dedup identical decisions across frames
        rows[(d["cat"], d["name"], d["tag"], d["kind"], d["path"])] = d
    if not rows and proc.returncode != 0:
        return None, f"engine exited {proc.returncode}: " + \
            (proc.stderr or "")[-300:]
    return list(rows.values()), None


def findings_for(level, rows):
    out = []

    def add(rule, d, note=""):
        out.append({
            "level": level, "rule": rule, "cat": d["cat"], "name": d["name"],
            "tag": d["tag"], "kind": d["kind"], "path": d["path"],
            "verts": d["verts"], "note": note,
        })

    for d in rows:
        kind = d["kind"]
        if kind == "box":
            add("box", d)
        elif kind == "mesh-missing":
            add("mesh-missing", d)
        elif kind == "sprite-unresolved":
            add("sprite-unresolved", d)
        if kind in ("mesh", "mesh-prebound") and not d["textured"]:
            add("untextured-mesh", d,
                "draws invisible/dark (school fire-door class)")
        if d["cat"] == "entity" and kind in ("mesh", "mesh-prebound") \
                and 0 <= d["verts"] < 12:
            # Entity visuals only: a 4-vert PLACEMENT quad (floors, water
            # sheets) is legitimate OMT level geometry, but an entity bound
            # to a <12-vert mesh is a degenerate OMT->ASE export stub
            # (firedoor.ASE class).
            add("stub-mesh", d, "degenerate export stub bound as a visual")
        if d["cat"] == "placement" and d["name"].upper().startswith("BLOCK") \
                and kind != "collision-skip":
            add("block-visible", d)
    return out


def finding_key(f):
    return f"{f['level']}|{f['rule']}|{f['cat']}|{f['name']}|{f['tag']}|{f['path']}"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    update_waivers = "--update-waivers" in sys.argv
    levels = args or discover_levels()

    waivers = {}
    if os.path.exists(WAIVERS):
        waivers = {w["key"]: w for w in json.load(open(WAIVERS))["waivers"]}

    all_findings, errors, stats = [], {}, {}
    for level in levels:
        rows, err = run_level(level)
        if err:
            errors[level] = err
            print(f"{level:12s}  ERROR  {err}")
            continue
        f = findings_for(level, rows)
        all_findings.extend(f)
        kinds = {}
        for d in rows:
            kinds[d["kind"]] = kinds.get(d["kind"], 0) + 1
        stats[level] = kinds
        print(f"{level:12s}  rows={len(rows):4d}  findings={len(f):3d}  " +
              " ".join(f"{k}={v}" for k, v in sorted(kinds.items())))

    new = [f for f in all_findings if finding_key(f) not in waivers]
    waived = [f for f in all_findings if finding_key(f) in waivers]

    os.makedirs(os.path.dirname(REPORT), exist_ok=True)
    json.dump({"levels": levels, "stats": stats, "errors": errors,
               "findings": all_findings, "new": new,
               "waived_count": len(waived)},
              open(REPORT, "w"), indent=1)

    print(f"\n{len(all_findings)} findings ({len(waived)} waived, "
          f"{len(new)} NEW) -> {os.path.relpath(REPORT, REPO)}")
    for f in new:
        print(f"  NEW {f['level']:10s} {f['rule']:18s} {f['cat']:9s} "
              f"{f['name']:24s} tag={f['tag']:18s} {f['path']}")

    if update_waivers:
        json.dump({"comment": "Known-accepted audit findings. Every entry "
                   "needs a reason before it is allowed to stay.",
                   "waivers": [dict(key=finding_key(f), reason="baseline "
                               "2026-06-12 — pre-existing, not yet triaged",
                               **f) for f in all_findings]},
                  open(WAIVERS, "w"), indent=1)
        print(f"baseline written: {len(all_findings)} waivers -> {WAIVERS}")
        return 0
    return 1 if (new or errors) else 0


if __name__ == "__main__":
    sys.exit(main())
