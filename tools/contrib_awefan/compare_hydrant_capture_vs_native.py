"""Recreate the firehydrant from captured D3D7 draws and compare to the native
OMT mesh + Canvas.

Outputs a side-by-side at /tmp/hydrant_capture_vs_native.png:
  [native canvas] [native mesh wireframe] [native textured]
  [capture tex  ] [capture geom wireframe] [capture textured]

Pipeline:
  1. Match awefan's Canvas decode against ~/jn-engine/assets/parsed/level1/
     level1_images/*.png to find the captured tex_id (done in survey:
     0028_64x64d16.png == tex_id 03d0c470).
  2. Walk build/frame16565.omtc:
       - track current SET_TEXTURE on stage 0
       - track current world transform via SET_TRANSFORM(which=1)
       - collect DRAW_PRIMITIVE records that bind the target tex_id
  3. Each captured vertex is FVF 0x152 (x,y,z, nx,ny,nz, diffuse, u,v); take
     them as model-space and apply the recorded world matrix to get
     world-space positions.
  4. Render with the same software rasterizer used for the native mesh,
     reusing the SAME captured texture (the texture exactly matches awefan's
     Canvas decode, verified pixel-identical).
"""
import sys, types, io, math, struct
from pathlib import Path

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(Path.home() / "jn-engine/instrument/receiver"))
sys.path.insert(0, str(Path.home() / "jn-engine/instrument/diff"))

def _stub():
    pkg = types.ModuleType('PySide6'); pkg.__path__=[]
    sys.modules['PySide6']=pkg
    for s in ('QtCore','QtGui','QtWidgets','QtMultimedia','QtMultimediaWidgets'):
        m=types.ModuleType(f'PySide6.{s}'); m.__getattr__=lambda n: type(n,(),{})
        sys.modules[f'PySide6.{s}']=m
_stub()

from omt_parse import OMTFormatParser, OMTCanvasDecoder  # awefan
from omt_3d import parse_3dsh_bytes  # awefan
from protocol import (Header, try_parse_record, RECORD_SET_TEXTURE,
    RECORD_SET_TRANSFORM, RECORD_DRAW_PRIMITIVE, RECORD_FRAME_BEGIN,
    RECORD_FRAME_END, SetTexture, SetTransform, DrawPrimitive)
from PIL import Image, ImageDraw, ImageFont

TARGET_TEX = 0x00183f68    # jhouse2 (verified pixel-identical to native Canvas)
NATIVE_CANV = 'jhouse2'
NATIVE_MESH = 'JHOUSE'     # try JHOUSE first; falls back to jhouse01 if no draws bind it
NATIVE_MESH_FALLBACK = 'jhouse01'
OMTC = Path.home() / "jn-engine/build/frame16565.omtc"
OMT  = Path.home() / "jn-engine/assets/omt/level1.omt"
OUT  = Path("/tmp/jhouse2_capture_vs_native.png")
CAPTURE_TEX_PNG = Path("/home/scotty/jn-engine/assets/parsed/level1/level1_images/0045_256x256d16.png")


# ---- capture walker ---------------------------------------------------------

def walk_capture_for_tex(path: Path, target_tex_id: int):
    """Return list of (world_matrix_4x4, vertices) for every DRAW_PRIMITIVE
    that was issued while target_tex_id was bound on stage 0."""
    data = path.read_bytes()
    hdr = Header.unpack(data[:Header.size()])
    cur_tex = {}              # stage -> tex_id
    transforms = {1: identity4()}  # 1 = D3DTS_WORLD
    out = []
    off = Header.size()
    while off < len(data):
        r = try_parse_record(data, off)
        if r is None: break
        rt, payload, next_off = r
        off = next_off
        if rt == RECORD_SET_TEXTURE:
            st = SetTexture.unpack(payload)
            cur_tex[st.stage] = st.tex_id
        elif rt == RECORD_SET_TRANSFORM:
            stt = SetTransform.unpack(payload)
            transforms[stt.which] = list(stt.matrix)
        elif rt == RECORD_DRAW_PRIMITIVE:
            if cur_tex.get(0) == target_tex_id:
                dp = DrawPrimitive.unpack(payload)
                world = transforms.get(1, identity4())
                out.append((world, dp.prim_type, dp.vertices))
    return out


def identity4():
    return [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]


