"""Visual proof: render fireHydrant01 from level1.omt with its texture applied.

Drives awefan4524's omt_parse + omt_3d directly. No display required.

Output:
  /tmp/awefan_proof_hydrant.png      — single composite (texture | wireframe | textured render)
  /tmp/awefan_proof_hydrant_tex.png  — extracted Canvas
  /tmp/awefan_proof_hydrant_mesh.obj — exported OBJ
"""
import sys, types, io, math, struct
from pathlib import Path
from collections import defaultdict

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))

# Stub PySide6 so omt_parse imports headlessly.
def _stub():
    pkg = types.ModuleType('PySide6'); pkg.__path__ = []
    sys.modules['PySide6'] = pkg
    for s in ('QtCore','QtGui','QtWidgets','QtMultimedia','QtMultimediaWidgets'):
        m = types.ModuleType(f'PySide6.{s}')
        m.__getattr__ = lambda n: type(n, (), {})
        sys.modules[f'PySide6.{s}'] = m
_stub()

from omt_parse import OMTFormatParser, OMTCanvasDecoder
from omt_3d import find_shapes_in_file, parse_3dsh_bytes, export_obj
from PIL import Image, ImageDraw, ImageFont

OMT_PATH = Path.home() / "jn-engine/assets/omt/level1.omt"
OUT_DIR = Path("/tmp")


def quiet_open(parser, path):
    saved = sys.stdout; sys.stdout = io.StringIO()
    try: parser.open_file(str(path))
    finally: sys.stdout = saved


def find_chunk(parser, name, ctype):
    for i, c in enumerate(parser.chunks):
        if c.get('name') == name and c.get('type') == ctype:
            return i, c
    return None, None


def extract_canvas(parser, chunk_idx) -> Image.Image:
    data = parser.get_chunk_data(chunk_idx)
    return OMTCanvasDecoder.decode_canvas(data)


def extract_mesh(parser, chunk_idx):
    data = parser.get_chunk_data(chunk_idx)
    # 3DSh chunk payload is a raw 3DSP block.
    mesh, _ = parse_3dsh_bytes(data, 0, '>')
    return mesh


def project(v, rot_y, rot_x, cam_dist, view_w, view_h, scale):
    cy, sy = math.cos(rot_y), math.sin(rot_y)
    cx, sx = math.cos(rot_x), math.sin(rot_x)
    x, y, z = v
    # rotate around Y, then X
    x2 = cy * x + sy * z
    z2 = -sy * x + cy * z
    y3 = cx * y - sx * z2
    z3 = sx * y + cx * z2
    # perspective
    w = z3 + cam_dist
    if w < 0.1: w = 0.1
    px = view_w * 0.5 + (x2 * scale / w) * view_w * 0.5
    py = view_h * 0.5 - (y3 * scale / w) * view_h * 0.5
    return px, py, w


def fan_triangulate(poly_v_idx, poly_uv):
    if len(poly_v_idx) < 3: return
    for i in range(1, len(poly_v_idx) - 1):
        yield (poly_v_idx[0], poly_v_idx[i], poly_v_idx[i+1]), \
              (poly_uv[0], poly_uv[i], poly_uv[i+1])


def sample_tex(tex_arr, tw, th, u, v):
    # GL_REPEAT
    u = u - math.floor(u); v = v - math.floor(v)
    x = int(u * (tw - 1)); y = int((1.0 - v) * (th - 1))
    return tex_arr[y * tw + x]


def render_textured(mesh, tex_img, W=480, H=480):
    """Software rasterize with per-pixel barycentric UV interpolation."""
    tex = tex_img.convert("RGBA")
    tw, th = tex.size
    tex_pixels = list(tex.getdata())

    # Auto-fit: find bounds, choose distance
    verts = mesh.vertices
    xs = [v[0] for v in verts]; ys=[v[1] for v in verts]; zs=[v[2] for v in verts]
    cx=(min(xs)+max(xs))/2; cy_=(min(ys)+max(ys))/2; cz=(min(zs)+max(zs))/2
    span = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs))
    verts_c = [(v[0]-cx, v[1]-cy_, v[2]-cz) for v in verts]
    cam_dist = span * 1.6
    scale = 1.2

    rot_y = math.radians(35)
    rot_x = math.radians(-20)

    proj = [project(v, rot_y, rot_x, cam_dist, W, H, scale) for v in verts_c]

    fb = bytearray(W*H*4)  # RGBA, init transparent
    # Background gradient
    for y in range(H):
        t = y/H
        r = int(40 + 30*t); g = int(45 + 35*t); b = int(60 + 50*t)
        for x in range(W):
            o = (y*W + x)*4
            fb[o]=r; fb[o+1]=g; fb[o+2]=b; fb[o+3]=255

    zbuf = [1e9]*(W*H)

    tris_drawn = 0
    for poly in mesh.polygons:
        if len(poly['v_idx']) < 3: continue
        for tri_v, tri_uv in fan_triangulate(poly['v_idx'], poly['uv']):
            p0,p1,p2 = [proj[i] for i in tri_v]
            (u0,v0),(u1,v1),(u2,v2) = tri_uv
            # Bounding box
            minx=max(0,int(min(p0[0],p1[0],p2[0])))
            maxx=min(W-1,int(max(p0[0],p1[0],p2[0])))
            miny=max(0,int(min(p0[1],p1[1],p2[1])))
            maxy=min(H-1,int(max(p0[1],p1[1],p2[1])))
            if maxx<minx or maxy<miny: continue
            # Edge function denom
            denom = (p1[1]-p2[1])*(p0[0]-p2[0]) + (p2[0]-p1[0])*(p0[1]-p2[1])
            if abs(denom) < 1e-6: continue
            tris_drawn += 1
            for yy in range(miny, maxy+1):
                for xx in range(minx, maxx+1):
                    w0 = ((p1[1]-p2[1])*(xx-p2[0]) + (p2[0]-p1[0])*(yy-p2[1])) / denom
                    w1 = ((p2[1]-p0[1])*(xx-p2[0]) + (p0[0]-p2[0])*(yy-p2[1])) / denom
                    w2 = 1 - w0 - w1
                    if w0<0 or w1<0 or w2<0: continue
                    z = w0*p0[2] + w1*p1[2] + w2*p2[2]
                    o = yy*W + xx
                    if z >= zbuf[o]: continue
                    zbuf[o] = z
                    u = w0*u0 + w1*u1 + w2*u2
                    v = w0*v0 + w1*v1 + w2*v2
                    r,g,b,a = sample_tex(tex_pixels, tw, th, u, v)
                    # cheap directional shading from triangle normal
                    fb[o*4]=r; fb[o*4+1]=g; fb[o*4+2]=b; fb[o*4+3]=255
    img = Image.frombytes("RGBA", (W,H), bytes(fb))
    return img, tris_drawn


