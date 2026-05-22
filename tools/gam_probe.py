#!/usr/bin/env python3
"""
Phase 8 Step 1 probe — uncommitted scratch tool.

Walks a .gam file using the same parsing logic as gam_parser.py / gam_loader.c,
reports exactly where the object loop ends, and dumps the trailing/unparsed
region for inspection.

Usage:
  python3 tools/gam_probe.py assets/gam/Level1.gam
  python3 tools/gam_probe.py assets/gam/Level1.gam --hex 0x00010000 256
  python3 tools/gam_probe.py assets/gam/Level1.gam --scan-floats
  python3 tools/gam_probe.py assets/gam/Level1.gam --scan-ordinals
"""
import struct, sys, json
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent


def parse_with_trace(data):
    """Re-implement gam_loader's walk and remember (start,end) of every parsed object."""
    magic = data[:4].decode('latin-1', errors='replace')
    obj_count = struct.unpack_from('>I', data, 4)[0]
    pos = 8
    objs = []
    truncated = False
    for oi in range(obj_count):
        start = pos
        if pos + 8 > len(data):
            truncated = True
            break
        fcc = data[pos:pos+4].decode('latin-1', errors='replace')
        prop_count = struct.unpack_from('>I', data, pos+4)[0]
        pos += 8
        props = {}
        for _pi in range(prop_count):
            if pos + 1 > len(data):
                truncated = True; break
            name_len = data[pos]; pos += 1
            if pos + name_len > len(data):
                truncated = True; break
            name = data[pos:pos+name_len].decode('latin-1', errors='replace'); pos += name_len
            if pos + 8 > len(data):
                truncated = True; break
            type_id = struct.unpack_from('>I', data, pos)[0]; pos += 4
            val_len = struct.unpack_from('>I', data, pos)[0]; pos += 4
            if type_id == 1:
                pos += 1
                value = data[pos:pos+val_len].decode('latin-1', errors='replace')
                pos += val_len
            elif type_id == 3:
                value = round(struct.unpack_from('>f', data, pos)[0], 4)
                pos += val_len
            elif type_id == 6:
                value = struct.unpack_from('>i', data, pos)[0]
                pos += val_len
            else:
                value = data[pos:pos+val_len].hex()
                pos += val_len
            props[name] = (type_id, value)
        # per-object 4-byte checksum
        pos += 4
        objs.append({'idx': oi, 'fcc': fcc, 'start': start, 'end': pos, 'props': props})
        if truncated:
            break
    return {'magic': magic, 'obj_count': obj_count, 'objects': objs,
            'parse_end': pos, 'truncated': truncated}


def fmt_addr(p): return f"0x{p:08x}"


def hexdump(data, start, length, width=16):
    out = []
    end = min(start + length, len(data))
    for off in range(start, end, width):
        chunk = data[off:off+width]
        hexs = ' '.join(f"{b:02x}" for b in chunk)
        ascii_s = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        out.append(f"{off:08x}  {hexs:<{width*3}}  {ascii_s}")
    return '\n'.join(out)


def scan_floats(data, start, end, lo=-200000.0, hi=200000.0, want=8):
    """Find runs of consecutive plausible BE float32s of length >= want."""
    runs = []
    cur_start = None
    cur_len = 0
    p = start
    while p + 4 <= end:
        v = struct.unpack_from('>f', data, p)[0]
        # plausible coord: finite, in level bounds
        ok = (v == v) and (v == v) and abs(v) < hi  # NaN check + bound
        # avoid -inf / +inf
        try:
            if v != v or v == float('inf') or v == -float('inf'):
                ok = False
        except Exception:
            ok = False
        if ok and lo < v < hi:
            if cur_start is None:
                cur_start = p
                cur_len = 1
            else:
                cur_len += 1
        else:
            if cur_start is not None and cur_len >= want:
                runs.append((cur_start, cur_len))
            cur_start = None; cur_len = 0
        p += 4
    if cur_start is not None and cur_len >= want:
        runs.append((cur_start, cur_len))
    return runs


def scan_ordinals(data, start, end, lo=1, hi=194):
    """Find offsets where a BE u32 in [lo,hi] is followed by 3 plausible BE float32s."""
    hits = []
    p = start
    while p + 16 <= end:
        ordv = struct.unpack_from('>I', data, p)[0]
        if lo <= ordv <= hi:
            f1 = struct.unpack_from('>f', data, p+4)[0]
            f2 = struct.unpack_from('>f', data, p+8)[0]
            f3 = struct.unpack_from('>f', data, p+12)[0]
            def ok(v):
                try:
                    return v == v and abs(v) < 200000.0
                except Exception:
                    return False
            if ok(f1) and ok(f2) and ok(f3) and (abs(f1) + abs(f2) + abs(f3) > 0):
                hits.append((p, ordv, f1, f2, f3))
        p += 1  # byte-aligned search
    return hits


