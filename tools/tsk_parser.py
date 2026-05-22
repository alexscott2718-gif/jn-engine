"""
TSK game-state parser for Jimmy Neutron Boy Genius.
Format:
  [4B magic: 'LV1B'][1B version][1B variant]
  [large zero-padded region -- lookup table, mostly empty]
  [STARTEXP record: string fields + 3 BE floats for start position]
  [entity list: u8 count + [u8 name_len][name][u32 BE state] per entry]
"""

import struct
import os
import sys
import json
from dataclasses import dataclass, field
from typing import List, Optional, Tuple


@dataclass
class StartInfo:
    task_name: str          # typically 'STARTEXP'
    param1: str             # usually 'none'
    gam_file: str           # e.g. 'level1b.gam'
    param2: str             # usually 'none'
    start_x: float
    start_y: float
    start_z: float


@dataclass
class EntityState:
    name: str
    state: int              # 32-bit BE value; 0 = inactive/default


@dataclass
class TskFile:
    path: str
    magic: str
    version: int
    variant: int
    start_info: Optional[StartInfo]
    entities: List[EntityState]


def _read_pstring(data: bytes, pos: int) -> Tuple[str, int]:
    """Read Pascal-style length-prefixed string. Returns (string, new_pos)."""
    nlen = data[pos]
    s = data[pos + 1: pos + 1 + nlen].decode('ascii', errors='replace')
    return s, pos + 1 + nlen


def _find_startexp(data: bytes) -> Optional[int]:
    """Find offset of STARTEXP task record (length-prefixed string 'STARTEXP')."""
    marker = b'\x08STARTEXP'
    idx = data.find(marker)
    return idx if idx >= 0 else None


def parse_tsk(path: str) -> TskFile:
    data = open(path, 'rb').read()

    magic = data[:4].decode('ascii', errors='replace')
    if magic != 'LV1B':
        raise ValueError(f'Not a TSK file: {path}')

    version = data[4]
    variant = data[5]

    # Locate STARTEXP record
    start_info = None
    se_pos = _find_startexp(data)
    if se_pos is not None:
        pos = se_pos
        task_name, pos = _read_pstring(data, pos)
        param1, pos = _read_pstring(data, pos)
        gam_file, pos = _read_pstring(data, pos)
        param2, pos = _read_pstring(data, pos)
        # 3 BE floats: start X, Y, Z
        x, y, z = struct.unpack_from('>3f', data, pos)
        start_info = StartInfo(
            task_name=task_name,
            param1=param1,
            gam_file=gam_file,
            param2=param2,
            start_x=x,
            start_y=y,
            start_z=z,
        )

    # Entity state list: search after STARTEXP region
    # Located right after the STARTEXP section (which is padded to 0x80 boundary)
    entities = []
    # Scan for entity list: u8 count followed by [u8 name_len][name][u32 BE state]*
    # Find the first non-zero count byte after STARTEXP that leads to valid ASCII names
    search_start = (se_pos + 0x80) if se_pos is not None else 6
    # Align to 0x80 (128-byte) boundary
    search_start = (search_start + 0x7F) & ~0x7F

    pos = search_start
    while pos < len(data):
        count = data[pos]
        if count == 0 or count > 64:
            pos += 1
            continue
        # Validate that 'count' entries follow with ASCII names
        test_pos = pos + 1
        ok = True
        test_entries = []
        for _ in range(count):
            if test_pos >= len(data):
                ok = False
                break
            nlen = data[test_pos]
            if nlen == 0 or nlen > 32 or test_pos + 1 + nlen + 4 > len(data):
                ok = False
                break
            name_bytes = data[test_pos + 1: test_pos + 1 + nlen]
            if not all(0x20 <= b < 0x7f for b in name_bytes):
                ok = False
                break
            state = struct.unpack_from('>I', data, test_pos + 1 + nlen)[0]
            test_entries.append(EntityState(name=name_bytes.decode('ascii'), state=state))
            test_pos += 1 + nlen + 4
        if ok and test_entries:
            entities = test_entries
            break
        pos += 1

    return TskFile(
        path=path,
        magic=magic,
        version=version,
        variant=variant,
        start_info=start_info,
        entities=entities,
    )


def tsk_summary(tsk: TskFile) -> str:
    lines = [
        f'File: {tsk.path}',
        f'Magic: {tsk.magic}, version=0x{tsk.version:02x}, variant=0x{tsk.variant:02x}',
    ]
    if tsk.start_info:
        si = tsk.start_info
        lines.append(f'Start: task={si.task_name!r} gam={si.gam_file!r}')
        lines.append(f'  Spawn: ({si.start_x:.1f}, {si.start_y:.1f}, {si.start_z:.1f})')
    if tsk.entities:
        lines.append(f'Entities ({len(tsk.entities)}):')
        for e in tsk.entities:
            flag = f' state={e.state}' if e.state else ''
            lines.append(f'  {e.name}{flag}')
    return '\n'.join(lines)


def export_tsk(tsk: TskFile, out_path: str):
    si = tsk.start_info
    d = {
        'file': tsk.path,
        'magic': tsk.magic,
        'version': tsk.version,
        'variant': tsk.variant,
        'start_info': {
            'task_name': si.task_name,
            'param1': si.param1,
            'gam_file': si.gam_file,
            'param2': si.param2,
            'spawn': [si.start_x, si.start_y, si.start_z],
        } if si else None,
        'entities': [{'name': e.name, 'state': e.state} for e in tsk.entities],
    }
    with open(out_path, 'w') as f:
        json.dump(d, f, indent=2)
    return d


def main():
    import argparse
    ap = argparse.ArgumentParser(description='Parse TSK game-state files')
    ap.add_argument('file', nargs='?', help='TSK file')
    ap.add_argument('--out', help='Output JSON path')
    args = ap.parse_args()

    if not args.file:
        ap.print_help()
        return

    tsk = parse_tsk(args.file)
    print(tsk_summary(tsk))

    if args.out:
        export_tsk(tsk, args.out)
        print(f'\nSaved: {args.out}')


if __name__ == '__main__':
    main()
