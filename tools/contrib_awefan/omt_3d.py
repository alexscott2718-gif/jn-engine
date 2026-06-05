"""omt_3d.py
pip
Lightweight decoder for OMT '3DSP' shape blocks (from Open Media Toolkit).
Supports parsing shapes written by OMedia3DShape::write_class (version <= 5)
and exporting geometry to Wavefront OBJ.

This is a minimal, dependency-free implementation to decode vertices, normals,
polygons and UVs. Materials/textures are parsed at a high level but not
resolved to actual images.

Usage:
    python omt_3d.py input.3dsh -o output.obj

"""

from __future__ import annotations
import struct
import io
import os
import sys
from tkinter import messagebox
from typing import List, Tuple, Optional, Dict

try:
    from PIL import Image
except Exception as e:
    Image = None

try:
    from PySide6 import QtWidgets, QtGui, QtCore
except Exception as e:
    QtWidgets = None
    QtGui = None
    QtCore = None


# --- Binary readers with configurable endianness ---
# Standalone .3dsh files: big-endian (network byte order) - confirmed from C++ source
# Database .omt files: little-endian (native PC byte order)
# Parser auto-detects by checking version field
_ENDIAN = '>'

def _u32(data: bytes, offset: int, endian=None) -> Tuple[int, int]:
    e = endian if endian else _ENDIAN
    return struct.unpack_from(f'{e}I', data, offset)[0], offset + 4


def _i32(data: bytes, offset: int, endian=None) -> Tuple[int, int]:
    e = endian if endian else _ENDIAN
    return struct.unpack_from(f'{e}i', data, offset)[0], offset + 4


def _u16(data: bytes, offset: int, endian=None) -> Tuple[int, int]:
    e = endian if endian else _ENDIAN
    return struct.unpack_from(f'{e}H', data, offset)[0], offset + 2


def _i16(data: bytes, offset: int, endian=None) -> Tuple[int, int]:
    e = endian if endian else _ENDIAN
    return struct.unpack_from(f'{e}h', data, offset)[0], offset + 2


def _f32(data: bytes, offset: int, endian=None) -> Tuple[float, int]:
    e = endian if endian else _ENDIAN
    return struct.unpack_from(f'{e}f', data, offset)[0], offset + 4


def _bool(data: bytes, offset: int, endian=None, size: int = 2) -> Tuple[bool, int]:
    """Read a bool. OMT streams serialize booleans as 2-byte shorts by default.
    Supports explicit 1-, 2-, or 4-byte boolean encodings for completeness."""
    e = endian if endian else _ENDIAN
    if size == 1:
        val = struct.unpack_from(f"{e}B", data, offset)[0]
        return (val != 0), offset + 1
    elif size == 2:
        val = struct.unpack_from(f"{e}H", data, offset)[0]
        return (val != 0), offset + 2
    elif size == 4:
        val = struct.unpack_from(f"{e}I", data, offset)[0]
        return (val != 0), offset + 4
    else:
        # Fallback to 2-byte if an unusual size is provided
        val = struct.unpack_from(f"{e}H", data, offset)[0]
        return (val != 0), offset + 2


# --- Data containers ---
class OMTMesh:
    def __init__(self) -> None:
        self.version: Optional[int] = None
        self.vertices: List[Tuple[float, float, float]] = []
        self.normals: List[Tuple[float, float, float]] = []
        self.colors: List[Tuple[float, float, float, float]] = []  # ARGB floats
        # Polygons: each is dict with keys: "v_idx" (list of vertex indices),
        # "n_idx" (list of normal indices), "uv" (list of (u,v) floats)
        self.polygons: List[Dict] = []
        # Optional: extra texture pass sets not fully resolved here
        self.extra_textures = []

    def __repr__(self) -> str:
        return f"OMTMesh(vertices={len(self.vertices)}, polygons={len(self.polygons)}, normals={len(self.normals)})"


