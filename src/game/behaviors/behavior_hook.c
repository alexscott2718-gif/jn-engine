/* behavior_hook.c - C3DHook (3HOO).
 *
 * This is the level-authored hook target, not the code-spawned grappling rope.
 * The class owns visual setup and a small post-C3DAI update wrapper; patrol,
 * target lookup, and movement come from C3DAI.
 */
#include "behavior_ai.h"
#include <stddef.h>

#define HOOK_PATROL_SPEED  250.0f
#define HOOK_ARRIVE_RADIUS 80.0f

static void hook_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = 100.0f;
    e->half_extents[1] = 100.0f;
    e->half_extents[2] = 100.0f;
}

static void hook_on_update(Entity *e, World *w, float dt) {
    const BehaviorAIPatrolConfig cfg = {
        .speed = HOOK_PATROL_SPEED,
        .arrive_radius = HOOK_ARRIVE_RADIUS,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_hook = {
    .on_spawn   = hook_on_spawn,
    .on_update  = hook_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
