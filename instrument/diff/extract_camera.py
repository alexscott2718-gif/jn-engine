#!/usr/bin/env python3
r"""
extract_camera.py -- M7a camera recovery from an OMT2 capture.

M6 found OMT2 never emits a VIEW transform: it bakes the camera into every
WORLD matrix.  For a draw of a static object,

    WORLD_baked = MODEL_true . VIEW             (D3D row-vector convention)

where MODEL_true is the object's true world placement.

The original game's static chunks are NOT placed by pure translation as the
M7 plan first assumed -- this tool's first run against m5_session.omtc showed
each chunk also carries its own Y-axis (yaw) rotation.  Write
MODEL_true = M_yaw . T  (yaw then translate).  Two facts survive that and
make recovery tractable:

  * Translation row of WORLD_baked = origin . R + tv  -- INDEPENDENT of the
    object's yaw (the yaw block never reaches the translation row).  So every
    static draw's WORLD translation row is a clean sample of  origin.R + tv.

  * The middle row of WORLD_baked's rotation block = R's middle row, for any
    object whose model rotation is about Y only ([0,1,0] . M_yaw = [0,1,0]).
    So R's middle row is the row shared by the overwhelming majority of
    perspective draws -- recover it by consensus.

With R's middle row m1 fixed, a rotation has exactly ONE remaining degree of
freedom (the camera yaw angle).  The tool sweeps that angle; for each
candidate R it registers the level1 placements (transformed by R) against the
captured translation cloud by Hough voting on tv; the angle whose best tv has
the most inliers wins.  Running the whole search with the placements as
authored and with their X negated, and keeping the better fit, IS the
rigorous answer to the Phase 12 level-mirroring question.

Output: a camera descriptor (GL right-handed, column-major) that
src/engine/capture.c loads via JN_CAPTURE_CAMERA, plus a printed report.

Known limitation -- the eye HEIGHT.  Point-cloud registration locks X, Z and
both camera angles (yaw + pitch) tightly, but the vertical axis is weakly
constrained: the level1 placement file stores mesh-AABB centres, not the
original's chunk model origins, and the two engines bake terrain height
differently.  The solved eye_y is therefore unreliable.  Use --eye-y to pin
it from a single eyeballed matched render (the matched-capture step, M7c);
everything else in the descriptor is trustworthy.

Usage:
  extract_camera.py <capture.omtc> [--frame N] [--emit cam.cam]
                    [--placements <file>] [--eps <world units>]
                    [--steps <yaw sweep resolution>] [--eye-y <height>]
"""

import os
import sys
import math

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "receiver"))
from protocol import *  # noqa: E402,F403

DEFAULT_PLACEMENTS = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "assets", "ase", "omt", "level1_placements.txt")


# ===========================================================================
# Incremental record reader (captures are hundreds of MB -- never hold whole)
# ===========================================================================
def iter_records(path, chunk=4 << 20):
    with open(path, "rb") as f:
        head = f.read(Header.size())
        if len(head) < Header.size():
            raise ValueError(f"{path}: too short for a stream header")
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


def is_perspective(proj):
    """True iff PROJ couples clip.w to the input position (a perspective
    projection).  Orthographic (HUD) projections have all w-coupling zero."""
    if proj is None:
        return False
    return max(abs(proj[3]), abs(proj[7]), abs(proj[11]),
               abs(proj[12]), abs(proj[13]), abs(proj[14])) > 1e-4


# ===========================================================================
# Pass 1 -- per-frame perspective draw counts (for auto frame pick)
# ===========================================================================
def scan_frame_counts(path):
    counts = []
    proj = None
    persp = 0
    idx = -1
    for item in iter_records(path):
        if item[0] == "header":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            idx += 1
            persp = 0
        elif rt == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            if st.which == TRANSFORM_PROJ:
                proj = st.matrix
        elif rt == RECORD_DRAW_PRIMITIVE:
            if is_perspective(proj):
                persp += 1
        elif rt == RECORD_FRAME_END:
            counts.append((idx, persp))
    return counts


