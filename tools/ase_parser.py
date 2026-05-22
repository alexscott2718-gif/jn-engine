#!/usr/bin/env python3
"""ASE (3DS Max ASCII Scene Export v2.00) parser for Jimmy Neutron assets."""

import json, sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional

@dataclass
class Material:
    name: str = ""; diffuse: tuple = (0.5,0.5,0.5)
    ambient: tuple = (0.1,0.1,0.1); specular: tuple = (0.9,0.9,0.9)
    opacity: float = 1.0; texture_file: str = ""

@dataclass
class Vertex:
    x: float; y: float; z: float

@dataclass
class Face:
    a: int; b: int; c: int; mat_id: int = 0

@dataclass
class TexVert:
    u: float; v: float

@dataclass
class TexFace:
    a: int; b: int; c: int

@dataclass
class PosKey:
    time: int; x: float; y: float; z: float

@dataclass
class RotKey:
    time: int; ax: float; ay: float; az: float; angle: float

@dataclass
class ScaleKey:
    time: int; x: float; y: float; z: float

@dataclass
class Animation:
    pos_keys: list = field(default_factory=list)
    rot_keys: list = field(default_factory=list)
    scale_keys: list = field(default_factory=list)

@dataclass
class Mesh:
    name: str = ""; parent: str = ""; mat_index: int = -1
    vertices: list = field(default_factory=list)
    faces: list = field(default_factory=list)
    tex_verts: list = field(default_factory=list)
    tex_faces: list = field(default_factory=list)
    pivot: tuple = (0.0,0.0,0.0)
    tm_row0: tuple = (1.0,0.0,0.0); tm_row1: tuple = (0.0,1.0,0.0)
    tm_row2: tuple = (0.0,0.0,1.0); tm_row3: tuple = (0.0,0.0,0.0)
    animation: Optional[Animation] = None

@dataclass
class Scene:
    filename: str = ""; first_frame: int = 0; last_frame: int = 0
    frame_speed: int = 10; ticks_per_frame: int = 480
    materials: list = field(default_factory=list)
    meshes: list = field(default_factory=list)


def tokenize(text):
    tokens, i = [], 0
    while i < len(text):
        ch = text[i]
        if ch in ' \t\r\n': i += 1
        elif ch == '"':
            j = text.index('"', i+1); tokens.append(text[i+1:j]); i = j+1
        elif ch in '{}': tokens.append(ch); i += 1
        else:
            j = i
            while j < len(text) and text[j] not in ' \t\r\n{}"': j += 1
            tokens.append(text[i:j]); i = j
    return tokens


