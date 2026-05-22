#!/usr/bin/env python3
r"""
diff.py -- M6 original-vs-demo derived-fact comparison.

Consumes two OMTC v1 captures (instrument/proxy/protocol.h wire format):

  * the ORIGINAL Jimmy Neutron game, captured by the ddraw proxy
    (instrument/receiver/receive.py serve -> .omtc).  D3D7: left-handed,
    row-vector transforms.
  * the DEMO (jn-engine), captured by src/engine/capture.c under -DJN_CAPTURE.
    GL: right-handed, column-vector transforms; capture.c transposes its
    matrices into the D3D row-major *layout*, but the *convention* is still GL.

The two engines batch geometry completely differently, so raw draw-call
diffing is noise.  diff.py instead reduces each capture to *derived facts*
and compares those:

  1. Camera      -- VIEW+PROJ -> eye position, look direction, FOV.
  2. Object set  -- WORLD * vertex AABB -> world-space point cloud; the cloud
                    is re-tested under X/Y/Z negation, which answers the
                    suspected level-mirroring directly.
  3. Textures    -- which textures are bound by 3D geometry (presence diff);
                    flags the missing ground texture / water canvases.
  4. Render state-- ambient colour, lighting enable, fog, lights, material;
                    feeds the "too dark" lighting gap with numbers.
  5. Terrain     -- large low-Y primitives (ground / topography); their count,
                    footprint and Y-spread feed the terrain-topography gap.

The capture convention is auto-detected: a demo capture has a `<path>.tex`
name sidecar beside it (capture.c writes one); the original does not.
Override with --orig-conv / --demo-conv if needed.

Usage:
  diff.py <orig.omtc> <demo.omtc> [--orig-frame N] [--demo-frame N]
  diff.py --camera <capture.omtc> [--frame N]      # dump a frame's camera
                                                   # (for the matched-camera
                                                   #  capture workflow)

If a frame is not given, the frame with the most 3D (perspective) draws is
used.  Coordinates are compared in the shared OMT world space -- both engines
load the same level1.omt, so X/Y/Z are nominally aligned and any mismatch is
itself a finding.
"""

import os
import sys
import math
import struct

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "receiver"))
from protocol import *  # noqa: E402,F403


# ===========================================================================
# D3D render-state IDs we care about (subset of D3DRENDERSTATETYPE)
# ===========================================================================
D3DRS_ZENABLE        = 7
D3DRS_FOGENABLE      = 28
D3DRS_SPECULARENABLE = 29
D3DRS_FOGCOLOR       = 34
D3DRS_FOGTABLEMODE   = 35
D3DRS_LIGHTING       = 137
D3DRS_AMBIENT        = 139
D3DRS_COLORVERTEX    = 141

RS_NAMES = {
    D3DRS_ZENABLE: "ZENABLE", D3DRS_FOGENABLE: "FOGENABLE",
    D3DRS_SPECULARENABLE: "SPECULARENABLE", D3DRS_FOGCOLOR: "FOGCOLOR",
    D3DRS_FOGTABLEMODE: "FOGTABLEMODE", D3DRS_LIGHTING: "LIGHTING",
    D3DRS_AMBIENT: "AMBIENT", D3DRS_COLORVERTEX: "COLORVERTEX",
}


def argb(v):
    """Unpack a D3DCOLOR (0xAARRGGBB) into an (a,r,g,b) 0..255 tuple."""
    return ((v >> 24) & 0xff, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff)


