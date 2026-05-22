#!/usr/bin/env python3
"""
GAM level data parser for Jimmy Neutron.

Binary format (all multi-byte ints are Big-Endian):
  [4B magic = level name] [4B object_count]
  For each object:
    [4B FourCC type] [4B prop_count]
    For each property:
      [u8 name_len] [name_bytes] [u32 type] [u32 val_len] [value_bytes]
    [4B checksum]  (skipped)

Property types:
  1 = string  : read val_len+1 bytes; first byte is redundant length prefix
  2 = raw     : val_len raw bytes (flags / multi-bool)
  3 = float32 : 4-byte BE IEEE 754
  6 = int32   : 4-byte BE signed int (-1 = "unset/unlimited")
"""

import json, struct, sys
from pathlib import Path


def _read_prop(data, pos):
    """Read one property record. Returns (name, type_id, value, new_pos)."""
    name_len = data[pos]; pos += 1
    name = data[pos:pos+name_len].decode('latin-1', errors='replace'); pos += name_len
    type_id = struct.unpack_from('>I', data, pos)[0]; pos += 4
    val_len  = struct.unpack_from('>I', data, pos)[0]; pos += 4

    if type_id == 1:                      # string (length-prefixed, prefix redundant)
        pos += 1                          # skip prefix byte
        value = data[pos:pos+val_len].decode('latin-1', errors='replace'); pos += val_len
    elif type_id == 3:                    # float32 BE
        value = round(struct.unpack_from('>f', data, pos)[0], 6); pos += val_len
    elif type_id == 6:                    # int32 BE (-1 = unset)
        value = struct.unpack_from('>i', data, pos)[0]; pos += val_len
    else:                                 # raw bytes (type 2 = flags, others unknown)
        value = data[pos:pos+val_len].hex(); pos += val_len

    return name, type_id, value, pos


def parse_gam(path) -> dict:
    data = Path(path).read_bytes()
    magic     = data[:4].decode('latin-1', errors='replace')
    obj_count = struct.unpack_from('>I', data, 4)[0]

    objects = []
    pos = 8
    for _ in range(obj_count):
        if pos + 8 > len(data): break
        obj_type   = data[pos:pos+4].decode('latin-1', errors='replace')
        prop_count = struct.unpack_from('>I', data, pos+4)[0]
        pos += 8

        props = {}
        for _ in range(prop_count):
            if pos >= len(data): break
            name, type_id, value, pos = _read_prop(data, pos)
            props[name] = value

        objects.append({'type': obj_type, 'properties': props})
        pos += 4  # skip per-object checksum

    return {'magic': magic, 'object_count': obj_count, 'objects': objects}


def gam_summary(d: dict):
    print(f"Level : {d['magic']}   objects: {d['object_count']}")
    for obj in d['objects']:
        p   = obj['properties']
        tag = p.get('ObjectTag', obj['type'])
        px  = p.get('PositionX', 0)
        py  = p.get('PositionY', 0)
        pz  = p.get('PositionZ', 0)
        task = p.get('TaskName', '')
        extra = f"  task={task!r}" if task and task != 'none' else ''
        hidden = '  HIDDEN' if p.get('InitiallyVisible', -1) == 0 else ''
        print(f"  {obj['type']} {tag:<22} pos=({px:>10.1f},{py:>8.1f},{pz:>10.1f}){extra}{hidden}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: gam_parser.py <file.gam> [--json out.json]")
        print("       gam_parser.py --all <gam_dir> <out_dir>")
        sys.exit(1)

    if sys.argv[1] == '--all':
        gam_dir, out_dir = Path(sys.argv[2]), Path(sys.argv[3])
        out_dir.mkdir(parents=True, exist_ok=True)
        ok = err = 0
        for f in sorted(gam_dir.glob('*.gam')):
            try:
                d = parse_gam(f)
                (out_dir / (f.stem + '.json')).write_text(json.dumps(d, indent=2))
                ok += 1
            except Exception as e:
                print(f"  ERROR {f.name}: {e}"); err += 1
        print(f"Parsed {ok} GAM files, {err} errors → {out_dir}"); sys.exit(0)

    d = parse_gam(sys.argv[1])
    json_out = None; i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--json' and i+1 < len(sys.argv): json_out = sys.argv[i+1]; i += 2
        else: i += 1
    if json_out:
        Path(json_out).write_text(json.dumps(d, indent=2))
        print(f"Wrote {json_out}")
    gam_summary(d)