class Parser:
    def __init__(self, tokens):
        self.t = tokens; self.p = 0

    def peek(self): return self.t[self.p] if self.p < len(self.t) else None
    def next(self): v = self.t[self.p]; self.p += 1; return v
    def expect(self, v):
        got = self.next()
        if got != v: raise ValueError(f"Expected {v!r} got {got!r} at {self.p}")
    def flt(self): return float(self.next())
    def int_(self): return int(self.next())
    def v3(self): return (self.flt(), self.flt(), self.flt())

    def skip_block(self):
        # opening '{' already consumed by caller
        depth = 1
        while self.p < len(self.t):
            tok = self.next()
            if tok == '{': depth += 1
            elif tok == '}':
                depth -= 1
                if depth == 0: return

    def skip_val(self):
        if self.peek() == '{': self.next(); self.skip_block()
        elif self.peek() not in ('{', '}', None): self.next()

    def parse_map(self):
        fname = ""
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok == '*BITMAP': fname = Path(self.next().replace('\\','/')).name
            else: self.skip_val()
        self.expect('}')
        return fname

    def parse_material(self):
        m = Material()
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok == '*MATERIAL_NAME':           m.name = self.next()
            elif tok == '*MATERIAL_DIFFUSE':      m.diffuse = self.v3()
            elif tok == '*MATERIAL_AMBIENT':      m.ambient = self.v3()
            elif tok == '*MATERIAL_SPECULAR':     m.specular = self.v3()
            elif tok == '*MATERIAL_TRANSPARENCY': m.opacity = 1.0 - self.flt()
            elif tok == '*MAP_DIFFUSE':           m.texture_file = self.parse_map()
            elif self.peek() == '{':              self.next(); self.skip_block()
            else:                                 self.skip_val()
        self.expect('}')
        return m

    def parse_material_list(self):
        mats = []
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok == '*MATERIAL_COUNT':  self.int_()
            elif tok == '*MATERIAL':      self.int_(); mats.append(self.parse_material())
            elif self.peek() == '{':      self.next(); self.skip_block()
            else:                         self.skip_val()
        self.expect('}')
        return mats

    def parse_mesh_block(self, mesh):
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok in ('*MESH_NUMVERTEX','*MESH_NUMFACES',
                       '*MESH_NUMTVERTEX','*MESH_NUMTVFACES'): self.int_()
            elif tok == '*MESH_VERTEX_LIST':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*MESH_VERTEX': self.int_(); mesh.vertices.append(Vertex(*self.v3()))
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*MESH_FACE_LIST':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*MESH_FACE':
                        int(self.next().rstrip(':')); a=b=c=mat_id=0
                        while self.peek() not in ('}','*MESH_FACE',None):
                            lbl = self.next()
                            if lbl == 'A:':            a = self.int_()
                            elif lbl == 'B:':          b = self.int_()
                            elif lbl == 'C:':          c = self.int_()
                            elif lbl == '*MESH_MTLID': mat_id = self.int_()
                            elif lbl in ('AB:','BC:','CA:'): self.int_()
                            elif lbl == '*MESH_SMOOTHING': self.next()
                            else: self.skip_val()
                        mesh.faces.append(Face(a,b,c,mat_id))
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*MESH_TVERTLIST':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*MESH_TVERT':
                        self.int_(); u=self.flt(); v=self.flt(); self.flt()
                        mesh.tex_verts.append(TexVert(u,v))
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*MESH_TFACELIST':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*MESH_TFACE':
                        self.int_()
                        mesh.tex_faces.append(TexFace(self.int_(),self.int_(),self.int_()))
                    else: self.skip_val()
                self.expect('}')
            elif self.peek() == '{': self.next(); self.skip_block()
            else: self.skip_val()
        self.expect('}')

    def parse_anim(self):
        anim = Animation()
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok == '*CONTROL_POS_TRACK':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*CONTROL_POS_SAMPLE':
                        tm=self.int_(); x,y,z=self.v3()
                        anim.pos_keys.append(PosKey(tm,x,y,z))
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*CONTROL_ROT_TRACK':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*CONTROL_ROT_SAMPLE':
                        tm=self.int_()
                        ax,ay,az,ang=self.flt(),self.flt(),self.flt(),self.flt()
                        anim.rot_keys.append(RotKey(tm,ax,ay,az,ang))
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*CONTROL_SCALE_TRACK':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*CONTROL_SCALE_SAMPLE':
                        tm=self.int_(); x,y,z=self.v3()
                        anim.scale_keys.append(ScaleKey(tm,x,y,z))
                        if self.peek() not in ('}','*CONTROL_SCALE_SAMPLE',None):
                            self.v3(); self.flt()
                    else: self.skip_val()
                self.expect('}')
            elif self.peek() == '{': self.next(); self.skip_block()
            else: self.skip_val()
        self.expect('}')
        return anim

    def parse_geomobject(self):
        mesh = Mesh()
        self.expect('{')
        while self.peek() != '}':
            tok = self.next()
            if tok == '*NODE_NAME':      mesh.name = self.next()
            elif tok == '*NODE_PARENT':  mesh.parent = self.next()
            elif tok == '*MATERIAL_REF': mesh.mat_index = self.int_()
            elif tok == '*NODE_TM':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*TM_ROW0':   mesh.tm_row0 = self.v3()
                    elif t == '*TM_ROW1': mesh.tm_row1 = self.v3()
                    elif t == '*TM_ROW2': mesh.tm_row2 = self.v3()
                    elif t == '*TM_ROW3': mesh.tm_row3 = self.v3()
                    elif t == '*TM_POS':  mesh.pivot = self.v3()
                    else: self.skip_val()
                self.expect('}')
            elif tok == '*MESH':         self.parse_mesh_block(mesh)
            elif tok == '*TM_ANIMATION': mesh.animation = self.parse_anim()
            elif self.peek() == '{':     self.next(); self.skip_block()
            else:                        self.skip_val()
        self.expect('}')
        return mesh

    def parse(self):
        scene = Scene()
        while self.p < len(self.t):
            tok = self.next()
            if tok == '*3DSMAX_ASCIIEXPORT': self.next()
            elif tok == '*COMMENT':          self.next()
            elif tok == '*SCENE':
                self.expect('{')
                while self.peek() != '}':
                    t = self.next()
                    if t == '*SCENE_FILENAME':        scene.filename = self.next()
                    elif t == '*SCENE_FIRSTFRAME':    scene.first_frame = self.int_()
                    elif t == '*SCENE_LASTFRAME':     scene.last_frame = self.int_()
                    elif t == '*SCENE_FRAMESPEED':    scene.frame_speed = self.int_()
                    elif t == '*SCENE_TICKSPERFRAME': scene.ticks_per_frame = self.int_()
                    elif self.peek() == '{':          self.next(); self.skip_block()
                    else:                             self.skip_val()
                self.expect('}')
            elif tok == '*MATERIAL_LIST':
                scene.materials = self.parse_material_list()
            elif tok in ('*GEOMOBJECT','*HELPEROBJECT'):
                scene.meshes.append(self.parse_geomobject())
            elif self.peek() == '{': self.next(); self.skip_block()
        return scene


