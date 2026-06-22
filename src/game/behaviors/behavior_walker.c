/* behavior_walker.c — AI/nav cluster.
 *
 *   vt_patrolpoint (3PAT, C3DPatrolPoint): a passive navigation-graph node.
 *     Its state lives entirely in mapped .gam fields — position plus
 *     `NextPatrolPoint` (e->next_patrol, the graph edge). No per-frame logic; it
 *     is invisible (entity_visual marks 3PAT invisible) and simply findable by
 *     tag. See docs/decomp/C3DPatrolPoint.md.
 *
 *   vt_walker (3CAR, a C3DAI consumer): a simple patrol walker. From shipped
 *     .gam data, Carl ("C3DCARL") authors PatrolPoint='CARL1' and CanMove=1; the
 *     3PAT chain is carl1 -> carl2 -> carl3. The walker heads to its current
 *     target waypoint, waits the point's WaitTime on arrival, then routes to that
 *     point's NextPatrolPoint. When a chain ends (NextPatrolPoint=none) it loops
 *     back to the start point so the patrol is continuously visible in QA (the
 *     original ends/idles per AIState; looping is a deliberate demo choice).
 *
 * Faithful to docs/decomp/C3DPatrolPoint.md's OnArrive: wait-anim/time, then
 * route to NextPatrolPoint. WaitAnim/sound/CallObjectTag dispatch are not yet
 * wired (no AI anim-state machine); movement + routing are.
 *
 * State (Entity scratch):
 *   patrol_point[] = current target waypoint tag (advances along the chain)
 *   start_point[]  = first waypoint tag (loop anchor; 3CAR doesn't use LOAD)
 *   user_flag      = behavior_ai patrol state
 *   user_float     = wait-timer seconds remaining
 */
#include "behaviors.h"
#include "behavior_ai.h"
#include <stddef.h>

#define WALK_SPEED     160.0f   /* units/sec (AISpeed authored as -1 = default) */
#define ARRIVE_RADIUS   60.0f

static void walker_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
}

static void walker_on_update(Entity *e, World *w, float dt) {
    const BehaviorAIPatrolConfig cfg = {
        .speed = WALK_SPEED,
        .arrive_radius = ARRIVE_RADIUS,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_walker = {
    .on_spawn   = walker_on_spawn,
    .on_update  = walker_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};

/* Passive nav-graph node. No per-frame logic; position + next_patrol are read
   by walkers. Invisible via entity_visual. */
const EntityVTable vt_patrolpoint = {
    .on_spawn   = NULL,
    .on_update  = NULL,
    .on_trigger = NULL,
    .flags      = 0,
};
