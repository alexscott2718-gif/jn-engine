/* behavior_yokian_ship.c - C3DYokianShip (3YSH).
 *
 * Note the duplicate FourCC: C3DYokianShield also registers 3YSH, but the
 * placed .gam rows are ship-tagged C3DAI actors. Native ports the placed ship
 * path here; the shield sprite remains a runtime Yokian helper.
 */
#include "behavior_ai.h"
#include <stddef.h>

#define YSHIP_PATROL_SPEED  240.0f
#define YSHIP_ARRIVE_RADIUS 140.0f

static void yokian_ship_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->home[0] = e->x;
    e->home[1] = e->y;
    e->home[2] = e->z;
    e->half_extents[0] = 180.0f;
    e->half_extents[1] = 120.0f;
    e->half_extents[2] = 180.0f;
}

static void yokian_ship_on_update(Entity *e, World *w, float dt) {
    BehaviorAIPatrolConfig cfg = {
        .speed = YSHIP_PATROL_SPEED,
        .arrive_radius = YSHIP_ARRIVE_RADIUS,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_yokian_ship = {
    .on_spawn   = yokian_ship_on_spawn,
    .on_update  = yokian_ship_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
