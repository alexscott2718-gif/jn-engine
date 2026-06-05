#!/usr/bin/env python3
"""Inspect RAD Granny-era .grn files used by JNvsJN.

This is intentionally metadata-first. It does not decode Granny mesh or
animation streams yet; it finds the stable header fields, exporter marker
strings, source .max paths, texture references, and likely object/bone names
needed to choose the next renderer/converter target.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
from pathlib import Path
from typing import Iterable


GRANNY_SIGNATURE = bytes.fromhex(
    "2a30390418466c668d6d26239a52c17a"
    "7010e04484232632253c0a64f726611f"
    "253c0a64f726611f44682a3ba878e461"
    "5858715f0839ac1d7a3d217f604af637"
)

PRINTABLE_RE = re.compile(rb"[\x20-\x7e]{4,}")
PATH_RE = re.compile(r"(?:[A-Za-z]:\\|/).+")
IMAGE_EXT_RE = re.compile(r"\.(?:bmp|png|tga|jpg|jpeg)$", re.I)
MODEL_EXT_RE = re.compile(r"\.max$", re.I)
MARKERS = {"__Standard", "__FileName", "__ObjectName", "__Description", "__Root"}
NAME_RE = re.compile(r"^[A-Za-z0-9 _#.\-]+$")


def u32le(data: bytes, off: int) -> int | None:
    if off + 4 > len(data):
        return None
    return struct.unpack_from("<I", data, off)[0]


def printable_strings(data: bytes, min_len: int = 4) -> list[dict[str, object]]:
    out: list[dict[str, object]] = []
    for match in PRINTABLE_RE.finditer(data):
        raw = match.group(0)
        if len(raw) < min_len:
            continue
        out.append({
            "offset": match.start(),
            "text": raw.decode("latin-1", errors="replace"),
        })
    return out


def unique(items: Iterable[str]) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for item in items:
        if item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def candidate_names(strings: list[dict[str, object]]) -> list[str]:
    names: list[str] = []
    marker_offsets = [int(s["offset"]) for s in strings if str(s["text"]) in MARKERS]
    min_offset = min(marker_offsets) if marker_offsets else 0
    for row in strings:
        if int(row["offset"]) < min_offset:
            continue
        text = str(row["text"])
        if text in MARKERS:
            continue
        if PATH_RE.match(text):
            continue
        if text.startswith("(C) Copyright"):
            continue
        if text in {"RAD 3D Studio MAX 4.x", "1.2b", "10-4-2000", "win32"}:
            continue
        if any(ch in text for ch in "\\/:"):
            continue
        if NAME_RE.match(text) and re.search(r"[A-Za-z]", text):
            names.append(text)
    return unique(names)


def marker_values(strings: list[dict[str, object]]) -> list[dict[str, object]]:
    out: list[dict[str, object]] = []
    for i, row in enumerate(strings):
        marker = str(row["text"])
        if marker not in MARKERS:
            continue
        values: list[str] = []
        for next_row in strings[i + 1:]:
            text = str(next_row["text"])
            if text in MARKERS:
                break
            values.append(text)
            if len(values) >= 12:
                break
        out.append({
            "offset": int(row["offset"]),
            "marker": marker,
            "values": values,
        })
    return out


def parse_grn(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    strings = printable_strings(data)
    texts = [str(s["text"]) for s in strings]

    header_size = u32le(data, 0x50)
    section_count = u32le(data, 0x44)
    tag_values = []
    for off in range(0x40, min(len(data), 0x100), 4):
        value = u32le(data, off)
        if value is not None and (value & 0xFFFF0000) == 0xCA5E0000:
            tag_values.append({"offset": off, "value": f"0x{value:08x}"})

    source_models = unique(t for t in texts if MODEL_EXT_RE.search(t))
    textures = unique(t for t in texts if IMAGE_EXT_RE.search(t))
    paths = unique(t for t in texts if PATH_RE.match(t))
    markers = [
        {"offset": int(s["offset"]), "text": str(s["text"])}
        for s in strings
        if str(s["text"]) in MARKERS
    ]
    names = candidate_names(strings)

    return {
        "path": str(path),
        "name": path.name,
        "size": len(data),
        "signature_ok": data.startswith(GRANNY_SIGNATURE),
        "header_payload_size_field": header_size,
        "header_payload_plus_signature_matches": (
            header_size is not None and header_size + len(GRANNY_SIGNATURE) == len(data)
        ),
        "section_count_field": section_count,
        "ca5e_tags_first_0x100": tag_values,
        "rad_exporter_offset": data.find(b"RAD 3D Studio"),
        "markers": markers,
        "marker_values": marker_values(strings),
        "source_models": source_models,
        "textures": textures,
        "paths": paths,
        "candidate_names": names[:80],
        "candidate_name_count": len(names),
        "printable_string_count": len(strings),
    }


def print_text(report: dict[str, object]) -> None:
    print(f"{report['name']}  size={report['size']}")
    print(f"  signature_ok={report['signature_ok']} "
          f"header_payload_size={report['header_payload_size_field']} "
          f"payload_plus_sig_matches={report['header_payload_plus_signature_matches']} "
          f"sections={report['section_count_field']}")
    print(f"  rad_exporter_offset={report['rad_exporter_offset']} "
          f"printable_strings={report['printable_string_count']}")

    for label in ("source_models", "textures"):
        values = report[label]
        print(f"  {label}:")
        if values:
            for value in values:
                print(f"    {value}")
        else:
            print("    <none>")

    print("  markers:")
    for row in report["marker_values"]:
        values = "; ".join(row["values"][:4]) if row["values"] else "<none>"
        print(f"    0x{row['offset']:x} {row['marker']}: {values}")

    print("  candidate_names:")
    for value in report["candidate_names"][:40]:
        print(f"    {value}")
    extra = report["candidate_name_count"] - min(40, len(report["candidate_names"]))
    if extra > 0:
        print(f"    ... +{extra} more")


def expand_paths(paths: list[Path]) -> list[Path]:
    out: list[Path] = []
    for path in paths:
        if path.is_dir():
            out.extend(sorted(path.glob("*.grn"), key=lambda p: p.name.lower()))
        else:
            out.append(path)
    return out


def print_summary(reports: list[dict[str, object]]) -> None:
    print(f"count={len(reports)}")
    print(f"signature_ok={sum(1 for r in reports if r['signature_ok'])}")
    print("payload_plus_signature_matches="
          f"{sum(1 for r in reports if r['header_payload_plus_signature_matches'])}")
    print(f"with_textures={sum(1 for r in reports if r['textures'])}")
    print(f"with_source_models={sum(1 for r in reports if r['source_models'])}")

    print("\nlargest:")
    for r in sorted(reports, key=lambda row: int(row["size"]), reverse=True)[:12]:
        textures = ", ".join(Path(t).name for t in r["textures"]) or "-"
        print(f"  {r['name']:28} {r['size']:8}  tex={textures}")

    print("\nsmallest:")
    for r in sorted(reports, key=lambda row: int(row["size"]))[:12]:
        textures = ", ".join(Path(t).name for t in r["textures"]) or "-"
        print(f"  {r['name']:28} {r['size']:8}  tex={textures}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("paths", nargs="+", type=Path)
    ap.add_argument("--json", action="store_true", help="emit JSON instead of text")
    ap.add_argument("--summary", action="store_true",
                    help="summarize all input files; directories expand to *.grn")
    args = ap.parse_args()

    paths = expand_paths(args.paths)
    reports = [parse_grn(path) for path in paths]
    if args.json:
        print(json.dumps(reports, indent=2))
    elif args.summary:
        print_summary(reports)
    else:
        for i, report in enumerate(reports):
            if i:
                print()
            print_text(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
