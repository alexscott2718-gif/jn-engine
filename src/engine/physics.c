#include "physics.h"
#include "assets/asset_cache.h"
#include "input.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static Entity *find_player(World *w) {
    for (Entity *e = w->head; e; e = e->next) {
        if (e->alive && e->vt && (e->vt->flags & ENTITY_FLAG_PLAYER)) return e;
    }
    return NULL;
}

static int aabb_overlap(const Entity *a, const Entity *b) {
    return  fabsf(a->x - b->x) <= (a->half_extents[0] + b->half_extents[0]) &&
            fabsf(a->y - b->y) <= (a->half_extents[1] + b->half_extents[1]) &&
            fabsf(a->z - b->z) <= (a->half_extents[2] + b->half_extents[2]);
}

/* Resolve a single dynamic-vs-static AABB collision on one axis.
   `axis` = 0/1/2 for x/y/z. Returns 1 if a collision was resolved on this axis. */
static int resolve_axis(Entity *dyn, const Entity *solid, int axis) {
    if (!aabb_overlap(dyn, solid)) return 0;

    float *pos[3] = { &dyn->x, &dyn->y, &dyn->z };
    float *vel[3] = { &dyn->vx, &dyn->vy, &dyn->vz };
    const float a_he = dyn->half_extents[axis];
    const float b_he = solid->half_extents[axis];
    const float b_p  = (axis == 0 ? solid->x : axis == 1 ? solid->y : solid->z);

    float delta = *pos[axis] - b_p;
    float min_sep = a_he + b_he;

    if (delta > 0.0f) {
        *pos[axis] = b_p + min_sep;
        if (*vel[axis] < 0.0f) *vel[axis] = 0.0f;
    } else {
        *pos[axis] = b_p - min_sep;
        if (*vel[axis] > 0.0f) *vel[axis] = 0.0f;
    }
    /* Y-axis collision from above lands the dynamic on the solid. */
    if (axis == 1 && delta > 0.0f) {
        dyn->on_ground = 1;
        /* Inherit the platform's vertical velocity so player rides it.
           Horizontal carry is handled elsewhere if needed. */
    }
    return 1;
}

static float edge2(float ax, float az, float bx, float bz, float px, float pz) {
    return (px - ax) * (bz - az) - (pz - az) * (bx - ax);
}

static int sample_mesh_height_xz(const AseModel *m, float local_x, float local_z, float *out_y) {
    if (!m || !m->frames || m->vertex_count < 3) return 0;
    int hit = 0;
    float best_y = -1.0e30f;
    const float *v = m->frames;
    for (int i = 0; i + 2 < m->vertex_count; i += 3) {
        const float *a = v + (size_t)(i + 0) * 8u;
        const float *b = v + (size_t)(i + 1) * 8u;
        const float *c = v + (size_t)(i + 2) * 8u;
        float area = edge2(a[0], a[2], b[0], b[2], c[0], c[2]);
        if (fabsf(area) < 1.0e-5f) continue;
        float w0 = edge2(b[0], b[2], c[0], c[2], local_x, local_z) / area;
        float w1 = edge2(c[0], c[2], a[0], a[2], local_x, local_z) / area;
        float w2 = edge2(a[0], a[2], b[0], b[2], local_x, local_z) / area;
        const float eps = -0.0005f;
        if (w0 < eps || w1 < eps || w2 < eps) continue;
        float y = w0 * a[1] + w1 * b[1] + w2 * c[1];
        if (y > best_y) best_y = y;
        hit = 1;
    }
    if (hit && out_y) *out_y = best_y;
    return hit;
}

/* A placement participates in walk-on collision if it is the ground or one of
   the original game's BLOCK* / BLOCKING_* collision proxies. */
static int placement_is_collider(const char *name) {
    return strncasecmp(name, "GROUND", 6) == 0 ||
           strncasecmp(name, "BLOCK",  5) == 0;
}

/* Global collision toggle (default ON; JN_NO_WORLD_COLLISION=1 disables). */
static int world_collision_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char *s = getenv("JN_NO_WORLD_COLLISION");
        cached = (s && s[0] && strcmp(s, "0") != 0) ? 0 : 1;
    }
    return cached;
}

