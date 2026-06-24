#include "collision.h"
#include "world.h"
#include "assets/asset_cache.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <float.h>

/* ---- tunables (engine-chosen; not pinned by decomp/.gam, documented here) --
   STEP_CAP: max curb the player steps up. Inherited from the old floor sample
     (physics.c used 100.0). Surfaces whose top is within STEP_CAP of the feet
     are walkable (ground-follow rises onto them) rather than walls.
   WALL_NY_MAX: a triangle counts as a wall when |normal.y| < WALL_NY_MAX
     (steeper than ~60deg from horizontal). Vertical building/fence faces have
     normal.y ~ 0; terraced GROUND slopes have normal.y near 1, so 0.5 cleanly
     separates the two. */
#define STEP_CAP     100.0f
#define WALL_NY_MAX  0.5f

typedef struct {
    float v0[3], v1[3], v2[3];   /* world-space verts (collision basis) */
    float n[3];                  /* upward-oriented unit normal */
    float xmin, xmax, zmin, zmax;
    float ymin, ymax;
    unsigned char is_block;      /* from a designed BLOCK/BLOCKING barrier mesh */
} CollTri;

struct CollisionWorld {
    CollTri *tris;
    int      tri_count;

    /* Uniform XZ grid over the collider bounds, stored CSR-style:
       cell c owns cell_tris[cell_start[c] .. cell_start[c+1]). */
    float    gx0, gz0;           /* grid min corner */
    float    cell_x, cell_z;     /* cell size */
    int      nx, nz;
    int     *cell_start;         /* length nx*nz + 1 */
    int     *cell_tris;          /* length cell_start[nx*nz] */
};

/* ---- small vector helpers ---- */
static void vsub(const float a[3], const float b[3], float o[3]) {
    o[0]=a[0]-b[0]; o[1]=a[1]-b[1]; o[2]=a[2]-b[2];
}
static void vcross(const float a[3], const float b[3], float o[3]) {
    o[0]=a[1]*b[2]-a[2]*b[1];
    o[1]=a[2]*b[0]-a[0]*b[2];
    o[2]=a[0]*b[1]-a[1]*b[0];
}
static float vdot(const float a[3], const float b[3]) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

/* ---- name predicates (shared source of truth) ---- */
static int is_blocks_toy(const char *name) {
    return strcasecmp(name, "Blocks_Out") == 0 ||
           strcasecmp(name, "Blocks_In")  == 0;
}
int collision_is_invisible(const char *name) {
    return strncasecmp(name, "BLOCK", 5) == 0 && !is_blocks_toy(name);
}
int collision_is_collider(const char *name) {
    if (strncasecmp(name, "GROUND", 6) == 0) return 1;   /* visible floor */
    return collision_is_invisible(name);                  /* BLOCK* / BLOCKING* */
}

/* ---- triangle XZ point test (barycentric, mirrors physics.c) ---- */
static float edge2(float ax, float az, float bx, float bz, float px, float pz) {
    return (px - ax) * (bz - az) - (pz - az) * (bx - ax);
}

/* Interpolated surface Y at (x,z) if (x,z) is inside the triangle's XZ
   projection; returns 1 + *out_y on hit. */
static int tri_height_xz(const CollTri *t, float x, float z, float *out_y) {
    const float *a = t->v0, *b = t->v1, *c = t->v2;
    float area = edge2(a[0],a[2], b[0],b[2], c[0],c[2]);
    if (fabsf(area) < 1.0e-5f) return 0;
    float w0 = edge2(b[0],b[2], c[0],c[2], x,z) / area;
    float w1 = edge2(c[0],c[2], a[0],a[2], x,z) / area;
    float w2 = edge2(a[0],a[2], b[0],b[2], x,z) / area;
    const float eps = -0.0005f;
    if (w0 < eps || w1 < eps || w2 < eps) return 0;
    *out_y = w0*a[1] + w1*b[1] + w2*c[1];
    return 1;
}

