/* behavior_creature.c — C3DDarwinFish (3FIS) / C3DGirlEatingPlant (3GIR).
 *
 * Shared native behavior for the "creatures one off set dressing" leaves that
 * derive from C3DEnemy -> C3DPickupType -> C3DAI (docs/decomp/C3DDarwinFish.md,
 * C3DGirlEatingPlant.md). Despite the C3DEnemy ancestry, neither leaf owns a
 * per-frame method: the only owned override is the asset registrar
 * (vfunc_04_067) that binds the walk/shrink/stop animation set
 * (darwinwalk/darwinshrink/darwinstop, plantwalk/plantshrink/plantwait). Their
 * runtime behavior is therefore purely the inherited C3DAI movement base — idle
 * in place, or patrol if the row authored a PatrolPoint — with no combat,
 * projectile, or pickup logic (family = creatures_one_off_set_dressing,
 * background set-dressing).
 *
 * Native model (a faithful subset of the inherited C3DAI base, matching the
 * vt_friend idiom without the friend-specific look-at / talk plumbing):
 *   - Spawn through behavior_ai_spawn_patrol (C3DAI patrol base).
 *   - Idle in place; patrol between nav nodes if a PatrolPoint is authored.
 *   - Visibility follows the inherited C3DAnimated InitiallyVisible / level gate.
 *   - Non-solid: these are background creatures, not collidable props.
 * entity_visual.c already resolves the stop meshes (darwinstop/plantstop).
 */
#include "behavior_ai.h"
#include <stddef.h>

#define CREATURE_WALK_SPEED   120.0f   /* C3DAI patrol pace for the wandering few */
#define CREATURE_ARRIVE        60.0f
#define CREATURE_HALF_X        70.0f
#define CREATURE_HALF_Y       110.0f
#define CREATURE_HALF_Z        70.0f

static void creature_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = CREATURE_HALF_X;
    e->half_extents[1] = CREATURE_HALF_Y;
    e->half_extents[2] = CREATURE_HALF_Z;
}

static void creature_on_update(Entity *e, World *w, float dt) {
    if (e->patrol_point[0]) {
        const BehaviorAIPatrolConfig cfg = {
            .speed = CREATURE_WALK_SPEED,
            .arrive_radius = CREATURE_ARRIVE,
            .loop_to_start = 1,
        };
        (void)behavior_ai_update_patrol(e, w, &cfg, dt);
        return;
    }

    if (!behavior_animated_update_base(e, w, dt)) return;
    behavior_ai_idle(e);
}

const EntityVTable vt_creature = {
    .on_spawn   = creature_on_spawn,
    .on_update  = creature_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
