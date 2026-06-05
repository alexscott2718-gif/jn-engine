#!/usr/bin/env python3
"""Convert .grntex files dumped by granny_proxy.c to PNG.

GRTX v1 format:
    "GRTX" version:u32 texid:u32 pixelptr:u32 descp:u32 format:u32
           width:u32 height:u32 bpp:u32 pitch:u32 nbytes:u32
    "NAME" len:u32 bytes      short Granny texture name
    "PATH" len:u32 bytes      original source BMP path
    "SRCN" len:u32 bytes      source .grn path recovered from the texture state
    "MATN" len:u32 bytes      OMedia material name
    "DATA" len:u32 bytes      raw rows, pitch bytes per row
    "END."
"""

from __future__ import annotations

import argparse
import glob
import struct
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


@dataclass
class GrnTexture:
    version: int
    texid: int
    pixelptr: int
    descp: int
    fmt: int
    width: int
    height: int
    bpp: int
    pitch: int
    nbytes: int
    records: dict[str, str]
    data: bytes


def _read_u32(data: bytes, off: int) -> tuple[int, int]:
    if off + 4 > len(data):
        raise ValueError("truncated u32")
    return struct.unpack_from("<I", data, off)[0], off + 4


def read_grntex(path: Path) -> GrnTexture:
    data = path.read_bytes()
    if data[:4] != b"GRTX":
        raise ValueError("bad GRTX magic")
    fields = struct.unpack_from("<10I", data, 4)
    version, texid, pixelptr, descp, fmt, width, height, bpp, pitch, nbytes = fields
    off = 4 + 10 * 4
    records: dict[str, str] = {}
    pixels = b""
    while off < len(data):
        tag = data[off:off + 4]
        off += 4
        if tag == b"END.":
            break
        if tag in (b"NAME", b"PATH", b"SRCN", b"MATN"):
            n, off = _read_u32(data, off)
            raw = data[off:off + n]
            if len(raw) != n:
                raise ValueError(f"truncated {tag.decode('ascii')}")
            off += n
            records[tag.decode("ascii")] = raw.decode("latin1")
        elif tag == b"DATA":
            n, off = _read_u32(data, off)
            pixels = data[off:off + n]
            if len(pixels) != n:
                raise ValueError("truncated DATA")
            off += n
        else:
            raise ValueError(f"unknown tag {tag!r} at {off - 4}")
    if version != 1:
        raise ValueError(f"unsupported GRTX version {version}")
    if bpp not in (3, 4):
        raise ValueError(f"unsupported bpp {bpp}")
    if len(pixels) < pitch * height:
        raise ValueError("DATA shorter than pitch*height")
    return GrnTexture(version, texid, pixelptr, descp, fmt, width, height, bpp,
                      pitch, nbytes, records, pixels)


def _safe_stem(tex: GrnTexture, fallback: str) -> str:
    raw = tex.records.get("PATH") or tex.records.get("NAME") or fallback
    stem = Path(raw.replace("\\", "/")).stem
    safe = "".join(ch if (ch.isalnum() or ch in "._-") else "_" for ch in stem)
    return safe.strip("._") or fallback


def _convert_rows(tex: GrnTexture, order: str, flip_y: bool) -> tuple[str, bytes]:
    if tex.bpp == 3 and order not in ("rgb", "bgr"):
        raise ValueError("RGB textures need order rgb or bgr")
    if tex.bpp == 4 and order not in ("rgba", "bgra"):
        raise ValueError("RGBA textures need order rgba or bgra")

    rows = [
        tex.data[y * tex.pitch:y * tex.pitch + tex.width * tex.bpp]
        for y in range(tex.height)
    ]
    if flip_y:
        rows.reverse()

    if order in ("rgb", "rgba"):
        return ("RGB" if tex.bpp == 3 else "RGBA"), b"".join(rows)

    out = bytearray()
    step = tex.bpp
    for row in rows:
        for i in range(0, len(row), step):
            px = row[i:i + step]
            if tex.bpp == 3:
                out.extend((px[2], px[1], px[0]))
            else:
                out.extend((px[2], px[1], px[0], px[3]))
    return ("RGB" if tex.bpp == 3 else "RGBA"), bytes(out)


def image_from_texture(tex: GrnTexture, order: str | None = None,
                       flip_y: bool = False) -> Image.Image:
    if order is None:
        order = "rgb" if tex.bpp == 3 else "rgba"
    mode, pixels = _convert_rows(tex, order, flip_y)
    img = Image.frombytes(mode, (tex.width, tex.height), pixels)
    return img


def write_png(tex: GrnTexture, out_path: Path, order: str, flip_y: bool) -> None:
    img = image_from_texture(tex, order, flip_y)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)


def iter_inputs(inputs: list[str]):
    for inp in inputs:
        p = Path(inp)
        if p.is_dir():
            yield from sorted(p.glob("*.grntex"))
        else:
            for item in sorted(glob.glob(inp)):
                yield Path(item)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("inputs", nargs="+", help=".grntex files, globs, or directories")
    ap.add_argument("-o", "--outdir", type=Path, default=None)
    ap.add_argument("--order", choices=("rgb", "bgr", "rgba", "bgra"), default=None,
                    help="channel order in the raw dump; default rgb/rgba")
    ap.add_argument("--flip-y", action="store_true", help="reverse row order")
    ap.add_argument("--variants", action="store_true",
                    help="write both RGB/BGR or RGBA/BGRA variants")
    args = ap.parse_args()

    files = list(iter_inputs(args.inputs))
    if not files:
        raise SystemExit("no .grntex inputs")
    if args.outdir:
        outdir = args.outdir
    elif len(args.inputs) == 1 and Path(args.inputs[0]).is_dir():
        outdir = Path(args.inputs[0]) / "png"
    else:
        outdir = Path("png")

    manifest = [
        "# png\ttexid\tdescp\tsrcn\tpath\tmatn\twidth\theight\tbpp\tpitch\torder\tflip_y"
    ]
    written = 0
    for f in files:
        tex = read_grntex(f)
        base = f"{_safe_stem(tex, f.stem)}_t{tex.texid:08x}"
        orders = ("rgb", "bgr") if tex.bpp == 3 else ("rgba", "bgra")
        if not args.variants:
            orders = (args.order or orders[0],)
        for order in orders:
            suffix = ""
            if args.variants:
                suffix += f"_{order}"
            if args.flip_y:
                suffix += "_flipy"
            out = outdir / f"{base}{suffix}.png"
            write_png(tex, out, order, args.flip_y)
            written += 1
            manifest.append(
                f"{out}\t{tex.texid:08x}\t{tex.descp:08x}\t"
                f"{tex.records.get('SRCN', '')}\t{tex.records.get('PATH', '')}\t"
                f"{tex.records.get('MATN', '')}\t{tex.width}\t{tex.height}\t"
                f"{tex.bpp}\t{tex.pitch}\t{order}\t{int(args.flip_y)}"
            )
            print(f"  OK   {f.name:18} {tex.width}x{tex.height} bpp={tex.bpp} "
                  f"desc={tex.descp:08x} {order} -> {out.name}")

    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / "grntex_png_manifest.tsv").write_text("\n".join(manifest) + "\n")
    print(f"\nwrote {written} png(s) + manifest {outdir / 'grntex_png_manifest.tsv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