# ===========================================================================
# Incremental record reader -- never holds the whole file (m5_session is 621MB)
# ===========================================================================
def iter_records(path, chunk=4 << 20):
    """Yield ('header', Header) then ('rec', type, payload) for every record."""
    with open(path, "rb") as f:
        head = f.read(Header.size())
        if len(head) < Header.size():
            raise ValueError(f"{path}: too short to hold a stream header")
        hdr = Header.unpack(head)
        if hdr.magic != OMTC_MAGIC:
            raise ValueError(f"{path}: bad magic {hdr.magic:08x}")
        if not (OMTC_VERSION_MIN <= hdr.version <= OMTC_VERSION):
            raise ValueError(
                f"{path}: version {hdr.version} outside accepted range "
                f"[{OMTC_VERSION_MIN}, {OMTC_VERSION}]")
        yield ("header", hdr)
        buf = bytearray()
        while True:
            data = f.read(chunk)
            if not data:
                break
            buf += data
            off = 0
            while True:
                rec = try_parse_record(buf, off)
                if rec is None:
                    break
                rt, payload, off = rec
                yield ("rec", rt, payload)
            if off:
                del buf[:off]


# ===========================================================================
# Projection / transform helpers
# ===========================================================================
def is_perspective(proj):
    """A projection is perspective iff clip.w depends on the input position,
    i.e. it has non-zero w-coupling terms.  Works for both the D3D row-vector
    layout (d[3],d[7],d[11] non-zero) and the GL column-vector layout
    (d[12],d[13],d[14] non-zero).  Orthographic projections have all six zero."""
    if proj is None:
        return False
    return max(abs(proj[3]), abs(proj[7]), abs(proj[11]),
               abs(proj[12]), abs(proj[13]), abs(proj[14])) > 1e-4


def fov_y(proj):
    """Vertical FOV in degrees.  proj[5] = cot(fovy/2) in row-major layout for
    both conventions (it is the Y scale on the matrix diagonal)."""
    if proj is None or abs(proj[5]) < 1e-9:
        return None
    return math.degrees(2.0 * math.atan(1.0 / abs(proj[5])))


def _m3(d, r, c):
    return d[r * 4 + c]


def decompose_camera(view, conv):
    """VIEW -> (eye xyz, look-direction unit xyz).  Convention-aware.

    GL  : column-vector view, p_eye = V . p_world.  Translation is column 3;
          eye = -R^T . t ; camera looks down -Z in eye space.
    D3D : row-vector view, p_eye = p_world . V.  Translation is row 3;
          eye = -M3 . t_row ; camera looks down +Z in eye space (left-handed).
    """
    if view is None:
        return None, None
    if conv == "gl":
        t = (view[3], view[7], view[11])                 # column 3
        # eye = -R^T t  ;  R[i][j] = _m3(view,i,j)
        eye = tuple(-(sum(_m3(view, i, k) * t[i] for i in range(3)))
                    for k in range(3))
        look = tuple(-_m3(view, 2, k) for k in range(3))  # -row 2
    else:  # d3d
        t = (view[12], view[13], view[14])               # row 3
        # eye = -M3 . t_row
        eye = tuple(-(sum(_m3(view, k, i) * t[i] for i in range(3)))
                    for k in range(3))
        look = tuple(_m3(view, 2, k) for k in range(3))   # +row 2
    n = math.sqrt(sum(c * c for c in look)) or 1.0
    look = tuple(c / n for c in look)
    return eye, look


IDENTITY4 = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]


def mat4_mul(a, b):
    """4x4 * 4x4, both row-major flat: C(r,c) = sum_k A(r,k) B(k,c)."""
    c = [0.0] * 16
    for r in range(4):
        for col in range(4):
            c[r * 4 + col] = sum(a[r * 4 + k] * b[k * 4 + col]
                                 for k in range(4))
    return c


def xform_point(M, p, conv):
    """Apply a 4x4 transform (row-major flat) to a 3-point, w=1.

    GL  (column-vector): out = M . p   -> out[r] = sum_c M(r,c) p[c]
    D3D (row-vector)   : out = p . M   -> out[c] = sum_r p[r] M(r,c)
    """
    x, y, z = p
    if conv == "gl":
        return (
            M[0] * x + M[1] * y + M[2] * z + M[3],
            M[4] * x + M[5] * y + M[6] * z + M[7],
            M[8] * x + M[9] * y + M[10] * z + M[11],
        )
    return (
        M[0] * x + M[4] * y + M[8] * z + M[12],
        M[1] * x + M[5] * y + M[9] * z + M[13],
        M[2] * x + M[6] * y + M[10] * z + M[14],
    )


