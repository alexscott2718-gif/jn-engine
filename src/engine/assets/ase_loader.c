#include "ase_loader.h"
#include "../glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VERTS  65536
#define MAX_FACES  65536

typedef struct { float x, y, z; } V3;
typedef struct { float u, v; }    V2;
typedef struct { int a, b, c; }   Tri;

static V3  s_pos[MAX_VERTS];
static V3  s_vnorm[MAX_VERTS];   /* per-vertex normals keyed by position index */
static V2  s_uv[MAX_VERTS];
static Tri s_faces[MAX_FACES];
static Tri s_tfaces[MAX_FACES];
static int s_face_mtlid[MAX_FACES];     /* MTLID per face (0 if none) */
static int s_face_vnorm[MAX_FACES][3];  /* normal index for each face corner */

/* Per-material parse state: bitmap filename + diffuse color harvested while
   inside a *MATERIAL block. */
static char  s_mat_tex[ASE_MAX_MATERIALS][256];
static float s_mat_diffuse[ASE_MAX_MATERIALS][3];
static int   s_mat_count = 0;

/* Output: interleaved pos(3) + uv(2) + normal(3) = 8 floats per vertex */
static float        s_vbuf[MAX_FACES * 3 * 8];
static unsigned int s_ibuf[MAX_FACES * 3];

static char *skip_ws(char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}
static char *next_tok(char **src) {
    char *p = skip_ws(*src);
    if (!*p) return NULL;
    char *start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    if (*p) { *p = '\0'; p++; }
    *src = p;
    return start;
}
/* Read a full double-quoted string (may contain spaces). Returns pointer to
   content inside the quotes; advances *src past the closing quote. If the
   first non-whitespace char isn't a quote, falls back to next_tok. */
static char *next_quoted(char **src) {
    char *p = skip_ws(*src);
    if (*p != '"') return next_tok(src);
    p++; /* skip opening quote */
    char *start = p;
    while (*p && *p != '"' && *p != '\r' && *p != '\n') p++;
    if (*p == '"') { *p = '\0'; p++; }
    *src = p;
    return start;
}

/* Extract basename from Windows path ("D:\Jimmy\car.png" -> "car.png").
   Exception: paths that already begin with "assets/" are repo-relative and
   passed through verbatim — used by tools/omt_mesh_export.py output. */
static void basename_of(const char *src, char *dst, size_t dstlen) {
    if (*src == '"') src++;
    if (strncmp(src, "assets/", 7) == 0) {
        snprintf(dst, dstlen, "%s", src);
    } else {
        const char *last = src;
        for (const char *p = src; *p; p++)
            if (*p == '\\' || *p == '/') last = p + 1;
        snprintf(dst, dstlen, "%s", last);
    }
    size_t n = strlen(dst);
    if (n > 0 && dst[n-1] == '"') dst[n-1] = '\0';
}

