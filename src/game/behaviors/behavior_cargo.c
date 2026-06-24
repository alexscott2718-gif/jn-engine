/* behavior_cargo.c — C3DYokCargo (3YCA), a SCENE-gated cargo prop.
 *
 * C3DYokCargo is a C3DAI leaf whose owned per-frame method vfunc_01_265
 * (docs/decomp/C3DYokCargo.md) is purely a visibility gate keyed on the level
 * code and SCENE:
 *
 *     level != "LEV5"  -> show
 *     level == "LEV5"  -> show iff SCENE > 0x1e9 (489); else hide
 *
 * So the cargo_ship is visible in every level it is placed (Level1D/1F/3/3D/5b/
 * 4c/5/5a) EXCEPT base level5, where it only appears late in that level's story
 * once SCENE has advanced past 489 (a beat reached via the AITrigger
 * story-progress chain — see docs/decomp/_scene_sequencer.md). The decomp body
 * does not move it, so this is a static, visibility-gated prop (no patrol).
 *
 * Regression-safety: same as C3DFowl/C3DCindy — SCENE < 0 (no CTaskList loaded,
 * i.e. a direct `--level` / audit / screenshot launch) is treated as "show", so
 * the harnesses are unaffected and the LEV5 hide only bites in campaign runs.
 */
#include "behavior_base.h"
#include "../game_flow.h"
#include <stddef.h>
#include <strings.h>

static int cargo_visible(void) {
    long scene = game_flow_entity_state("SCENE");
    if (scene < 0) return 1;  /* no CTaskList loaded (direct --level / QA) -> show */
    if (strcasecmp(game_flow_current_level(), "level5") != 0) return 1; /* != LEV5 */
    return scene > 0x1e9;     /* LEV5: visible only after SCENE > 489 */
}

static void cargo_apply(Entity *e) {
    if (cargo_visible()) {
        e->visible = 1;
    } else {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    }
}

static void cargo_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);   /* InitiallyVisible / level gate */
    if (e->visible) cargo_apply(e);
}

static void cargo_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    cargo_apply(e);
}

const EntityVTable vt_cargo = {
    .on_spawn   = cargo_on_spawn,
    .on_update  = cargo_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
