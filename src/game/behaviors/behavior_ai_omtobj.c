/* behavior_ai_omtobj.c - C3DAIOmtObj (3AIO).
 *
 * The AI-bearing sibling of C3DOmtObj (docs/decomp/C3DAIOmtObj.md). Its one
 * owned slot (LoadAIOmtShapeAndCollision) resolves OmtDatabase/OmtIndex to a
 * 3DSh shape, runs the C3DAnimated initial flags, forwards Radius to the
 * collision/state setter, and applies TerrainColl to the inherited collision
 * hooks. Crucially it does NOT call C3DAI::PostLoadAI, so the AI target/state
 * setup is skipped — the placed rows are static crash-pod / pod / "friedeggs"
 * props (no patrol/seek at runtime).
 *
 * The visual resolver already binds the authored OMT shape, so this vtable
 * supplies the gameplay half (mirroring behavior_omtobj.c): inherited
 * visibility/progress gates plus Radius-derived collision extents, with
 * solidity cleared when TerrainColl==0 (collision hooks disabled) or
 * HasCollision==0.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

#define AI_OMTOBJ_DEFAULT_RADIUS 5.0f

static void ai_omtobj_apply_radius(Entity *e) {
    float r = gam_prop_f(e, "Radius", AI_OMTOBJ_DEFAULT_RADIUS);
    if (r <= 0.0f) r = AI_OMTOBJ_DEFAULT_RADIUS;
    e->half_extents[0] = r;
    e->half_extents[1] = r;
    e->half_extents[2] = r;
}

static void ai_omtobj_apply_collision(Entity *e) {
    /* TerrainColl: 0 disables the inherited terrain/collision hooks; -1 (the
       default) and 1 leave collision on. HasCollision==0 also clears solidity. */
    int terrain = gam_prop_i(e, "TerrainColl", -1);
    if (!e->visible || terrain == 0 || gam_prop_i(e, "HasCollision", -1) == 0)
        e->runtime_flags &= ~ENTITY_FLAG_SOLID;
    else
        e->runtime_flags |= ENTITY_FLAG_SOLID;
}

static void ai_omtobj_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    ai_omtobj_apply_radius(e);
    ai_omtobj_apply_collision(e);
}

static void ai_omtobj_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) {
        e->runtime_flags &= ~ENTITY_FLAG_SOLID;
        return;
    }
    ai_omtobj_apply_collision(e);
}

const EntityVTable vt_ai_omtobj = {
    .on_spawn   = ai_omtobj_on_spawn,
    .on_update  = ai_omtobj_on_update,
    .on_trigger = NULL,
    .flags      = ENTITY_FLAG_SOLID,
};