/* ---- closest point on a triangle to p (Ericson, RTCD) ---- */
static void closest_pt_tri(const float p[3], const float a[3],
                           const float b[3], const float c[3], float out[3]) {
    float ab[3], ac[3], ap[3];
    vsub(b,a,ab); vsub(c,a,ac); vsub(p,a,ap);
    float d1 = vdot(ab,ap), d2 = vdot(ac,ap);
    if (d1 <= 0.0f && d2 <= 0.0f) { out[0]=a[0]; out[1]=a[1]; out[2]=a[2]; return; }
    float bp[3]; vsub(p,b,bp);
    float d3 = vdot(ab,bp), d4 = vdot(ac,bp);
    if (d3 >= 0.0f && d4 <= d3) { out[0]=b[0]; out[1]=b[1]; out[2]=b[2]; return; }
    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        out[0]=a[0]+v*ab[0]; out[1]=a[1]+v*ab[1]; out[2]=a[2]+v*ab[2]; return;
    }
    float cp[3]; vsub(p,c,cp);
    float d5 = vdot(ab,cp), d6 = vdot(ac,cp);
    if (d6 >= 0.0f && d5 <= d6) { out[0]=c[0]; out[1]=c[1]; out[2]=c[2]; return; }
    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        out[0]=a[0]+w*ac[0]; out[1]=a[1]+w*ac[1]; out[2]=a[2]+w*ac[2]; return;
    }
    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        out[0]=b[0]+w*(c[0]-b[0]); out[1]=b[1]+w*(c[1]-b[1]); out[2]=b[2]+w*(c[2]-b[2]); return;
    }
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom, w = vc * denom;
    out[0]=a[0]+ab[0]*v+ac[0]*w;
    out[1]=a[1]+ab[1]*v+ac[1]*w;
    out[2]=a[2]+ab[2]*v+ac[2]*w;
}

/* ---- grid helpers ---- */
static int grid_ix(const CollisionWorld *cw, float x) {
    int i = (int)((x - cw->gx0) / cw->cell_x);
    return i < 0 ? 0 : (i >= cw->nx ? cw->nx - 1 : i);
}
static int grid_iz(const CollisionWorld *cw, float z) {
    int i = (int)((z - cw->gz0) / cw->cell_z);
    return i < 0 ? 0 : (i >= cw->nz ? cw->nz - 1 : i);
}

/* Inclusive cell range a triangle's XZ AABB spans (clamped to the grid). */
static void tri_cell_range(const CollisionWorld *cw, const CollTri *t,
                           int *ix0, int *ix1, int *iz0, int *iz1) {
    *ix0 = grid_ix(cw, t->xmin); *ix1 = grid_ix(cw, t->xmax);
    *iz0 = grid_iz(cw, t->zmin); *iz1 = grid_iz(cw, t->zmax);
}

/* Total XZ-projected area of a mesh's walkable (near-horizontal) triangles. Used
   to tell a genuine floor/terrain mesh (large walkable surface) from a prop
   (compact) when a level ships no authored GROUND floor — see collision_build. */
static float mesh_walkable_area(const AseModel *m) {
    if (!m || !m->frames || m->vertex_count < 3) return 0.0f;
    float area = 0.0f;
    const float *v = m->frames;
    for (int k = 0; k + 2 < m->vertex_count; k += 3) {
        const float *a = v + (size_t)(k + 0) * 8u;
        const float *b = v + (size_t)(k + 1) * 8u;
        const float *c = v + (size_t)(k + 2) * 8u;
        float e1[3], e2[3], nrm[3];
        vsub(b, a, e1); vsub(c, a, e2); vcross(e1, e2, nrm);
        float len = sqrtf(vdot(nrm, nrm));
        if (len < 1.0e-6f) continue;
        if (fabsf(nrm[1] / len) < WALL_NY_MAX) continue;   /* wall, not floor */
        area += fabsf((b[0]-a[0])*(c[2]-a[2]) - (c[0]-a[0])*(b[2]-a[2])) * 0.5f;
    }
    return area;
}

/* A placement that contributes geometry to the CollisionWorld. Always the
   GROUND/BLOCK colliders. In levels with NO authored GROUND floor mesh (most
   non-Retroville levels — see PROJECT_HISTORY Era 15), the walkable floor is a
   plain visible mesh (station, Plane01, firstroom, a cave shell, ...), so any
   placement with a large walkable surface is also taken as a collider, giving
   the player a real floor (and the mesh's own walls) instead of the safety
   plane. Gated to GROUND-less levels so the Retroville GROUND+BLOCKING setup is
   untouched. */
