/* behavior_digger.c - C3DDigger (3DIG).
 *
 * The spec shows no Digger-owned update, contact, task, or trigger method after
 * construction. Runtime is inherited C3DAnimated: visibility/progress gates and
 * animation time advance.
 */
#include "behavior_base.h"
#include <stddef.h>

static void digger_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->half_extents[0] = 130.0f;
    e->half_extents[1] = 130.0f;
    e->half_extents[2] = 130.0f;
}

static void digger_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
}

const EntityVTable vt_digger = {
    .on_spawn   = digger_on_spawn,
    .on_update  = digger_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
