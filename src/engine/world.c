#include "world.h"
#include "glad.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void world_init(World *w) {
    w->head = NULL; w->count = 0; w->ground_y = 0.0f;
    w->safety_floor_enabled = 0;
    w->safety_floor_y = 0.0f;
    w->safety_floor_cx = 0.0f;
    w->safety_floor_cz = 0.0f;
    w->safety_floor_half_x = 0.0f;
    w->safety_floor_half_z = 0.0f;
    w->placements = NULL; w->placement_count = 0;
}

Entity *world_add(World *w) {
    Entity *e = calloc(1, sizeof(Entity));
    if (!e) return NULL;
    e->alive = 1;
    e->visible = 1;
    e->omt_index = -1;           /* 0 is a valid OMT shape chunk id */
    e->next = w->head;
    w->head = e;
    w->count++;
    return e;
}

Entity *world_find_type(const World *w, const char *type) {
    for (Entity *e = w->head; e; e = e->next)
        if (strcmp(e->type, type) == 0) return e;
    return NULL;
}

void world_destroy(World *w) {
    Entity *e = w->head;
    while (e) { Entity *n = e->next; free(e); e = n; }
    w->head = NULL; w->count = 0;
    free(w->placements);
    w->placements = NULL; w->placement_count = 0;
    w->safety_floor_enabled = 0;
}

/* Slab method: clip the parametric segment p + t*(q-p) against an AABB and
   return tmin if it enters within [0,1], else 1.0 for no hit. */
static float segment_hit_aabb(float px, float py, float pz,
                              float dx, float dy, float dz,
                              float cx, float cy, float cz,
                              float hx, float hy, float hz) {
    float tmin = 0.0f, tmax = 1.0f;
    float p[3] = { px, py, pz };
    float d[3] = { dx, dy, dz };
    float c[3] = { cx, cy, cz };
    float h[3] = { hx, hy, hz };
    for (int a = 0; a < 3; a++) {
        if (fabsf(d[a]) < 1e-6f) {
            if (p[a] < c[a] - h[a] || p[a] > c[a] + h[a]) return 1.0f;
        } else {
            float inv = 1.0f / d[a];
            float t1 = (c[a] - h[a] - p[a]) * inv;
            float t2 = (c[a] + h[a] - p[a]) * inv;
            if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return 1.0f;
        }
    }
    return tmin;
}

float world_query_segment(const World *w, const Entity *ignore,
                          float px, float py, float pz,
                          float qx, float qy, float qz) {
    float dx = qx - px, dy = qy - py, dz = qz - pz;
    float best = 1.0f;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || e == ignore) continue;
        if (!(e->runtime_flags & ENTITY_FLAG_SOLID)) continue;
        float t = segment_hit_aabb(px, py, pz, dx, dy, dz,
                                   e->x, e->y, e->z,
                                   e->half_extents[0], e->half_extents[1], e->half_extents[2]);
        if (t < best) best = t;
    }
    return best;
}

/* ---- placeholder unit cube ---- */
/* 8 unique corners, 12 triangles (36 indices), position only */
static const float BOX_VERTS[] = {
    -1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,  /* back  */
    -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1,  /* front */
};
static const unsigned int BOX_IDX[] = {
    0,1,2, 2,3,0,  /* back   */
    4,5,6, 6,7,4,  /* front  */
    0,4,7, 7,3,0,  /* left   */
    1,5,6, 6,2,1,  /* right  */
    3,2,6, 6,7,3,  /* top    */
    0,1,5, 5,4,0,  /* bottom */
};

static unsigned int g_box_vao = 0, g_box_vbo = 0, g_box_ebo = 0;

int world_box_init(void) {
    glGenVertexArrays(1, &g_box_vao);
    glGenBuffers(1, &g_box_vbo);
    glGenBuffers(1, &g_box_ebo);

    glBindVertexArray(g_box_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_box_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BOX_VERTS), BOX_VERTS, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_box_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(BOX_IDX), BOX_IDX, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return 1;
}

unsigned int world_box_vao(void)         { return g_box_vao; }
int          world_box_index_count(void) { return 36; }

void world_box_destroy(void) {
    if (g_box_vao) glDeleteVertexArrays(1, &g_box_vao);
    if (g_box_vbo) glDeleteBuffers(1, &g_box_vbo);
    if (g_box_ebo) glDeleteBuffers(1, &g_box_ebo);
    g_box_vao = g_box_vbo = g_box_ebo = 0;
}