#define FLOOR_AREA_THRESH 1.0e6f
static int placement_collides(const WorldPlacement *pl, AseModel *m, int has_ground) {
    if (collision_is_collider(pl->name)) return 1;
    if (!has_ground && mesh_walkable_area(m) > FLOOR_AREA_THRESH) return 1;
    return 0;
}

CollisionWorld *collision_build(const World *w) {
    if (!w || w->placement_count <= 0) return NULL;

    int has_ground = 0;
    for (int i = 0; i < w->placement_count; i++)
        if (strncasecmp(w->placements[i].name, "GROUND", 6) == 0) { has_ground = 1; break; }

    /* Pass 1: count colliding triangles. */
    int tri_cap = 0;
    for (int i = 0; i < w->placement_count; i++) {
        const WorldPlacement *pl = &w->placements[i];
        AseModel *m = model_cache_get(pl->ase_path);
        if (!m || !m->frames || m->vertex_count < 3) continue;
        if (!placement_collides(pl, m, has_ground)) continue;
        tri_cap += m->vertex_count / 3;
    }
    if (tri_cap == 0) return NULL;

    CollisionWorld *cw = (CollisionWorld *)calloc(1, sizeof(*cw));
    if (!cw) return NULL;
    cw->tris = (CollTri *)malloc((size_t)tri_cap * sizeof(CollTri));
    if (!cw->tris) { free(cw); return NULL; }

    /* Pass 2: bake triangles into world space (collision basis). */
    float bx0 = FLT_MAX, bz0 = FLT_MAX, bx1 = -FLT_MAX, bz1 = -FLT_MAX;
    int tc = 0;
    for (int i = 0; i < w->placement_count; i++) {
        const WorldPlacement *pl = &w->placements[i];
        AseModel *m = model_cache_get(pl->ase_path);
        if (!m || !m->frames || m->vertex_count < 3) continue;
        if (!placement_collides(pl, m, has_ground)) continue;
        const int is_block = collision_is_invisible(pl->name);
        const float ox = pl->x, oz = -pl->z;   /* placement world translation */
        const float *v = m->frames;
        for (int k = 0; k + 2 < m->vertex_count; k += 3) {
            const float *a = v + (size_t)(k + 0) * 8u;
            const float *b = v + (size_t)(k + 1) * 8u;
            const float *c = v + (size_t)(k + 2) * 8u;
            CollTri *t = &cw->tris[tc];
            t->v0[0]=ox+a[0]; t->v0[1]=a[1]; t->v0[2]=oz+a[2];
            t->v1[0]=ox+b[0]; t->v1[1]=b[1]; t->v1[2]=oz+b[2];
            t->v2[0]=ox+c[0]; t->v2[1]=c[1]; t->v2[2]=oz+c[2];
            float e1[3], e2[3], nrm[3];
            vsub(t->v1, t->v0, e1);
            vsub(t->v2, t->v0, e2);
            vcross(e1, e2, nrm);
            float len = sqrtf(vdot(nrm, nrm));
            if (len < 1.0e-6f) continue;          /* degenerate */
            float inv = 1.0f / len;
            nrm[0]*=inv; nrm[1]*=inv; nrm[2]*=inv;
            if (nrm[1] < 0.0f) { nrm[0]=-nrm[0]; nrm[1]=-nrm[1]; nrm[2]=-nrm[2]; }
            t->n[0]=nrm[0]; t->n[1]=nrm[1]; t->n[2]=nrm[2];
            t->xmin = fminf(t->v0[0], fminf(t->v1[0], t->v2[0]));
            t->xmax = fmaxf(t->v0[0], fmaxf(t->v1[0], t->v2[0]));
            t->zmin = fminf(t->v0[2], fminf(t->v1[2], t->v2[2]));
            t->zmax = fmaxf(t->v0[2], fmaxf(t->v1[2], t->v2[2]));
            t->ymin = fminf(t->v0[1], fminf(t->v1[1], t->v2[1]));
            t->ymax = fmaxf(t->v0[1], fmaxf(t->v1[1], t->v2[1]));
            t->is_block = (unsigned char)is_block;
            if (t->xmin < bx0) bx0 = t->xmin;
            if (t->xmax > bx1) bx1 = t->xmax;
            if (t->zmin < bz0) bz0 = t->zmin;
            if (t->zmax > bz1) bz1 = t->zmax;
            tc++;
        }
    }
    cw->tri_count = tc;
    if (tc == 0) { free(cw->tris); free(cw); return NULL; }

    /* Uniform grid: aim for ~1 triangle per cell, capped so a few huge
       perimeter tris can't blow up the cell count. */
    int n = (int)sqrtf((float)tc);
    if (n < 8)   n = 8;
    if (n > 128) n = 128;
    cw->nx = n; cw->nz = n;
    cw->gx0 = bx0; cw->gz0 = bz0;
    float spanx = bx1 - bx0, spanz = bz1 - bz0;
    cw->cell_x = (spanx > 1.0f ? spanx : 1.0f) / (float)cw->nx;
    cw->cell_z = (spanz > 1.0f ? spanz : 1.0f) / (float)cw->nz;
    /* Nudge cell size up by an epsilon so xmax/zmax map inside the grid. */
    cw->cell_x *= 1.0001f; cw->cell_z *= 1.0001f;

    int ncell = cw->nx * cw->nz;
    cw->cell_start = (int *)calloc((size_t)ncell + 1, sizeof(int));
    if (!cw->cell_start) { collision_free(cw); return NULL; }

    /* Count insertions per cell, then prefix-sum (CSR). */
    for (int i = 0; i < tc; i++) {
        int ix0,ix1,iz0,iz1; tri_cell_range(cw, &cw->tris[i], &ix0,&ix1,&iz0,&iz1);
        for (int jz = iz0; jz <= iz1; jz++)
            for (int jx = ix0; jx <= ix1; jx++)
                cw->cell_start[jz * cw->nx + jx + 1]++;
    }
    for (int c = 0; c < ncell; c++)
        cw->cell_start[c + 1] += cw->cell_start[c];
    int total = cw->cell_start[ncell];
    cw->cell_tris = (int *)malloc((size_t)(total > 0 ? total : 1) * sizeof(int));
    if (!cw->cell_tris) { collision_free(cw); return NULL; }

    /* Fill, using a temporary write cursor per cell. */
    int *cursor = (int *)malloc((size_t)ncell * sizeof(int));
    if (!cursor) { collision_free(cw); return NULL; }
    for (int c = 0; c < ncell; c++) cursor[c] = cw->cell_start[c];
    for (int i = 0; i < tc; i++) {
        int ix0,ix1,iz0,iz1; tri_cell_range(cw, &cw->tris[i], &ix0,&ix1,&iz0,&iz1);
        for (int jz = iz0; jz <= iz1; jz++)
            for (int jx = ix0; jx <= ix1; jx++) {
                int c = jz * cw->nx + jx;
                cw->cell_tris[cursor[c]++] = i;
            }
    }
    free(cursor);
    if (getenv("JN_DEBUG_COLLIDE"))
        fprintf(stderr, "[collision] %d tris, grid %dx%d, bounds x[%.0f,%.0f] z[%.0f,%.0f]\n",
                cw->tri_count, cw->nx, cw->nz, bx0, bx1, bz0, bz1);
    return cw;
}