# ===========================================================================
# Frame extraction -- one pass, per-draw render state snapshotted
# ===========================================================================
class Draw:
    __slots__ = ("tex_id", "wmin", "wmax", "persp")

    def __init__(self, tex_id, wmin, wmax, persp):
        self.tex_id = tex_id
        self.wmin = wmin            # world-space AABB
        self.wmax = wmax
        self.persp = persp          # drawn under a perspective projection?

    @property
    def center(self):
        return tuple((self.wmin[i] + self.wmax[i]) * 0.5 for i in range(3))

    @property
    def footprint(self):            # XZ extent
        return ((self.wmax[0] - self.wmin[0]),
                (self.wmax[2] - self.wmin[2]))

    @property
    def y_extent(self):
        return self.wmax[1] - self.wmin[1]


class FrameData:
    def __init__(self, index):
        self.index = index
        self.draws = []
        self.view = None
        self.proj = None
        self.render_states = {}
        self.lights = {}
        self.material = None
        self.textures = {}          # tex_id -> TextureDef
        self.view_baked = False     # True if the engine never emits VIEW


def scan_frame_draw_counts(path):
    """Light first pass: per-frame count of perspective draws (no vertices)."""
    counts = []
    proj = None
    persp_draws = 0
    idx = -1
    for item in iter_records(path):
        if item[0] == "header":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            idx += 1
            persp_draws = 0
        elif rt == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            if st.which == TRANSFORM_PROJ:
                proj = st.matrix
        elif rt == RECORD_DRAW_PRIMITIVE:
            if is_perspective(proj):
                persp_draws += 1
        elif rt == RECORD_FRAME_END:
            counts.append((idx, persp_draws))
    return counts


