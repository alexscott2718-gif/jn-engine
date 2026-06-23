/* behavior_eye.c - C3DEye (3EYE).
 *
 * Eye only owns visual setup in docs/decomp/C3DEye.md. The placed rows carry
 * C3DAICar/C3DAI fields (PatrolPoint, TargetName, VisibleRange, AIState=3), so
 * the native leaf reuses the shared patrol base. C3DAICar horn/contact effects
 * remain a base-class backlog item.
 */
#include "behavior_ai.h"
#include <stddef.h>

#define EYE_PATROL_SPEED  400.0f
#define EYE_ARRIVE_RADIUS 120.0f

static void eye_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = 140.0f;
    e->half_extents[1] = 100.0f;
    e->half_extents[2] = 140.0f;
}

static void eye_on_update(Entity *e, World *w, float dt) {
    const BehaviorAIPatrolConfig cfg = {
        .speed = EYE_PATROL_SPEED,
        .arrive_radius = EYE_ARRIVE_RADIUS,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_eye = {
    .on_spawn   = eye_on_spawn,
    .on_update  = eye_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
