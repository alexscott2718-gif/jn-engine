/* behavior_arrow.c — C3DArrow (3ARR).
 *
 * C3DArrow is a C3DSpriteType nav billboard. Its owned decompiled methods only
 * register RequiredTask/RequiredLevel/ExactLevel and reset inherited sprite
 * state; the visual resolver already draws the authored sprites.omt chunk 33.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void arrow_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    if (!behavior_required_task_gate_allows(e))
        e->visible = 0;
}

static void arrow_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (!behavior_required_task_gate_allows(e))
        e->visible = 0;
}

const EntityVTable vt_arrow = {
    .on_spawn   = arrow_on_spawn,
    .on_update  = arrow_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