def extract_frame(path, want_index, conv):
    """Second pass: fully decode frame `want_index` into a FrameData.

    D3D render state is *sticky* -- the game re-emits SetTransform/SetRenderState
    /SetLight only when something changes, often once at level load.  So all
    that state is tracked across every frame and the running value is what is
    snapshotted for the target frame; only DRAW_PRIMITIVE is frame-gated."""
    fd = FrameData(want_index)
    world = None
    view = None
    proj = None
    cur_tex = 0
    render_states = {}
    lights = {}
    material = None
    textures = {}
    cam_view = cam_proj = None      # state at the frame's first 3D draw
    idx = -1
    capturing = False
    for item in iter_records(path):
        if item[0] == "header":
            fd.header = item[1]
            continue
        rt, payload = item[1], item[2]

        if rt == RECORD_FRAME_BEGIN:
            idx += 1
            capturing = (idx == want_index)
            if idx > want_index:
                break
            continue

        # ---- sticky state: tracked on every frame --------------------------
        if rt == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            if st.which == TRANSFORM_WORLD:
                world = st.matrix
            elif st.which == TRANSFORM_VIEW:
                view = st.matrix
            elif st.which == TRANSFORM_PROJ:
                proj = st.matrix
        elif rt == RECORD_SET_TEXTURE:
            t = SetTexture.unpack(payload)
            if t.stage == 0:
                cur_tex = t.tex_id
        elif rt == RECORD_SET_RENDERSTATE:
            rs = SetRenderState.unpack(payload)
            render_states[rs.state] = rs.value
        elif rt == RECORD_SET_LIGHT:
            lt = SetLight.unpack(payload)
            lights[lt.index] = lt
        elif rt == RECORD_SET_MATERIAL:
            material = SetMaterial.unpack(payload)
        elif rt == RECORD_TEXTURE_DEF:
            td = TextureDef.unpack(payload)
            textures[td.tex_id] = td

        # ---- frame-gated: draws ------------------------------------------
        if capturing and rt == RECORD_DRAW_PRIMITIVE:
            dp = DrawPrimitive.unpack(payload)
            if not dp.vertices:
                continue
            persp = is_perspective(proj)
            if persp and cam_view is None:
                cam_view, cam_proj = view, proj
            # View-space AABB: OMT2 never emits a VIEW transform -- it bakes
            # the camera into every WORLD matrix -- so the original's WORLD
            # already maps model->view space. The demo emits a real VIEW, so
            # compose VIEW.WORLD. Both then land in camera-relative space, the
            # only space the two captures can be compared in.
            wm = world if world else IDENTITY4
            M = mat4_mul(view, wm) if view is not None else wm
            wmin = [1e30, 1e30, 1e30]
            wmax = [-1e30, -1e30, -1e30]
            for v in dp.vertices:
                wx, wy, wz = xform_point(M, (v.x, v.y, v.z), conv)
                for i, c in enumerate((wx, wy, wz)):
                    if c < wmin[i]:
                        wmin[i] = c
                    if c > wmax[i]:
                        wmax[i] = c
            fd.draws.append(Draw(cur_tex, tuple(wmin), tuple(wmax), persp))
        elif capturing and rt == RECORD_FRAME_END:
            break

    fd.view = cam_view if cam_view is not None else view
    fd.proj = cam_proj if cam_proj is not None else proj
    fd.view_baked = (view is None)      # OMT2 bakes the camera into WORLD
    fd.render_states = render_states
    fd.lights = lights
    fd.material = material
    fd.textures = textures

    # Normalize handedness: GL view space is right-handed (looks down -Z),
    # D3D view space is left-handed (looks down +Z). Negating Z alone turns the
    # GL frame left-handed but leaves the screen-X axis pointing opposite to
    # D3D's once the projections' X mappings are accounted for, so the full
    # GL->D3D view-space reconciliation is a 180-deg turn about Y: negate both
    # X and Z. Without the X half, a matched (un-mirrored) demo spuriously
    # favours a negate-X best-fit in section 2. The rigorous world-space
    # mirroring answer is extract_camera.py's object-anchored solve (NO mirror
    # at frame 6844); this normalization makes section 2 agree with it.
    if conv == "gl":
        for d in fd.draws:
            xmin, xmax = d.wmin[0], d.wmax[0]
            zmin, zmax = d.wmin[2], d.wmax[2]
            d.wmin = (-xmax, d.wmin[1], -zmax)
            d.wmax = (-xmin, d.wmax[1], -zmin)
    return fd


# ===========================================================================
# Texture naming
# ===========================================================================
def load_demo_tex_sidecar(omtc_path):
    """Demo: capture.c writes `<path>.tex` -- lines `<hex id> <w> <h> <name>`."""
    side = omtc_path + ".tex"
    names = {}
    if not os.path.exists(side):
        return names
    with open(side) as f:
        for line in f:
            parts = line.split(None, 3)
            if len(parts) == 4:
                names[int(parts[0], 16)] = parts[3].strip()
    return names


def tex_label(tex_id, frame, demo_names):
    if tex_id == 0:
        return None
    if tex_id in demo_names:
        return os.path.basename(demo_names[tex_id])
    td = frame.textures.get(tex_id)
    if td:
        return f"tex_{td.sha1.hex()[:8]}_{td.width}x{td.height}"
    return f"tex_{tex_id:08x}"


WATER_HINTS = ("water", "stream", "river", "wave", "liquid")
GROUND_HINTS = ("ground", "terrain", "grass", "dirt", "mud", "floor", "land")


# ===========================================================================
# Geometry analysis
# ===========================================================================
def world_bounds(draws):
    lo = [1e30, 1e30, 1e30]
    hi = [-1e30, -1e30, -1e30]
    for d in draws:
        for i in range(3):
            lo[i] = min(lo[i], d.wmin[i])
            hi[i] = max(hi[i], d.wmax[i])
    return tuple(lo), tuple(hi)


