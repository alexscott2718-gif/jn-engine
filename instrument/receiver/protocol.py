"""
OMT2 instrumentation wire protocol (M4+)
Python mirror of protocol.h
Binary, little-endian. Version mismatch is rejected.
"""

import struct
from typing import NamedTuple, List, Optional

OMTC_MAGIC = 0x434d544f  # "OMTC" little-endian
OMTC_VERSION = 3
OMTC_VERSION_MIN = 1  # receiver/diff accept [MIN, VERSION] inclusive
                      # v3 adds TEXTURE_PIXELS (raw locked-surface bytes per
                      # texture, one-shot) for pixel-faithful .omtc replay.

# Record types (proxy -> receiver)
RECORD_FRAME_BEGIN = 1
RECORD_FRAME_END = 2
RECORD_SET_TRANSFORM = 3
RECORD_VIEWPORT = 4
RECORD_SET_TEXTURE = 5
RECORD_TEXTURE_DEF = 6
RECORD_SET_RENDERSTATE = 7
RECORD_SET_TEXSTAGESTATE = 8
RECORD_SET_LIGHT = 9
RECORD_SET_MATERIAL = 10
RECORD_DRAW_PRIMITIVE = 11
RECORD_DRAW_INDEXED = 12
RECORD_FRAME_MARK = 13  # v2: tagged frame (receiver-driven)
RECORD_TEXTURE_PIXELS = 14  # v3: raw locked-surface bytes (one-shot per tex)

# Command records (receiver -> proxy, same framing, disjoint type space)
CMD_CAMERA_DELTA = 0x80
CMD_CAMERA_CLEAR = 0x81
CMD_CAPTURE_START = 0x82
CMD_CAPTURE_STOP = 0x83
CMD_MARK_FRAME = 0x84
CMD_REDUMP_TEXTURES = 0x85
CMD_KILLSWITCH = 0x86

# Transform types
TRANSFORM_WORLD = 0
TRANSFORM_VIEW = 1
TRANSFORM_PROJ = 2

# FVF for M4
FVF_TEST = 0x152

# SHA-1 length
SHA1_LEN = 20


class Header(NamedTuple):
    magic: int
    version: int
    pid: int
    screen_w: int
    screen_h: int

    @staticmethod
    def unpack(data):
        magic, version, pid, screen_w, screen_h = struct.unpack('<IHIHH', data)
        return Header(magic, version, pid, screen_w, screen_h)

    @staticmethod
    def size():
        return 14


class FrameBegin(NamedTuple):
    seq: int
    t_ms: int

    @staticmethod
    def unpack(data):
        seq, t_ms = struct.unpack('<II', data)
        return FrameBegin(seq, t_ms)

    @staticmethod
    def size():
        return 8


class FrameEnd(NamedTuple):
    seq: int
    draw_count: int
    dropped: int

    @staticmethod
    def unpack(data):
        seq, draw_count, dropped = struct.unpack('<III', data)
        return FrameEnd(seq, draw_count, dropped)

    @staticmethod
    def size():
        return 12


class SetTransform(NamedTuple):
    which: int
    matrix: tuple  # 16 floats

    @staticmethod
    def unpack(data):
        which = struct.unpack('<B', data[:1])[0]
        matrix = struct.unpack('<16f', data[1:65])
        return SetTransform(which, matrix)

    @staticmethod
    def size():
        return 65


class Viewport(NamedTuple):
    x: int
    y: int
    w: int
    h: int
    minz: float
    maxz: float

    @staticmethod
    def unpack(data):
        x, y, w, h, minz, maxz = struct.unpack('<iiiiff', data)
        return Viewport(x, y, w, h, minz, maxz)

    @staticmethod
    def size():
        return 24


class SetTexture(NamedTuple):
    stage: int
    tex_id: int

    @staticmethod
    def unpack(data):
        stage, tex_id = struct.unpack('<BI', data[:5])
        return SetTexture(stage, tex_id)

    @staticmethod
    def size():
        return 5


class TextureDef(NamedTuple):
    tex_id: int
    width: int
    height: int
    d3dfmt: int
    sha1: bytes

    @staticmethod
    def unpack(data):
        tex_id, w, h, d3dfmt = struct.unpack('<IHHI', data[:12])
        sha1 = data[12:12+SHA1_LEN]
        return TextureDef(tex_id, w, h, d3dfmt, sha1)

    @staticmethod
    def size():
        return 12 + SHA1_LEN


class TexturePixels(NamedTuple):
    """v3 TEXTURE_PIXELS: packed raw locked-surface bytes following the
    header. `pixels` is exactly `pixel_bytes` long (w * h * (bpp+7)/8)."""
    tex_id: int
    width: int
    height: int
    bpp: int
    pixel_bytes: int
    pixels: bytes

    @staticmethod
    def unpack(data):
        tex_id, w, h, bpp, n = struct.unpack('<IHHII', data[:16])
        return TexturePixels(tex_id, w, h, bpp, n, bytes(data[16:16 + n]))


class SetRenderState(NamedTuple):
    state: int
    value: int

    @staticmethod
    def unpack(data):
        state, value = struct.unpack('<II', data)
        return SetRenderState(state, value)

    @staticmethod
    def size():
        return 8