def mul_vec4_mat4(v, m):
    """v is (x,y,z,1); m is row-major 16 floats. Returns (x',y',z',w')."""
    x,y,z,w = v
    return (
        m[0]*x + m[4]*y + m[8]*z  + m[12]*w,
        m[1]*x + m[5]*y + m[9]*z  + m[13]*w,
        m[2]*x + m[6]*y + m[10]*z + m[14]*w,
        m[3]*x + m[7]*y + m[11]*z + m[15]*w,
    )


# ---- rasterizer (lifted from render_hydrant.py) ----------------------------

def project(v, rot_y, rot_x, cam_dist, view_w, view_h, scale):
    cy, sy = math.cos(rot_y), math.sin(rot_y)
    cx, sx = math.cos(rot_x), math.sin(rot_x)
    x, y, z = v
    x2 = cy*x + sy*z; z2 = -sy*x + cy*z
    y3 = cx*y - sx*z2; z3 = sx*y + cx*z2
    w = z3 + cam_dist
    if w < 0.1: w = 0.1
    return view_w*0.5 + (x2*scale/w)*view_w*0.5, view_h*0.5 - (y3*scale/w)*view_h*0.5, w


def sample_tex(px, tw, th, u, v):
    u = u - math.floor(u); v = v - math.floor(v)
    x = int(u*(tw-1)); y = int(v*(th-1))  # D3D V=0 at top (no flip)
    return px[y*tw + x]


def render_tris(verts_xyz, tris_idx_uv, tex_img, W=480, H=480, label=""):
    tex = tex_img.convert("RGBA")
    tw, th = tex.size
    px = list(tex.getdata())

    xs = [v[0] for v in verts_xyz]; ys=[v[1] for v in verts_xyz]; zs=[v[2] for v in verts_xyz]
    cx=(min(xs)+max(xs))/2; cy_=(min(ys)+max(ys))/2; cz=(min(zs)+max(zs))/2
    span = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs)) or 1.0
    centered = [(v[0]-cx, v[1]-cy_, v[2]-cz) for v in verts_xyz]
    cam=span*1.6; sc=1.2
    rot_y=math.radians(35); rot_x=math.radians(-20)
    proj = [project(v, rot_y, rot_x, cam, W, H, sc) for v in centered]

    fb = bytearray(W*H*4)
    for y in range(H):
        t=y/H; r=int(40+30*t); g=int(45+35*t); b=int(60+50*t)
        for x in range(W):
            o=(y*W+x)*4
            fb[o]=r;fb[o+1]=g;fb[o+2]=b;fb[o+3]=255
    zbuf=[1e9]*(W*H)

    drawn=0
    for (i0,i1,i2),(uv0,uv1,uv2) in tris_idx_uv:
        if i0>=len(proj) or i1>=len(proj) or i2>=len(proj): continue
        p0,p1,p2 = proj[i0],proj[i1],proj[i2]
        u0,v0=uv0; u1,v1=uv1; u2,v2=uv2
        minx=max(0,int(min(p0[0],p1[0],p2[0])))
        maxx=min(W-1,int(max(p0[0],p1[0],p2[0])))
        miny=max(0,int(min(p0[1],p1[1],p2[1])))
        maxy=min(H-1,int(max(p0[1],p1[1],p2[1])))
        if maxx<minx or maxy<miny: continue
        denom = (p1[1]-p2[1])*(p0[0]-p2[0]) + (p2[0]-p1[0])*(p0[1]-p2[1])
        if abs(denom)<1e-6: continue
        drawn+=1
        for yy in range(miny, maxy+1):
            for xx in range(minx, maxx+1):
                w0 = ((p1[1]-p2[1])*(xx-p2[0]) + (p2[0]-p1[0])*(yy-p2[1])) / denom
                w1 = ((p2[1]-p0[1])*(xx-p2[0]) + (p0[0]-p2[0])*(yy-p2[1])) / denom
                w2 = 1 - w0 - w1
                if w0<0 or w1<0 or w2<0: continue
                z = w0*p0[2] + w1*p1[2] + w2*p2[2]
                o = yy*W + xx
                if z >= zbuf[o]: continue
                zbuf[o]=z
                u = w0*u0 + w1*u1 + w2*u2
                v = w0*v0 + w1*v1 + w2*v2
                r,g,b,a = sample_tex(px, tw, th, u, v)
                fb[o*4]=r;fb[o*4+1]=g;fb[o*4+2]=b;fb[o*4+3]=255
    img = Image.frombytes("RGBA",(W,H),bytes(fb))
    return img, drawn


