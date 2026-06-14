#!/usr/bin/env python3
"""Generate runtime audio handle maps from original OMT Wave tables.

The OMT audio payload stores RIFF/WAVE chunks in byte order, then a trailing
`Wave` table whose entries are the handles used by GAM SoundIndex/MusicIndex.
The existing extracted WAV filenames use RIFF order; this script bridges the
two without renaming the proprietary audio payloads.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def riff_entries(data: bytes) -> dict[tuple[int, int], int]:
    out: dict[tuple[int, int], int] = {}
    pos = 0
    idx = 0
    while True:
        off = data.find(b"RIFF", pos)
        if off < 0:
            break
        if off + 12 <= len(data) and data[off + 8 : off + 12] == b"WAVE":
            riff_size = struct.unpack_from("<I", data, off + 4)[0]
            out[(off, 8 + riff_size)] = idx
            idx += 1
        pos = off + 1
    return out


def parse_wave_table(path: Path) -> list[tuple[int, int, int, str]]:
    data = path.read_bytes()
    riffs = riff_entries(data)
    if not riffs:
        return []

    last_wav_end = max(off + size for (off, size) in riffs)
    tail = data[last_wav_end:]
    entries: list[tuple[int, int, int, str]] = []
    seen: set[tuple[int, int, int, str]] = set()

    for i in range(0, max(0, len(tail) - 13)):
        name_len = tail[i + 12]
        if not (0 < name_len <= 64):
            continue
        end = i + 13 + name_len
        if end > len(tail):
            continue
        raw_name = tail[i + 13 : end]
        if not all(0x20 <= b < 0x7F for b in raw_name):
            continue

        handle = struct.unpack_from(">I", tail, i)[0]
        off = struct.unpack_from(">I", tail, i + 4)[0]
        size = struct.unpack_from(">I", tail, i + 8)[0]
        if (off, size) not in riffs:
            continue
        row = (handle, off, size, raw_name.decode("ascii"))
        if row not in seen:
            entries.append(row)
            seen.add(row)

    entries.sort(key=lambda r: r[0])
    return entries


def load_manifest_files(parsed_dir: Path, stem: str) -> dict[tuple[int, int], str]:
    manifest = parsed_dir / stem / f"{stem}.json"
    if not manifest.exists():
        return {}
    data = json.loads(manifest.read_text())
    out: dict[tuple[int, int], str] = {}
    for audio in data.get("audio", []):
        file_path = Path(audio["file"])
        rel = Path("assets") / "parsed" / stem / f"{stem}_audio" / file_path.name
        out[(int(audio["offset"]), int(audio["size"]))] = rel.as_posix()
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--omt-dir", default="/home/scotty/xp-jnbg-original/omt")
    ap.add_argument("--parsed-dir", default="assets/parsed")
    args = ap.parse_args()

    omt_dir = Path(args.omt_dir)
    parsed_dir = Path(args.parsed_dir)
    written = 0

    for omt in sorted(omt_dir.glob("*.omt")):
        stem = omt.stem.lower()
        target_dir = parsed_dir / stem
        if not target_dir.exists():
            continue
        rows = parse_wave_table(omt)
        if not rows:
            continue
        files = load_manifest_files(parsed_dir, stem)
        out_path = target_dir / f"{stem}_audio_handles.tsv"
        lines = [
            "# handle\twav_path\tname\toffset\tsize",
            f"# source\t{omt}",
        ]
        for handle, off, size, name in rows:
            wav = files.get((off, size))
            if not wav:
                continue
            lines.append(f"{handle}\t{wav}\t{name}\t{off}\t{size}")
        out_path.write_text("\n".join(lines) + "\n")
        written += 1

    print(f"wrote {written} audio handle maps")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
