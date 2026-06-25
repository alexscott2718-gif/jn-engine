/* behavior_resolver_inert.c - native coverage for resolver-only tail rows.
 *
 * The last used-in-level FourCCs without native vtables are not portable
 * gameplay gaps:
 *
 *   3ROK C3DRock   - 99 Level5b pool rows, all authored at (0,0,0). The class
 *                    owns only destruction; real placement is runtime-driven by
 *                    an unported controller, so drawing/colliding the origin
 *                    pool would be less faithful than hiding it.
 *   3SPR C3DSprite - 15 rows with no serialized SpriteSize/SpriteDatabase/
 *                    SpriteIndex. C3DSprite's canvas loader has a fallback, but
 *                    the default canvas/size for these rows is unresolved.
 *   3DAI C3DAI     - 4 bare AI-base rows with no useful serialized fields or
 *                    fixed visual asset; the current corpus treats them as
 *                    origin/dummy framework rows.
 *
 * Registering a vtable here makes that decision explicit for the behavior lens
 * while preserving the existing resolver output: invisible, non-solid, no
 * movement, no trigger side effects.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>

static void resolver_inert_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void resolver_inert_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

const EntityVTable vt_resolver_inert = {
    .on_spawn   = resolver_inert_on_spawn,
    .on_update  = resolver_inert_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
