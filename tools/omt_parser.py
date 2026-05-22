"""
OMT2 container parser for Jimmy Neutron Boy Genius game assets.
Format: 0MF2 magic, contains either:
  - Image sprites: OmCv/OmGW chunk pairs (OMedia Canvas format)
  - Audio: raw RIFF/WAV blocks with trailing index table
"""

import struct
import os
import sys
import json
import zlib
from dataclasses import dataclass, field, asdict
from typing import List, Optional


# ---------------------------------------------------------------------------
# PNG writer (stdlib only — no Pillow required)
# ---------------------------------------------------------------------------

def _png_chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', crc)


def _write_png(path: str, width: int, height: int, rgba_rows: List[bytes]):
    """Write a PNG file from a list of raw RGBA rows (each width*4 bytes)."""
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    raw = bytearray()
    for row in rgba_rows:
        raw += b'\x00'  # filter byte: None
        raw += row
    idat = zlib.compress(bytes(raw), 6)
    png = (sig
           + _png_chunk(b'IHDR', ihdr)
           + _png_chunk(b'IDAT', idat)
           + _png_chunk(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(png)


# ---------------------------------------------------------------------------
# PackBits RLE decompressor
# ---------------------------------------------------------------------------

def _rle_unpack(src: bytes, dest_len: int, unit_size: int):
    """
    PackBits-style RLE decompressor working in unit_size-byte units.
    Returns (output_bytes, source_bytes_consumed).

    Format observed in OMT v2 files (verified against open-source OMT
    converter and Jimmy Neutron asset data):
      - Each control "unit" occupies unit_size bytes; the FIRST byte is
        the signed control value n.  For 32-bit mode (unit_size=4), the
        remaining 3 bytes are LE int32 sign-extension padding (0xFF for
        negative, 0x00 for positive).  Equivalent to reading the unit as
        an LE int32 and casting to (short).
      - n >= 0:           literal copy of (n+1) data units
      - -128 < n < 0:     repeat next 1 data unit (-n+1) times
      - n == -128:        NOP (skip)
      - Data units are unit_size bytes each, copied verbatim.
    """
    out = bytearray(dest_len)
    di = 0
    si = 0
    dest_units = dest_len // unit_size
    units_out = 0

    src_len = len(src)
    while units_out < dest_units and si + unit_size <= src_len:
        ctrl = src[si]
        si += unit_size  # consume whole control unit
        n = ctrl if ctrl < 128 else ctrl - 256

        if n >= 0:
            count = n + 1
            for _ in range(count):
                if units_out >= dest_units or si + unit_size > src_len:
                    return bytes(out), si
                out[di:di + unit_size] = src[si:si + unit_size]
                si += unit_size
                di += unit_size
                units_out += 1
        elif n != -128:
            count = -n + 1
            if si + unit_size > src_len:
                return bytes(out), si
            unit = src[si:si + unit_size]
            si += unit_size
            for _ in range(count):
                if units_out >= dest_units:
                    return bytes(out), si
                out[di:di + unit_size] = unit
                di += unit_size
                units_out += 1

    return bytes(out), si


# ---------------------------------------------------------------------------
# OmCv/OmGW image header parser
# ---------------------------------------------------------------------------

@dataclass
class ImageHeader:
    width: int
    height: int
    depth: int          # bpp: 8, 16, or 32
    compression: int    # RLE unit mode: 0=8bit, 1=16bit, 2=32bit
    version: int
    stride: int         # uncompressed bytes per row
    dotransparent: bool
    transp_packed: int  # ARGB BE, only valid when dotransparent
    dopalette: bool
    bufsize: int        # compressed buffer total size
    buf_offset: int     # byte offset from start of omgw_data to row buffer


def _parse_omgw_header(omgw_data: bytes) -> ImageHeader:
    """
    Parse OmGW image header.  All fields big-endian; bool = s16 BE.
    Layout (relative to first byte after OmGW tag):
      0  s16  width
      2  s16  height
      4  s16  depth
      6  u8   compression
      7  u8   version
      8  s16  stride (bytes per row uncompressed)
     10  s16  dotransparent (bool)
     12  [u32 transp_packed — only if version>0 and dotransparent]
     +0  s16  dopalette (bool)
     +2  [palette — skipped]
     +?  u32  bufsize
     +4  row data
    """
    pos = 0
    width      = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
    height     = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
    depth      = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
    compression = omgw_data[pos]; pos += 1
    version     = omgw_data[pos]; pos += 1
    stride     = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
    dotransparent = bool(struct.unpack_from('>h', omgw_data, pos)[0]); pos += 2

    transp_packed = 0
    if version > 0 and dotransparent:
        transp_packed = struct.unpack_from('>I', omgw_data, pos)[0]; pos += 4

    dopalette = bool(struct.unpack_from('>h', omgw_data, pos)[0]); pos += 2

    if dopalette:
        # palette header tag (4 bytes) + palette size depends on depth;
        # read the pal header to skip: OmPa/OPa2 tag + ncolors (s16 BE) + 4 bytes/color
        pal_tag = omgw_data[pos:pos + 4]; pos += 4
        ncolors = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
        pos += ncolors * 4  # each palette entry is 4 bytes

    bufsize = struct.unpack_from('>I', omgw_data, pos)[0]; pos += 4

    return ImageHeader(
        width=width, height=height, depth=depth,
        compression=compression, version=version,
        stride=stride,
        dotransparent=dotransparent, transp_packed=transp_packed,
        dopalette=dopalette,
        bufsize=bufsize, buf_offset=pos,
    )


# ---------------------------------------------------------------------------
# Image decoder: RLE → RGBA rows
# ---------------------------------------------------------------------------

UNIT_SIZES = {0: 1, 1: 2, 2: 4}


def _decode_image(hdr: ImageHeader, omgw_data: bytes) -> List[bytes]:
    """
    Decompress OmGW image data into a list of RGBA byte rows.
    Returns list of `height` rows, each `width * 4` bytes (R, G, B, A).

    Pixel storage in OMT files:
      - 32 bpp: 4 raw bytes per pixel in RGBA byte order (R, G, B, A).
      - 16 bpp: 2 bytes per pixel, BE u16 with ARGB1555 layout in the
                value (bit 15 = alpha bit, 14-10 = R5, 9-5 = G5, 4-0 = B5).
                The alpha bit is NOT used as per-pixel opacity -- it is
                ignored. All non-chroma-key pixels are fully opaque
                (matches OMT canvas behavior of `fill_alpha(0xFF)`).
      - transp_packed: RGBA-packed u32 for 32 bpp (R in high byte, A in
                low). For 16 bpp it holds the BE u16 value in its low 16
                bits. Verified: sprites[0] background pixel bytes
                [00, 42, 3F, BA] match transp_packed=0x00423FBA
                byte-for-byte; sprite[184] 16bpp pixel bytes [25, 0C]
                read as BE = 0x250C, matching transp_packed=0x0000250C.
    """
    pos = hdr.buf_offset
    unit_size = UNIT_SIZES.get(hdr.compression, 1)

    transp_r = (hdr.transp_packed >> 24) & 0xFF
    transp_g = (hdr.transp_packed >> 16) & 0xFF
    transp_b = (hdr.transp_packed >>  8) & 0xFF
    transp_r5 = (hdr.transp_packed >> 10) & 0x1F
    transp_g5 = (hdr.transp_packed >>  5) & 0x1F
    transp_b5 =  hdr.transp_packed        & 0x1F

    rgba_rows = []

    blank_row = bytes(hdr.width * 4)
    end_of_buf = len(omgw_data)

    for _ in range(hdr.height):
        # Per-row: [s16 BE compressed_len][compressed_bytes]
        # NOTE: pos advances by the bytes actually consumed by rle_unpack,
        # not by `nbyter`. The C code does the same: when nbyter is not a
        # multiple of unit_size, the trailing stub bytes stay in the
        # buffer and are reinterpreted by the next row's header read.
        # If the buffer is truncated short of `height` rows (occurs in a
        # few malformed assets), pad the rest with transparent pixels.
        if pos + 2 > end_of_buf:
            rgba_rows.append(blank_row)
            continue
        nbyter = struct.unpack_from('>h', omgw_data, pos)[0]; pos += 2
        comp_row = omgw_data[pos:pos + nbyter]
        raw_row, consumed = _rle_unpack(comp_row, hdr.stride, unit_size)
        pos += consumed

        row_rgba = bytearray(hdr.width * 4)

        if hdr.depth == 32:
            # File byte order: [R, G, B, A] per pixel
            for x in range(hdr.width):
                o = x * 4
                r = raw_row[o]
                g = raw_row[o + 1]
                b = raw_row[o + 2]
                a = raw_row[o + 3]
                if hdr.dotransparent and r == transp_r and g == transp_g and b == transp_b:
                    r = g = b = a = 0
                row_rgba[x*4:x*4+4] = bytes([r, g, b, a])

        elif hdr.depth == 16:
            for x in range(hdr.width):
                o = x * 2
                pix = (raw_row[o] << 8) | raw_row[o + 1]  # BE u16
                r5 = (pix >> 10) & 0x1F
                g5 = (pix >>  5) & 0x1F
                b5 =  pix        & 0x1F
                if hdr.dotransparent and r5 == transp_r5 and g5 == transp_g5 and b5 == transp_b5:
                    r = g = b = a = 0
                else:
                    # Alpha bit (bit 15) is ignored; non-chroma pixels are opaque
                    r = (r5 << 3) | (r5 >> 2)
                    g = (g5 << 3) | (g5 >> 2)
                    b = (b5 << 3) | (b5 >> 2)
                    a = 255
                row_rgba[x*4:x*4+4] = bytes([r, g, b, a])

        else:
            # 8 bpp — paletted; not yet implemented, output grey
            for x in range(hdr.width):
                v = raw_row[x] if x < len(raw_row) else 0
                row_rgba[x*4:x*4+4] = bytes([v, v, v, 255])

        rgba_rows.append(bytes(row_rgba))

    return rgba_rows


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class ImageChunk:
    index: int
    offset: int          # file offset of OmCv tag
    width: int
    height: int
    depth: int
    compression: int
    stride: int
    dotransparent: bool
    transp_packed: int
    dopalette: bool
    bufsize: int
    _omgw_data: bytes = field(repr=False)  # raw bytes from OmGW tag onward
    _hdr: object = field(repr=False)       # parsed ImageHeader


@dataclass
class AudioEntry:
    index: int
    name: str
    offset: int
    size: int
    raw_data: bytes = field(repr=False)


@dataclass
class OmtFile:
    path: str
    total_size: int
    has_images: bool
    has_audio: bool
    images: List[ImageChunk] = field(default_factory=list)
    audio: List[AudioEntry] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Image chunk finder
# ---------------------------------------------------------------------------

def _parse_images(data: bytes) -> List[ImageChunk]:
    """
    Parse OmCv/OmGW image chunks sequentially.
    We cannot scan for all OmCv positions up front because the compressed
    image buffer may contain the byte sequence 'OmCv' as a false match.
    Instead, parse each header to get bufsize, then advance exactly past
    the buffer before searching for the next chunk.
    """
    chunks = []
    search_from = 0
    i = 0

    while True:
        start = data.find(b'OmCv', search_from)
        if start == -1:
            break

        # OmCv(4) + attr(4) + OmGW(4) = 12 bytes
        if start + 12 > len(data) or data[start + 8:start + 12] != b'OmGW':
            search_from = start + 1
            continue

        omgw_start = start + 12
        if omgw_start + 20 > len(data):
            break

        try:
            hdr = _parse_omgw_header(data[omgw_start:])
        except Exception:
            search_from = start + 1
            continue

        # Slice exactly as much omgw_data as the header + buffer need
        omgw_end = omgw_start + hdr.buf_offset + hdr.bufsize
        omgw_data = data[omgw_start:omgw_end]

        chunks.append(ImageChunk(
            index=i,
            offset=start,
            width=hdr.width,
            height=hdr.height,
            depth=hdr.depth,
            compression=hdr.compression,
            stride=hdr.stride,
            dotransparent=hdr.dotransparent,
            transp_packed=hdr.transp_packed,
            dopalette=hdr.dopalette,
            bufsize=hdr.bufsize,
            _omgw_data=omgw_data,
            _hdr=hdr,
        ))

        search_from = omgw_end  # skip past this chunk's data
        i += 1

    return chunks


# ---------------------------------------------------------------------------
# Audio parser
# ---------------------------------------------------------------------------

def _parse_audio(data: bytes) -> List[AudioEntry]:
    entries = []
    riff_positions = []
    pos = 0
    while True:
        idx = data.find(b'RIFF', pos)
        if idx == -1:
            break
        if idx + 12 <= len(data) and data[idx + 8:idx + 12] == b'WAVE':
            riff_size = struct.unpack_from('<I', data, idx + 4)[0]
            riff_positions.append((idx, riff_size))
        pos = idx + 1

    if not riff_positions:
        return entries

    last_wav_end = max(o + 8 + s for o, s in riff_positions)
    names = {}

    for riff_off, riff_sz in riff_positions:
        off_bytes = struct.pack('>I', riff_off)
        search_pos = last_wav_end
        while True:
            found = data.find(off_bytes, search_pos)
            if found == -1:
                break
            name_len = data[found + 8] if found + 9 <= len(data) else 0
            if 0 < name_len <= 64 and found + 9 + name_len <= len(data):
                raw_name = data[found + 9:found + 9 + name_len]
                if all(0x20 <= b < 0x7f for b in raw_name):
                    names[riff_off] = raw_name.decode('ascii')
            search_pos = found + 1

    for i, (riff_off, riff_size) in enumerate(riff_positions):
        total = 8 + riff_size
        raw = data[riff_off:riff_off + total]
        name = names.get(riff_off, f'audio_{i}')
        entries.append(AudioEntry(index=i, name=name, offset=riff_off,
                                  size=total, raw_data=raw))

    return entries


# ---------------------------------------------------------------------------
# Top-level parse
# ---------------------------------------------------------------------------

def parse_omt(path: str) -> OmtFile:
    data = open(path, 'rb').read()
    if data[:4] != b'0MF2':
        raise ValueError(f'Not an OMT file: {path}')

    total_size = struct.unpack_from('>I', data, 4)[0]
    has_images = b'OmCv' in data
    has_audio  = b'RIFF' in data

    return OmtFile(
        path=path,
        total_size=total_size,
        has_images=has_images,
        has_audio=has_audio,
        images=_parse_images(data) if has_images else [],
        audio=_parse_audio(data)   if has_audio  else [],
    )


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

def omt_summary(omt: OmtFile) -> str:
    lines = [f'File: {omt.path}', f'Size: {omt.total_size} bytes']
    if omt.has_images:
        lines.append(f'Image chunks: {len(omt.images)}')
        for img in omt.images:
            tp = f'#{img.transp_packed:08x}' if img.dotransparent else 'none'
            lines.append(
                f'  [{img.index:3d}] {img.width}x{img.height} '
                f'depth={img.depth} comp={img.compression} '
                f'stride={img.stride} transp={tp} buf={img.bufsize}B'
            )
    if omt.has_audio:
        lines.append(f'Audio entries: {len(omt.audio)}')
        for a in omt.audio:
            lines.append(f'  [{a.index}] {a.name!r} offset={a.offset} size={a.size}')
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Export
# ---------------------------------------------------------------------------

def export_omt(omt: OmtFile, out_dir: str):
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.splitext(os.path.basename(omt.path))[0]
    meta = {
        'file': omt.path,
        'total_size': omt.total_size,
        'has_images': omt.has_images,
        'has_audio': omt.has_audio,
    }

    if omt.has_images:
        img_dir = os.path.join(out_dir, base + '_images')
        os.makedirs(img_dir, exist_ok=True)
        img_meta = []
        errors = []
        for img in omt.images:
            fname = f'{img.index:04d}_{img.width}x{img.height}d{img.depth}.png'
            out_path = os.path.join(img_dir, fname)
            try:
                rgba_rows = _decode_image(img._hdr, img._omgw_data)
                _write_png(out_path, img.width, img.height, rgba_rows)
                status = 'ok'
            except Exception as e:
                status = f'error: {e}'
                errors.append((img.index, str(e)))
            img_meta.append({
                'index': img.index,
                'width': img.width,
                'height': img.height,
                'depth': img.depth,
                'compression': img.compression,
                'stride': img.stride,
                'dotransparent': img.dotransparent,
                'transp_packed': img.transp_packed,
                'dopalette': img.dopalette,
                'bufsize': img.bufsize,
                'file': out_path,
                'status': status,
            })
        meta['images'] = img_meta
        if errors:
            meta['image_errors'] = errors

    if omt.has_audio:
        aud_dir = os.path.join(out_dir, base + '_audio')
        os.makedirs(aud_dir, exist_ok=True)
        aud_meta = []
        for a in omt.audio:
            safe = ''.join(c if c.isalnum() or c in '-_' else '_' for c in a.name)
            out_path = os.path.join(aud_dir, f'{a.index:04d}_{safe}.wav')
            with open(out_path, 'wb') as f:
                f.write(a.raw_data)
            aud_meta.append({
                'index': a.index, 'name': a.name,
                'offset': a.offset, 'size': a.size,
                'file': out_path,
            })
        meta['audio'] = aud_meta

    json_path = os.path.join(out_dir, base + '.json')
    with open(json_path, 'w') as f:
        json.dump(meta, f, indent=2)

    return meta


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    import argparse
    ap = argparse.ArgumentParser(description='Parse OMT2 container files')
    ap.add_argument('file', nargs='?', help='OMT file to parse')
    ap.add_argument('--out', help='Output directory for extracted assets')
    ap.add_argument('--all', nargs=2, metavar=('OMT_DIR', 'OUT_DIR'),
                    help='Batch parse all .omt files in a directory')
    args = ap.parse_args()

    if args.all:
        omt_dir, out_dir = args.all
        files = sorted(f for f in os.listdir(omt_dir) if f.lower().endswith('.omt'))
        total_errors = 0
        for fname in files:
            path = os.path.join(omt_dir, fname)
            try:
                omt = parse_omt(path)
                sub = os.path.join(out_dir, os.path.splitext(fname)[0])
                meta = export_omt(omt, sub)
                img_errors = len(meta.get('image_errors', []))
                tag = f'ERR({img_errors})' if img_errors else 'OK '
                print(f'{tag} {fname}: {len(omt.images)} images, {len(omt.audio)} audio')
                total_errors += img_errors
            except Exception as e:
                print(f'ERR {fname}: {e}')
                total_errors += 1
        print(f'\n{len(files)} files, {total_errors} decode errors')
        return

    if not args.file:
        ap.print_help()
        return

    omt = parse_omt(args.file)
    print(omt_summary(omt))

    if args.out:
        export_omt(omt, args.out)
        print(f'\nExtracted to: {args.out}')


if __name__ == '__main__':
    main()