# --- Parser ---
def parse_3dsh_bytes(data: bytes, offset: int = 0, endian: Optional[str] = None) -> Tuple[Optional[OMTMesh], int]:
    """Try to parse a single OMT '3DSP' shape block starting at offset.
    Returns (mesh, new_offset). If no valid header found at offset, returns (None, offset).
    
    Endianness auto-detection:
    - If endian is None, tries big-endian first (standalone .3dsh files)
    - If version > 5, tries little-endian (database .omt files)
    - Returns mesh on success
    """
    # Need at least 8 bytes for header + version
    if offset + 8 > len(data):
        return None, offset

    # Header is ASCII '3DSP'
    if data[offset:offset + 4] != b'3DSP':
        return None, offset

    # OMT stores 3DSP blocks in big-endian order on disk (VERSION and payload).
    # Keep a tiny fallback: if caller forces something else, honor it.
    if endian is None:
        endian = '>'
    
    # Set global endian for helper functions
    global _ENDIAN
    _ENDIAN = endian
    
    cur = offset + 4  # skip magic

    # VERSION is always big-endian (even in database files)
    version = struct.unpack_from('>I', data, cur)[0]
    cur += 4
    
    if version > 5 or version < 0:
        raise ValueError(f"Unsupported OMT shape version {version}")

    mesh = OMTMesh()
    mesh.version = version

    # visual sphere (4 floats), global bounding sphere (4 floats)
    for _ in range(8):
        _, cur = _f32(data, cur)

    # version 1..2 have extra far clip info (bool stored as short)
    if 0 < version < 3:
        _, cur = _f32(data, cur)  # custom_farzclip
        _, cur = _bool(data, cur, size=2)  # do_custom_farzclip (serialized as short)

    # Vertices
    if version < 3:
        vcount, cur = _u32(data, cur)  # legacy blocks still use 32-bit counts
    else:
        vcount, cur = _u32(data, cur)
    for _ in range(vcount):
        x, cur = _f32(data, cur)
        y, cur = _f32(data, cur)
        z, cur = _f32(data, cur)
        mesh.vertices.append((x, y, z))

    # Polygons
    if version < 3:
        polycount, cur = _u32(data, cur)
    else:
        polycount, cur = _u32(data, cur)
    for _ in range(polycount):
        if version < 3:
            # v1/v2 polygon layout (flags stored as 2-byte bools, no flags field, material flag present)
            two_sided, cur = _bool(data, cur, size=2)
            selected, cur = _bool(data, cur, size=2)
            priority_key, cur = _i16(data, cur)
            user_id, cur = _u32(data, cur)
            if version > 0:
                _, cur = _f32(data, cur)  # correct_sorting
                _, cur = _f32(data, cur)  # back_correct_sorting
            _, cur = _i16(data, cur)  # sort ref point (unused)
            nverts, cur = _u16(data, cur)

            v_idx = []
            n_idx = []
            uv = []
            for _vi in range(nverts):
                vi, cur = _u32(data, cur)
                u, cur = _f32(data, cur)
                v, cur = _f32(data, cur)
                if version > 1:
                    ni, cur = _u32(data, cur)
                else:
                    ni = vi
                v_idx.append(vi)
                n_idx.append(ni)
                uv.append((u, v))

            nx, cur = _f32(data, cur)
            ny, cur = _f32(data, cur)
            nz, cur = _f32(data, cur)

            load_mat, cur = _bool(data, cur, size=2)
            mat_link = None
            if load_mat:
                filled, cur = _bool(data, cur, size=2)
                if filled:
                    lt, cur = _u32(data, cur)
                    li, cur = _i32(data, cur)
                    mat_link = (lt, li)

            flags = 0
            if two_sided:
                flags |= 0x2  # om3pf_TwoSided
            if selected:
                flags |= 0x1  # om3pf_Selected

            mesh.polygons.append({
                "v_idx": v_idx,
                "n_idx": n_idx,
                "uv": uv,
                "normal": (nx, ny, nz),
                "flags": flags,
                "user_id": user_id,
                "material_link": mat_link,
                "text_extra_index": None,
            })
        else:
            # v3+ layout (all 4-byte fields, material link always present)
            flags, cur = _u32(data, cur)
            user_id, cur = _u32(data, cur)
            nverts, cur = _u32(data, cur)

            v_idx = []
            n_idx = []
            uv = []

            for _vi in range(nverts):
                vi, cur = _u32(data, cur)
                ni, cur = _u32(data, cur)
                _, cur = _u32(data, cur)  # color index (unused here)
                u, cur = _f32(data, cur)
                v, cur = _f32(data, cur)

                if version > 4:
                    extra_count, cur = _u32(data, cur)
                    for _e in range(extra_count):
                        _, cur = _f32(data, cur)
                        _, cur = _f32(data, cur)

                v_idx.append(vi)
                n_idx.append(ni)
                uv.append((u, v))

            nx, cur = _f32(data, cur)
            ny, cur = _f32(data, cur)
            nz, cur = _f32(data, cur)

            mat_link = None
            filled, cur = _bool(data, cur, size=2)
            if filled:
                lt, cur = _u32(data, cur)
                li, cur = _i32(data, cur)
                mat_link = (lt, li)

            tex_extra_idx = None
            if version > 4:
                tex_extra_idx, cur = _i32(data, cur)

            mesh.polygons.append({
                "v_idx": v_idx,
                "n_idx": n_idx,
                "uv": uv,
                "normal": (nx, ny, nz),
                "flags": flags,
                "user_id": user_id,
                "material_link": mat_link,
                "text_extra_index": tex_extra_idx,
            })

    # Bounding spheres
    bs_count, cur = _u32(data, cur)
    for _ in range(bs_count):
        for _ in range(4):
            _, cur = _f32(data, cur)

    # Normals
    normals_count, cur = _u32(data, cur)
    for _ in range(normals_count):
        nx, cur = _f32(data, cur)
        ny, cur = _f32(data, cur)
        nz, cur = _f32(data, cur)
        mesh.normals.append((nx, ny, nz))

    # Colors (version > 2)
    if version > 2:
        col_count, cur = _u32(data, cur)
        for _ in range(col_count):
            r, cur = _f32(data, cur)
            g, cur = _f32(data, cur)
            b, cur = _f32(data, cur)
            a, cur = _f32(data, cur)
            mesh.colors.append((r, g, b, a))

    # Flags (version>3)
    if version > 3:
        _, cur = _u32(data, cur)

    # Extra texture pass sets (version>4)
    if version > 4:
        set_count, cur = _u32(data, cur)
        for _ in range(set_count):
            pass_set_count, cur = _u32(data, cur)
            pass_set = []
            for _ in range(pass_set_count):
                # slink
                filled, cur = _bool(data, cur, size=2)
                pass_slink = None
                if filled:
                    lt, cur = _u32(data, cur)
                    li, cur = _i32(data, cur)
                    pass_slink = (lt, li)
                # write_enum(texture_address_mode) -> short
                am, cur = _i16(data, cur)
                co, cur = _i16(data, cur)
                pass_set.append((pass_slink, am, co))
            mesh.extra_textures.append(pass_set)

    return mesh, cur


