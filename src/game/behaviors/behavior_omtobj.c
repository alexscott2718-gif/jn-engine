/* behavior_omtobj.c - C3DOmtObj (3OMT).
 *
 * C3DOmtObj's class-owned logic resolves OmtDatabase/OmtIndex to a 3DSh shape,
 * normalizes material state, applies SecondPass/HasCollision, then runs the
 * inherited C3DAnimated initial flags. The native visual resolver already
 * performs the authored OMT shape binding; this vtable supplies the gameplay
 * half: inherited visibility/progress gates plus Radius/HasCollision collision
 * setup.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

#define OMTOBJ_DEFAULT_RADIUS 5.0f

static void omtobj_apply_radius(Entity *e) {
    float r = gam_prop_f(e, "Radius", OMTOBJ_DEFAULT_RADIUS);
    if (r <= 0.0f) r = OMTOBJ_DEFAULT_RADIUS;
    e->half_extents[0] = r;
    e->half_extents[1] = r;
    e->half_extents[2] = r;
}

static void omtobj_apply_collision(Entity *e) {
    if (!e->visible || gam_prop_i(e, "HasCollision", -1) == 0)
        e->runtime_flags &= ~ENTITY_FLAG_SOLID;
    else
        e->runtime_flags |= ENTITY_FLAG_SOLID;
}

static void omtobj_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    omtobj_apply_radius(e);
    omtobj_apply_collision(e);
}

static void omtobj_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) {
        e->runtime_flags &= ~ENTITY_FLAG_SOLID;
        return;
    }
    omtobj_apply_collision(e);
}

const EntityVTable vt_omtobj = {
    .on_spawn   = omtobj_on_spawn,
    .on_update  = omtobj_on_update,
    .on_trigger = NULL,
    .flags      = ENTITY_FLAG_SOLID,
};