def render_wireframe(mesh, W=480, H=480):
    verts = mesh.vertices
    xs=[v[0] for v in verts]; ys=[v[1] for v in verts]; zs=[v[2] for v in verts]
    cx=(min(xs)+max(xs))/2; cy_=(min(ys)+max(ys))/2; cz=(min(zs)+max(zs))/2
    span = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs))
    verts_c=[(v[0]-cx,v[1]-cy_,v[2]-cz) for v in verts]
    rot_y=math.radians(35); rot_x=math.radians(-20)
    cam=span*1.6; sc=1.2
    pr=[project(v, rot_y, rot_x, cam, W, H, sc) for v in verts_c]
    img = Image.new("RGBA",(W,H),(20,25,35,255))
    d=ImageDraw.Draw(img)
    for poly in mesh.polygons:
        vs=poly['v_idx']
        if len(vs)<3: continue
        pts=[(pr[i][0],pr[i][1]) for i in vs]
        d.polygon(pts, outline=(180,210,255,255))
    return img


def compose(tex_img, wire_img, tex_render_img, info_lines, path):
    W = 1500; H = 540
    out = Image.new("RGBA", (W, H), (15,15,20,255))
    d = ImageDraw.Draw(out)

    # Texture panel (centered, 480x480)
    tex_panel = tex_img.convert("RGBA")
    if tex_panel.size != (480,480):
        tex_panel = tex_panel.resize((480,480), Image.NEAREST)
    out.paste(tex_panel, (10, 40))

    out.paste(wire_img, (510, 40))
    out.paste(tex_render_img, (1010, 40))

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)
        small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
    except Exception:
        font = ImageFont.load_default(); small = font
    d.text((10,10), "Canvas: firehydrant (decoded via omt_parse.OMTCanvasDecoder)", fill="white", font=font)
    d.text((510,10), "Mesh: fireHydrant01 wireframe", fill="white", font=font)
    d.text((1010,10), "Software-rasterized textured render", fill="white", font=font)
    y = 530
    for ln in info_lines:
        d.text((10, y), ln, fill=(200,210,220,255), font=small); y += 16
    out.save(path)


def main():
    parser = OMTFormatParser()
    quiet_open(parser, OMT_PATH)

    # Canvas
    ci, cchunk = find_chunk(parser, "firehydrant", "Canv")
    assert ci is not None, "no firehydrant Canv chunk"
    tex = extract_canvas(parser, ci)
    assert tex is not None, "canvas decode failed"
    tex.save(OUT_DIR / "awefan_proof_hydrant_tex.png")

    # Mesh
    mi, mchunk = find_chunk(parser, "fireHydrant01", "3DSh")
    assert mi is not None, "no fireHydrant01 3DSh chunk"
    mesh = extract_mesh(parser, mi)
    assert mesh is not None, "mesh parse failed"

    # OBJ export
    obj_path = OUT_DIR / "awefan_proof_hydrant_mesh.obj"
    export_obj(mesh, str(obj_path))

    info = [
        f"OMT file: {OMT_PATH}",
        f"Canvas chunk: id={cchunk['id']} size={cchunk['size']} → {tex.size[0]}x{tex.size[1]} RGBA",
        f"Mesh chunk: id={mchunk['id']} verts={len(mesh.vertices)} polys={len(mesh.polygons)} version={mesh.version}",
        f"Material links: {sum(1 for p in mesh.polygons if p.get('material_link'))}/{len(mesh.polygons)}",
    ]
    print("\n".join(info))

    wire = render_wireframe(mesh)
    textured, ntris = render_textured(mesh, tex)
    info.append(f"Triangles rasterized: {ntris}")
    print(info[-1])

    compose(tex, wire, textured, info, OUT_DIR / "awefan_proof_hydrant.png")
    print(f"\nwrote: {OUT_DIR / 'awefan_proof_hydrant.png'}")
    print(f"wrote: {OUT_DIR / 'awefan_proof_hydrant_tex.png'}")
    print(f"wrote: {obj_path}")


if __name__ == "__main__":
    main()
