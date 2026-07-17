#!/usr/bin/env python3
"""Validate the append-only ground-truth request ledger and its mutation gate."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DEFAULT_PATH = REPO / "docs" / "ground_truth_requests.md"
LEDGER_MARKER = "## Request ledger\n"
SLUG = r"[a-z0-9](?:[a-z0-9-]{0,40}[a-z0-9])?"
HEADING = re.compile(
    rf"^## (GTR-\d{{8}}-{SLUG}) — `(3SPR|3ROK|3DAI)`$"
)
FIELD = re.compile(r"^- ([A-Za-z ]+):(?: (.*))?$")
KEY = re.compile(rf"^`(ground-truth-\d{{8}}-{SLUG})`$")
FIELDS = (
    "Status",
    "Target",
    "Blocking question",
    "Evidence requested",
    "Suggested capture",
    "Acceptance",
    "Deliver to",
    "Idempotency key",
)
STATUSES = {"`open`", "`captured`", "`resolved`", "`declined`"}


class LedgerError(ValueError):
    pass


def _fail(message: str) -> None:
    raise LedgerError(message)


def validate(text: str) -> int:
    if text.count(LEDGER_MARKER) != 1:
        _fail("ledger marker must occur exactly once")
    _preamble, ledger = text.split(LEDGER_MARKER, 1)
    starts = [match.start() for match in re.finditer(r"(?m)^## GTR-", ledger)]
    if not starts or ledger[: starts[0]].strip():
        _fail("ledger must contain only request sections")
    starts.append(len(ledger))

    request_ids: set[str] = set()
    keys: set[str] = set()
    count = 0
    for begin, end in zip(starts, starts[1:]):
        lines = ledger[begin:end].strip().splitlines()
        heading = HEADING.fullmatch(lines[0])
        if heading is None:
            _fail("request heading is malformed")
        request_id, target = heading.groups()
        if request_id in request_ids:
            _fail(f"duplicate request ID: {request_id}")
        request_ids.add(request_id)

        fields: list[tuple[str, str, int]] = []
        for index, line in enumerate(lines[1:], start=1):
            match = FIELD.fullmatch(line)
            if match:
                fields.append((match.group(1), match.group(2) or "", index))
        if tuple(name for name, _value, _index in fields) != FIELDS:
            _fail(f"{request_id} fields are missing, duplicated, or out of order")
        values = {name: value for name, value, _index in fields}
        positions = {name: index for name, _value, index in fields}
        if len(lines) < 2 or lines[1] != "":
            _fail(f"{request_id} must have one blank line after its heading")
        if values["Status"] not in STATUSES:
            _fail(f"{request_id} has an invalid status")
        target_prefix = f"`{target}` / "
        if (
            not values["Target"].startswith(target_prefix)
            or not values["Target"][len(target_prefix) :].strip()
        ):
            _fail(f"{request_id} target does not match its heading")
        for name in (
            "Blocking question",
            "Suggested capture",
            "Acceptance",
            "Deliver to",
        ):
            if not values[name].strip():
                _fail(f"{request_id} has an empty {name}")
        evidence_lines = lines[
            positions["Evidence requested"] + 1 : positions["Suggested capture"]
        ]
        if values["Evidence requested"] or not evidence_lines:
            _fail(f"{request_id} evidence must be a nested bullet list")
        if any(not line.startswith("  - ") or not line[4:].strip() for line in evidence_lines):
            _fail(f"{request_id} evidence contains a malformed bullet")
        allowed_lines = {0, 1, *(index for _name, _value, index in fields)}
        allowed_lines.update(
            range(
                positions["Evidence requested"] + 1,
                positions["Suggested capture"],
            )
        )
        if allowed_lines != set(range(len(lines))):
            _fail(f"{request_id} contains text outside the fixed schema")

        key_match = KEY.fullmatch(values["Idempotency key"])
        if key_match is None:
            _fail(f"{request_id} idempotency key is malformed")
        key = key_match.group(1)
        if key != request_id.lower().replace("gtr-", "ground-truth-", 1):
            _fail(f"{request_id} idempotency key does not match its ID")
        if key in keys:
            _fail(f"duplicate idempotency key: {key}")
        keys.add(key)
        count += 1
    return count


def _expect_rejected(text: str, description: str) -> None:
    try:
        validate(text)
    except LedgerError:
        return
    raise AssertionError(f"mutation was not rejected: {description}")


def _replace_in_ledger(text: str, old: str, new: str) -> str:
    preamble, ledger = text.split(LEDGER_MARKER, 1)
    mutated = ledger.replace(old, new, 1)
    if mutated == ledger:
        raise AssertionError(f"selftest fixture not found: {old}")
    return preamble + LEDGER_MARKER + mutated


def selftest(text: str) -> None:
    validate(text)
    _expect_rejected(
        _replace_in_ledger(
            text,
            "- Status: `open`",
            "- Status: `unknown`",
        ),
        "invalid status",
    )
    _expect_rejected(
        _replace_in_ledger(
            text,
            "  - A sanitized runtime field dump",
            "A sanitized runtime field dump",
        ),
        "flattened evidence bullet",
    )
    _expect_rejected(
        _replace_in_ledger(
            text,
            "ground-truth-20260717-3spr-defaults",
            "ground-truth-20260717-wrong-key",
        ),
        "mismatched idempotency key",
    )
    _expect_rejected(
        _replace_in_ledger(
            text,
            "- Blocking question: What default canvas",
            "unexpected free-form text\n- Blocking question: What default canvas",
        ),
        "text outside the fixed schema",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs="?", type=Path, default=DEFAULT_PATH)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    text = args.path.read_text(encoding="utf-8")
    count = validate(text)
    if args.selftest:
        selftest(text)
    print(f"ground-truth request ledger valid: {count} requests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