int ase_load(AseModel *m, const char *path) {
    memset(m, 0, sizeof(*m));

    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "ase_load: cannot open %s\n", path); return 0; }

    int  n_pos=0, n_uv=0, n_face=0, n_tface=0;
    int  in_vlist=0, in_flist=0, in_tvert=0, in_tface=0, in_normals=0;
    int  in_mlist=0;            /* inside *MATERIAL_LIST { ... } */
    int  mat_depth=0;           /* brace depth inside MATERIAL_LIST (1 at list level, 2 inside a *MATERIAL) */
    int  cur_mat_idx = -1;      /* current submaterial being parsed */
    int  cur_face_ni = -1;      /* current face index while parsing normals */
    int  cur_vnorm_count = 0;
    int  geomobject_mat_ref = -1; /* MATERIAL_REF from the GEOMOBJECT block */

    s_mat_count = 0;
    for (int i = 0; i < ASE_MAX_MATERIALS; i++) {
        s_mat_tex[i][0] = '\0';
        s_mat_diffuse[i][0] = 1.0f;
        s_mat_diffuse[i][1] = 1.0f;
        s_mat_diffuse[i][2] = 1.0f;
    }

    float bmin[3]={ 1e30f, 1e30f, 1e30f}, bmax[3]={-1e30f,-1e30f,-1e30f};

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        /* Count braces in the raw line before tokenization mangles it. */
        int n_open=0, n_close=0;
        for (char *q = line; *q; q++) {
            if (*q == '{') n_open++;
            else if (*q == '}') n_close++;
        }

        char *p = skip_ws(line);
        char *tok = next_tok(&p);

        /* Apply brace deltas to material-list depth regardless of token. */
        int prev_mat_depth = mat_depth;
        if (in_mlist) {
            mat_depth += n_open - n_close;
            if (mat_depth < 0) mat_depth = 0;
            if (prev_mat_depth >= 2 && mat_depth < 2) cur_mat_idx = -1;
            if (mat_depth == 0) in_mlist = 0;
        }

        if (!tok) continue;

        /* Section entry/exit */
        if (strcmp(tok, "*MESH_VERTEX_LIST")  == 0) { in_vlist=1; in_flist=in_tvert=in_tface=in_normals=0; }
        else if (strcmp(tok, "*MESH_FACE_LIST")    == 0) { in_flist=1; in_vlist=in_tvert=in_tface=in_normals=0; }
        else if (strcmp(tok, "*MESH_TVERTLIST")    == 0) { in_tvert=1; in_vlist=in_flist=in_tface=in_normals=0; }
        else if (strcmp(tok, "*MESH_TFACELIST")    == 0) { in_tface=1; in_vlist=in_flist=in_tvert=in_normals=0; }
        else if (strcmp(tok, "*MESH_NORMALS")      == 0) { in_normals=1; in_vlist=in_flist=in_tvert=in_tface=0; cur_face_ni=-1; }
        else if (strcmp(tok, "*MATERIAL_LIST")     == 0) {
            in_mlist = 1;
            mat_depth = n_open - n_close;   /* opens to 1 in normal ASE files */
            if (mat_depth < 0) mat_depth = 0;
            cur_mat_idx = -1;
        }
        else if (tok[0] == '}') {
            in_vlist=in_flist=in_tvert=in_tface=in_normals=0;
            cur_face_ni=-1;
        }

        /* New submaterial inside MATERIAL_LIST. Form: "*MATERIAL <n> {".
           We only accept these at mat_depth == 2 (just opened) so that
           "*MATERIAL_CLASS" etc. don't trip the check. */
        else if (in_mlist && mat_depth >= 2 && strcmp(tok, "*MATERIAL") == 0) {
            char *idxs = next_tok(&p);
            if (idxs) {
                cur_mat_idx = atoi(idxs);
                if (cur_mat_idx >= 0 && cur_mat_idx < ASE_MAX_MATERIALS) {
                    if (cur_mat_idx + 1 > s_mat_count) s_mat_count = cur_mat_idx + 1;
                }
            }
        }

        /* Diffuse/ambient color inside a *MATERIAL block. */
        else if (in_mlist && cur_mat_idx >= 0 && cur_mat_idx < ASE_MAX_MATERIALS &&
                 strcmp(tok, "*MATERIAL_DIFFUSE") == 0) {
            char *rs = next_tok(&p), *gs = next_tok(&p), *bs = next_tok(&p);
            if (rs && gs && bs) {
                s_mat_diffuse[cur_mat_idx][0] = strtof(rs, NULL);
                s_mat_diffuse[cur_mat_idx][1] = strtof(gs, NULL);
                s_mat_diffuse[cur_mat_idx][2] = strtof(bs, NULL);
            }
        }

        /* Texture bitmap. Track into the current material slot if we have one;
           legacy single-bitmap field gets filled from material 0 later. */
        else if (strcmp(tok, "*BITMAP") == 0) {
            char *quoted = next_quoted(&p);
            if (quoted) {
                if (in_mlist && cur_mat_idx >= 0 && cur_mat_idx < ASE_MAX_MATERIALS) {
                    if (s_mat_tex[cur_mat_idx][0] == '\0')
                        basename_of(quoted, s_mat_tex[cur_mat_idx], sizeof(s_mat_tex[cur_mat_idx]));
                } else if (m->tex_file[0] == '\0') {
                    /* Stray bitmap outside MATERIAL_LIST — keep prior behavior. */
                    basename_of(quoted, m->tex_file, sizeof(m->tex_file));
                }
            }
        }

        /* Vertices */
        else if (in_vlist && strcmp(tok, "*MESH_VERTEX") == 0 && n_pos < MAX_VERTS) {
            next_tok(&p);
            float x=strtof(next_tok(&p),NULL), y=strtof(next_tok(&p),NULL), z=strtof(next_tok(&p),NULL);
            s_pos[n_pos++] = (V3){x, z, -y}; /* Max → GL coord */
        }

        /* Faces */
        else if (in_flist && strcmp(tok, "*MESH_FACE") == 0 && n_face < MAX_FACES) {
            next_tok(&p);
            int a=0,b=0,c=0,mtlid=0;
            char *lbl;
            while ((lbl=next_tok(&p))!=NULL) {
                if      (strcmp(lbl,"A:")==0) a=atoi(next_tok(&p));
                else if (strcmp(lbl,"B:")==0) b=atoi(next_tok(&p));
                else if (strcmp(lbl,"C:")==0) c=atoi(next_tok(&p));
                else if (strcmp(lbl,"AB:")==0||strcmp(lbl,"BC:")==0||strcmp(lbl,"CA:")==0) next_tok(&p);
                else if (strcmp(lbl,"*MESH_SMOOTHING")==0) next_tok(&p);
                else if (strcmp(lbl,"*MESH_MTLID")==0) mtlid=atoi(next_tok(&p));
            }
            s_faces[n_face] = (Tri){a,b,c};
            s_face_mtlid[n_face] = mtlid;
            n_face++;
        }

        /* GEOMOBJECT material reference — determines which top-level material owns
           this mesh. For Standard materials this overrides per-face MTLID (see
           the post-parse collapse below). */
        else if (strcmp(tok, "*MATERIAL_REF") == 0) {
            char *ns = next_tok(&p);
            if (ns) geomobject_mat_ref = atoi(ns);
        }

        /* UV verts. ASE/3DSMax stores V bottom-up (v=0 = bottom of texture);
           tex_loader loads textures bottom-up via stbi vertical flip, so V is
           passed through unmodified — the two conventions already agree. */
        else if (in_tvert && strcmp(tok,"*MESH_TVERT")==0 && n_uv < MAX_VERTS) {
            next_tok(&p);
            float u=strtof(next_tok(&p),NULL), v=strtof(next_tok(&p),NULL);
            s_uv[n_uv++]=(V2){u, v};
        }

        /* UV faces */
        else if (in_tface && strcmp(tok,"*MESH_TFACE")==0 && n_tface < MAX_FACES) {
            next_tok(&p);
            int a=atoi(next_tok(&p)),b=atoi(next_tok(&p)),c=atoi(next_tok(&p));
            s_tfaces[n_tface++]=(Tri){a,b,c};
        }

        /* Normals */
        else if (in_normals && strcmp(tok,"*MESH_FACENORMAL")==0) {
            cur_face_ni = atoi(next_tok(&p));
            /* face normal - read and discard (we use per-vertex normals) */
            cur_vnorm_count = 0;
        }
        else if (in_normals && strcmp(tok,"*MESH_VERTEXNORMAL")==0 && cur_face_ni >= 0 && cur_face_ni < MAX_FACES) {
            int vi = atoi(next_tok(&p));
            float nx=strtof(next_tok(&p),NULL), ny=strtof(next_tok(&p),NULL), nz=strtof(next_tok(&p),NULL);
            /* Convert normal: same coord swap as positions */
            V3 n = (V3){nx, nz, -ny};
            /* Map to vertex slot for this face corner */
            if (cur_vnorm_count < 3) {
                s_face_vnorm[cur_face_ni][cur_vnorm_count] = vi;
                /* Store normal indexed by vertex position index */
                if (vi < MAX_VERTS) s_vnorm[vi] = n;
                cur_vnorm_count++;
            }
        }
    }
    fclose(f);

    if (n_pos==0 || n_face==0) {
        fprintf(stderr, "ase_load: no geometry in %s\n", path);
        return 0;
    }

    /* Reject meshes with out-of-range face indices. Some OMT exports (Phase 7
       variable-face-stride bug, e.g. BLOCKING_01.ASE) produce garbage face data
       like A:1065353216 — indexing s_pos[] with that would segfault. */
    for (int fi=0; fi<n_face; fi++) {
        int a=s_faces[fi].a, b=s_faces[fi].b, c=s_faces[fi].c;
        if (a<0||a>=n_pos||b<0||b>=n_pos||c<0||c>=n_pos) {
            fprintf(stderr, "ase_load: %s has corrupt face %d (%d,%d,%d) "
                            "vs n_pos=%d — skipping mesh\n",
                    path, fi, a, b, c, n_pos);
            return 0;
        }
    }

    /* Fallback: compute flat face normals if no MESH_NORMALS block */
    int have_normals = (s_vnorm[0].x != 0 || s_vnorm[0].y != 0 || s_vnorm[0].z != 0);
    if (!have_normals) {
        for (int fi=0; fi<n_face; fi++) {
            V3 a=s_pos[s_faces[fi].a], b=s_pos[s_faces[fi].b], c=s_pos[s_faces[fi].c];
            float ex=b.x-a.x, ey=b.y-a.y, ez=b.z-a.z;
            float fx=c.x-a.x, fy=c.y-a.y, fz=c.z-a.z;
            float nx=ey*fz-ez*fy, ny=ez*fx-ex*fz, nz=ex*fy-ey*fx;
            float len=sqrtf(nx*nx+ny*ny+nz*nz);
            if (len>0){nx/=len;ny/=len;nz/=len;}
            V3 fn=(V3){nx,ny,nz};
            s_vnorm[s_faces[fi].a]=s_vnorm[s_faces[fi].b]=s_vnorm[s_faces[fi].c]=fn;
        }
    }

    int have_uvs = (n_uv>0 && n_tface==n_face);

    /* If MATERIAL_REF points to a Standard material (i.e. it has a bitmap), all
       faces belong to that material regardless of per-face MTLID. This matches
       the original engine: MTLID only matters when the referenced material is
       Multi/Sub-Object; for Standard materials the object uses exactly one
       texture. Without this, ASEs whose MATERIAL_REF != 0 (e.g. board, phone,
       firestrato) render most faces with the wrong texture. */
    if (geomobject_mat_ref >= 0 && geomobject_mat_ref < ASE_MAX_MATERIALS
        && s_mat_tex[geomobject_mat_ref][0] != '\0') {
        for (int fi = 0; fi < n_face; fi++)
            s_face_mtlid[fi] = geomobject_mat_ref;
    }

    /* Clamp face MTLID to the declared MATERIAL_COUNT. Legacy 3DSMax exports
       (Jimmy/NPC ASEs) carry junk MTLIDs > the real material count — stale
       authoring data — but in the original engine those faces all rendered
       with material 0. Without this clamp, multi-mtlid faces split into
       textureless draw groups and the character ends up patchwork.
       OMT-exported ASEs always emit MATERIAL_COUNT == distinct-mid count,
       so the clamp is a no-op there. */
    int mat_cap = s_mat_count > 0 ? s_mat_count : 1;
    for (int fi=0; fi<n_face; fi++) {
        if (s_face_mtlid[fi] < 0 || s_face_mtlid[fi] >= mat_cap)
            s_face_mtlid[fi] = 0;
    }

    /* Group faces by MTLID. Compact mtlids into 0..n_groups-1 in order of
       first appearance, so the EBO is laid out as N contiguous draw ranges. */
    int  group_of_face[MAX_FACES];
    int  group_mtlid[ASE_MAX_MATERIALS];
    int  n_groups = 0;
    for (int fi=0; fi<n_face; fi++) {
        int mid = s_face_mtlid[fi];
        int g = -1;
        for (int k=0; k<n_groups; k++) {
            if (group_mtlid[k] == mid) { g = k; break; }
        }
        if (g < 0) {
            if (n_groups >= ASE_MAX_MATERIALS) {
                /* Too many MTLIDs — collapse remainder into the last group. */
                g = n_groups - 1;
                if (g < 0) g = 0;
            } else {
                g = n_groups;
                group_mtlid[n_groups] = mid;
                n_groups++;
            }
        }
        group_of_face[fi] = g;
    }
    if (n_groups == 0) n_groups = 1;

    /* Per-group face count + running offset (in faces, ×3 to get indices). */
    int  group_face_count[ASE_MAX_MATERIALS] = {0};
    int  group_face_off[ASE_MAX_MATERIALS]   = {0};
    for (int fi=0; fi<n_face; fi++) group_face_count[group_of_face[fi]]++;
    for (int k=1; k<n_groups; k++)
        group_face_off[k] = group_face_off[k-1] + group_face_count[k-1];

    /* Cursor per group (advances within its slice as we emit faces). */
    int  group_cursor[ASE_MAX_MATERIALS] = {0};

    /* Emit vbuf/ibuf. For each face, write its 3 vertices at the slot dictated
       by its group's slice: (group_face_off[g] + cursor[g]) × 3. */
    for (int fi=0; fi<n_face; fi++) {
        int g = group_of_face[fi];
        int slot = (group_face_off[g] + group_cursor[g]) * 3;
        group_cursor[g]++;

        int vi[3]={s_faces[fi].a, s_faces[fi].b, s_faces[fi].c};
        int ti[3]={0,0,0};
        if (have_uvs){ti[0]=s_tfaces[fi].a;ti[1]=s_tfaces[fi].b;ti[2]=s_tfaces[fi].c;}

        for (int k=0; k<3; k++) {
            int base=(slot+k)*8;
            V3 pos=s_pos[vi[k]];
            V3 nrm=s_vnorm[vi[k]];
            s_vbuf[base+0]=pos.x; s_vbuf[base+1]=pos.y; s_vbuf[base+2]=pos.z;
            if (have_uvs && ti[k]<n_uv) {
                s_vbuf[base+3]=s_uv[ti[k]].u; s_vbuf[base+4]=s_uv[ti[k]].v;
            } else {
                s_vbuf[base+3]=s_vbuf[base+4]=0.0f;
            }
            s_vbuf[base+5]=nrm.x; s_vbuf[base+6]=nrm.y; s_vbuf[base+7]=nrm.z;

            for (int d=0;d<3;d++){
                float val=((float*)&pos)[d];
                if (val<bmin[d]) bmin[d]=val;
                if (val>bmax[d]) bmax[d]=val;
            }
            s_ibuf[slot+k]=slot+k;
        }
    }
    int out_v = n_face * 3;

    for (int d=0;d<3;d++){
        m->min[d]=bmin[d]; m->max[d]=bmax[d]; m->center[d]=(bmin[d]+bmax[d])*0.5f;
    }

    glGenVertexArrays(1,&m->vao);
    glGenBuffers(1,&m->vbo);
    glGenBuffers(1,&m->ebo);
    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
    glBufferData(GL_ARRAY_BUFFER, out_v*8*sizeof(float), s_vbuf, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, out_v*sizeof(unsigned int), s_ibuf, GL_STATIC_DRAW);

    /* attrib 0: pos */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    /* attrib 1: uv */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    /* attrib 2: normal */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float)));

    glBindVertexArray(0);
    m->index_count=out_v;

    /* Populate per-material draw ranges. For each group g, look up the source
       MTLID and pull its bitmap out of s_mat_tex[mtlid] (the ASE numbers
       MATERIAL entries directly, so MTLID = MATERIAL_LIST index). */
    m->material_count = n_groups;
    for (int k=0; k<n_groups; k++) {
        AseMaterial *am = &m->materials[k];
        am->index_offset = group_face_off[k] * 3;
        am->index_count  = group_face_count[k] * 3;
        am->texture_id   = 0;
        int mtlid = group_mtlid[k];
        if (mtlid >= 0 && mtlid < ASE_MAX_MATERIALS) {
            snprintf(am->tex_file, sizeof(am->tex_file), "%s", s_mat_tex[mtlid]);
            am->diffuse[0] = s_mat_diffuse[mtlid][0];
            am->diffuse[1] = s_mat_diffuse[mtlid][1];
            am->diffuse[2] = s_mat_diffuse[mtlid][2];
        } else {
            am->tex_file[0] = '\0';
            am->diffuse[0] = am->diffuse[1] = am->diffuse[2] = 1.0f;
        }
    }
    /* Legacy single-bitmap mirror: pick the first non-empty material bitmap
       so older callers reading m->tex_file still see a sensible value. */
    if (m->tex_file[0] == '\0') {
        for (int k=0; k<n_groups; k++) {
            if (m->materials[k].tex_file[0]) {
                snprintf(m->tex_file, sizeof(m->tex_file), "%s", m->materials[k].tex_file);
                break;
            }
        }
    }

    printf("ase_load: %s  faces=%d  mats=%d  ref=%d  tex='%s'\n",
           path, n_face, n_groups, geomobject_mat_ref, m->tex_file[0]?m->tex_file:"(none)");
    return 1;
}

void ase_free(AseModel *m) {
    if (m->vao) glDeleteVertexArrays(1,&m->vao);
    if (m->vbo) glDeleteBuffers(1,&m->vbo);
    if (m->ebo) glDeleteBuffers(1,&m->ebo);
    if (m->texture_id) glDeleteTextures(1,&m->texture_id);
    /* Per-material textures are owned by the asset cache; do not free here. */
    memset(m,0,sizeof(*m));
}