class SetTexStageState(NamedTuple):
    stage: int
    state: int
    value: int

    @staticmethod
    def unpack(data):
        stage, state, value = struct.unpack('<BII', data)
        return SetTexStageState(stage, state, value)

    @staticmethod
    def size():
        return 9


class Vertex(NamedTuple):
    """Vertex for FVF 0x152: XYZ·NORMAL·DIFFUSE·TEX1 (36 bytes)"""
    x: float
    y: float
    z: float
    nx: float
    ny: float
    nz: float
    diffuse: int
    u: float
    v: float

    @staticmethod
    def unpack(data):
        x, y, z, nx, ny, nz, diffuse, u, v = struct.unpack('<3f3fI2f', data)
        return Vertex(x, y, z, nx, ny, nz, diffuse, u, v)

    @staticmethod
    def size():
        return 36


class SetLight(NamedTuple):
    """SET_LIGHT: index u8, then a raw D3DLIGHT7 struct (104 bytes)."""
    index: int
    ltype: int           # D3DLIGHTTYPE: 1=POINT 2=SPOT 3=DIRECTIONAL
    diffuse: tuple       # rgba
    specular: tuple
    ambient: tuple
    position: tuple      # xyz
    direction: tuple     # xyz
    range: float
    falloff: float
    atten: tuple         # (a0, a1, a2)
    theta: float
    phi: float

    @staticmethod
    def unpack(data):
        index = data[0]
        f = struct.unpack('<I 4f 4f 4f 3f 3f 7f', data[1:1 + 104])
        return SetLight(index, f[0], f[1:5], f[5:9], f[9:13],
                        f[13:16], f[16:19], f[19], f[20],
                        (f[21], f[22], f[23]), f[24], f[25])


class SetMaterial(NamedTuple):
    """SET_MATERIAL: a raw D3DMATERIAL7 struct (68 bytes)."""
    diffuse: tuple
    ambient: tuple
    specular: tuple
    emissive: tuple
    power: float

    @staticmethod
    def unpack(data):
        f = struct.unpack('<4f 4f 4f 4f f', data[:68])
        return SetMaterial(f[0:4], f[4:8], f[8:12], f[12:16], f[16])


class DrawPrimitive(NamedTuple):
    prim_type: int
    fvf: int
    vtx_count: int
    vertices: List[Vertex]

    @staticmethod
    def unpack(data):
        prim_type, fvf, vtx_count = struct.unpack('<BII', data[:9])
        vertices = []
        offset = 9
        for _ in range(vtx_count):
            v = Vertex.unpack(data[offset:offset+Vertex.size()])
            vertices.append(v)
            offset += Vertex.size()
        return DrawPrimitive(prim_type, fvf, vtx_count, vertices)


class FrameMark(NamedTuple):
    """v2 FRAME_MARK: tag flowed back to a captured frame via OMTC_CMD_MARK_FRAME."""
    seq: int
    tag: int

    @staticmethod
    def unpack(data):
        seq, tag = struct.unpack('<II', data)
        return FrameMark(seq, tag)

    @staticmethod
    def size():
        return 8


class Frame(NamedTuple):
    """One captured frame"""
    begin: Optional[FrameBegin]
    end: Optional[FrameEnd]
    transforms: dict  # which -> matrix
    viewport: Optional[Viewport]
    textures: dict  # tex_id -> TextureDef
    texture_bindings: dict  # stage -> tex_id
    render_states: dict  # state -> value
    texstage_states: dict  # (stage, state) -> value
    draw_calls: List[DrawPrimitive]
    lights: dict  # index -> SetLight
    material: Optional[SetMaterial]
    mark: Optional[int] = None  # v2: FRAME_MARK tag, when present


RECORD_HEADER_SIZE = 4


def unpack_record_header(data):
    """Returns (type, payload_len) from 4-byte header"""
    type_byte = data[0]
    len_hi = data[1]
    len_lo = struct.unpack('<H', data[2:4])[0]
    payload_len = (len_hi << 16) | len_lo
    return type_byte, payload_len


def pack_record(rec_type, payload=b""):
    """Pack a single record (header + payload). Used by the M7b control CLI."""
    n = len(payload)
    if n > 0xFFFFFF:
        raise ValueError(f"record payload too large: {n}")
    header = bytes((rec_type & 0xFF, (n >> 16) & 0xFF))
    header += struct.pack('<H', n & 0xFFFF)
    return header + payload


def try_parse_record(buf, offset):
    """Parse one record from buf starting at offset.

    Returns (rec_type, payload, next_offset), or None if buf does not yet
    hold a complete record at offset (streaming mode -- caller waits for
    more bytes).
    """
    if offset + RECORD_HEADER_SIZE > len(buf):
        return None
    rec_type, payload_len = unpack_record_header(
        buf[offset:offset + RECORD_HEADER_SIZE])
    end = offset + RECORD_HEADER_SIZE + payload_len
    if end > len(buf):
        return None
    payload = buf[offset + RECORD_HEADER_SIZE:end]
    return rec_type, payload, end
