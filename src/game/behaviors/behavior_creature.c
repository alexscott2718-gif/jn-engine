/* behavior_creature.c — the C3DAI "set-dressing creature" leaves:
 *   3FIS C3DDarwinFish, 3GIR C3DGirlEatingPlant, 3DIN C3DDino
 *        (C3DEnemy -> C3DPickupType -> C3DAI), and
 *   3CML C3DCamel, 3SPW C3DSparrow (plain C3DAI -> C3DAnimated).
 *
 * Their only owned override is the asset registrar (vfunc_04_067 / vfunc_01_259)
 * that binds a walk/shrink/stop animation set — e.g. dinowalk/dinoshrink/dinostop,
 * darwinwalk/darwinshrink/darwinstop, plantwalk/plantshrink/plantwait — plus a
 * scale. None owns a per-frame method, so runtime behavior is the inherited C3DAI
 * movement base: idle in place, or patrol if a PatrolPoint is authored. Camel and
 * Sparrow are plain C3DAI (no pickup ancestry); Sparrow's mesh is level-conditional
 * (vulture in LEV5), already resolved by entity_visual.c.
 *
 * SHRINK / PICKUP (faithfulness note, game-owner ground truth 2026-06-23): these
 * creatures are NOT inert set-dressing. The shrink ray, fired at certain of them
 * (Dino, Darwin, Humphrey, ...), shrinks them — they play HISHRINK, scale down, and
 * become small MOVING pickups the player can collect. That is exactly why the chain
 * carries C3DPickupType (the global pickup-state table) and why each registers a
 * HISHRINK frame. The ACTIVE mechanic is deferred, not denied: the shrink-on-contact
 * -> HISHRINK -> moving-pickup transition body is not decompiled (C3DShrinkRay only
 * animates the ray; 3SHR has zero .gam placements), so wiring it would invent the
 * fire input / scale / scoring. Per user direction (2026-06-23) we only record the
 * truth here; see docs/decomp/C3DShrinkRay.md + PROJECT_HISTORY.
 *
 * Native model (the inherited C3DAI base, matching the vt_friend idiom without the
 * friend look-at / talk plumbing):
 *   - Spawn through behavior_ai_spawn_patrol (C3DAI patrol base).
 *   - Idle in place; patrol between nav nodes if a PatrolPoint is authored.
 *   - Visibility follows the inherited C3DAnimated InitiallyVisible / level gate.
 *   - Non-solid: these are background creatures, not collidable props.
 * entity_visual.c already resolves the stop meshes (dinostop/darwinstop/plantstop/
 * camel Box01/vulture01).
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
