/* behavior_cindy.c - C3DCindy (3CIN).
 *
 * Cindy is a C3DFriends leaf: movement and talk-marker maintenance are
 * inherited from C3DAI/C3DFriends, while the leaf adds SCENE task visibility
 * windows. Talk reward side effects are deferred until the task-state mutator
 * path exists; this native leaf already preserves TaskName/TalkTrigger fields.
 */
#include "behavior_ai.h"
#include "../game_flow.h"
#include <stddef.h>
#include <strings.h>

#define CINDY_WALK_SPEED    160.0f
#define CINDY_ARRIVE_RADIUS  60.0f

static long cindy_scene_state(const Entity *e) {
    const char *key = (e && e->task_name[0]) ? e->task_name : "SCENE";
    return game_flow_entity_state(key);
}

static int cindy_progress_visible(const Entity *e) {
    long scene = cindy_scene_state(e);
    if (scene < 0) return 1;  /* free-level QA / no CTaskList loaded */

    const char *level = game_flow_current_level();
    if (strcasecmp(level, "level4") == 0)
        return scene >= 0x104 && scene < 0x118;
    if (strcasecmp(level, "level3d") == 0)
        return scene >= 0x190 && scene < 0x1a4;
    if (strcasecmp(level, "level4a") == 0)
        return scene >= 0x1f4;
    return 1;
}

static void cindy_hide(Entity *e) {
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    behavior_ai_idle(e);
}

static void cindy_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = 90.0f;
    e->half_extents[1] = 170.0f;
    e->half_extents[2] = 90.0f;
    if (!cindy_progress_visible(e))
        cindy_hide(e);
}

static void cindy_on_update(Entity *e, World *w, float dt) {
    if (!cindy_progress_visible(e)) {
        cindy_hide(e);
        return;
    }

    const BehaviorAIPatrolConfig cfg = {
        .speed = CINDY_WALK_SPEED,
        .arrive_radius = CINDY_ARRIVE_RADIUS,
        /* Cindy's routes encode loops explicitly (level4 CINDY7 -> CINDY1).
           Short routes ending in "none" should terminate instead of ping-pong. */
        .loop_to_start = 0,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_cindy = {
    .on_spawn   = cindy_on_spawn,
    .on_update  = cindy_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
