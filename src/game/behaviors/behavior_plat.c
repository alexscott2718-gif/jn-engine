#include "behaviors.h"
#include <stddef.h>
#include <math.h>

#define PLAT_AMPLITUDE  300.0f
#define PLAT_FREQ_HZ    0.4f

static void plat_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = 120.0f;
    e->half_extents[1] = 20.0f;
    e->half_extents[2] = 120.0f;
    /* Stash base Y in user_float so we can oscillate around it. */
    e->user_float = e->y;
}

static void plat_on_update(Entity *e, World *w, float dt) {
    (void)w;
    static float t = 0.0f;
    t += dt;
    /* Per-platform phase offset based on world X to avoid all-in-sync motion. */
    float phase = e->x * 0.001f;
    float new_y = e->user_float + PLAT_AMPLITUDE * sinf(6.28318f * PLAT_FREQ_HZ * t + phase);
    /* Velocity recorded so player on top is carried (used by physics). */
    e->vy = (new_y - e->y) / (dt > 0.0001f ? dt : 0.0001f);
    e->y = new_y;
}

const EntityVTable vt_plat = {
    .on_spawn = plat_on_spawn,
    .on_update = plat_on_update,
    .on_trigger = NULL,
    .flags = ENTITY_FLAG_SOLID,
};
