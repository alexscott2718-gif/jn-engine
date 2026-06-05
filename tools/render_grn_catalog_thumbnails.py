#!/usr/bin/env python3
"""Render simple textured thumbnails for captured GRN GLBs.

This is a small software rasterizer for the catalog page. It avoids any GPU or
browser dependency: read POSITION/TEXCOORD_0/indices + embedded PNG from each
GLB, project from a fixed view, and write a thumbnail PNG.
"""

from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path

import pygltflib as gl
from PIL import Image


def vec_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vec_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vec_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def vec_norm(v):
    mag = math.sqrt(vec_dot(v, v))
    if mag <= 1e-12:
        return (0.0, 0.0, 1.0)
    return (v[0] / mag, v[1] / mag, v[2] / mag)


def accessor_bytes(doc, blob, accessor_index):
    acc = doc.accessors[accessor_index]
    view = doc.bufferViews[acc.bufferView]
    off = (view.byteOffset or 0) + (acc.byteOffset or 0)
    return acc, blob[off:off + view.byteLength]


def read_vec_accessor(doc, blob, accessor_index, ncomp):
    acc, data = accessor_bytes(doc, blob, accessor_index)
    if acc.componentType != gl.FLOAT:
        raise ValueError("expected FLOAT accessor")
    values = struct.unpack_from(f"<{acc.count * ncomp}f", data, 0)
    return [values[i:i + ncomp] for i in range(0, len(values), ncomp)]


def read_indices(doc, blob, accessor_index):
    acc, data = accessor_bytes(doc, blob, accessor_index)
    if acc.componentType == gl.UNSIGNED_INT:
        return list(struct.unpack_from(f"<{acc.count}I", data, 0))
    if acc.componentType == gl.UNSIGNED_SHORT:
        return list(struct.unpack_from(f"<{acc.count}H", data, 0))
    raise ValueError("unsupported index component type")


def read_texture(doc, blob):
    if not doc.images:
        return None
    image = doc.images[0]
    if image.bufferView is None:
        return None
    view = doc.bufferViews[image.bufferView]
    off = view.byteOffset or 0
    data = blob[off:off + view.byteLength]
    from io import BytesIO
    return Image.open(BytesIO(data)).convert("RGB")


def project(points, width, height):
    eye = vec_norm((0.85, 0.45, 1.45))
    forward = vec_norm((-eye[0], -eye[1], -eye[2]))
    right = vec_norm(vec_cross(forward, (0.0, 1.0, 0.0)))
    up = vec_cross(right, forward)

    view = []
    for p in points:
        view.append((vec_dot(p, right), vec_dot(p, up), vec_dot(p, forward)))

    min_x = min(p[0] for p in view)
    max_x = max(p[0] for p in view)
    min_y = min(p[1] for p in view)
    max_y = max(p[1] for p in view)
    sx = max_x - min_x
    sy = max_y - min_y
    scale = min((width * 0.72) / max(sx, 1e-6), (height * 0.72) / max(sy, 1e-6))
    cx = (min_x + max_x) * 0.5
    cy = (min_y + max_y) * 0.5
    return [
        ((p[0] - cx) * scale + width * 0.5,
         height * 0.52 - (p[1] - cy) * scale,
         p[2])
        for p in view
    ]


def sample_texture(tex, u, v):
    if tex is None:
        return (185, 185, 185)
    w, h = tex.size
    u = u - math.floor(u)
    v = v - math.floor(v)
    x = min(w - 1, max(0, int(u * w)))
    y = min(h - 1, max(0, int(v * h)))
    return tex.getpixel((x, y))


def raster_triangle(img, zbuf, pts, uvs, tex):
    (x0, y0, z0), (x1, y1, z1), (x2, y2, z2) = pts
    area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
    if abs(area) < 1e-6:
        return
    w, h = img.size
    min_x = max(0, int(math.floor(min(x0, x1, x2))))
    max_x = min(w - 1, int(math.ceil(max(x0, x1, x2))))
    min_y = max(0, int(math.floor(min(y0, y1, y2))))
    max_y = min(h - 1, int(math.ceil(max(y0, y1, y2))))
    pix = img.load()
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            px = x + 0.5
            py = y + 0.5
            w0 = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) / area
            w1 = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) / area
            w2 = 1.0 - w0 - w1
            if w0 < -1e-5 or w1 < -1e-5 or w2 < -1e-5:
                continue
            z = w0 * z0 + w1 * z1 + w2 * z2
            if z >= zbuf[y][x]:
                continue
            zbuf[y][x] = z
            u = w0 * uvs[0][0] + w1 * uvs[1][0] + w2 * uvs[2][0]
            v = w0 * uvs[0][1] + w1 * uvs[1][1] + w2 * uvs[2][1]
            pix[x, y] = sample_texture(tex, u, v)


def render_glb(path: Path, out: Path, width: int, height: int):
    doc = gl.GLTF2().load(str(path))
    blob = doc.binary_blob()
    prim = doc.meshes[0].primitives[0]
    positions = read_vec_accessor(doc, blob, prim.attributes.POSITION, 3)
    texcoords = read_vec_accessor(doc, blob, prim.attributes.TEXCOORD_0, 2)
    indices = read_indices(doc, blob, prim.indices)
    tex = read_texture(doc, blob)
    projected = project(positions, width, height)

    img = Image.new("RGB", (width, height), (28, 32, 32))
    zbuf = [[float("inf")] * width for _ in range(height)]
    for i in range(0, len(indices), 3):
        tri = indices[i:i + 3]
        raster_triangle(
            img,
            zbuf,
            [projected[j] for j in tri],
            [texcoords[j] for j in tri],
            tex,
        )
    out.parent.mkdir(parents=True, exist_ok=True)
    img.save(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("glb_dir", type=Path)
    ap.add_argument("out_dir", type=Path)
    ap.add_argument("--width", type=int, default=480)
    ap.add_argument("--height", type=int, default=360)
    args = ap.parse_args()

    files = sorted(args.glb_dir.glob("*.glb"))
    if not files:
        raise SystemExit(f"no GLBs in {args.glb_dir}")
    for f in files:
        out = args.out_dir / f"{f.stem}.png"
        render_glb(f, out, args.width, args.height)
        print(f"{f.name} -> {out.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
