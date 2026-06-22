#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include <stddef.h>
#include <stdio.h>

static void load_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, 60.0f, 60.0f, 60.0f);
}

static void load_on_trigger(Entity *e, Entity *by) {
    if (e->user_flag) return; /* fire once */
    e->user_flag = 1;
    printf("[LOAD] level change requested by %s (tag='%s' -> '%s' spawn='%s')\n",
           by ? by->type : "?", e->tag, e->target_level, e->start_point);
    if (e->target_level[0])
        gamestate_request_level_swap(e->target_level, e->start_point);
    else
        gamestate_request_level_change();
}

const EntityVTable vt_load = {
    .on_spawn = load_on_spawn,
    .on_update = NULL,
    .on_trigger = load_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