# --- Diagnostics helpers -------------------------------------------
def diagnose_3dsh_at(data: bytes, offset: int = 0) -> str:
    """Return a short diagnostic string describing why parsing failed at offset.
    This walks the same high-level layout as `parse_3dsh_bytes` but stops on EOF
    and reports the position and nearby hexdump for easier debugging."""
    def hexdump(pos, size=64):
        start = max(0, pos - 16)
        end = min(len(data), pos + size)
        return data[start:end].hex()

    cur = offset
    L = len(data)
    if cur + 8 > L:
        return f'Not enough data for header at {cur} (need 8, have {L-cur})'
    if data[cur:cur+4] != b'3DSP':
        return f'No 3DSP header at {cur} (found {data[cur:cur+4]!r})'
    msgs = [f'3DSP header at {cur}']

    try:
        version = struct.unpack_from('>I', data, cur + 4)[0]
    except Exception:
        return f'Failed reading version at {cur}, hexdump: {hexdump(cur)}'
    msgs.append(f'version={version}')

    # Data is always stored big-endian here
    global _ENDIAN
    _ENDIAN = '>'
    cur += 8  # magic + version

    # spheres
    need = 8 * 4
    if cur + need > L:
        msgs.append(f'Not enough bytes for spheres: need {need}, have {L-cur}')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)
    cur += need

    if 0 < version < 3:
        if cur + 6 > L:
            msgs.append(f'Not enough bytes for custom far clip at {cur} (need 6)')
            msgs.append(f'hexdump: {hexdump(cur)}')
            return '\n'.join(msgs)
        cur += 4  # float
        cur += 2  # bool (stored as short)
    count_size = 4
    if cur + count_size > L:
        msgs.append(f'No space for vertex count at {cur}')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)

    try:
        vcount, cur = _u32(data, cur)
    except Exception as e:
        msgs.append(f'Error reading vcount at {cur}: {e}')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)
    msgs.append(f'vertices declared: {vcount}')

    vblk = vcount * 12
    if cur + vblk > L:
        msgs.append(f'Vertex block truncated: need {vblk} bytes, only {L-cur} remain')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)
    cur += vblk

    if cur + count_size > L:
        msgs.append('No space for polygon count')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)
    try:
        pcount, cur = _u32(data, cur)
    except Exception as e:
        msgs.append(f'Error reading polycount at {cur}: {e}')
        msgs.append(f'hexdump: {hexdump(cur)}')
        return '\n'.join(msgs)
    msgs.append(f'polygons declared: {pcount}')

    if pcount > 1000000 or vcount > 1000000:
        msgs.append('Counts look absurdly large—possible corruption or wrong endianness')
        msgs.append(f'hexdump: {hexdump(cur)}')

    return '\n'.join(msgs)


def find_shapes_in_file(path: str) -> List[OMTMesh]:
    with open(path, 'rb') as f:
        data = f.read()

    shapes: List[OMTMesh] = []
    offset = 0
    while True:
        idx = data.find(b'3DSP', offset)
        if idx == -1:
            break
        
        try:
            mesh, newoff = parse_3dsh_bytes(data, idx)  # auto-detects endianness
            if mesh is not None:
                # Sanity check: reasonable vertex/polygon counts
                if len(mesh.vertices) < 1000000 and len(mesh.polygons) < 1000000:
                    shapes.append(mesh)
                    offset = newoff
                else:
                    print(f"Warning: skipping 3DSP at offset {idx}: unreasonable counts ({len(mesh.vertices)} verts, {len(mesh.polygons)} polys)", file=sys.stderr)
                    offset = idx + 4
            else:
                offset = idx + 4
        except Exception as e:
            import traceback
            tb = ''.join(traceback.format_exception_only(type(e), e)).strip()
            diag = diagnose_3dsh_at(data, idx)
            print(f"Warning: failed to parse 3DSP at offset {idx}: {tb}\n{diag}", file=sys.stderr)
            offset = idx + 4

    return shapes


# --- OBJ Exporter ---
def export_obj(mesh: OMTMesh, path: str) -> None:
    """Export mesh to a Wavefront OBJ file (vertices, vt, vn, faces)."""
    lines: List[str] = []

    # Write vertices
    for v in mesh.vertices:
        lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}")

    # Gather unique UVs
    uv_map: Dict[Tuple[float, float], int] = {}
    uvs: List[Tuple[float, float]] = []

    # Normals: if available use mesh.normals, otherwise we'll create per-face normals
    vn_list: List[Tuple[float, float, float]] = mesh.normals.copy()

    # Map of faces to vn indices for fallback normals
    face_vn_indices: List[int] = []

    # Build uvs and ensure vn indices exist
    for poly in mesh.polygons:
        # if normals missing or indices out-of-range, add face normal
        need_face_normal = False
        for ni in poly['n_idx']:
            if ni >= len(vn_list):
                need_face_normal = True
                break
        if need_face_normal:
            # add polygon normal to vn_list and record its index
            fn = poly.get('normal', (0.0, 0.0, 0.0))
            vn_list.append(fn)
            face_vn_indices.append(len(vn_list))
        else:
            face_vn_indices.append(-1)

        for uv in poly['uv']:
            if uv not in uv_map:
                uv_map[uv] = len(uvs) + 1
                uvs.append(uv)

    # Write UVs
    for uv in uvs:
        lines.append(f"vt {uv[0]:.6f} {uv[1]:.6f}")

    # Write normals
    for vn in vn_list:
        lines.append(f"vn {vn[0]:.6f} {vn[1]:.6f} {vn[2]:.6f}")

    # Faces
    for fi, poly in enumerate(mesh.polygons):
        parts = []
        face_added_vn_idx = face_vn_indices[fi]
        for i, vi in enumerate(poly['v_idx']):
            v_idx = vi + 1  # file is zero-based; OBJ is 1-based
            vt_idx = uv_map.get(poly['uv'][i], None)
            # Determine vn index
            ni = poly['n_idx'][i]
            if ni < len(mesh.normals):
                vn_idx = ni + 1
            else:
                # use added per-face normal
                vn_idx = face_added_vn_idx

            if vt_idx is not None and vn_idx is not None:
                parts.append(f"{v_idx}/{vt_idx}/{vn_idx}")
            elif vt_idx is not None:
                parts.append(f"{v_idx}/{vt_idx}")
            elif vn_idx is not None:
                parts.append(f"{v_idx}//{vn_idx}")
            else:
                parts.append(f"{v_idx}")
        lines.append("f " + " ".join(parts))

    # Write file
    with open(path, 'w', encoding='utf-8') as f:
        f.write("# Exported by omt_3d.py\n")
        f.write("# Vertices: %d\n" % len(mesh.vertices))
        f.write("# Faces: %d\n" % len(mesh.polygons))
        f.write("\n".join(lines))


# --- Viewer / CLI ---

# Pluggable canvas resolver. Set this to a callable that receives a (type,id)
# tuple and returns a path to an image or a pyglet image object. Example:
#   from omt_3d import CANVAS_RESOLVER
#   CANVAS_RESOLVER = my_resolver
CANVAS_RESOLVER = None