void collision_free(CollisionWorld *cw) {
    if (!cw) return;
    free(cw->tris);
    free(cw->cell_start);
    free(cw->cell_tris);
    free(cw);
}

int collision_ground_height(const CollisionWorld *cw, float x, float z,
                            float ref_y, float *out_y, float n[3]) {
    if (!cw || cw->tri_count == 0) return 0;
    int ix = grid_ix(cw, x), iz = grid_iz(cw, z);
    int c = iz * cw->nx + ix;
    int s = cw->cell_start[c], e = cw->cell_start[c + 1];
    int hit = 0;
    float best_y = -FLT_MAX;
    const CollTri *best = NULL;
    const float cap = ref_y + STEP_CAP;
    for (int j = s; j < e; j++) {
        const CollTri *t = &cw->tris[cw->cell_tris[j]];
        if (t->ymin > cap) continue;        /* whole tri above the step cap */
        float y;
        if (!tri_height_xz(t, x, z, &y)) continue;
        if (y > cap) continue;               /* don't snap up onto a roof */
        if (y > best_y) { best_y = y; best = t; hit = 1; }
    }
    if (hit) {
        if (out_y) *out_y = best_y;
        if (n) { n[0]=best->n[0]; n[1]=best->n[1]; n[2]=best->n[2]; }
    }
    return hit;
}

