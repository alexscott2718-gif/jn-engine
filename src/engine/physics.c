#include "physics.h"
#include <math.h>
#include <stddef.h>

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

void physics_step(World *w, float dt) {
    if (dt <= 0.0f) return;

    /* 1. Integrate gravity + velocity for physics entities. */
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || !e->vt) continue;
        if (!(e->vt->flags & ENTITY_FLAG_PHYSICS)) continue;

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

        /* Ground plane fallback. */
        float floor_y = w->ground_y + e->half_extents[1];
        if (e->y < floor_y) {
            e->y = floor_y;
            if (e->vy < 0.0f) e->vy = 0.0f;
            e->on_ground = 1;
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
