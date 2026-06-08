"""
TSK game-state parser for Jimmy Neutron Boy Genius (original / Neutron.exe).

Format (measured from NewGame.tsk, 2026-06-08):
  [4B magic: 'LV1B'][1B version=0x41][1B variant=0xa0]
  [large zero-filled lookup-table region]
  [STARTEXP record (at 0x4226 in NewGame.tsk):
      pstring task_name   e.g. 'STARTEXP'
      pstring param1      e.g. 'none'
      pstring gam_file    e.g. 'level1b.gam'
      pstring param2      e.g. 'none'
      3x BE float         spawn X, Y, Z]
  [zero gap of variable length, then the entity-state list:
      u8 count            number of entries (12 in NewGame.tsk)
      count x:
          pstring object_tag   e.g. 'MOM', 'SCENE', 'REACTOR'
          u32 BE   state       0 = default/inactive; non-zero = custom initial state
   the list runs to EOF]

This parser targets the original-game LV1B layout only. The Jimmy Neutron vs.
Jimmy Negatron save files (`JimmyGame*.tsk`) use a different ('DUMMY'/sequel)
serialization and are intentionally out of scope here.
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


def _try_entity_list(data: bytes, pos: int) -> Optional[List[EntityState]]:
    """Try to read a `u8 count` + count×(pstring,u32) list at `pos`.

    Returns the parsed entities only if the records are well-formed AND they
    consume the file up to EOF (allowing a short all-zero trailer). Otherwise
    None. The EOF anchor is what tells a genuine count byte apart from a stray
    non-zero byte sitting in the zero gap before the list.
    """
    count = data[pos]
    if count == 0 or count > 64:
        return None
    p = pos + 1
    out: List[EntityState] = []
    for _ in range(count):
        if p >= len(data):
            return None
        nlen = data[p]
        if nlen == 0 or nlen > 32 or p + 1 + nlen + 4 > len(data):
            return None
        name = data[p + 1: p + 1 + nlen]
        if not all(0x20 <= b < 0x7f for b in name):
            return None
        state = struct.unpack_from('>I', data, p + 1 + nlen)[0]
        out.append(EntityState(name=name.decode('ascii'), state=state))
        p += 1 + nlen + 4
    # The list must reach EOF; tolerate a short zero-padded tail only.
    if any(b != 0 for b in data[p:]):
        return None
    return out


def _scan_entity_list(data: bytes, start: int) -> List[EntityState]:
    """Scan forward from `start` for the entity-state list (see module doc)."""
    for pos in range(start, len(data)):
        if data[pos] == 0:
            continue
        entities = _try_entity_list(data, pos)
        if entities is not None:
            return entities
    return []


def parse_tsk(path: str) -> TskFile:
    data = open(path, 'rb').read()

    magic = data[:4].decode('ascii', errors='replace')
    if magic != 'LV1B':
        raise ValueError(f'Not a TSK file: {path}')

    version = data[4]
    variant = data[5]

    # Locate STARTEXP record
    start_info = None
    after_spawn = 6  # fallback scan origin if STARTEXP is absent
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
        after_spawn = pos + 12  # entity list lives somewhere past the spawn floats

    # Entity-state list.
    #
    # It sits after the STARTEXP spawn floats, separated by a variable-length
    # zero gap, and is introduced by a single u8 count byte followed by
    # `count` x (pstring tag, u32 BE state) records that run to EOF.
    #
    # Scan forward from just past the spawn floats for the first count byte
    # whose `count` records parse cleanly as ASCII-tagged pairs AND consume
    # the file up to (or to within a short zero tail of) EOF. The EOF anchor
    # disambiguates the real count byte from stray non-zero bytes in the gap.
    entities = _scan_entity_list(data, after_spawn)

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
        'file': os.path.basename(tsk.path),
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
    ap.add_argument('--json', action='store_true',
                    help='Write <file>.json alongside the input')
    args = ap.parse_args()

    if not args.file:
        ap.print_help()
        return

    tsk = parse_tsk(args.file)
    print(tsk_summary(tsk))

    out_path = args.out
    if args.json and not out_path:
        out_path = args.file + '.json'
    if out_path:
        export_tsk(tsk, out_path)
        print(f'\nSaved: {out_path}')


if __name__ == '__main__':
    main()
