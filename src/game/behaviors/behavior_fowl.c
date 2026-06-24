/* behavior_fowl.c — C3DFowl (3FOW), a SCENE-gated set-dressing creature.
 *
 * C3DFowl is a C3DFriends -> C3DAI leaf. Its one defining owned method is the
 * per-frame visibility gate vfunc_01_265 (docs/decomp/C3DFowl.md): the fowl is
 * shown ONLY while the global SCENE task value sits inside a per-level window,
 * and hidden otherwise (and hidden outright in any level the switch doesn't
 * name). The windows, read straight from the decompiled body:
 *
 *     LEV4  (level4)  : 0x121 < SCENE < 0x136
 *     LV4A  (level4a) : SCENE > 499
 *     LV4C  (level4c) : 0x1cb < SCENE < 0x1d6   <- the FOWLINV story beat
 *     else            : hidden
 *
 * SCENE now moves at runtime via the AITrigger story-progress patch table
 * (behavior_ai_trigger.c / docs/decomp/_scene_sequencer.md): in level4c the
 * `fowlinv` trigger advances SCENE 0x1cc -> 0x1d6, which closes this window — so
 * the fowl appears during that beat and vanishes when the player resolves it.
 *
 * Regression-safety: a direct `--level X` launch loads no CTaskList, so
 * game_flow_entity_state("SCENE") returns -1; like C3DCindy, that is treated as
 * "show" so the audit/screenshot harnesses (SCENE unloaded) keep rendering the
 * fowl. The window only bites in campaign (--newgame / --menu) runs.
 *
 * Movement: inherited C3DAI idle (the fowl is set-dressing; talk/look-at reward
 * plumbing is deferred with the rest of the friend cast).
 */
#include "behavior_ai.h"
#include "behavior_base.h"
#include "../game_flow.h"
#include <stddef.h>
#include <strings.h>

static int fowl_visible(const Entity *e) {
    (void)e;
    long scene = game_flow_entity_state("SCENE");
    if (scene < 0) return 1;  /* no CTaskList loaded (direct --level / QA) -> show */

    const char *level = game_flow_current_level();
    if (strcasecmp(level, "level4")  == 0) return scene > 0x121 && scene < 0x136;
    if (strcasecmp(level, "level4a") == 0) return scene > 499;
    if (strcasecmp(level, "level4c") == 0) return scene > 0x1cb && scene < 0x1d6;
    return 0;  /* decomp else-branch: hidden in any other level */
}

static void fowl_hide(Entity *e) {
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void fowl_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);   /* InitiallyVisible / level gate */
    e->half_extents[0] = 80.0f;
    e->half_extents[1] = 120.0f;
    e->half_extents[2] = 80.0f;
    if (!fowl_visible(e))
        fowl_hide(e);
}

static void fowl_on_update(Entity *e, World *w, float dt) {
    (void)dt;
    if (!e->alive) return;
    if (!fowl_visible(e)) {
        fowl_hide(e);
        return;
    }
    e->visible = 1;
    behavior_ai_idle(e);
}

const EntityVTable vt_fowl = {
    .on_spawn   = fowl_on_spawn,
    .on_update  = fowl_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
