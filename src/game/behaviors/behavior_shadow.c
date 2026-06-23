/* behavior_shadow.c - C3DShadow (3TAR).
 *
 * C3DShadow is a C3DPermanentSprite -> C3DSprite leaf with zero owned vtable
 * methods (docs/decomp/C3DShadow.md): all behavior is inherited. It is a
 * world_props_terrain decor billboard with no authored collision, so the
 * native behavior is the inherited visibility/progress clock only, keeping the
 * row non-solid while the visual resolver draws the authored sprite. Same
 * shape as vt_cone / vt_leaves.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void shadow_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
}

static void shadow_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
}

const EntityVTable vt_shadow = {
    .on_spawn   = shadow_on_spawn,
    .on_update  = shadow_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
