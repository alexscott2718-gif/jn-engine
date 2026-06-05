#include "behaviors.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>

/* C3DMovingPlatform (3MOP): patrols between its spawn position ("home") and a
   named PatrolPoint marker, smoothly ping-ponging. Solid so the player rides it.
   If the PatrolPoint can't be resolved the platform simply stays put (still a
   solid, visible block). */

#define MOP_PERIOD 5.0f   /* seconds for a full home -> target -> home cycle */

static void movplat_on_spawn(Entity *e, World *w) {
    e->half_extents[0] = 140.0f;
    e->half_extents[1] = 24.0f;
    e->half_extents[2] = 140.0f;
    e->home[0] = e->patrol_to[0] = e->x;
    e->home[1] = e->patrol_to[1] = e->y;
    e->home[2] = e->patrol_to[2] = e->z;
    e->user_float = 0.0f;   /* cycle phase (seconds) */

    if (e->patrol_point[0]) {
        for (Entity *o = w->head; o; o = o->next) {
            if (o == e || !o->tag[0]) continue;
            if (strcasecmp(o->tag, e->patrol_point) == 0) {
                e->patrol_to[0] = o->x;
                e->patrol_to[1] = o->y;
                e->patrol_to[2] = o->z;
                break;
            }
        }
    }
    float dx = e->patrol_to[0]-e->home[0], dy = e->patrol_to[1]-e->home[1], dz = e->patrol_to[2]-e->home[2];
    float dist = sqrtf(dx*dx+dy*dy+dz*dz);
    printf("[MOP] '%s' patrol='%s' %s travel=%.0f\n",
           e->tag, e->patrol_point,
           dist > 1.0f ? "MOVING" : "static(no target)", dist);
}

static void movplat_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (dt <= 0.0001f) return;
    e->user_float += dt;
    /* Smooth 0->1->0 ping-pong via a raised cosine. */
    float s = 0.5f - 0.5f * cosf(6.28318f * (e->user_float / MOP_PERIOD));
    float nx = e->home[0] + (e->patrol_to[0] - e->home[0]) * s;
    float ny = e->home[1] + (e->patrol_to[1] - e->home[1]) * s;
    float nz = e->home[2] + (e->patrol_to[2] - e->home[2]) * s;
    /* Record velocity so physics can carry a rider (vy is used for vertical
       carry; vx/vz are informational). */
    e->vx = (nx - e->x) / dt;
    e->vy = (ny - e->y) / dt;
    e->vz = (nz - e->z) / dt;
    e->x = nx; e->y = ny; e->z = nz;
}

const EntityVTable vt_movplat = {
    .on_spawn   = movplat_on_spawn,
    .on_update  = movplat_on_update,
    .on_trigger = NULL,
    .flags      = ENTITY_FLAG_SOLID,
};