def find_strings(data, start, end, min_len=4):
    """Find printable ASCII runs >= min_len in unparsed region."""
    out = []
    cur_start = None
    for p in range(start, end):
        b = data[p]
        if 32 <= b < 127:
            if cur_start is None:
                cur_start = p
        else:
            if cur_start is not None and p - cur_start >= min_len:
                out.append((cur_start, data[cur_start:p].decode('latin-1')))
            cur_start = None
    if cur_start is not None and end - cur_start >= min_len:
        out.append((cur_start, data[cur_start:end].decode('latin-1')))
    return out


def find_fourcc_after(data, start, end, fourccs):
    """Locate offsets where 4 ASCII bytes match one of fourccs."""
    fset = set(fourccs)
    hits = []
    for p in range(start, end - 4):
        s = data[p:p+4]
        try:
            if s.decode('latin-1') in fset:
                hits.append((p, s.decode('latin-1')))
        except Exception:
            pass
    return hits


def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    path = Path(sys.argv[1])
    data = path.read_bytes()
    total = len(data)

    trace = parse_with_trace(data)
    parsed_end = trace['parse_end']
    tail = total - parsed_end
    print(f"file        : {path}  size={total} ({fmt_addr(total)})")
    print(f"magic       : {trace['magic']!r}")
    print(f"obj_count   : {trace['obj_count']}")
    print(f"objs parsed : {len(trace['objects'])}  truncated={trace['truncated']}")
    print(f"parse_end   : {parsed_end} ({fmt_addr(parsed_end)})")
    print(f"tail bytes  : {tail} ({100*tail/total:.1f}% of file)")

    # FourCC histogram
    from collections import Counter
    counts = Counter(o['fcc'] for o in trace['objects'])
    print("FourCC histogram (top 20):")
    for k, n in counts.most_common(20):
        print(f"  {k!r:8} {n}")

    # XYZ bounds across parsed entities
    xs, ys, zs = [], [], []
    for o in trace['objects']:
        p = o['props']
        if 'PositionX' in p and p['PositionX'][0] == 3:
            xs.append(p['PositionX'][1])
        if 'PositionY' in p and p['PositionY'][0] == 3:
            ys.append(p['PositionY'][1])
        if 'PositionZ' in p and p['PositionZ'][0] == 3:
            zs.append(p['PositionZ'][1])
    if xs:
        print(f"entity X range: [{min(xs):.1f}, {max(xs):.1f}] (n={len(xs)})")
        print(f"entity Y range: [{min(ys):.1f}, {max(ys):.1f}]")
        print(f"entity Z range: [{min(zs):.1f}, {max(zs):.1f}]")

    args = sys.argv[2:]
    if not args:
        # Default: dump first 256 bytes after parse_end + tail string scan
        print()
        if tail > 0:
            print(f"--- hexdump tail starting at {fmt_addr(parsed_end)} (first 256 bytes) ---")
            print(hexdump(data, parsed_end, min(256, tail)))
            print()
            print(f"--- printable strings in tail (len>=4) ---")
            strs = find_strings(data, parsed_end, total, 4)
            for off, s in strs[:80]:
                print(f"  {fmt_addr(off)}  {s!r}")
            if len(strs) > 80:
                print(f"  ... +{len(strs)-80} more")
            print()
            print(f"--- BE u32 ordinals in [1..194] followed by 3 floats (first 30) ---")
            hits = scan_ordinals(data, parsed_end, total)
            for p, ordv, f1, f2, f3 in hits[:30]:
                print(f"  {fmt_addr(p)}  id={ordv:3}  xyz=({f1:.1f},{f2:.1f},{f3:.1f})")
            print(f"  (total {len(hits)} candidate hits)")
        else:
            print("No tail region — parsed_end == file_end. Look for unread bytes between parsed objects.")
        return

    cmd = args[0]
    if cmd == '--hex':
        off = int(args[1], 0); length = int(args[2])
        print(hexdump(data, off, length))
    elif cmd == '--scan-floats':
        lo, hi = (parsed_end, total)
        runs = scan_floats(data, lo, hi)
        print(f"Float runs (>=8 consecutive plausible) in [{fmt_addr(lo)}, {fmt_addr(hi)}):")
        for s, n in runs[:50]:
            print(f"  {fmt_addr(s)}  len={n}")
    elif cmd == '--scan-ordinals':
        hits = scan_ordinals(data, parsed_end, total)
        for p, ordv, f1, f2, f3 in hits:
            print(f"  {fmt_addr(p)}  id={ordv:3}  xyz=({f1:.1f},{f2:.1f},{f3:.1f})")
    elif cmd == '--fourcc':
        names = args[1:]
        hits = find_fourcc_after(data, parsed_end, total, names)
        for p, s in hits:
            print(f"  {fmt_addr(p)}  {s}")
    else:
        print(f"unknown cmd {cmd}"); sys.exit(2)


if __name__ == '__main__':
    main()
