#include "physics.h"
#include "collision.h"
#include "input.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static Entity *find_player(World *w) {
    for (Entity *e = w->head; e; e = e->next) {
        if (e->alive && (e->runtime_flags & ENTITY_FLAG_PLAYER)) return e;
    }
    return NULL;
}

int physics_aabb_overlap(const Entity *a, const Entity *b) {
    if (!a || !b) return 0;
    return  fabsf(a->x - b->x) <= (a->half_extents[0] + b->half_extents[0]) &&
            fabsf(a->y - b->y) <= (a->half_extents[1] + b->half_extents[1]) &&
            fabsf(a->z - b->z) <= (a->half_extents[2] + b->half_extents[2]);
}

/* Resolve a single dynamic-vs-static AABB collision on one axis.
   `axis` = 0/1/2 for x/y/z. Returns 1 if a collision was resolved on this axis. */
static int resolve_axis(Entity *dyn, const Entity *solid, int axis) {
    if (!physics_aabb_overlap(dyn, solid)) return 0;

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

/* Global world-collision toggle (default ON; JN_NO_WORLD_COLLISION=1 disables
   the mesh ground-follow + wall resolve entirely, leaving only the safety
   floor / entity AABB pass). */
static int world_collision_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char *s = getenv("JN_NO_WORLD_COLLISION");
        cached = (s && s[0] && strcmp(s, "0") != 0) ? 0 : 1;
    }
    return cached;
}

/* Whether an entity participates in world (terrain + wall) collision. Mirrors
   the SOLID-clearing done by behavior_prop.c / behavior_ai_omtobj.c: an object
   with TerrainColl==0 (terrain/collision hooks disabled) or HasCollision==0 is
   non-colliding and should pass through the mesh world. The player always
   collides. Only the player currently carries ENTITY_FLAG_PHYSICS, so this is
   forward-looking for when NPCs/enemies/vehicles gain physics. */
static int entity_terrain_collides(const Entity *e) {
    if (e->runtime_flags & ENTITY_FLAG_PLAYER) return 1;
    if (gam_prop_i(e, "TerrainColl",  -1) == 0) return 0;
    if (gam_prop_i(e, "HasCollision", -1) == 0) return 0;
    return 1;
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
        if (!(e->runtime_flags & ENTITY_FLAG_PHYSICS)) continue;

        if ((e->runtime_flags & ENTITY_FLAG_PLAYER) && input_noclip_enabled()) {
            e->x += e->vx * dt;
            e->y += e->vy * dt;
            e->z += e->vz * dt;
            e->on_ground = 1;
            continue;
        }

        const int terrain = entity_terrain_collides(e);

        e->vy -= PHYSICS_GRAVITY * dt;
        if (e->vy < -PHYSICS_MAX_FALL) e->vy = -PHYSICS_MAX_FALL;

        /* Resolve axes independently so we can detect ground contact cleanly. */
        e->x += e->vx * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->runtime_flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 0);
        }

        e->z += e->vz * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->runtime_flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 2);
        }

        /* Mesh wall collision: push the entity out of BLOCKING walls/fences in
           XZ and slide along faces. Runs after the horizontal integration so
           the Y settle below samples the floor at the corrected XZ. Curbs and
           steps under the step cap are NOT walls (the ground-follow rises onto
           them). Gated by entity_terrain_collides so TerrainColl==0 /
           HasCollision==0 objects pass through (player always collides). */
        if (terrain && world_collision_enabled() && w->collision) {
            float pos[3] = { e->x, e->y, e->z };
            float vel[3] = { e->vx, e->vy, e->vz };
            collision_resolve_horizontal(w->collision, pos, e->half_extents, vel);
            e->x = pos[0]; e->z = pos[2];
            e->vx = vel[0]; e->vz = vel[2];
        }

        int was_on_ground = e->on_ground;
        e->on_ground = 0;
        e->y += e->vy * dt;
        for (Entity *s = w->head; s; s = s->next) {
            if (s == e || !s->alive || !s->vt) continue;
            if (!(s->runtime_flags & ENTITY_FLAG_SOLID)) continue;
            resolve_axis(e, s, 1);
        }
        if (terrain && !e->on_ground) {
            float terrain_y = 0.0f;
            float feet_y = e->y - e->half_extents[1];
            /* Mesh ground-follow: highest collider surface under the feet at or
               below feet + step cap (engine/collision.c). This is the
               authoritative floor wherever collider geometry exists. */
            int have_floor = 0;
            if (world_collision_enabled() && w->collision)
                have_floor = collision_ground_height(w->collision, e->x, e->z,
                                                      feet_y, &terrain_y, NULL);
            /* Safety-floor backstop (gated; see configure_safety_floor): only
               wins where there is no mesh floor under the entity — e.g. levels
               whose walkable surface is not yet a GROUND/BLOCK collider mesh,
               or when the player leaves the terrain. */
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
            if (!(t->runtime_flags & ENTITY_FLAG_TRIGGER)) continue;
            if (!physics_aabb_overlap(p, t)) continue;
            if (t->vt->on_trigger) t->vt->on_trigger(t, p);
        }
    }
}
