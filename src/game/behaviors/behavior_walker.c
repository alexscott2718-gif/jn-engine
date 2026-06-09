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
 *   user_flag      = 0 walking, 1 waiting
 *   user_float     = wait-timer seconds remaining
 */
#include "behaviors.h"
#include <math.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>

#define WALK_SPEED     160.0f   /* units/sec (AISpeed authored as -1 = default) */
#define ARRIVE_RADIUS   60.0f

static Entity *find_waypoint(World *w, const char *tag) {
    if (!tag || !tag[0]) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (strncmp(e->type, "3PAT", 4) != 0) continue;
        if (strcasecmp(e->tag, tag) == 0) return e;
    }
    return NULL;
}

static void walker_on_spawn(Entity *e, World *w) {
    (void)w;
    /* Remember the first waypoint so the patrol can loop. */
    memcpy(e->start_point, e->patrol_point, sizeof(e->start_point));
    e->user_flag  = 0;     /* walking */
    e->user_float = 0.0f;
}

static void walker_on_update(Entity *e, World *w, float dt) {
    if (!e->patrol_point[0]) return;   /* no route authored */

    if (e->user_flag == 1) {           /* waiting at a point */
        e->user_float -= dt;
        if (e->user_float <= 0.0f) e->user_flag = 0;
        return;
    }

    Entity *wp = find_waypoint(w, e->patrol_point);
    if (!wp) { e->patrol_point[0] = '\0'; return; }  /* dangling edge -> idle */

    float dx = wp->x - e->x;
    float dz = wp->z - e->z;
    float dist = sqrtf(dx*dx + dz*dz);

    if (dist <= ARRIVE_RADIUS) {
        /* Arrived: route to the next edge (loop to start when the chain ends),
           then honour this point's WaitTime before resuming. */
        const char *next = wp->next_patrol[0] ? wp->next_patrol : e->start_point;
        /* next aliases wp->next_patrol or e->start_point; copy before any write
           to e->patrol_point (start_point is a separate field, so safe here). */
        char buf[32];
        size_t n = strlen(next);
        if (n >= sizeof(buf)) n = sizeof(buf) - 1;
        memcpy(buf, next, n); buf[n] = '\0';
        memcpy(e->patrol_point, buf, sizeof(e->patrol_point));
        float wt = gam_prop_f(wp, "WaitTime", 0.0f);
        if (wt > 0.0f) { e->user_flag = 1; e->user_float = wt; }
        return;
    }

    /* Walk toward the waypoint and face the travel direction. The engine yaw
       forward convention is (-sin ry, -cos ry) (see behavior_fan airflow). */
    float inv = 1.0f / dist;
    e->x += dx * inv * WALK_SPEED * dt;
    e->z += dz * inv * WALK_SPEED * dt;
    e->ry = atan2f(-dx, -dz);
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