def classify_ground(draws):
    """Return the 3D draws that look like ground / terrain: a primitive is
    'flat horizontal' if its Y-extent is small relative to its XZ footprint.
    The original tiles its terrain into many such quads; the demo has one big
    flat ground quad.  We keep flat draws whose footprint is non-trivial
    (above a tenth of the largest flat footprint) so single stray triangles do
    not count."""
    persp = [d for d in draws if d.persp]
    flat = []
    for d in persp:
        fx, fz = d.footprint
        diag = math.hypot(fx, fz)
        if diag < 1.0:
            continue
        if d.y_extent < 0.20 * diag:        # shallow in Y vs its XZ size
            flat.append((d, fx * fz))
    if not flat:
        return []
    biggest = max(area for _, area in flat)
    return [d for d, area in flat if area >= 0.10 * biggest]


def nn_mean(cloud_a, cloud_b):
    """Mean nearest-neighbour distance from each point of A to cloud B."""
    if not cloud_a or not cloud_b:
        return float("inf")
    total = 0.0
    for pa in cloud_a:
        best = 1e30
        for pb in cloud_b:
            dx = pa[0] - pb[0]
            dy = pa[1] - pb[1]
            dz = pa[2] - pb[2]
            d = dx * dx + dy * dy + dz * dz
            if d < best:
                best = d
        total += math.sqrt(best)
    return total / len(cloud_a)


# ===========================================================================
# Report
# ===========================================================================
def hr(title):
    print("\n" + "=" * 72)
    print(title)
    print("=" * 72)


def fmt_vec(v, p=1):
    if v is None:
        return "(n/a)"
    return "(" + ", ".join(f"{c:.{p}f}" for c in v) + ")"