# ===========================================================================
# Pass 2 -- decode one frame: the perspective PROJ + every perspective draw's
# WORLD matrix.  PROJ is sticky and the game re-points it at the HUD at the
# end of every frame, so the projection we want is the one live at the FIRST
# perspective draw, not the last PROJ seen.
# ===========================================================================
def extract_frame(path, want):
    """Decode frame `want`: returns (persp_proj, [WORLD matrix, ...])."""
    proj = None
    world = None
    persp_proj = None
    worlds = []
    idx = -1
    capturing = False
    for item in iter_records(path):
        if item[0] == "header":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            idx += 1
            capturing = (idx == want)
            if idx > want:
                break
            continue
        if rt == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            if st.which == TRANSFORM_WORLD:
                world = st.matrix
            elif st.which == TRANSFORM_PROJ:
                proj = st.matrix
        elif rt == RECORD_DRAW_PRIMITIVE and capturing:
            if not is_perspective(proj):
                continue
            if persp_proj is None:
                persp_proj = proj
            if world is not None:
                worlds.append(world)
        elif rt == RECORD_FRAME_END and capturing:
            break
    return persp_proj, worlds


# ===========================================================================
# Projection decomposition
# ===========================================================================
def decompose_proj(proj):
    """D3D row-major perspective PROJ -> (fovY deg, aspect, near, far).

    proj[5]  = cot(fovY/2) ;  proj[0] = cot(fovY/2)/aspect
    NDC z(z) = (proj[10] z + proj[14]) / (proj[11] z + proj[15]) ;
    near is where that is 0, far where it is 1."""
    fov_y = math.degrees(2.0 * math.atan(1.0 / abs(proj[5])))
    aspect = abs(proj[5]) / abs(proj[0])
    a, b = proj[10], proj[14]      # numerator: a z + b
    c, d = proj[11], proj[15]      # denominator: c z + d
    near = (-b / a) if abs(a) > 1e-12 else 1.0
    far = ((d - b) / (a - c)) if abs(a - c) > 1e-12 else 1e5
    return fov_y, aspect, near, far


# ===========================================================================
# Small vector / matrix helpers
# ===========================================================================
def vadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vcross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def vnorm(a):
    l = math.sqrt(vdot(a, a)) or 1.0
    return (a[0] / l, a[1] / l, a[2] / l)


ROT_IDX = (0, 1, 2, 4, 5, 6, 8, 9, 10)   # 3x3 block within a 16-flat matrix


def rot_mid_row(m):
    """Middle row of a 16-flat matrix's rotation block: indices 4,5,6."""
    return (m[4], m[5], m[6])


def mat3_mul_vec_row(p, R):
    """Row-vector . 3x3 (row-major):  out[c] = sum_k p[k] R[k*3+c]."""
    return tuple(sum(p[k] * R[k * 3 + c] for k in range(3)) for c in range(3))


# ===========================================================================
# Recover R's middle row -- the row shared by yaw-only-rotated static objects
# ===========================================================================
def recover_mid_row(worlds):
    """Consensus middle row of the perspective WORLD rotation blocks.  Returns
    (m1 unit vector, support fraction)."""
    tally = {}
    for m in worlds:
        key = tuple(round(x, 2) for x in rot_mid_row(m))
        tally[key] = tally.get(key, 0) + 1
    if not tally:
        return None, 0.0
    best_key = max(tally, key=tally.get)
    total = sum(tally.values())
    # Average the exact middle rows in the dominant rounded cluster.
    acc = [0.0, 0.0, 0.0]
    n = 0
    for m in worlds:
        if tuple(round(x, 2) for x in rot_mid_row(m)) == best_key:
            mr = rot_mid_row(m)
            acc[0] += mr[0]; acc[1] += mr[1]; acc[2] += mr[2]
            n += 1
    return vnorm((acc[0] / n, acc[1] / n, acc[2] / n)), tally[best_key] / total


