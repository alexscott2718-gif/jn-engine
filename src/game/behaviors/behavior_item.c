#include "behaviors.h"
#include "../gamestate.h"
#include <stddef.h>
#include <math.h>

#define ITEM_BOB_AMP   12.0f
#define ITEM_BOB_FREQ  1.2f
#define ITEM_SPIN_RATE 2.4f

static void item_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = 30.0f;
    e->half_extents[1] = 30.0f;
    e->half_extents[2] = 30.0f;
    e->user_flag = 0;             /* 0 = uncollected, 1 = collected */
    e->user_float = e->y;         /* base y for bob */
    gamestate_item_added();
}

static void item_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;
    static float t = 0.0f;
    t += dt;
    e->y  = e->user_float + ITEM_BOB_AMP * sinf(6.28318f * ITEM_BOB_FREQ * t + e->x * 0.01f);
    e->ry += ITEM_SPIN_RATE * dt;
}

static void item_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag) return;
    e->user_flag = 1;
    e->alive = 0;
    gamestate_item_collected();
}

const EntityVTable vt_item = {
    .on_spawn = item_on_spawn,
    .on_update = item_on_update,
    .on_trigger = item_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
