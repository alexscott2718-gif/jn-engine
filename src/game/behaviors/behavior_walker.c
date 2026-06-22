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
 *     Carl's Level 1B row is a special friend/action state (AIState=9), not a
 *     generic patrol: C3DCarl's LV1B hook places him on CARLESC3 for the
 *     rescue/escape scene, so it is handled as a thin leaf override below.
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
#include <string.h>
#include <strings.h>

#define WALK_SPEED     160.0f   /* units/sec (AISpeed authored as -1 = default) */
#define ARRIVE_RADIUS   60.0f

static Entity *walker_find_tag(World *w, const char *tag) {
    if (!w || !tag || !tag[0]) return NULL;
    for (Entity *it = w->head; it; it = it->next) {
        if (strcasecmp(it->tag, tag) == 0) return it;
    }
    return NULL;
}

static int walker_is_level1b_carl(const Entity *e, World *w, Entity **esc3_out) {
    if (esc3_out) *esc3_out = NULL;
    if (!e || strncmp(e->type, "3CAR", 4) != 0) return 0;
    if (strcasecmp(e->tag, "C3DCARL") != 0) return 0;
    if (gam_prop_i(e, "AIState", -1) != 9) return 0;

    Entity *esc3 = walker_find_tag(w, "CARLESC3");
    if (!esc3) return 0;
    if (esc3_out) *esc3_out = esc3;
    return 1;
}

static int walker_apply_level1b_carl_hook(Entity *e, World *w) {
    Entity *esc3 = NULL;
    if (!walker_is_level1b_carl(e, w, &esc3)) return 0;

    /* C3DCarl::ApplyCarlProgressVisibility: on LV1B, scene states 0..0x3c
       keep Carl enabled; scene > 0x1e copies the CARLESC3 transform and enters
       action state 6. Without the task system, use the row's first talk state
       (50) as the local default so direct Level 1B loads match the rescue path. */
    int scene = behavior_progress_gate_enabled()
        ? behavior_progress_level()
        : gam_prop_i(e, "TalkState0", 0x32);

    behavior_ai_idle(e);
    e->patrol_point[0] = '\0';
    e->start_point[0] = '\0';
    e->user_flag = 0;
    e->user_float = 0.0f;

    if (scene < 0 || scene > 0x3c) {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
        return 1;
    }

    if (scene > 0x1e) {
        e->x = esc3->x;
        e->y = esc3->y;
        e->z = esc3->z;
        e->rx = esc3->rx;
        e->ry = esc3->ry;
        e->rz = esc3->rz;
        e->user_flag = 6;
    }
    return 1;
}

static void walker_on_spawn(Entity *e, World *w) {
    behavior_ai_spawn_patrol(e);
    (void)walker_apply_level1b_carl_hook(e, w);
}

static void walker_on_update(Entity *e, World *w, float dt) {
    if (walker_is_level1b_carl(e, w, NULL)) {
        if (behavior_animated_update_base(e, w, dt))
            (void)walker_apply_level1b_carl_hook(e, w);
        return;
    }

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