def parse_ase(path) -> Scene:
    text = Path(path).read_text(encoding='latin-1', errors='replace')
    return Parser(tokenize(text)).parse()

def scene_to_dict(s: Scene) -> dict:
    def ad(a):
        if not a: return None
        return {'pos_keys':   [{'t':k.time,'x':k.x,'y':k.y,'z':k.z} for k in a.pos_keys],
                'rot_keys':   [{'t':k.time,'ax':k.ax,'ay':k.ay,'az':k.az,'angle':k.angle} for k in a.rot_keys],
                'scale_keys': [{'t':k.time,'x':k.x,'y':k.y,'z':k.z} for k in a.scale_keys]}
    return {
        'source': s.filename,
        'frames': {'first':s.first_frame,'last':s.last_frame,'fps':s.frame_speed,'ticks':s.ticks_per_frame},
        'materials': [{'name':m.name,'diffuse':list(m.diffuse),'opacity':m.opacity,'texture':m.texture_file} for m in s.materials],
        'meshes': [{'name':m.name,'parent':m.parent,'mat_index':m.mat_index,
                    'pivot':list(m.pivot),
                    'transform':[list(m.tm_row0),list(m.tm_row1),list(m.tm_row2),list(m.tm_row3)],
                    'vertices':[[v.x,v.y,v.z] for v in m.vertices],
                    'faces':[{'a':f.a,'b':f.b,'c':f.c,'mat':f.mat_id} for f in m.faces],
                    'tex_verts':[[t.u,t.v] for t in m.tex_verts],
                    'tex_faces':[{'a':f.a,'b':f.b,'c':f.c} for f in m.tex_faces],
                    'animation':ad(m.animation)} for m in s.meshes]}

def export_obj(scene: Scene, out_path: str):
    lines = ['# jn-engine ASE export']; off = 1
    for m in scene.meshes:
        if not m.vertices: continue
        lines.append(f'o {m.name}')
        for v in m.vertices: lines.append(f'v {v.x:.6f} {v.z:.6f} {-v.y:.6f}')
        for tv in m.tex_verts: lines.append(f'vt {tv.u:.6f} {1.0-tv.v:.6f}')
        if m.tex_faces and len(m.tex_faces) == len(m.faces):
            for f,tf in zip(m.faces,m.tex_faces):
                lines.append(f'f {f.a+off}/{tf.a+off} {f.b+off}/{tf.b+off} {f.c+off}/{tf.c+off}')
        else:
            for f in m.faces: lines.append(f'f {f.a+off} {f.b+off} {f.c+off}')
        off += len(m.vertices)
    Path(out_path).write_text('\n'.join(lines))

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: ase_parser.py <file.ASE> [--obj out.obj] [--json out.json]")
        print("       ase_parser.py --all <ase_dir> <out_dir>")
        sys.exit(1)
    if sys.argv[1] == '--all':
        ase_dir, out_dir = Path(sys.argv[2]), Path(sys.argv[3])
        out_dir.mkdir(parents=True, exist_ok=True)
        ok = err = 0
        for f in sorted(list(ase_dir.glob('*.ASE')) + list(ase_dir.glob('*.ase'))):
            try:
                scene = parse_ase(f)
                (out_dir/(f.stem+'.json')).write_text(json.dumps(scene_to_dict(scene),indent=2))
                ok += 1
            except Exception as e:
                print(f"  ERROR {f.name}: {e}"); err += 1
        print(f"Parsed {ok} ASE, {err} errors → {out_dir}"); sys.exit(0)

    scene = parse_ase(sys.argv[1])
    obj_out = json_out = None; i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--obj' and i+1 < len(sys.argv):    obj_out = sys.argv[i+1]; i += 2
        elif sys.argv[i] == '--json' and i+1 < len(sys.argv): json_out = sys.argv[i+1]; i += 2
        else: i += 1
    if obj_out:  export_obj(scene, obj_out)
    if json_out: Path(json_out).write_text(json.dumps(scene_to_dict(scene),indent=2))
    print(f"Source:    {scene.filename}")
    print(f"Frames:    {scene.first_frame}-{scene.last_frame} @ {scene.frame_speed}fps")
    print(f"Materials: {len(scene.materials)}")
    for m in scene.materials: print(f"  [{m.name}] tex={m.texture_file or '(none)'}")
    print(f"Meshes:    {len(scene.meshes)}")
    for m in scene.meshes:
        anim = f" anim={len(m.animation.pos_keys)}pk/{len(m.animation.rot_keys)}rk" if m.animation else ""
        print(f"  [{m.name}] verts={len(m.vertices)} faces={len(m.faces)}{anim}")