def render_wire(verts_xyz, tris_idx, W=480, H=480, color=(180,210,255,255)):
    xs=[v[0] for v in verts_xyz]; ys=[v[1] for v in verts_xyz]; zs=[v[2] for v in verts_xyz]
    cx=(min(xs)+max(xs))/2; cy_=(min(ys)+max(ys))/2; cz=(min(zs)+max(zs))/2
    span = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs)) or 1.0
    centered=[(v[0]-cx,v[1]-cy_,v[2]-cz) for v in verts_xyz]
    cam=span*1.6; sc=1.2; rot_y=math.radians(35); rot_x=math.radians(-20)
    proj=[project(v, rot_y, rot_x, cam, W, H, sc) for v in centered]
    img = Image.new("RGBA",(W,H),(20,25,35,255))
    d = ImageDraw.Draw(img)
    for i0,i1,i2 in tris_idx:
        if max(i0,i1,i2) >= len(proj): continue
        d.polygon([(proj[i0][0],proj[i0][1]),
                   (proj[i1][0],proj[i1][1]),
                   (proj[i2][0],proj[i2][1])], outline=color)
    return img


# ---- native side -----------------------------------------------------------

def native_pieces():
    parser = OMTFormatParser()
    saved=sys.stdout; sys.stdout=io.StringIO()
    parser.open_file(str(OMT)); sys.stdout=saved
    # canvas
    tex = None
    for i,c in enumerate(parser.chunks):
        if c.get('name')==NATIVE_CANV and c.get('type')=='Canv':
            tex = OMTCanvasDecoder.decode_canvas(parser.get_chunk_data(i)); break
    # mesh — try primary, then fallback
    mesh = None
    for target in (NATIVE_MESH, NATIVE_MESH_FALLBACK):
        for i,c in enumerate(parser.chunks):
            if c.get('name')==target and c.get('type')=='3DSh':
                mesh,_ = parse_3dsh_bytes(parser.get_chunk_data(i), 0, '>')
                if mesh:
                    print(f"  using native 3DSh: {target}")
                    break
        if mesh: break
    verts = [(v[0],v[1],v[2]) for v in mesh.vertices]
    tris = []  # ((i0,i1,i2),(uv0,uv1,uv2))
    tris_idx = []
    for p in mesh.polygons:
        vi = p['v_idx']; uvs = p['uv']
        for k in range(1, len(vi)-1):
            tris.append(((vi[0],vi[k],vi[k+1]), (uvs[0],uvs[k],uvs[k+1])))
            tris_idx.append((vi[0],vi[k],vi[k+1]))
    return tex, verts, tris, tris_idx


# ---- capture side ----------------------------------------------------------

def capture_pieces():
    draws = walk_capture_for_tex(OMTC, TARGET_TEX)
    print(f"  capture: {len(draws)} draw call(s) bind tex_id {TARGET_TEX:#010x}")
    # Concatenate all vertices, applying their world matrix.
    verts = []
    tris = []
    tris_idx = []
    base = 0
    for world, prim_type, vlist in draws:
        # prim_type: D3DPT_TRIANGLELIST=4, TRIANGLESTRIP=5, TRIANGLEFAN=6
        wv = []
        for v in vlist:
            x,y,z,w = mul_vec4_mat4((v.x, v.y, v.z, 1.0), world)
            wv.append((x,y,z))
            verts.append((x,y,z))
        if prim_type == 4:  # TRIANGLELIST
            for k in range(0, len(vlist), 3):
                if k+2 >= len(vlist): break
                tris.append(((base+k,base+k+1,base+k+2),
                             ((vlist[k].u,vlist[k].v),
                              (vlist[k+1].u,vlist[k+1].v),
                              (vlist[k+2].u,vlist[k+2].v))))
                tris_idx.append((base+k,base+k+1,base+k+2))
        elif prim_type == 5:  # TRIANGLESTRIP
            for k in range(len(vlist)-2):
                a,b_,c = (k,k+1,k+2) if k%2==0 else (k+1,k,k+2)
                tris.append(((base+a,base+b_,base+c),
                             ((vlist[a].u,vlist[a].v),
                              (vlist[b_].u,vlist[b_].v),
                              (vlist[c].u,vlist[c].v))))
                tris_idx.append((base+a,base+b_,base+c))
        elif prim_type == 6:  # TRIANGLEFAN
            for k in range(1, len(vlist)-1):
                tris.append(((base,base+k,base+k+1),
                             ((vlist[0].u,vlist[0].v),
                              (vlist[k].u,vlist[k].v),
                              (vlist[k+1].u,vlist[k+1].v))))
                tris_idx.append((base,base+k,base+k+1))
        base += len(vlist)
    print(f"  capture: {len(verts)} verts, {len(tris)} tris assembled")
    return verts, tris, tris_idx