def report(orig, demo, orig_conv, demo_conv, demo_names):
    o_persp = [d for d in orig.draws if d.persp]
    d_persp = [d for d in demo.draws if d.persp]

    # ---- 1. CAMERA --------------------------------------------------------
    hr("1. CAMERA")
    o_fov = fov_y(orig.proj)
    d_fov = fov_y(demo.proj)
    if orig.view_baked:
        print("  original  VIEW transform never emitted -- OMT2 bakes the")
        print("            camera into every WORLD matrix. Eye/look are not")
        print(f"            separable from the capture; fovY={o_fov:.1f} deg"
              if o_fov else "            fovY=(n/a)")
    else:
        o_eye, o_look = decompose_camera(orig.view, orig_conv)
        print(f"  original  eye={fmt_vec(o_eye)}  look={fmt_vec(o_look, 3)}  "
              f"fovY={o_fov:.1f} deg" if o_fov else f"  original  eye=...")
    if demo.view_baked:
        print("  demo      VIEW transform never emitted.")
    else:
        d_eye, d_look = decompose_camera(demo.view, demo_conv)
        print(f"  demo      eye={fmt_vec(d_eye)}  look={fmt_vec(d_look, 3)}  "
              f"fovY={d_fov:.1f} deg" if d_fov else f"  demo      eye=...")
    if o_fov and d_fov:
        print(f"  fovY delta: {abs(o_fov - d_fov):.2f} deg")
    print("  NOTE: because the original bakes VIEW into WORLD, the only space")
    print("  the two captures share is camera-relative (view) space. Sections")
    print("  2 and 5 below work there; a camera-matched demo capture (see")
    print("  `diff.py --camera`) makes that comparison exact.")

    # ---- 2. OBJECT SET / MIRRORING ---------------------------------------
    hr("2. OBJECT SET  (camera-relative; mirroring test)")
    o_cloud = [d.center for d in o_persp]
    d_cloud = [d.center for d in d_persp]
    print(f"  original 3D draws: {len(o_cloud)}    demo 3D draws: {len(d_cloud)}")
    print("  (positions are view-space / camera-relative -- see section 1)")
    if o_cloud and d_cloud:
        o_lo, o_hi = world_bounds(o_persp)
        d_lo, d_hi = world_bounds(d_persp)
        print(f"  original view-space bounds: {fmt_vec(o_lo)} .. {fmt_vec(o_hi)}")
        print(f"  demo     view-space bounds: {fmt_vec(d_lo)} .. {fmt_vec(d_hi)}")
        base = nn_mean(d_cloud, o_cloud)
        variants = {"identity": d_cloud}
        for axis, name in ((0, "negate-X"), (1, "negate-Y"), (2, "negate-Z")):
            variants[name] = [tuple(-c if i == axis else c
                                    for i, c in enumerate(p)) for p in d_cloud]
        scores = {k: nn_mean(v, o_cloud) for k, v in variants.items()}
        print("  mean nearest-neighbour distance, demo cloud vs original cloud:")
        for k in ("identity", "negate-X", "negate-Y", "negate-Z"):
            print(f"    {k:10s}: {scores[k]:10.1f}")
        best = min(scores, key=scores.get)
        if best == "identity":
            print("  -> best fit is IDENTITY: no mirroring detected.")
        elif scores[best] < 0.6 * scores["identity"]:
            print(f"  -> best fit is {best}: LEVEL IS MIRRORED on that axis.")
        else:
            print(f"  -> {best} fits marginally better; inconclusive "
                  f"(cameras likely unmatched -- re-run camera-matched).")

    # ---- 3. TEXTURE SET ---------------------------------------------------
    hr("3. TEXTURE SET  (3D geometry only)")
    o_tex = {}
    for d in o_persp:
        if d.tex_id:
            o_tex.setdefault(d.tex_id, 0)
            o_tex[d.tex_id] += 1
    d_tex = {}
    for d in d_persp:
        if d.tex_id:
            d_tex.setdefault(d.tex_id, 0)
            d_tex[d.tex_id] += 1
    o_untex = sum(1 for d in o_persp if not d.tex_id)
    d_untex = sum(1 for d in d_persp if not d.tex_id)
    print(f"  original: {len(o_tex)} distinct textures bound, "
          f"{o_untex} untextured draws")
    print(f"  demo    : {len(d_tex)} distinct textures bound, "
          f"{d_untex} untextured draws")
    d_labels = sorted({tex_label(t, demo, demo_names) for t in d_tex})
    print(f"  demo textures ({len(d_labels)}):")
    for lab in d_labels:
        tag = ""
        ll = lab.lower()
        if any(h in ll for h in WATER_HINTS):
            tag = "  <- water"
        elif any(h in ll for h in GROUND_HINTS):
            tag = "  <- ground"
        print(f"    {lab}{tag}")
    o_named = sorted(lab for t in o_tex
                     if not (lab := tex_label(t, orig, {})).startswith("tex_"))
    if o_named:
        print(f"  original named textures: {o_named}")
    else:
        print("  original textures: all synthetic-named "
              "(no Pillow / PNG hash match -- M5 caveat).")
    d_has_water = any(any(h in (l or '').lower() for h in WATER_HINTS)
                      for l in d_labels)
    d_has_ground = any(any(h in (l or '').lower() for h in GROUND_HINTS)
                       for l in d_labels)
    print(f"  demo binds a water-type texture : {'yes' if d_has_water else 'NO'}")
    print(f"  demo binds a ground-type texture: {'yes' if d_has_ground else 'NO'}")

    # ---- 4. RENDER STATE / LIGHTING --------------------------------------
    hr("4. RENDER STATE / LIGHTING")
    for label, fr in (("original", orig), ("demo", demo)):
        rs = fr.render_states
        print(f"  {label}:")
        if not rs and label == "demo":
            print("    (demo capture emits no render states -- jn-engine drives"
                  " lighting in-shader)")
        amb = rs.get(D3DRS_AMBIENT)
        if amb is not None:
            a, r, g, b = argb(amb)
            print(f"    AMBIENT     = 0x{amb:08x}  rgb=({r},{g},{b})")
        lit = rs.get(D3DRS_LIGHTING)
        if lit is not None:
            print(f"    LIGHTING    = {'ON' if lit else 'OFF'}")
        fog = rs.get(D3DRS_FOGENABLE)
        if fog is not None:
            print(f"    FOGENABLE   = {'ON' if fog else 'OFF'}")
            fc = rs.get(D3DRS_FOGCOLOR)
            if fc is not None:
                _, r, g, b = argb(fc)
                print(f"    FOGCOLOR    = rgb=({r},{g},{b})")
        print(f"    lights set  = {len(fr.lights)}")
        for idx, lt in sorted(fr.lights.items()):
            lt_name = {1: "POINT", 2: "SPOT", 3: "DIR"}.get(lt.ltype, lt.ltype)
            print(f"      [{idx}] {lt_name} "
                  f"diffuse={tuple(round(c,2) for c in lt.diffuse)} "
                  f"dir={tuple(round(c,2) for c in lt.direction)}")
        if fr.material:
            m = fr.material
            print(f"    material    diffuse="
                  f"{tuple(round(c,2) for c in m.diffuse)} "
                  f"ambient={tuple(round(c,2) for c in m.ambient)}")
    print("  -> lighting gap: compare original AMBIENT/lights above against")
    print("     jn-engine's fixed in-shader light (renderer.c: directional")
    print("     0.577,0.577,0.577). A low original AMBIENT with few/no lights")
    print("     quantifies how much the demo over-brightens or under-lights.")

    # ---- 5. TERRAIN / GROUND GEOMETRY ------------------------------------
    hr("5. TERRAIN / GROUND GEOMETRY")
    print("  'ground-class' = a 3D primitive flat in Y relative to its XZ")
    print("  footprint. Measured in camera space, so this is most reliable")
    print("  with a near-level, camera-matched capture.")
    o_ground = classify_ground(orig.draws)
    d_ground = classify_ground(demo.draws)
    for label, g in (("original", o_ground), ("demo", d_ground)):
        print(f"  {label}: {len(g)} ground-class primitive(s)")
        if g:
            ys = [d.center[1] for d in g]
            ymin = min(d.wmin[1] for d in g)
            ymax = max(d.wmax[1] for d in g)
            textured = sum(1 for d in g if d.tex_id)
            print(f"    Y-spread of ground pieces: {ymin:.1f} .. {ymax:.1f} "
                  f"(span {ymax - ymin:.1f})")
            print(f"    textured ground pieces   : {textured}/{len(g)}")
            big = max(g, key=lambda d: d.footprint[0] * d.footprint[1])
            print(f"    largest footprint        : "
                  f"{big.footprint[0]:.0f} x {big.footprint[1]:.0f}")
    o_yspan = (max((d.wmax[1] for d in o_ground), default=0)
               - min((d.wmin[1] for d in o_ground), default=0))
    d_yspan = (max((d.wmax[1] for d in d_ground), default=0)
               - min((d.wmin[1] for d in d_ground), default=0))
    print(f"  -> terrain topography: original ground Y-span {o_yspan:.1f} vs "
          f"demo {d_yspan:.1f}.")
    if o_ground and d_ground and o_yspan > d_yspan * 2.0 + 5.0:
        print("     Original ground spans far more Y than the demo's ->")
        print("     consistent with the missing terrain-topography gap")
        print("     (confirm with a camera-matched capture).")
    o_gt = sum(1 for d in o_ground if d.tex_id)
    d_gt = sum(1 for d in d_ground if d.tex_id)
    if o_gt and not d_gt:
        print("     Original ground is textured; demo ground is not ->")
        print("     confirms the missing ground-texture gap.")

    hr("PHASE 12 GAP SUMMARY")
    print("  ground texture : see section 3 (demo ground-texture bound?) "
          "+ section 5")
    print("  terrain topo   : see section 5 (ground Y-span original vs demo)")
    print("  stream / water : see section 3 (demo water-texture bound?)")
    print("  level mirroring: see section 2 (best-fit negation axis)")
    print("  lighting       : see section 4 (AMBIENT + lights original vs demo)")
    print()


