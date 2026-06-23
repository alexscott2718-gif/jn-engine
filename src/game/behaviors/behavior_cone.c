/* behavior_cone.c - C3DCone (3CON).
 *
 * The cone leaf has no owned gameplay method beyond destruction; shipped rows
 * are C3DSpriteType billboards (sprites.omt chunk 41, SpriteSize=70) with no
 * authored collision fields. Native behavior is therefore just the inherited
 * sprite/object visibility clock, leaving the visual resolver to draw the
 * authored sprite and keeping the row non-solid.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void cone_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
}

static void cone_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
}

const EntityVTable vt_cone = {
    .on_spawn   = cone_on_spawn,
    .on_update  = cone_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