def compose(panels, labels, info, path):
    pad = 10; cell_w = 480; cell_h = 480
    cols = 3; rows = 2
    W = pad + cols*(cell_w+pad); H = pad + 40 + rows*(cell_h+40)
    out = Image.new("RGBA",(W,H),(15,15,20,255))
    d = ImageDraw.Draw(out)
    try:
        f_big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16)
        f_sm  = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    except:
        f_big=f_sm=ImageFont.load_default()
    for r in range(rows):
        for c in range(cols):
            x = pad + c*(cell_w+pad); y = 30 + r*(cell_h+40)
            idx = r*cols + c
            if idx >= len(panels): continue
            img = panels[idx]
            if img.size != (cell_w, cell_h):
                img = img.resize((cell_w, cell_h), Image.NEAREST)
            out.paste(img, (x, y+20))
            d.text((x, y), labels[idx], fill="white", font=f_big)
    yi = H - 20*len(info) - 10
    for ln in info:
        d.text((10, yi), ln, fill=(180,200,220,255), font=f_sm); yi += 18
    out.save(path)


def main():
    print("=== native ===")
    n_tex, n_verts, n_tris, n_tris_idx = native_pieces()
    print(f"  native: {len(n_verts)} verts, {len(n_tris)} tris, canvas {n_tex.size}")
    n_wire = render_wire(n_verts, n_tris_idx)
    n_img, n_drawn = render_tris(n_verts, n_tris, n_tex, label="native")
    print(f"  native rasterized: {n_drawn} tris")

    print("=== capture ===")
    c_verts, c_tris, c_tris_idx = capture_pieces()
    if not c_verts:
        print("  NO CAPTURE DRAWS BIND THIS TEXTURE — frame16565 doesn't show the hydrant")
        c_tex = Image.open(CAPTURE_TEX_PNG)
        c_wire = Image.new("RGBA",(480,480),(20,25,35,255))
        d = ImageDraw.Draw(c_wire)
        try: ff = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
        except: ff = ImageFont.load_default()
        d.text((20, 220), f"tex_id 0x{TARGET_TEX:08x}\nnot drawn in this captured frame", fill=(220,180,180,255), font=ff)
        c_img = c_wire.copy()
        info_extra = [f"capture frame does not draw tex_id 0x{TARGET_TEX:08x}"]
    else:
        c_tex = Image.open(CAPTURE_TEX_PNG)
        c_wire = render_wire(c_verts, c_tris_idx, color=(255,200,180,255))
        c_img, c_drawn = render_tris(c_verts, c_tris, c_tex)
        info_extra = [f"capture: {len(c_verts)} verts, {len(c_tris)} tris, {c_drawn} rasterized"]

    panels = [n_tex, n_wire, n_img, c_tex, c_wire, c_img]
    labels = [f"NATIVE — Canvas '{NATIVE_CANV}' (awefan decoder)",
              f"NATIVE — '{NATIVE_MESH}/{NATIVE_MESH_FALLBACK}' wire",
              "NATIVE — textured render",
              f"CAPTURE — tex_id 0x{TARGET_TEX:08x}",
              "CAPTURE — wire (world-space draws)",
              "CAPTURE — textured render"]
    info = [
        f"OMT:     {OMT}",
        f"omtc:    {OMTC}",
        f"target tex_id: 0x{TARGET_TEX:08x}  (pixel-identical to native Canvas firehydrant)",
        f"native:  {len(n_verts)} verts, {len(n_tris)} tris",
    ] + info_extra
    compose(panels, labels, info, OUT)
    print(f"wrote: {OUT}")


if __name__ == "__main__":
    main()
