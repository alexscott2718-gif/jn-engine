#!/usr/bin/env python3
"""Harvest the chrome HUD digit glyphs from the full Level-1 capture.

The HUD counters use a runtime-generated chrome/metallic digit font (not the
on-disk green_font/fontsmall). Each distinct digit bitmap the game produced was
dumped once as a TEXTURE_PIXELS record. The accepted frame 8881 only contains the
digit values visible then (big 1,7 ; small 2,5,0); the rest of the 0-9 set lives
elsewhere in the full stream wherever the counters showed those values.

This scans build/level1_v4_hudfix.omtc for every TEXTURE_PIXELS of digit-cell
size (32x32 big, 16x16 small), decodes through the TEXTURE_FORMAT masks, dedups
by pixel content, and writes a contact sheet for labeling. Decoding mirrors
instrument/diff/inspect_replay_v4.py:texture_image().

Skips the multi-GB draw payloads by length; only small texture records are read.
"""
import argparse, hashlib, mmap, os, struct
from collections import defaultdict

R_DEF, R_PIXELS, R_FORMAT = 6, 14, 15
DIGIT_DIMS = {(32, 32), (16, 16)}


def decode(w, h, bpp, pixels, masks):
    from PIL import Image
    bc = (bpp + 7) // 8
    need = w * h * bc
    if bc <= 0 or bc > 4 or len(pixels) < need:
        return None
    rm, gm, bm, am, flags = masks
    if bpp == 32 and rm == 0x00FF0000 and gm == 0x0000FF00 and bm == 0xFF and am == 0xFF000000:
        return Image.frombytes("RGBA", (w, h), pixels[:need], "raw", "BGRA")
    if bpp == 32 and rm == 0:  # no format seen: assume BGRA (the common case)
        return Image.frombytes("RGBA", (w, h), pixels[:need], "raw", "BGRA")

    def shift(m):
        return ((m & -m).bit_length() - 1) if m else 0

    def exp(v, m):
        if not m:
            return 0
        bits = bin(m >> shift(m)).count("1")
        mx = (1 << bits) - 1
        return ((v & m) >> shift(m)) * 255 // mx if mx else 0
    out = bytearray(w * h * 4)
    has_a = bool(am)
    for i in range(w * h):
        val = int.from_bytes(pixels[i*bc:i*bc+bc], "little")
        out[i*4+0] = exp(val, rm); out[i*4+1] = exp(val, gm)
        out[i*4+2] = exp(val, bm); out[i*4+3] = exp(val, am) if has_a else 255
    return Image.frombytes("RGBA", (w, h), bytes(out))


def scan(path):
    fmt_masks = {}   # tex_id -> (r,g,b,a,flags)
    uniques = {}     # sha1 -> (w,h,image)
    counts = defaultdict(int)
    f = open(path, "rb")
    mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    n = len(mm)
    off = 14  # stream header
    seen = 0
    while off + 4 <= n:
        rt = mm[off]
        plen = (mm[off+1] << 16) | (mm[off+2] | (mm[off+3] << 8))
        off += 4
        if rt == R_FORMAT and plen >= 28:
            p = mm[off:off+28]
            tid, flags, rgbc, rm, gm, bm, am = struct.unpack_from("<IIIIIII", p, 0)
            fmt_masks[tid] = (rm, gm, bm, am, flags)
        elif rt == R_PIXELS and plen >= 16:
            p = mm[off:off+16]
            tid, w, h, bpp, nbytes = struct.unpack_from("<IHHII", p, 0)
            seen += 1
            if (w, h) in DIGIT_DIMS:
                pixels = mm[off+16:off+16+nbytes]
                masks = fmt_masks.get(tid, (0, 0, 0, 0, 0))
                img = decode(w, h, bpp, pixels, masks)
                if img is not None:
                    sha = hashlib.sha1(img.tobytes()).hexdigest()
                    if sha not in uniques:
                        uniques[sha] = (w, h, img)
                        counts[(w, h)] += 1
        off += plen
    mm.close(); f.close()
    print(f"scanned {seen} TEXTURE_PIXELS; unique digit-sized: "
          f"{dict(counts)} ({len(uniques)} total)")
    return uniques


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--capture", default="build/level1_v4_hudfix.omtc")
    ap.add_argument("--out", default="build/hud_digit_harvest")
    a = ap.parse_args()
    from PIL import Image, ImageDraw
    os.makedirs(a.out, exist_ok=True)
    uniques = scan(a.capture)

    for size in [(32, 32), (16, 16)]:
        items = [(s, im) for s, (w, h, im) in uniques.items() if (w, h) == size]
        items.sort(key=lambda x: x[0])
        for i, (sha, im) in enumerate(items):
            im.save(os.path.join(a.out, f"cand_{size[0]}_{i:03d}_{sha[:8]}.png"))
        if not items:
            continue
        cols = 16; sc = 4
        cw = size[0]*sc
        rows = (len(items)+cols-1)//cols
        sheet = Image.new("RGBA", (cols*(cw+6)+6, rows*(cw+18)+6), (30, 30, 38, 255))
        dr = ImageDraw.Draw(sheet)
        for i, (sha, im) in enumerate(items):
            cx = 6+(i % cols)*(cw+6); cy = 6+(i//cols)*(cw+18)
            sheet.alpha_composite(im.resize((cw, cw), Image.NEAREST), (cx, cy))
            dr.text((cx, cy+cw), str(i), fill=(220, 220, 120))
        sheet.save(os.path.join(a.out, f"sheet_{size[0]}x{size[1]}.png"))
        print(f"  {len(items)} unique {size[0]}x{size[1]} -> sheet_{size[0]}x{size[1]}.png")


if __name__ == "__main__":
    main()