# ===========================================================================
# Parameterise a rotation whose middle row is fixed -- 1 DOF (the camera yaw)
# ===========================================================================
def perp_basis(m1):
    """Two orthonormal vectors spanning the plane perpendicular to m1."""
    seed = (1.0, 0.0, 0.0) if abs(m1[0]) < 0.9 else (0.0, 1.0, 0.0)
    a = vnorm(vcross(seed, m1))
    b = vcross(m1, a)
    return a, b


def rotation_from_yaw(m1, a, b, alpha):
    """Row-major 3x3 rotation: row1 = m1, row0 spins in the (a,b) plane,
    row2 = row0 x m1 so the result is a proper (det +1) rotation."""
    ca, sa = math.cos(alpha), math.sin(alpha)
    r0 = (ca * a[0] + sa * b[0], ca * a[1] + sa * b[1], ca * a[2] + sa * b[2])
    r2 = vcross(r0, m1)
    return [r0[0], r0[1], r0[2], m1[0], m1[1], m1[2], r2[0], r2[1], r2[2]]


def is_orthonormal(R, tol=1e-2):
    for i in range(3):
        for j in range(3):
            dot = sum(R[i * 3 + k] * R[j * 3 + k] for k in range(3))
            if abs(dot - (1.0 if i == j else 0.0)) > tol:
                return False
    return True


# ===========================================================================
# level1 placement loading
# ===========================================================================
def load_placements(path):
    """Returns [(name, (x, y, z)), ...] from level1_placements.txt."""
    out = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) < 5:
                continue
            try:
                xyz = (float(parts[2]), float(parts[3]), float(parts[4]))
            except ValueError:
                continue
            out.append((parts[0], xyz))
    return out


# ===========================================================================
# Registration -- find (yaw, tv) aligning placements.R + tv to the capture
# ===========================================================================
def vote_tv(qs, cap_pts, eps):
    """Hough vote: every (placement q, capture c) proposes tv = c - q; return
    (best tv-bin centre approx, vote count) for the heaviest eps-sized bin."""
    inv = 1.0 / eps
    bins = {}
    for q in qs:
        for c in cap_pts:
            key = (round((c[0] - q[0]) * inv),
                   round((c[1] - q[1]) * inv),
                   round((c[2] - q[2]) * inv))
            bins[key] = bins.get(key, 0) + 1
    if not bins:
        return None, 0
    best = max(bins, key=bins.get)
    return (best[0] * eps, best[1] * eps, best[2] * eps), bins[best]


def exact_register(qs, cap_pts, tv0, eps):
    """Given an approximate tv, snap each placement to its nearest captured
    point, keep the inliers within eps, and return (tv refined, inlier count,
    mean residual)."""
    eps2 = eps * eps
    acc = [0.0, 0.0, 0.0]
    n = 0
    resid = 0.0
    for q in qs:
        px, py, pz = q[0] + tv0[0], q[1] + tv0[1], q[2] + tv0[2]
        bd, bc = 1e30, None
        for c in cap_pts:
            dx, dy, dz = px - c[0], py - c[1], pz - c[2]
            dd = dx * dx + dy * dy + dz * dz
            if dd < bd:
                bd, bc = dd, c
        if bd <= eps2 and bc is not None:
            acc[0] += bc[0] - q[0]
            acc[1] += bc[1] - q[1]
            acc[2] += bc[2] - q[2]
            resid += math.sqrt(bd)
            n += 1
    if n == 0:
        return tv0, 0, float("inf")
    return (acc[0] / n, acc[1] / n, acc[2] / n), n, resid / n