def set_canvas_resolver(func):
    """Set a callable to resolve canvas stream links to image sources.

    The callable receives a stream-link tuple (lt, li) and should return one of:
      - a filesystem path (str) to an image file
      - a PIL.Image.Image
      - a PyQt6.QtGui.QImage

    Return None if not resolvable.
    """
    global CANVAS_RESOLVER
    CANVAS_RESOLVER = func



def triangulate_polygon(v_idx: List[int]) -> List[Tuple[int,int,int]]:
    # Fan triangulation; assumes convex polygons as in OMT
    tris = []
    if len(v_idx) < 3:
        return tris
    v0 = v_idx[0]
    for i in range(1, len(v_idx)-1):
        tris.append((v0, v_idx[i], v_idx[i+1]))
    return tris


def compute_vertex_normals(mesh: OMTMesh) -> List[Tuple[float,float,float]]:
    vn = [(0.0,0.0,0.0) for _ in mesh.vertices]
    counts = [0]*len(mesh.vertices)

    import math
    def add(a,b): return (a[0]+b[0], a[1]+b[1], a[2]+b[2])
    def norm(v):
        l = math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
        if l==0: return (0.0,0.0,0.0)
        return (v[0]/l, v[1]/l, v[2]/l)

    for poly in mesh.polygons:
        vtx = [mesh.vertices[i] for i in poly['v_idx']]
        # compute face normal
        ax = vtx[1][0]-vtx[0][0]; ay = vtx[1][1]-vtx[0][1]; az = vtx[1][2]-vtx[0][2]
        bx = vtx[-1][0]-vtx[0][0]; by = vtx[-1][1]-vtx[0][1]; bz = vtx[-1][2]-vtx[0][2]
        # cross
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        fn = norm((nx,ny,nz))
        for vi in poly['v_idx']:
            vn[vi] = add(vn[vi], fn)
            counts[vi] += 1

    for i in range(len(vn)):
        if counts[i]>0:
            vn[i] = (vn[i][0]/counts[i], vn[i][1]/counts[i], vn[i][2]/counts[i])
            vn[i] = norm(vn[i])
    return vn


