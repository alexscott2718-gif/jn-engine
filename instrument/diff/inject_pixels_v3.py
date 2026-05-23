#!/usr/bin/env python3
r"""
inject_pixels_v3.py -- bootstrap a v3 .omtc from a v2 capture + local PNGs.

The proxy v3 extension (TEXTURE_PIXELS) needs an XP redeploy + recapture to
take effect for real. To validate the v3 pipeline end-to-end *without* XP,
this tool reads an existing v2 .omtc + the heuristic texmap (sidecar PNGs),
and writes a v3 .omtc by inserting one TEXTURE_PIXELS record right after each
TEXTURE_DEF whose tex_id is in the map. Pixels come from the PNG converted to
D3D A8R8G8B8 byte order (BGRA, 32 bpp).

The result is NOT pixel-exact (PNGs aren't the original surface bytes), but
it exercises the proxy's v3 record format, the receiver's passthrough, the
extractor's v3 awareness, and the replay's REC_TEXTURE_PIXELS handler -- so
when the real XP recapture happens, the only variable is "do the bytes
match what's in the stream", everything else is already proven.

Usage:
  inject_pixels_v3.py <in.omtc> <texmap.json> [--out build/in_v3.omtc]
"""
import os
import sys
import json
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))
from protocol import (Header, OMTC_MAGIC, pack_record, try_parse_record,  # noqa
                      RECORD_TEXTURE_DEF)

RECORD_TEXTURE_PIXELS = 14   # v3


def png_to_bgra(path):
    """Decode a PNG and return (w, h, bgra_bytes). Returns None on failure."""
    from PIL import Image
    try:
        img = Image.open(path).convert("RGBA")
    except Exception as e:
        print(f"  ! cannot decode {path}: {e}", file=sys.stderr)
        return None
    w, h = img.size
    raw = bytearray(img.tobytes())     # R,G,B,A
    # Swap R<->B for BGRA (D3D A8R8G8B8 byte order, little-endian).
    for i in range(0, len(raw), 4):
        raw[i], raw[i + 2] = raw[i + 2], raw[i]
    return w, h, bytes(raw)


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        print(__doc__); sys.exit(1)
    src, texmap_path = args[0], args[1]
    out = None
    i = 2
    while i < len(args):
        if args[i] == "--out":
            out = args[i + 1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)
    if out is None:
        base, ext = os.path.splitext(src)
        out = base + "_v3" + ext

    with open(texmap_path) as f:
        texmap = json.load(f)
    # Pre-decode PNGs for the texmap (cache).
    cache = {}
    for tid_hex, png in texmap.items():
        if png not in cache:
            cache[png] = png_to_bgra(png)
    n_decoded = sum(1 for v in cache.values() if v is not None)
    print(f"[inject] {n_decoded}/{len(cache)} PNG sources decoded",
          file=sys.stderr)

    n_injected = 0
    n_skipped = 0
    with open(src, "rb") as f, open(out, "wb") as g:
        # Header verbatim (v2 stream). We don't bump the file header's version
        # field because Receiver accepts >=1; the new records pass through.
        head = f.read(Header.size())
        g.write(head)
        buf = bytearray()
        while True:
            data = f.read(4 << 20)
            if not data:
                break
            buf += data
            off = 0
            while True:
                rec = try_parse_record(buf, off)
                if rec is None:
                    break
                rt, payload, off = rec
                # Re-emit the original record.
                g.write(pack_record(rt, payload))
                # On TEXTURE_DEF: if we have pixels for this tex_id, append
                # a TEXTURE_PIXELS record right after.
                if rt == RECORD_TEXTURE_DEF:
                    tid = struct.unpack_from('<I', payload, 0)[0]
                    key = f"{tid:08x}"
                    png = texmap.get(key)
                    if png and cache.get(png):
                        w, h, bgra = cache[png]
                        # Header: tex_id u32, w u16, h u16, bpp u32, pixel_bytes u32
                        hdr = struct.pack('<IHHII', tid, w, h, 32, len(bgra))
                        g.write(pack_record(RECORD_TEXTURE_PIXELS,
                                            hdr + bgra))
                        n_injected += 1
                    else:
                        n_skipped += 1
            if off:
                del buf[:off]
    sz = os.path.getsize(out)
    print(f"[inject] wrote {out} ({sz} B); injected {n_injected} "
          f"TEXTURE_PIXELS, skipped {n_skipped} (unmapped tex_ids)",
          file=sys.stderr)


if __name__ == "__main__":
    main()
