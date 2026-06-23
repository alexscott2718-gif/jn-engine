/* behavior_lightobj.c - C3DLightObj (3LIO).
 *
 * C3DLightObj is a C3DSpriteType-derived light-data row. The decompiled leaf
 * only registers authored color/alpha/pulse/sound properties; it has no owned
 * update, touch, or trigger body. Native behavior is therefore the shared
 * C3DObject/C3DSprite lifecycle gate, with no collision and no invented light
 * renderer side effect. entity_visual.c already marks 3LIO invisible.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void lightobj_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void lightobj_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

const EntityVTable vt_lightobj = {
    .on_spawn   = lightobj_on_spawn,
    .on_update  = lightobj_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
