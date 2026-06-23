/* behavior_rocket_ai.c — C3DRocket (3RCK), the placed AI patrol rocket.
 *
 * Distinct from the player-rideable C3DRocketShip (3ROC, behavior_vehicle.c's
 * vt_rocket). C3DRocket derives from C3DAI and is authored directly into levels
 * (9 .gam rows across 5 levels) — it is NOT a code-spawned projectile. Each row
 * carries the inherited C3DAI fields (PatrolPoint "rock1"/"rock1b"/"rc1",
 * TargetName "JIM1", VisibleRange 2500, AIState 1..3), so it flies its authored
 * patrol chain (docs/decomp/C3DRocket.md).
 *
 * Native model (a faithful subset of the C3DAI patrol the rocket inherits):
 *   - Spawn through the shared C3DAI patrol base (behavior_ai): patrol if a
 *     PatrolPoint was authored (all current rows have one), else idle.
 *   - Per frame, advance along the PatrolPoint -> NextPatrolPoint nav chain via
 *     the shared behavior_ai patrol primitive — the same path vt_ai_vehicle
 *     uses for self-driving traffic. The rocket is invisible in this build
 *     (entity_visual hides 3RCK; most rows also author InitiallyVisible=0), so
 *     the inherited InitiallyVisible/level gate in the patrol base naturally
 *     keeps the boot-hidden instances inert until scripting would reveal them.
 *
 * Deferred (matches the decomp's distinctive but non-essential extras):
 *   - The ten-entry C3DNewSmokePuff exhaust pool emitted every 0.1s while
 *     AIState==3 (needs the C3DNewSmokePuff effect class, not yet ported).
 *   - The objects.omt id-15 sprite binding (the rocket is rendered hidden here).
 */
#include "behaviors.h"
#include "behavior_ai.h"
#include <stddef.h>

#define ROCKET_AI_SPEED   600.0f   /* a rocket cruises faster than ground traffic */
#define ROCKET_AI_ARRIVE  150.0f   /* large, fast body — generous waypoint radius */

static void rocket_ai_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
}

static void rocket_ai_on_update(Entity *e, World *w, float dt) {
    const BehaviorAIPatrolConfig cfg = {
        .speed = ROCKET_AI_SPEED,
        .arrive_radius = ROCKET_AI_ARRIVE,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

/* flags=0: like vt_ai_vehicle, the patrolling rocket is not AABB-solid in this
   native subset (it flies through the scene rather than blocking the player). */
const EntityVTable vt_rocket_ai = {
    .on_spawn   = rocket_ai_on_spawn,
    .on_update  = rocket_ai_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
