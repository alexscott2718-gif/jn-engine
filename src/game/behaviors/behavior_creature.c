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
 * PICTURE AWARD (2026-08-19): 19 creature rows author PIC_NUMBER — 3FIS 12,
 * 3GIR 5, 3DIN 2 — so they are part of the picture economy and their award path
 * lives here, not in behavior_item.c's vt_item (different FourCC, different
 * vtable). creature_on_trigger runs the shared CPickupType collection core.
 *
 * What is deliberately NOT done: the vtable keeps flags = 0, so the engine's
 * overlap dispatch never calls that on_trigger. Adding ENTITY_FLAG_TRIGGER would
 * make walking into a dino collect it, which is exactly the invented mechanic the
 * note above refuses — in the original these are collectible only *after* the
 * shrink ray turns them into pickups. The award path is built and testable
 * (JN_TEST_PICTURES sweeps it directly); the thing that should fire it is still
 * the undecompiled shrink transition. When that lands, it calls this.
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
#include "behavior_base.h"
#include "../gamestate.h"
#include <stddef.h>
#include <string.h>

#define CREATURE_WALK_SPEED   120.0f   /* C3DAI patrol pace for the wandering few */
#define CREATURE_ARRIVE        60.0f
#define CREATURE_HALF_X        70.0f
#define CREATURE_HALF_Y       110.0f
#define CREATURE_HALF_Z        70.0f

static void creature_on_spawn(Entity *e, World *w) {
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = CREATURE_HALF_X;
    e->half_extents[1] = CREATURE_HALF_Y;
    e->half_extents[2] = CREATURE_HALF_Z;
    e->user_flag = 0;
    /* A creature already taken on an earlier visit stays taken; same
       PostLoadPickupItem rule the 3PIC rows follow. No creature row authors
       InitallyActive, so the other half of the gate is inert here. */
    behavior_pickup_spawn_gate(e, w);
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

/* The CPickupType half of the chain. Reachable only from a caller that has
   decided the creature is collectible (the deferred shrink transition, or the
   JN_TEST_PICTURES sweep) — see the header note on why flags stays 0. */
static void creature_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag) return;
    if (!behavior_pickup_gate_allows(e)) return;   /* no creature row authors
                                                      RequiredPicNum today */
    if (behavior_pickup_taken(e)) return;

    e->user_flag = 1;
    behavior_pickup_mark_taken(e);
    e->visible = 0;
    e->alive = 0;
    behavior_pickup_award_pictures(e);
    if (e->points) gamestate_add_points(e->points);
}

const EntityVTable vt_creature = {
    .on_spawn   = creature_on_spawn,
    .on_update  = creature_on_update,
    .on_trigger = creature_on_trigger,
    .flags      = 0,
};