void collision_resolve_horizontal(const CollisionWorld *cw,
                                   float pos[3], const float half[3],
                                   float vel[3]) {
    if (!cw || cw->tri_count == 0) return;
    /* Approximate the player box as a vertical cylinder of this radius. */
    const float radius = fmaxf(half[0], half[2]);
    const float feet = pos[1] - half[1];
    const float head = pos[1] + half[1];
    const float wall_floor = feet + STEP_CAP;  /* a real wall rises above this */

    /* A couple of relaxation passes so inside-corner cases settle. */
    for (int pass = 0; pass < 3; pass++) {
        int ix0 = grid_ix(cw, pos[0] - radius), ix1 = grid_ix(cw, pos[0] + radius);
        int iz0 = grid_iz(cw, pos[2] - radius), iz1 = grid_iz(cw, pos[2] + radius);
        int moved = 0;
        for (int jz = iz0; jz <= iz1; jz++)
            for (int jx = ix0; jx <= ix1; jx++) {
                int c = jz * cw->nx + jx;
                int s = cw->cell_start[c], e = cw->cell_start[c + 1];
                for (int j = s; j < e; j++) {
                    const CollTri *t = &cw->tris[cw->cell_tris[j]];
                    if (fabsf(t->n[1]) >= WALL_NY_MAX) continue;   /* floor/slope, not a wall */
                    if (t->ymax <= wall_floor) continue;           /* steppable curb */
                    if (t->ymin >= head) continue;                 /* above the head */
                    if (t->ymax <= feet) continue;                 /* below the feet */
                    float q[3];
                    closest_pt_tri(pos, t->v0, t->v1, t->v2, q);
                    float dx = pos[0] - q[0], dz = pos[2] - q[2];
                    float d2 = dx*dx + dz*dz;
                    if (d2 >= radius*radius) continue;             /* not penetrating */
                    float d = sqrtf(d2);
                    float nx, nz;
                    if (d > 1.0e-4f) { nx = dx/d; nz = dz/d; }
                    else { nx = t->n[0]; nz = t->n[2];           /* on the face: use its normal */
                           float l = sqrtf(nx*nx+nz*nz);
                           if (l < 1.0e-4f) continue; nx/=l; nz/=l; }
                    float push = radius - d;
                    pos[0] += nx * push;
                    pos[2] += nz * push;
                    /* Slide: remove the into-wall velocity component (XZ only). */
                    float vn = vel[0]*nx + vel[2]*nz;
                    if (vn < 0.0f) { vel[0] -= vn*nx; vel[2] -= vn*nz; }
                    moved = 1;
                }
            }
        if (!moved) break;
    }
}

/* Moeller-Trumbore segment/triangle test. Returns t in [0,1] or >1 on miss. */
static float seg_tri(const float p[3], const float d[3], const CollTri *t) {
    float e1[3], e2[3], pv[3];
    vsub(t->v1, t->v0, e1);
    vsub(t->v2, t->v0, e2);
    vcross(d, e2, pv);
    float det = vdot(e1, pv);
    if (fabsf(det) < 1.0e-9f) return 2.0f;
    float inv = 1.0f / det;
    float tv[3]; vsub(p, t->v0, tv);
    float u = vdot(tv, pv) * inv;
    if (u < 0.0f || u > 1.0f) return 2.0f;
    float qv[3]; vcross(tv, e1, qv);
    float vparam = vdot(d, qv) * inv;
    if (vparam < 0.0f || u + vparam > 1.0f) return 2.0f;
    float tt = vdot(e2, qv) * inv;
    return (tt >= 0.0f && tt <= 1.0f) ? tt : 2.0f;
}