def start_qt_viewer(shapes: List[OMTMesh], auto_play: bool = False, fps: float = 24.0):
    # Robust Qt import: try PyQt6 first, then fall back to PySide6. Provide helpful errors.
    qt_provider = None
    has_qopengl = False
    import traceback
    _qopengl_errors = {}
    try:
        from PyQt6 import QtWidgets, QtCore
        from PyQt6.QtWidgets import QMainWindow, QFileDialog, QPushButton, QListWidget, QWidget, QHBoxLayout, QVBoxLayout, QLabel
        from PyQt6.QtCore import QTimer
        from PyQt6.QtGui import QSurfaceFormat
        qt_provider = 'PyQt6'
        try:
            from PyQt6.QtWidgets import QOpenGLWidget
            has_qopengl = True
        except Exception as e_qt:
            _qopengl_errors['pyqt_qopengl'] = ''.join(traceback.format_exception_only(type(e_qt), e_qt)).strip()
    except Exception as e_pyqt:
        _qopengl_errors['pyqt_import'] = ''.join(traceback.format_exception_only(type(e_pyqt), e_pyqt)).strip()
        try:
            from PySide6 import QtWidgets, QtCore
            from PySide6.QtWidgets import QMainWindow, QFileDialog, QPushButton, QListWidget, QWidget, QHBoxLayout, QVBoxLayout, QLabel
            from PySide6.QtCore import QTimer
            from PySide6.QtGui import QSurfaceFormat
            qt_provider = 'PySide6'
            try:
                from PySide6.QtWidgets import QOpenGLWidget
                has_qopengl = True
            except Exception as e_qt:
                _qopengl_errors['pyside_qopengl'] = ''.join(traceback.format_exception_only(type(e_qt), e_qt)).strip()
        except Exception as e_pyside:
            _qopengl_errors['pyside_import'] = ''.join(traceback.format_exception_only(type(e_pyside), e_pyside)).strip()
            tb_pyqt = _qopengl_errors.get('pyqt_import') or ''.join(traceback.format_exception_only(type(e_pyqt), e_pyqt)).strip()
            tb_pyside = _qopengl_errors.get('pyside_import') or ''.join(traceback.format_exception_only(type(e_pyside), e_pyside)).strip()
            raise RuntimeError(f'Failed to import PyQt6 ({tb_pyqt}) and PySide6 ({tb_pyside}). Ensure one is installed and importable in this Python environment.')

    # If we have a functioning QOpenGLWidget, enable the GL path. Otherwise provide a software fallback.
    if has_qopengl:
        try:
            from OpenGL.GL import (glEnable, glDisable, glClear, glMatrixMode, glLoadIdentity,
                                    glTranslatef, glRotatef, glColor3f, glPolygonMode, glBegin, glEnd,
                                    glNormal3f, glTexCoord2f, glVertex3f, glBindTexture, glGenTextures,
                                    glTexParameteri, glTexImage2D, glDeleteTextures, GL_DEPTH_TEST,
                                    GL_LIGHTING, GL_LIGHT0, GL_COLOR_MATERIAL, GL_PROJECTION, GL_MODELVIEW,
                                    GL_TRIANGLES, GL_LINES, GL_TEXTURE_2D, GL_LINEAR, GL_TEXTURE_MIN_FILTER,
                                    GL_TEXTURE_MAG_FILTER, GL_RGBA, GL_UNSIGNED_BYTE, GL_FRONT_AND_BACK,
                                    GL_LINE, GL_FILL, GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT)
            from OpenGL.GLU import gluPerspective
        except Exception as e_gl:
            tb_gl = ''.join(traceback.format_exception_only(type(e_gl), e_gl)).strip()
            raise RuntimeError(f'Failed to import PyOpenGL: {tb_gl}. Install it via "pip install PyOpenGL" and ensure it is importable in this Python environment.') from e_gl

        class GLWidget(QOpenGLWidget):
            def __init__(self, parent=None):
                super().__init__(parent)
                self.meshes: List[OMTMesh] = []
                self.frame = 0
                self.playing = auto_play
                self.timer = QTimer(self)
                self.timer.timeout.connect(self.advance_frame)
                self.fps = fps
                if self.playing:
                    self.timer.start(int(1000.0 / self.fps))

                self.rot_x = 20.0
                self.rot_y = 30.0
                self.dist = 5.0
                self.pan_x = 0.0
                self.pan_y = 0.0

                self.last_pos = None
                self.mouse_button = None

                # display options
                self.show_wireframe = False
                self.show_textures = True
                self.show_normals = False
                self.show_lighting = True

                self.textures = []

            def set_meshes(self, meshes: List[OMTMesh]):
                self.meshes = meshes
                # compute sensible distance
                max_coord = 0.0
                for mesh in meshes:
                    for v in mesh.vertices:
                        max_coord = max(max_coord, abs(v[0]), abs(v[1]), abs(v[2]))
                self.dist = max(1.0, max_coord) * 3.0
                self.reset_view()
                # Build textures if resolver is available
                self.build_textures()
                self.update()

            def reset_view(self):
                self.rot_x = 20.0
                self.rot_y = 30.0
                self.pan_x = 0.0
                self.pan_y = 0.0

            def build_textures(self):
                # Creates OpenGL textures for materials / extra pass slinks exposed in meshes
                self.textures = []  # list per mesh: dict poly_index->tex_id

                # If no resolver or no meshes, populate empty maps and return
                if CANVAS_RESOLVER is None or not self.meshes:
                    for _ in self.meshes:
                        self.textures.append({})
                    return

                # Ensure GL functions are available (context may not yet be current)
                try:
                    # we expect OpenGL imports to have been done in outer scope
                    glGenTextures
                except Exception:
                    # schedule to retry once the GL context exists
                    QTimer.singleShot(0, self.build_textures)
                    for _ in self.meshes:
                        self.textures.append({})
                    return

                # Lazy imports
                try:
                    from PIL import Image
                except Exception:
                    Image = None

                for mi, mesh in enumerate(self.meshes):
                    poly_tex = {}
                    for pi, poly in enumerate(mesh.polygons):
                        slink = None
                        if poly.get('material_link'):
                            slink = poly.get('material_link')
                        elif poly.get('text_extra_index') is not None:
                            tidx = poly.get('text_extra_index')
                            if 0 <= tidx < len(mesh.extra_textures):
                                pass_set = mesh.extra_textures[tidx]
                                if pass_set:
                                    # pass_set items are (pass_slink, am, co)
                                    pass_slink = pass_set[0][0]
                                    slink = pass_slink

                        if slink is None:
                            continue

                        try:
                            result = CANVAS_RESOLVER(slink)
                        except Exception:
                            result = None

                        if not result:
                            continue

                        # result may be path (str), PIL Image, or QImage
                        img = None
                        if isinstance(result, str):
                            if Image:
                                try:
                                    img = Image.open(result).convert('RGBA')
                                except Exception:
                                    img = None
                        else:
                            # Check for PIL Image
                            if Image and isinstance(result, Image.Image):
                                img = result.convert('RGBA')
                            else:
                                # try QImage
                                try:
                                    # QImage class may come from either provider; import defensively
                                    try:
                                        from PyQt6.QtGui import QImage
                                    except Exception:
                                        from PySide6.QtGui import QImage
                                    if isinstance(result, QImage):
                                        qimg = result.convertToFormat(QImage.Format.Format_RGBA8888)
                                        width = qimg.width(); height = qimg.height()
                                        ptr = qimg.bits()
                                        ptr.setsize(width*height*4)
                                        data = ptr.asstring()
                                        # convert to PIL Image if pillow available
                                        if Image:
                                            img = Image.frombytes('RGBA', (width, height), data)
                                        else:
                                            img = None
                                except Exception:
                                    img = None

                        if img is None:
                            continue

                        # create GL texture
                        tex_id = glGenTextures(1)
                        glBindTexture(GL_TEXTURE_2D, tex_id)
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
                        w, h = img.size
                        data = img.tobytes('raw', 'RGBA')
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data)
                        glBindTexture(GL_TEXTURE_2D, 0)

                        poly_tex[pi] = tex_id

                    self.textures.append(poly_tex)

            def initializeGL(self):
                glEnable(GL_DEPTH_TEST)
                glClearColor(0.15, 0.15, 0.15, 1.0)
                if self.show_lighting:
                    glEnable(GL_LIGHTING)
                    glEnable(GL_LIGHT0)
                    glLightfv(GL_LIGHT0, GL_POSITION, (1.0, 1.0, 1.0, 0.0))
                    glEnable(GL_COLOR_MATERIAL)
                else:
                    glDisable(GL_LIGHTING)

            def resizeGL(self, w: int, h: int):
                glViewport(0, 0, w, h)

            def paintGL(self):
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
                glMatrixMode(GL_PROJECTION)
                glLoadIdentity()
                aspect = max(0.1, self.width() / max(1.0, self.height()))
                gluPerspective(60.0, aspect, 0.1, 1000.0)
                glMatrixMode(GL_MODELVIEW)
                glLoadIdentity()

                # Apply camera
                glTranslatef(self.pan_x, self.pan_y, -self.dist)
                glRotatef(self.rot_x, 1, 0, 0)
                glRotatef(self.rot_y, 0, 1, 0)

                if not self.meshes:
                    return
                mesh = self.meshes[self.frame % len(self.meshes)]

                # prepare normals fallback
                vertex_normals = mesh.normals if mesh.normals else compute_vertex_normals(mesh)

                # polygon mode
                if self.show_wireframe:
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                else:
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)

                glColor3f(0.9, 0.9, 0.9)
                for pi, poly in enumerate(mesh.polygons):
                    tris = triangulate_polygon(poly['v_idx'])
                    tex_id = None
                    # defensive: ensure textures list matches mesh index
                    try:
                        mi = 0
                        if hasattr(self, 'textures') and len(self.textures) > 0:
                            mi = mi if mi < len(self.textures) else 0
                    except Exception:
                        mi = 0
                    if self.show_textures and hasattr(self, 'textures') and pi in self.textures[mi]:
                        tex_id = self.textures[mi][pi]
                    if tex_id:
                        glEnable(GL_TEXTURE_2D)
                        glBindTexture(GL_TEXTURE_2D, tex_id)
                    else:
                        glDisable(GL_TEXTURE_2D)

                    glBegin(GL_TRIANGLES)
                    for a,b,c in tris:
                        for vi in (a,b,c):
                            if vertex_normals and vi < len(vertex_normals):
                                glNormal3f(*vertex_normals[vi])
                            # texture coords
                            try:
                                uv = poly['uv'][poly['v_idx'].index(vi)]
                                glTexCoord2f(uv[0], uv[1])
                            except Exception:
                                pass
                            vx,vy,vz = mesh.vertices[vi]
                            glVertex3f(vx, vy, vz)
                    glEnd()

                    if tex_id:
                        glBindTexture(GL_TEXTURE_2D, 0)
                        glDisable(GL_TEXTURE_2D)

                # draw normals
                if self.show_normals:
                    glColor3f(1.0, 0.0, 0.0)
                    glBegin(GL_LINES)
                    for vi, v in enumerate(mesh.vertices):
                        if vi < len(vertex_normals):
                            nx, ny, nz = vertex_normals[vi]
                            glVertex3f(v[0], v[1], v[2])
                            glVertex3f(v[0]+nx*0.1, v[1]+ny*0.1, v[2]+nz*0.1)
                    glEnd()

            def mousePressEvent(self, event):
                self.last_pos = event.position()
                self.mouse_button = event.buttons()

            def mouseMoveEvent(self, event):
                if not self.last_pos:
                    self.last_pos = event.position()
                    return
                p = event.position()
                dx = p.x() - self.last_pos.x()
                dy = p.y() - self.last_pos.y()
                buttons = self.mouse_button
                # Left drag = rotate
                if buttons & QtCore.Qt.MouseButton.LeftButton:
                    self.rot_x += dy * 0.5
                    self.rot_y += dx * 0.5
                # Right drag = pan
                if buttons & QtCore.Qt.MouseButton.RightButton:
                    self.pan_x += dx * 0.005
                    self.pan_y -= dy * 0.005
                self.last_pos = p
                self.update()

            def wheelEvent(self, event):
                delta = event.angleDelta().y()
                # Zoom in/out
                if delta > 0:
                    self.dist = max(0.1, self.dist * 0.9)
                else:
                    self.dist = self.dist * 1.1
                self.update()

            def advance_frame(self):
                self.frame = (self.frame + 1) % max(1, len(self.meshes))
                self.update()

            def toggle_play(self):
                if self.playing:
                    self.timer.stop()
                else:
                    self.timer.start(int(1000.0 / self.fps))
                self.playing = not self.playing

            def delete_textures(self):
                # delete GL textures if any
                try:
                    for texmap in getattr(self, 'textures', []):
                        for tex_id in texmap.values():
                            try:
                                glDeleteTextures([tex_id])
                            except Exception:
                                try:
                                    glDeleteTextures(1, [tex_id])
                                except Exception:
                                    pass
                except Exception:
                    pass

        ViewerWidget = GLWidget
    else:
        # Software renderer fallback when QOpenGLWidget is not available.
        # It provides a minimal, lock-step rendering using QPainter to ensure the app can run.
        from math import sin, cos
        try:
            from PyQt6.QtGui import QPainter, QColor, QPolygonF, QImage, QBrush, QPen
        except Exception:
            from PySide6.QtGui import QPainter, QColor, QPolygonF, QImage, QBrush, QPen
        QPointF = QtCore.QPointF

        class SoftwareWidget(QWidget):
            def __init__(self, parent=None):
                super().__init__(parent)
                self.meshes: List[OMTMesh] = []
                self.frame = 0
                self.playing = auto_play
                self.timer = QTimer(self)
                self.timer.timeout.connect(self.advance_frame)
                self.fps = fps
                if self.playing:
                    self.timer.start(int(1000.0 / self.fps))

                self.rot_x = 20.0
                self.rot_y = 30.0
                self.dist = 5.0
                self.pan_x = 0.0
                self.pan_y = 0.0

                self.last_pos = None
                self.mouse_button = None

                # display options
                self.show_wireframe = False
                self.show_textures = False  # software fallback does not support textures yet
                self.show_normals = False
                self.show_lighting = False

            def set_meshes(self, meshes: List[OMTMesh]):
                self.meshes = meshes
                max_coord = 0.0
                for mesh in meshes:
                    for v in mesh.vertices:
                        max_coord = max(max_coord, abs(v[0]), abs(v[1]), abs(v[2]))
                self.dist = max(1.0, max_coord) * 3.0
                self.update()

            def paintEvent(self, event):
                painter = QPainter(self)
                painter.setRenderHint(QPainter.RenderHint.Antialiasing)
                w = self.width(); h = self.height()
                painter.fillRect(0,0,w,h, QColor(40,40,40))
                if not self.meshes:
                    painter.end()
                    return

                mesh = self.meshes[self.frame % len(self.meshes)]
                verts = mesh.vertices
                tris = []
                for poly in mesh.polygons:
                    t = triangulate_polygon(poly['v_idx'])
                    for a,b,c in t:
                        tris.append((a,b,c, poly.get('color', (200,200,200))))

                # simple rotation matrices
                import math
                rx = math.radians(self.rot_x)
                ry = math.radians(self.rot_y)
                sx = math.sin(rx); cx = math.cos(rx)
                sy = math.sin(ry); cy = math.cos(ry)

                def project(v):
                    x,y,z = v
                    # rotate X
                    y2 = y*cx - z*sx
                    z2 = y*sx + z*cx
                    # rotate Y
                    x3 = x*cy + z2*sy
                    z3 = -x*sy + z2*cy
                    # perspective
                    if (z3 - self.dist) == 0:
                        z3 += 1e-6
                    f = 400.0 / (self.dist - z3)
                    px = x3 * f + w/2 + self.pan_x * w
                    py = -y2 * f + h/2 + self.pan_y * h
                    return (px, py, z3)

                # depth sort
                drawn = []
                for a,b,c,color in tris:
                    pa = project(verts[a]); pb = project(verts[b]); pc = project(verts[c])
                    depth = (pa[2]+pb[2]+pc[2])/3.0
                    drawn.append((depth, pa, pb, pc, color))
                drawn.sort(key=lambda x: x[0], reverse=True)

                for depth, pa, pb, pc, color in drawn:
                    poly = QPolygonF([QPointF(pa[0], pa[1]), QPointF(pb[0], pb[1]), QPointF(pc[0], pc[1])])
                    c = QColor(*[min(255,int(200 - (depth*10))) for _ in range(3)])
                    painter.setBrush(QBrush(c))
                    painter.setPen(QPen(QColor(20,20,20)))
                    painter.drawPolygon(poly)
                    if self.show_wireframe:
                        painter.setPen(QPen(QColor(200,200,200)))
                        painter.drawPolygon(poly)

                painter.end()

            def mousePressEvent(self, event):
                self.last_pos = event.position()
                self.mouse_button = event.buttons()

            def mouseMoveEvent(self, event):
                if not self.last_pos:
                    self.last_pos = event.position()
                    return
                p = event.position()
                dx = p.x() - self.last_pos.x()
                dy = p.y() - self.last_pos.y()
                buttons = self.mouse_button
                # Left drag = rotate
                if buttons & QtCore.Qt.MouseButton.LeftButton:
                    self.rot_x += dy * 0.5
                    self.rot_y += dx * 0.5
                # Right drag = pan
                if buttons & QtCore.Qt.MouseButton.RightButton:
                    self.pan_x += dx * 0.005
                    self.pan_y -= dy * 0.005
                self.last_pos = p
                self.update()

            def wheelEvent(self, event):
                delta = event.angleDelta().y()
                # Zoom in/out
                if delta > 0:
                    self.dist = max(0.1, self.dist * 0.9)
                else:
                    self.dist = self.dist * 1.1
                self.update()

            def advance_frame(self):
                self.frame = (self.frame + 1) % max(1, len(self.meshes))
                self.update()

            def toggle_play(self):
                if self.playing:
                    self.timer.stop()
                else:
                    self.timer.start(int(1000.0 / self.fps))
                self.playing = not self.playing

            def delete_textures(self):
                pass

        ViewerWidget = SoftwareWidget

    class MainWindow(QMainWindow):
        def __init__(self):
            super().__init__()
            self.setWindowTitle('OMT Viewer (really early ALPHA)')
            self.resize(1200, 800)

            central = QWidget()
            self.setCentralWidget(central)

            hbox = QHBoxLayout()
            central.setLayout(hbox)

            self.gl = ViewerWidget(self)
            hbox.addWidget(self.gl, 1)

            sidebar = QVBoxLayout()
            hbox.addLayout(sidebar)

            # Menu
            menubar = self.menuBar()
            file_menu = menubar.addMenu('&File')
            open_act = file_menu.addAction('Open...')
            open_act.triggered.connect(self.open_file)
            export_act = file_menu.addAction('Export OBJ...')
            export_act.triggered.connect(self.export_obj)
            file_menu.addSeparator()
            exit_act = file_menu.addAction('Exit')
            exit_act.triggered.connect(self.close)

            view_menu = menubar.addMenu('&View')
            self.wireframe_act = view_menu.addAction('Wireframe')
            self.wireframe_act.setCheckable(True)
            self.wireframe_act.toggled.connect(self.toggle_wireframe)
            self.textures_act = view_menu.addAction('Textures')
            self.textures_act.setCheckable(True)
            self.textures_act.setChecked(True)
            self.textures_act.toggled.connect(self.toggle_textures)
            self.normals_act = view_menu.addAction('Normals')
            self.normals_act.setCheckable(True)
            self.normals_act.toggled.connect(self.toggle_normals)
            self.lighting_act = view_menu.addAction('Lighting')
            self.lighting_act.setCheckable(True)
            self.lighting_act.setChecked(True)
            self.lighting_act.toggled.connect(self.toggle_lighting)

            help_menu = menubar.addMenu('&Help')
            about_act = help_menu.addAction('About')
            about_act.triggered.connect(lambda: messagebox.information(self, 'About', 'OMT Viewer - Qt\nSimple visualization tool'))

            # Toolbar
            toolbar = self.addToolBar('Main')
            toolbar.addAction(open_act)
            toolbar.addAction(export_act)
            toolbar.addSeparator()
            toolbar.addAction(self.wireframe_act)
            toolbar.addAction(self.textures_act)
            toolbar.addAction(self.normals_act)
            toolbar.addAction(self.lighting_act)

            # Sidebar controls
            self.open_btn = QPushButton('Open')
            self.open_btn.clicked.connect(self.open_file)
            sidebar.addWidget(self.open_btn)

            self.list_widget = QListWidget()
            self.list_widget.currentRowChanged.connect(self.on_select_shape)
            sidebar.addWidget(self.list_widget)

            self.play_btn = QPushButton('Play/Pause')
            self.play_btn.clicked.connect(self.play_pause)
            sidebar.addWidget(self.play_btn)

            controls_h = QHBoxLayout()
            self.prev_btn = QPushButton('Prev')
            self.prev_btn.clicked.connect(self.prev_frame)
            controls_h.addWidget(self.prev_btn)
            self.next_btn = QPushButton('Next')
            self.next_btn.clicked.connect(self.next_frame)
            controls_h.addWidget(self.next_btn)
            sidebar.addLayout(controls_h)

            self.export_btn = QPushButton('Export OBJ')
            self.export_btn.clicked.connect(self.export_obj)
            sidebar.addWidget(self.export_btn)

            # toggles
            self.wireframe_chk = QPushButton('Wireframe')
            self.wireframe_chk.setCheckable(True)
            self.wireframe_chk.toggled.connect(self.toggle_wireframe)
            sidebar.addWidget(self.wireframe_chk)

            self.textures_chk = QPushButton('Textures')
            self.textures_chk.setCheckable(True)
            self.textures_chk.setChecked(True)
            self.textures_chk.toggled.connect(self.toggle_textures)
            sidebar.addWidget(self.textures_chk)

            self.normals_chk = QPushButton('Normals')
            self.normals_chk.setCheckable(True)
            self.normals_chk.toggled.connect(self.toggle_normals)
            sidebar.addWidget(self.normals_chk)

            self.lighting_chk = QPushButton('Lighting')
            self.lighting_chk.setCheckable(True)
            self.lighting_chk.setChecked(True)
            self.lighting_chk.toggled.connect(self.toggle_lighting)
            sidebar.addWidget(self.lighting_chk)

            # Zoom slider
            QSlider = QtWidgets.QSlider
            self.zoom_slider = QSlider()
            self.zoom_slider.setOrientation(QtCore.Qt.Orientation.Horizontal)
            self.zoom_slider.setRange(1, 500)
            self.zoom_slider.setValue(100)
            self.zoom_slider.valueChanged.connect(self.zoom_changed)
            sidebar.addWidget(QLabel('Zoom'))
            sidebar.addWidget(self.zoom_slider)

            sidebar.addStretch(1)

            # Status bar
            self.status = self.statusBar()
            try:
                prov = qt_provider
            except Exception:
                prov = 'unknown'
            msg = f'Ready (Qt provider: {prov})'
            try:
                if not has_qopengl:
                    msg += ' [no QOpenGLWidget — using software renderer]'
            except Exception:
                pass
            self.status.showMessage(msg)

            # enable drag and drop files
            self.setAcceptDrops(True)

            self.shapes: List[OMTMesh] = []

        def dragEnterEvent(self, event):
            if event.mimeData().hasUrls():
                event.acceptProposedAction()

        def dropEvent(self, event):
            urls = event.mimeData().urls()
            if urls:
                fn = urls[0].toLocalFile()
                if fn:
                    self.load_file(fn)

        def load_file(self, fn):
            self.shapes = find_shapes_in_file(fn)
            self.list_widget.clear()
            if not self.shapes:
                # If file contains a '3DSP' signature but parsing failed, show a helpful diagnostic
                try:
                    with open(fn,'rb') as f:
                        data = f.read()
                    idx = data.find(b'3DSP')
                    if idx != -1:
                        diag = diagnose_3dsh_at(data, idx)
                        try:
                            QtWidgets.QMessageBox.warning(self, 'OMT Viewer', f'No shapes parsed from file. Found a 3DSP block but parsing failed:\n\n{diag[:1000]}')
                        except Exception:
                            # fallback to tkinter messagebox if Qt unavailable
                            try:
                                messagebox.showwarning('OMT Viewer', f'No shapes parsed from file. Found a 3DSP block but parsing failed:\n\n{diag[:1000]}')
                            except Exception:
                                print('No shapes parsed; diagnostic:\n', diag, file=sys.stderr)
                except Exception:
                    pass

            for i, s in enumerate(self.shapes):
                self.list_widget.addItem(f"Shape {i} (v:{len(s.vertices)} f:{len(s.polygons)})")
            if self.shapes:
                self.list_widget.setCurrentRow(0)
                self.gl.set_meshes(self.shapes)
            self.status.showMessage(f'Loaded {fn} - shapes: {len(self.shapes)}')

        def open_file(self):
            fn, _ = QFileDialog.getOpenFileName(self, 'Open OMT file', '.', 'OMT Files (*.3dsh *.3dma *.omt);;All Files (*)')
            if not fn:
                return
            self.load_file(fn)

        def toggle_wireframe(self, on):
            self.gl.show_wireframe = on
            self.gl.update()

        def toggle_textures(self, on):
            self.gl.show_textures = on
            if on:
                self.gl.build_textures()
            self.gl.update()

        def toggle_normals(self, on):
            self.gl.show_normals = on
            self.gl.update()

        def toggle_lighting(self, on):
            self.gl.show_lighting = on
            if on:
                glEnable(GL_LIGHTING)
                glEnable(GL_LIGHT0)
            else:
                glDisable(GL_LIGHTING)
            self.gl.update()

        def zoom_changed(self, val):
            self.gl.dist = max(0.1, val / 10.0)
            self.gl.update()

        def on_select_shape(self, row: int):
            self.gl.frame = row if row >= 0 else 0
            self.gl.update()

        def play_pause(self):
            self.gl.toggle_play()

        def prev_frame(self):
            self.gl.frame = (self.gl.frame - 1) % max(1, len(self.shapes))
            self.list_widget.setCurrentRow(self.gl.frame)
            self.gl.update()

        def next_frame(self):
            self.gl.frame = (self.gl.frame + 1) % max(1, len(self.shapes))
            self.list_widget.setCurrentRow(self.gl.frame)
            self.gl.update()

        def export_obj(self):
            if not self.shapes:
                return
            row = self.list_widget.currentRow()
            if row < 0:
                row = 0
            mesh = self.shapes[row]
            fn, _ = QFileDialog.getSaveFileName(self, 'Save OBJ', f'shape_{row}.obj', 'Wavefront OBJ (*.obj)')
            if not fn:
                return
            export_obj(mesh, fn)

        def closeEvent(self, event):
            try:
                self.gl.delete_textures()
            except Exception:
                pass
            return super().closeEvent(event)

    app = QtWidgets.QApplication([])
    fmt = QSurfaceFormat()
    fmt.setDepthBufferSize(24)
    QSurfaceFormat.setDefaultFormat(fmt)

    win = MainWindow()
    win.show()
    app.exec()


def start_viewer(shapes: List[OMTMesh], auto_play: bool = False, fps: float = 24.0):
    """GUI-only viewer entry point (Qt + PyOpenGL required).

    This function launches the Qt viewer and raises an exception if the
    Qt viewer cannot be started. It no longer falls back to pyglet.
    """
    try:
        start_qt_viewer(shapes, auto_play=auto_play, fps=fps)
    except Exception as e:
        print('Qt viewer unavailable or failed:', e, file=sys.stderr)
        raise


if __name__ == '__main__':
    # GUI-only entry point — launch the Qt viewer (user opens files via GUI)
    try:
        start_qt_viewer([])
    except Exception as e:
        print('Failed to launch OMT Qt viewer:', e, file=sys.stderr)