/* Highest collision surface at (x,z) that is at or below ref_y + STEP — so the
   player lands on terrain / floors / steps and never snaps up onto a roof that
   is above them. Samples GROUND + every BLOCK* collision mesh whose XZ-AABB
   contains the point. */
static int world_terrain_height(const World *w, float x, float z,
                                float ref_y, float *out_y) {
    if (!world_collision_enabled()) return 0;
    const float STEP = 100.0f;   /* max step-up height */
    int hit = 0;
    float best_y = -1.0e30f;
    for (int i = 0; i < w->placement_count; i++) {
        const WorldPlacement *pl = &w->placements[i];
        if (!placement_is_collider(pl->name)) continue;
        AseModel *m = model_cache_get(pl->ase_path);
        if (!m) continue;
        /* Native Level 1 draws placements at (x, 0, -z); collide in that basis. */
        float lx = x - pl->x;
        float lz = z - (-pl->z);
        /* Cheap XZ-AABB reject before the triangle scan. */
        if (lx < m->min[0] || lx > m->max[0] ||
            lz < m->min[2] || lz > m->max[2]) continue;
        float y;
        if (!sample_mesh_height_xz(m, lx, lz, &y)) continue;
        if (y > ref_y + STEP) continue;   /* don't teleport up onto a roof above us */
        if (y > best_y) best_y = y;
        hit = 1;
    }
    if (hit && out_y) *out_y = best_y;
    return hit;
}

static int world_safety_floor_height(const World *w, float x, float z, float *out_y) {
    if (!w->safety_floor_enabled) return 0;
    if (fabsf(x - w->safety_floor_cx) > w->safety_floor_half_x) return 0;
    if (fabsf(z - w->safety_floor_cz) > w->safety_floor_half_z) return 0;
    if (out_y) *out_y = w->safety_floor_y;
    return 1;
}

void physics_step(World *w, float dt) {
    if (dt <= 0.0f) return;

    /* 1. Integrate gravity + velocity for physics entities. */
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || !e->vt) continue;
        if (!(e->vt->flags & ENTITY_FLAG_PHYSICS)) continue;

        if ((e->vt->flags & ENTITY_FLAG_PLAYER) && input_noclip_enabled()) {
            e->x += e->vx * dt;
            e->y += e->vy * dt;
            e->z += e->vz * dt;
            e->on_ground = 1;
            continue;
        }

        e->vy -= PHYSICS_GRAVITY * dt;
        if (e->vy < -PHYSICS_MAX_FALL) e->vy = -PHYSICS_MAX_FALL;

        /* Resolve axes independently so we can detect ground contact cleanly. */
        e->x += e->vx * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->vt->flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 0);
        }

        e->z += e->vz * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->vt->flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 2);
        }

        int was_on_ground = e->on_ground;
        e->on_ground = 0;
        e->y += e->vy * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->vt->flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 1);
        }
        if (!e->on_ground) {
            float terrain_y;
            float feet_y = e->y - e->half_extents[1];
            int have_floor = world_terrain_height(w, e->x, e->z, feet_y, &terrain_y);
            float safety_y;
            if (world_safety_floor_height(w, e->x, e->z, &safety_y) &&
                (!have_floor || safety_y > terrain_y)) {
                terrain_y = safety_y;
                have_floor = 1;
            }
            if (have_floor) {
                float floor_y = terrain_y + e->half_extents[1];
                if (e->y < floor_y) {
                    e->y = floor_y;
                    if (e->vy < 0.0f) e->vy = 0.0f;
                    e->on_ground = 1;
                }
            }
        }

        (void)was_on_ground;
    }

    /* 2. Fire triggers when the player overlaps trigger entities. */
    Entity *p = find_player(w);
    if (p && p->alive) {
        for (Entity *t = w->head; t; t = t->next) {
            if (t == p || !t->alive || !t->vt) continue;
            if (!(t->vt->flags & ENTITY_FLAG_TRIGGER)) continue;
            if (!aabb_overlap(p, t)) continue;
            if (t->vt->on_trigger) t->vt->on_trigger(t, p);
        }
    }
}
