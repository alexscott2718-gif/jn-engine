/* behavior_stalactite.c — C3DStalagtite (3STA).
 *
 * C3DStalagtite (note the in-binary spelling) is a world_props_terrain leaf on
 * the plain C3DAnimated -> C3DObject chain — no AI, pickup, or projectile
 * ancestry (docs/decomp/C3DStalagtite.md). Its two owned methods are a
 * trigger-activated response, not a per-frame integrator: vfunc_04_072 plays a
 * sound and asserts an "active" flag on itself plus up to three child objects,
 * and vfunc_01_010 fires the exit/deactivate slot once. That drop/relay path is
 * driven by the scripted trigger-chain dispatch, which is not yet ported (see
 * PROJECT_HISTORY / the AITrigger note), so the faithful minimal port is a
 * static hanging prop: the inherited visibility/progress gate, the authored
 * stalactite mesh (already resolved by entity_visual.c), and no collision — an
 * un-fallen ceiling spike is non-solid until its trigger fires.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void stalactite_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void stalactite_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

const EntityVTable vt_stalactite = {
    .on_spawn   = stalactite_on_spawn,
    .on_update  = stalactite_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
