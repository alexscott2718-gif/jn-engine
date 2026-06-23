/* behavior_leaves.c - C3DLeaves (3LEA).
 *
 * C3DLeaves is a C3DSpriteType -> C3DSprite leaf with zero owned vtable
 * methods (docs/decomp/C3DLeaves.md): all behavior is the inherited sprite/
 * object lifecycle. Shipped rows are authored billboards (sprite resolver
 * binds the leaf canvas) with no authored collision, so the native behavior
 * is just the inherited visibility/progress clock, keeping the row non-solid
 * and letting the visual resolver draw the authored sprite. Same shape as
 * vt_cone.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void leaves_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
}

static void leaves_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
}

const EntityVTable vt_leaves = {
    .on_spawn   = leaves_on_spawn,
    .on_update  = leaves_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
