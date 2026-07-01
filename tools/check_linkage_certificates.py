#!/usr/bin/env python3
"""Enforce the `linked` status contract for the linked-parity branch.

Single source of truth: ``docs/linkage_certificates.csv`` (the manifest). Each
row certifies one ``(class, aspect)`` at status ``linked`` or ``linked-blocked``.

A ``linked`` claim is only trusted when:
  * its ``oracle`` script exists and exits 0 (a headless faithfulness proof), and
  * its ``linkage_doc`` exists (the recovered-body + method-map evidence).
A ``linked-blocked`` row must carry a ``note`` explaining which gameplay /
by-eye / by-ear evidence it needs (that work returns to ``native-port``).

This is the gate that makes the "% linked" metric un-fakeable: no green oracle,
no ``linked``. ``tools/build_vtable_parity_report.py`` calls this at the end, so
the audit cannot be regenerated while a linked claim is uncertified.

Usage:
  python3 tools/check_linkage_certificates.py            # validate + write scoreboard
  python3 tools/check_linkage_certificates.py --selftest # prove the gate catches a bad oracle
  python3 tools/check_linkage_certificates.py --no-write  # validate only
"""
from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from datetime import date
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
MANIFEST = REPO / "docs" / "linkage_certificates.csv"
SCOREBOARD = REPO / "docs" / "linkage_progress.md"
VALID = {"linked", "linked-blocked"}


def run_oracle(path: Path) -> tuple[bool, str]:
    if not path.exists():
        return False, f"oracle missing: {path}"
    try:
        r = subprocess.run(
            [sys.executable, str(path)],
            cwd=str(REPO),
            capture_output=True,
            text=True,
            timeout=300,
        )
    except Exception as e:  # pragma: no cover - defensive
        return False, f"oracle raised: {e}"
    if r.returncode != 0:
        tail = (r.stdout + r.stderr).strip().splitlines()[-6:]
        return False, "oracle FAILED (exit %d):\n      %s" % (
            r.returncode,
            "\n      ".join(tail),
        )
    out = r.stdout.strip().splitlines()
    return True, (out or ["ok"])[-1]


def load_manifest() -> list[dict]:
    if not MANIFEST.exists():
        return []
    with MANIFEST.open(newline="") as f:
        return list(csv.DictReader(f))


def validate(rows: list[dict]) -> tuple[list[str], dict]:
    errors: list[str] = []
    linked: list[dict] = []
    blocked: list[dict] = []
    seen: set[tuple[str, str]] = set()
    for i, row in enumerate(rows, start=2):  # header is line 1
        cls = (row.get("class") or "").strip()
        aspect = (row.get("aspect") or "").strip()
        status = (row.get("status") or "").strip()
        note = (row.get("note") or "").strip()
        loc = f"row {i} ({cls or '?'}/{aspect or '-'})"
        if not cls:
            errors.append(f"{loc}: empty class")
            continue
        if status not in VALID:
            errors.append(f"{loc}: status {status!r} not in {sorted(VALID)}")
            continue
        key = (cls, aspect)
        if key in seen:
            errors.append(f"{loc}: duplicate (class, aspect)")
            continue
        seen.add(key)
        if status == "linked-blocked":
            if not note:
                errors.append(
                    f"{loc}: linked-blocked requires a note (what evidence is needed)"
                )
            blocked.append(row)
            continue
        # status == "linked": must be proven.
        doc = (row.get("linkage_doc") or "").strip()
        if not doc or not (REPO / doc).exists():
            errors.append(f"{loc}: linked requires an existing linkage_doc (got {doc!r})")
        oracle = (row.get("oracle") or "").strip()
        if not oracle:
            errors.append(f"{loc}: linked requires an oracle path")
            continue
        ok, msg = run_oracle(REPO / oracle)
        if not ok:
            errors.append(f"{loc}: {msg}")
        else:
            row["_proof"] = msg
            linked.append(row)
    return errors, {"linked": linked, "blocked": blocked}


def write_scoreboard(res: dict) -> None:
    linked, blocked = res["linked"], res["blocked"]
    lines = [
        "# Linked-Parity Scoreboard",
        "",
        f"Generated {date.today().isoformat()} by `tools/check_linkage_certificates.py`.",
        "",
        "Source of truth: `docs/linkage_certificates.csv`. A `linked` row is counted",
        "here only after its oracle ran green in this pass (see",
        "`docs/linked_parity_plan.md` for the Linkage Certificate L1-L5 contract).",
        "",
        f"- **linked (oracle-verified):** {len(linked)}",
        f"- **linked-blocked (returns to native-port):** {len(blocked)}",
        "",
        "## linked",
        "",
        "| class | aspect | domain | oracle | proof |",
        "|---|---|---|---|---|",
    ]
    for r in linked:
        lines.append(
            f"| `{r.get('class','')}` | {r.get('aspect','')} | {r.get('domain','')} "
            f"| `{r.get('oracle','')}` | {r.get('_proof','')} |"
        )
    if not linked:
        lines.append("| _(none yet — gate armed)_ | | | | |")
    lines += [
        "",
        "## linked-blocked (needs gameplay / by-eye / by-ear evidence)",
        "",
        "| class | aspect | domain | why it cannot be linked here |",
        "|---|---|---|---|",
    ]
    for r in blocked:
        lines.append(
            f"| `{r.get('class','')}` | {r.get('aspect','')} | {r.get('domain','')} "
            f"| {r.get('note','')} |"
        )
    lines.append("")
    SCOREBOARD.write_text("\n".join(lines))


def selftest() -> int:
    """Prove the runner classifies a passing and a failing oracle correctly."""
    with tempfile.TemporaryDirectory() as d:
        good = Path(d) / "good.py"
        good.write_text("print('PASS selftest'); raise SystemExit(0)")
        bad = Path(d) / "bad.py"
        bad.write_text("import sys; print('MISMATCH'); sys.exit(1)")
        ok_good, _ = run_oracle(good)
        ok_bad, _ = run_oracle(bad)
    if ok_good and not ok_bad:
        print("selftest PASS: gate accepts a green oracle and rejects a failing one")
        return 0
    print(f"selftest FAIL: good={ok_good} bad={ok_bad}")
    return 1


def main(argv: list[str]) -> int:
    if "--selftest" in argv:
        return selftest()
    rows = load_manifest()
    errors, res = validate(rows)
    if errors:
        print("LINKAGE GATE FAILED:")
        for e in errors:
            print("  - " + e)
        return 1
    if "--no-write" not in argv:
        write_scoreboard(res)
    print(
        f"linkage gate OK: {len(res['linked'])} linked (oracle-verified), "
        f"{len(res['blocked'])} linked-blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