def solve_for_flip(cap_pts, placements, m1, eps, steps, flip):
    """Sweep the camera yaw; return the best (R, tv, inliers, residual, yaw)
    for the given X-flip choice."""
    a, b = perp_basis(m1)
    # The demo draws every static placement at (x, 0, z) -- height is baked
    # into the mesh (main.c renderer_draw_model(pm,0,pl->x,0,pl->z,...)), and
    # the original's OMT chunks come from the same exporter source.  Register
    # against y=0 so the recovered camera lands directly in the demo's world
    # space; the placement file's center_y is a mesh-AABB centre, not an origin.
    pls = [((-x if flip else x), 0.0, z) for _, (x, _y, z) in placements]
    # Coarse sweep -- score each yaw by its heaviest tv vote bin.
    best = (-1, 0.0)            # (score, alpha)
    for s in range(steps):
        alpha = 2.0 * math.pi * s / steps
        R = rotation_from_yaw(m1, a, b, alpha)
        qs = [mat3_mul_vec_row(p, R) for p in pls]
        _, score = vote_tv(qs, cap_pts, eps)
        if score > best[0]:
            best = (score, alpha)
    # Fine sweep around the coarse winner.
    coarse = best[1]
    span = 2.0 * math.pi / steps
    best = (-1, coarse, None, None)
    fine = 41
    for s in range(fine):
        alpha = coarse + span * (s - fine // 2) / (fine // 2)
        R = rotation_from_yaw(m1, a, b, alpha)
        qs = [mat3_mul_vec_row(p, R) for p in pls]
        tv_approx, _ = vote_tv(qs, cap_pts, eps)
        if tv_approx is None:
            continue
        tv, inliers, residual = exact_register(qs, cap_pts, tv_approx, eps)
        # score: inliers first, then lower residual
        score = inliers - residual / (eps * 1000.0)
        if score > best[0]:
            best = (score, alpha, (R, tv, inliers, residual))
    if best[2] is None:
        return None
    R, tv, inliers, residual = best[2]
    return {"R": R, "tv": tv, "inliers": inliers, "residual": residual,
            "yaw": best[1], "flip": flip}


# ===========================================================================
# Batch frame extraction -- single pass for many requested frames.
# Useful when solving VIEW for a small set of keyframes scattered across a
# multi-GB capture (calling extract_frame() N times re-scans the file N times).
# ===========================================================================
def extract_frames_batch(path, want_frames):
    """Decode the requested frame indices in one linear pass.

    Returns {frame_idx: (persp_proj, [WORLD matrix, ...])}.  Frames that were
    requested but never appeared in the capture map to (None, []).  persp_proj
    is the PROJ live at that frame's FIRST perspective draw (PROJ is sticky and
    the game flips it for the HUD at frame end -- the first draw's PROJ is the
    one used for the world view).
    """
    want = set(int(f) for f in want_frames)
    result = {f: (None, []) for f in want}
    proj = None
    world = None
    idx = -1
    capturing = False
    cur_frame_persp_proj = None
    cur_worlds = None
    max_wanted = max(want) if want else -1
    for item in iter_records(path):
        if item[0] == "header":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            idx += 1
            capturing = idx in want
            if capturing:
                cur_frame_persp_proj = None
                cur_worlds = []
            else:
                cur_frame_persp_proj = None
                cur_worlds = None
            if idx > max_wanted:
                break
            continue
        if rt == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            if st.which == TRANSFORM_WORLD:
                world = st.matrix
            elif st.which == TRANSFORM_PROJ:
                proj = st.matrix
        elif rt == RECORD_DRAW_PRIMITIVE and capturing:
            if not is_perspective(proj):
                continue
            if cur_frame_persp_proj is None:
                cur_frame_persp_proj = proj
            if world is not None:
                cur_worlds.append(world)
        elif rt == RECORD_FRAME_END and capturing:
            result[idx] = (cur_frame_persp_proj, cur_worlds or [])
            capturing = False
            cur_worlds = None
    return result


# ===========================================================================
# Row-major flat-16 matrix utilities (D3D row-vector convention)
# ===========================================================================
def mat4_mul_row(A, B):
    """Row-major flat-16 multiply: C[i,j] = sum_k A[i,k] * B[k,j].

    In D3D row-vector convention v . A . B applies A first then B, so the
    combined matrix is mat4_mul_row(A, B)."""
    C = [0.0] * 16
    for i in range(4):
        ai0 = A[i * 4 + 0]; ai1 = A[i * 4 + 1]
        ai2 = A[i * 4 + 2]; ai3 = A[i * 4 + 3]
        for j in range(4):
            C[i * 4 + j] = (ai0 * B[0 * 4 + j] +
                            ai1 * B[1 * 4 + j] +
                            ai2 * B[2 * 4 + j] +
                            ai3 * B[3 * 4 + j])
    return C


def view16_from_R_tv(R, tv):
    """Build the row-major flat-16 D3D VIEW matrix from the 9-float row-major
    rotation R and 3-float translation row tv.  Layout:
        [ R[0,0] R[0,1] R[0,2] 0 ]
        [ R[1,0] R[1,1] R[1,2] 0 ]
        [ R[2,0] R[2,1] R[2,2] 0 ]
        [ tv[0]  tv[1]  tv[2]  1 ]
    """
    return [R[0], R[1], R[2], 0.0,
            R[3], R[4], R[5], 0.0,
            R[6], R[7], R[8], 0.0,
            tv[0], tv[1], tv[2], 1.0]


def view_inv16_from_R_tv(R, tv):
    """Inverse of the rigid VIEW = [R 0; tv 1] (row-major row-vector):
        VIEW^-1 = [R^T 0; eye 1]   with   eye = -tv . R^T .
    """
    # eye = -tv . R^T  ;  (tv . R^T)[j] = sum_i tv[i] * R[j,i]
    eye0 = -(tv[0] * R[0] + tv[1] * R[1] + tv[2] * R[2])
    eye1 = -(tv[0] * R[3] + tv[1] * R[4] + tv[2] * R[5])
    eye2 = -(tv[0] * R[6] + tv[1] * R[7] + tv[2] * R[8])
    return [R[0], R[3], R[6], 0.0,
            R[1], R[4], R[7], 0.0,
            R[2], R[5], R[8], 0.0,
            eye0, eye1, eye2, 1.0]


# ===========================================================================
# Per-frame VIEW solve -- callable wrapper around the recovery pipeline.
# Returns the solved (R, tv, ...) along with a 16-float VIEW and VIEW^-1 so
# downstream tooling can compose cross-frame transforms without re-deriving
# them.  force_flip=False locks identity-mirroring (Phase 12 finding); pass
# None to let registration pick.
# ===========================================================================
def solve_view(persp_proj, worlds, placements, eps=150.0, steps=360,
               force_flip=None):
    """Solve VIEW for one frame's perspective draws + WORLD matrices.

    Returns a dict on success; None on failure.  Keys:
        m1          -- recovered middle row of R (3-tuple)
        m1_support  -- consensus support fraction
        R           -- 9-flat row-major rotation
        tv          -- 3-tuple translation row
        flip        -- True if X-flip won registration
        inliers     -- number of placements registered within eps
        residual    -- mean per-inlier residual (world units)
        yaw         -- recovered camera yaw (radians)
        fov, near, far, aspect  -- from decompose_proj()
        eye, right, up, fwd     -- camera basis vectors
        view16, view_inv16      -- row-major flat-16 D3D-convention matrices
    """
    if persp_proj is None or not worlds:
        return None
    m1, support = recover_mid_row(worlds)
    if m1 is None:
        return None
    # Distinct WORLD translation rows: yaw-invariant samples of origin.R + tv.
    cap_pts = []
    seen = set()
    for m in worlds:
        t = (m[12], m[13], m[14])
        key = tuple(round(c, 1) for c in t)
        if key in seen:
            continue
        seen.add(key)
        cap_pts.append(t)
    if not cap_pts:
        return None
    if force_flip is True or force_flip is False:
        sol = solve_for_flip(cap_pts, placements, m1, eps, steps, force_flip)
        if sol is None:
            return None
    else:
        sol_id = solve_for_flip(cap_pts, placements, m1, eps, steps, False)
        sol_fx = solve_for_flip(cap_pts, placements, m1, eps, steps, True)
        in_id = sol_id["inliers"] if sol_id else -1
        in_fx = sol_fx["inliers"] if sol_fx else -1
        if in_id < 0 and in_fx < 0:
            return None
        sol = sol_fx if in_fx > in_id else sol_id
    R, tv = sol["R"], sol["tv"]
    fov, aspect, near, far = decompose_proj(persp_proj)
    eye, right, up, fwd = camera_basis(R, tv)
    return {
        "m1": tuple(m1),
        "m1_support": support,
        "R": list(R),
        "tv": tuple(tv),
        "flip": sol["flip"],
        "inliers": sol["inliers"],
        "residual": sol["residual"],
        "yaw": sol["yaw"],
        "fov": fov,
        "aspect": aspect,
        "near": near,
        "far": far,
        "eye": eye,
        "right": right,
        "up": up,
        "fwd": fwd,
        "view16": view16_from_R_tv(R, tv),
        "view_inv16": view_inv16_from_R_tv(R, tv),
    }


# ===========================================================================
# Camera basis -> GL matrices
# ===========================================================================
def camera_basis(R, tv):
    """D3DXMatrixLookAtLH lays the view rotation out with
        column 0 = right, column 1 = up, column 2 = forward (+Z) ;
        tv = (-dot(right,eye), -dot(up,eye), -dot(forward,eye)).
    So  eye = -(tv0*right + tv1*up + tv2*forward)."""
    right = (R[0], R[3], R[6])
    up = (R[1], R[4], R[7])
    fwd = (R[2], R[5], R[8])
    eye = tuple(-(tv[0] * right[i] + tv[1] * up[i] + tv[2] * fwd[i])
                for i in range(3))
    return eye, right, up, fwd


def gl_lookat(eye, fwd):
    """Right-handed GL view matrix (column-major flat), matching
    renderer.c mat4_lookat: camera looks down -Z, world up = +Y."""
    f = vnorm(fwd)
    r = vcross(f, (0.0, 1.0, 0.0))
    if vdot(r, r) < 1e-9:
        r = (1.0, 0.0, 0.0)
    r = vnorm(r)
    u = vcross(r, f)
    m = [0.0] * 16
    m[0], m[4], m[8], m[12] = r[0], r[1], r[2], -vdot(r, eye)
    m[1], m[5], m[9], m[13] = u[0], u[1], u[2], -vdot(u, eye)
    m[2], m[6], m[10], m[14] = -f[0], -f[1], -f[2], vdot(f, eye)
    m[3], m[7], m[11], m[15] = 0.0, 0.0, 0.0, 1.0
    return m


def gl_perspective(fov_deg, aspect, near, far):
    """Right-handed GL projection (column-major flat), matching
    renderer.c mat4_perspective."""
    f = 1.0 / math.tan(math.radians(fov_deg) * 0.5)
    m = [0.0] * 16
    m[0] = f / aspect
    m[5] = f
    m[10] = (far + near) / (near - far)
    m[11] = -1.0
    m[14] = (2.0 * far * near) / (near - far)
    return m


# ===========================================================================
# Descriptor emission
# ===========================================================================
def fmt_mat(m):
    return " ".join(f"{x:.8g}" for x in m)


def emit_descriptor(path, screen, view, proj, fov, near, far, eye,
                    sol, n_place):
    with open(path, "w") as f:
        f.write("# jn-engine M7a camera descriptor "
                "(GL right-handed, column-major)\n")
        f.write("# generated by instrument/diff/extract_camera.py\n")
        f.write(f"# fovY={fov:.3f} deg  near={near:.2f} far={far:.2f}\n")
        f.write(f"# eye={eye[0]:.2f},{eye[1]:.2f},{eye[2]:.2f}\n")
        f.write(f"# mirroring={'X-FLIPPED' if sol['flip'] else 'identity'}  "
                f"registration: {sol['inliers']}/{n_place} placements, "
                f"residual={sol['residual']:.2f}\n")
        f.write(f"screen {screen[0]} {screen[1]}\n")
        f.write(f"fovy {fov:.6f}\n")
        f.write(f"near {near:.6f}\n")
        f.write(f"far {far:.6f}\n")
        f.write(f"eye {eye[0]:.6f} {eye[1]:.6f} {eye[2]:.6f}\n")
        f.write(f"view {fmt_mat(view)}\n")
        f.write(f"proj {fmt_mat(proj)}\n")


# ===========================================================================
def main():
    args = sys.argv[1:]
    path = None
    frame = None
    emit = None
    placements_path = DEFAULT_PLACEMENTS
    eps = 150.0
    steps = 360
    eye_y = None
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--frame" and i + 1 < len(args):
            frame = int(args[i + 1]); i += 1
        elif a == "--emit" and i + 1 < len(args):
            emit = args[i + 1]; i += 1
        elif a == "--placements" and i + 1 < len(args):
            placements_path = args[i + 1]; i += 1
        elif a == "--eps" and i + 1 < len(args):
            eps = float(args[i + 1]); i += 1
        elif a == "--steps" and i + 1 < len(args):
            steps = int(args[i + 1]); i += 1
        elif a == "--eye-y" and i + 1 < len(args):
            eye_y = float(args[i + 1]); i += 1
        elif a.startswith("-"):
            print(f"unknown option {a}")
            sys.exit(1)
        elif path is None:
            path = a
        else:
            print("extra positional argument", a)
            sys.exit(1)
        i += 1
    if path is None:
        print(__doc__)
        sys.exit(1)

    # ---- screen size from the stream header ------------------------------
    hdr = next(iter(iter_records(path)))[1]
    screen = (hdr.screen_w, hdr.screen_h)

    # ---- frame selection -------------------------------------------------
    if frame is None:
        print("scanning for the frame with the most perspective draws ...")
        counts = scan_frame_counts(path)
        if not counts:
            print("no frames found")
            sys.exit(1)
        frame = max(counts, key=lambda c: c[1])[0]
        print(f"  -> frame {frame} "
              f"({max(c[1] for c in counts)} perspective draws)")

    proj, worlds = extract_frame(path, frame)
    if proj is None or not worlds:
        print(f"frame {frame}: no perspective geometry to solve from")
        sys.exit(1)

    print("\n" + "=" * 70)
    print(f"CAPTURE   {path}")
    print(f"frame     {frame}    screen {screen[0]}x{screen[1]}")
    print(f"perspective draws with a WORLD matrix: {len(worlds)}")
    print("=" * 70)

    # ---- projection ------------------------------------------------------
    fov, aspect, near, far = decompose_proj(proj)
    print("\n[1] PROJECTION")
    print(f"  fovY   = {fov:.3f} deg")
    print(f"  aspect = {aspect:.4f}  (screen ratio "
          f"{screen[0] / screen[1]:.4f})")
    print(f"  near   = {near:.2f}    far = {far:.2f}")

    # ---- R's middle row --------------------------------------------------
    m1, support = recover_mid_row(worlds)
    print("\n[2] VIEW ROTATION -- middle row (yaw-invariant consensus)")
    print(f"  m1 = ({m1[0]:+.5f}, {m1[1]:+.5f}, {m1[2]:+.5f})")
    print(f"  shared by {support * 100:.1f}% of perspective draws")
    if support < 0.30:
        print("  WARNING: weak consensus -- the camera-pitch estimate, and so")
        print("  every downstream result, is unreliable for this frame.")

    # ---- captured static translation cloud -------------------------------
    cap_pts = []
    seen = set()
    for m in worlds:
        t = (m[12], m[13], m[14])
        k = tuple(round(c, 1) for c in t)
        if k in seen:
            continue
        seen.add(k)
        cap_pts.append(t)
    print(f"\n[3] STATIC OBJECT CLOUD: {len(cap_pts)} distinct WORLD "
          f"translation rows (yaw-invariant samples of origin.R + tv)")

    # ---- registration + mirroring test -----------------------------------
    placements = load_placements(placements_path)
    print(f"\n[4] REGISTER vs {len(placements)} level1 placements "
          f"(eps={eps:.0f}, yaw sweep {steps} steps)")
    sol_id = solve_for_flip(cap_pts, placements, m1, eps, steps, False)
    sol_fx = solve_for_flip(cap_pts, placements, m1, eps, steps, True)
    for label, s in (("identity ", sol_id), ("X-flipped", sol_fx)):
        if s:
            print(f"  {label}: {s['inliers']:3d}/{len(placements)} inliers  "
                  f"residual {s['residual']:8.2f}  "
                  f"yaw {math.degrees(s['yaw']):7.2f} deg")
        else:
            print(f"  {label}: no solution")

    if not sol_id and not sol_fx:
        print("  -> registration FAILED entirely.")
        sys.exit(1)
    in_id = sol_id["inliers"] if sol_id else -1
    in_fx = sol_fx["inliers"] if sol_fx else -1
    sol = sol_fx if in_fx > in_id else sol_id

    if max(in_id, in_fx) < 3:
        print("  -> WEAK: <3 inliers. Rotation/projection are still valid but")
        print("     the absolute eye position is unreliable. Try --frame /")
        print("     a wider --eps, or more placements.")
    elif abs(in_id - in_fx) <= 1:
        print(f"  -> inconclusive mirroring (scores within 1); using "
              f"{'X-flip' if sol['flip'] else 'identity'}.")
    else:
        print(f"  -> {'LEVEL IS X-MIRRORED' if sol['flip'] else 'NO mirroring'}"
              f"  (Phase 12 mirroring measurement).")

    if not is_orthonormal(sol["R"]):
        print("  WARNING: solved rotation is not orthonormal -- result suspect.")

    # ---- camera basis + GL matrices --------------------------------------
    eye, right, up, fwd = camera_basis(sol["R"], sol["tv"])
    print("\n[5] CAMERA  (level1 placement world space"
          f"{', X-mirrored' if sol['flip'] else ''})")
    if eye_y is not None:
        print(f"  eye_y solved {eye[1]:.2f} -> overridden to {eye_y:.2f} "
              f"(--eye-y)")
        eye = (eye[0], eye_y, eye[2])
    else:
        print("  NOTE: eye_y is weakly constrained -- see --eye-y in --help.")
    print(f"  eye     = ({eye[0]:.2f}, {eye[1]:.2f}, {eye[2]:.2f})")
    print(f"  forward = ({fwd[0]:+.4f}, {fwd[1]:+.4f}, {fwd[2]:+.4f})")
    print(f"  up      = ({up[0]:+.4f}, {up[1]:+.4f}, {up[2]:+.4f})")

    gl_view = gl_lookat(eye, fwd)
    gl_proj = gl_perspective(fov, aspect, near, far)

    if emit:
        emit_descriptor(emit, screen, gl_view, gl_proj, fov, near, far,
                        eye, sol, len(placements))
        print(f"\nwrote camera descriptor -> {emit}")
        print("render the demo from it:")
        print(f"  make capture && JN_CAPTURE=demo.omtc "
              f"JN_CAPTURE_CAMERA={emit} JN_SCREENSHOT=1 ./jnengine")
    else:
        print("\n(no --emit given; pass --emit cam.cam to write a descriptor)")


if __name__ == "__main__":
    main()