# ===========================================================================
# Frame selection
# ===========================================================================
def detect_conv(path, override):
    if override:
        return override
    # demo capture.c writes a `<path>.tex` sidecar; the proxy does not.
    return "gl" if os.path.exists(path + ".tex") else "d3d"


def pick_frame(path, requested):
    if requested is not None:
        return requested
    counts = scan_frame_draw_counts(path)
    if not counts:
        raise ValueError(f"{path}: no frames found")
    best = max(counts, key=lambda c: c[1])
    print(f"  [{os.path.basename(path)}] auto-picked frame {best[0]} "
          f"({best[1]} 3D draws, of {len(counts)} frames)")
    return best[0]


# ===========================================================================
def cmd_camera(path, frame_req):
    conv = detect_conv(path, None)
    idx = pick_frame(path, frame_req)
    fd = extract_frame(path, idx, conv)
    fov = fov_y(fd.proj)
    print(f"\ncapture : {path}")
    print(f"convention: {conv}")
    print(f"frame   : {idx}")
    if fd.view_baked:
        print("eye     : (n/a -- VIEW baked into WORLD by this engine)")
        print("look    : (n/a)")
        print(f"fovY    : {fov:.2f} deg" if fov else "fovY    : (n/a)")
        print("\nThis engine never emits a VIEW transform, so the camera is")
        print("not separable. Match cameras from the demo side instead, or")
        print("recover VIEW via M7's camera channel.")
    else:
        eye, look = decompose_camera(fd.view, conv)
        print(f"eye     : {fmt_vec(eye, 3)}")
        print(f"look    : {fmt_vec(look, 4)}")
        print(f"fovY    : {fov:.2f} deg" if fov else "fovY    : (n/a)")
        print("\nfeed these into the demo's camera for a matched-camera "
              "capture,\nthen re-run: diff.py <orig.omtc> <demo.omtc>")


