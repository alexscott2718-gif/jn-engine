/* behavior_light.c - C3DLight (3LIG).
 *
 * C3DLight derives from OMediaLight: its InitObject registers twelve authored
 * light properties (LightType, LightRange, CAttenuation, Dif/Spec/Amb RGB) and
 * its one owned override just stashes the light handle. It has no visual, no
 * collision, and no per-frame gameplay body. The native renderer's lighting is
 * measured OFF (flat full-bright — see PROJECT_HISTORY §Invariants), so a
 * faithful port preserves the authored fields without inventing a lighting side
 * effect: an invisible data row that runs the shared visibility/progress gate
 * and clears solidity/trigger. entity_visual.c already marks 3LIG invisible.
 *
 * (Sibling of behavior_lightobj.c / 3LIO; that one is a C3DSpriteType light-data
 * row, this one is the OMediaLight scene light. Both are inert data carriers.)
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void light_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void light_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

const EntityVTable vt_light = {
    .on_spawn   = light_on_spawn,
    .on_update  = light_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