float collision_segment(const CollisionWorld *cw,
                        const float p[3], const float q[3], float n[3]) {
    if (!cw || cw->tri_count == 0) return 1.0f;
    float d[3]; vsub(q, p, d);
    int ix0 = grid_ix(cw, fminf(p[0], q[0])), ix1 = grid_ix(cw, fmaxf(p[0], q[0]));
    int iz0 = grid_iz(cw, fminf(p[2], q[2])), iz1 = grid_iz(cw, fmaxf(p[2], q[2]));
    float best = 1.0f;
    const CollTri *bt = NULL;
    for (int jz = iz0; jz <= iz1; jz++)
        for (int jx = ix0; jx <= ix1; jx++) {
            int c = jz * cw->nx + jx;
            int s = cw->cell_start[c], e = cw->cell_start[c + 1];
            for (int j = s; j < e; j++) {
                const CollTri *t = &cw->tris[cw->cell_tris[j]];
                float tt = seg_tri(p, d, t);
                if (tt <= 1.0f && tt < best) { best = tt; bt = t; }
            }
        }
    if (bt && n) {
        float nn[3] = { bt->n[0], bt->n[1], bt->n[2] };
        if (vdot(nn, d) > 0.0f) { nn[0]=-nn[0]; nn[1]=-nn[1]; nn[2]=-nn[2]; }
        n[0]=nn[0]; n[1]=nn[1]; n[2]=nn[2];
    }
    return best;
}

int collision_nearest_wall(const CollisionWorld *cw, const float p[3],
                           float search, float out_pt[3], float out_n[3],
                           float *out_ymin, float *out_ymax) {
    if (!cw || cw->tri_count == 0) return 0;
    const float MIN_WALL_H = 2.0f * STEP_CAP;   /* a genuine blocker, not a curb */
    /* The test seats the player squarely against the face and drives straight
       in, so require a NEAR-VERTICAL wall (|n.y| small): a tilted face's surface
       at the player's height is offset from where the seating puts it, which
       would make the synthetic drive ride over its edge rather than press the
       face. The runtime resolve still handles tilted walls; this is only about
       picking a clean wall to validate against. */
    const float MAX_NY = 0.15f;
    int ix0 = grid_ix(cw, p[0] - search), ix1 = grid_ix(cw, p[0] + search);
    int iz0 = grid_iz(cw, p[2] - search), iz1 = grid_iz(cw, p[2] + search);
    float best = search * search;
    const CollTri *bt = NULL;
    float bq[3] = {0,0,0};
    for (int jz = iz0; jz <= iz1; jz++)
        for (int jx = ix0; jx <= ix1; jx++) {
            int c = jz * cw->nx + jx;
            int s = cw->cell_start[c], e = cw->cell_start[c + 1];
            for (int j = s; j < e; j++) {
                const CollTri *t = &cw->tris[cw->cell_tris[j]];
                if (!t->is_block) continue;   /* validate designed BLOCK walls only */
                if (fabsf(t->n[1]) >= MAX_NY) continue;
                if (t->ymax - t->ymin < MIN_WALL_H) continue;
                float q[3];
                closest_pt_tri(p, t->v0, t->v1, t->v2, q);
                float dx=p[0]-q[0], dy=p[1]-q[1], dz=p[2]-q[2];
                float d2 = dx*dx + dy*dy + dz*dz;
                if (d2 < best) { best = d2; bt = t; bq[0]=q[0]; bq[1]=q[1]; bq[2]=q[2]; }
            }
        }
    if (!bt) return 0;
    if (out_pt) { out_pt[0]=bq[0]; out_pt[1]=bq[1]; out_pt[2]=bq[2]; }
    if (out_ymin) *out_ymin = bt->ymin;
    if (out_ymax) *out_ymax = bt->ymax;
    if (out_n) {
        float nx = bt->n[0], nz = bt->n[2];
        float l = sqrtf(nx*nx + nz*nz);
        if (l < 1.0e-4f) { out_n[0]=1.0f; out_n[1]=0.0f; out_n[2]=0.0f; }
        else {
            nx/=l; nz/=l;
            /* Orient outward (toward p) so callers can offset along it. */
            if ((p[0]-bq[0])*nx + (p[2]-bq[2])*nz < 0.0f) { nx=-nx; nz=-nz; }
            out_n[0]=nx; out_n[1]=0.0f; out_n[2]=nz;
        }
    }
    return 1;
}