def main():
    args = sys.argv[1:]
    orig_conv = demo_conv = None
    orig_frame = demo_frame = None
    cam_frame = None
    positionals = []
    camera_mode = False
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--camera":
            camera_mode = True
        elif a == "--orig-conv" and i + 1 < len(args):
            orig_conv = args[i + 1]; i += 1
        elif a == "--demo-conv" and i + 1 < len(args):
            demo_conv = args[i + 1]; i += 1
        elif a == "--orig-frame" and i + 1 < len(args):
            orig_frame = int(args[i + 1]); i += 1
        elif a == "--demo-frame" and i + 1 < len(args):
            demo_frame = int(args[i + 1]); i += 1
        elif a == "--frame" and i + 1 < len(args):
            cam_frame = int(args[i + 1]); i += 1
        elif a.startswith("-"):
            print(f"unknown option {a}")
            sys.exit(1)
        else:
            positionals.append(a)
        i += 1

    if camera_mode:
        if len(positionals) != 1:
            print("usage: diff.py --camera <capture.omtc> [--frame N]")
            sys.exit(1)
        cmd_camera(positionals[0], cam_frame)
        return

    if len(positionals) != 2:
        print(__doc__)
        sys.exit(1)
    orig_path, demo_path = positionals
    orig_conv = detect_conv(orig_path, orig_conv)
    demo_conv = detect_conv(demo_path, demo_conv)
    print(f"original : {orig_path}  (convention: {orig_conv})")
    print(f"demo     : {demo_path}  (convention: {demo_conv})")
    if orig_conv == demo_conv:
        print("  WARNING: both captures detected as the same convention -- "
              "pass --orig-conv/--demo-conv to override.")

    oi = pick_frame(orig_path, orig_frame)
    di = pick_frame(demo_path, demo_frame)
    orig = extract_frame(orig_path, oi, orig_conv)
    demo = extract_frame(demo_path, di, demo_conv)
    demo_names = load_demo_tex_sidecar(demo_path)
    print(f"\noriginal frame {oi}: {len(orig.draws)} draws")
    print(f"demo     frame {di}: {len(demo.draws)} draws")
    report(orig, demo, orig_conv, demo_conv, demo_names)


if __name__ == "__main__":
    main()
