#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>
#include <stdio.h>

static void trig_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, 80.0f, 80.0f, 80.0f);
    e->user_flag = 0;
}

static void trig_on_trigger(Entity *e, Entity *by) {
    if (e->user_flag) return;
    e->user_flag = 1;
    printf("[TRIG] fired (tag='%s') by %s\n", e->tag, by ? by->type : "?");
}

const EntityVTable vt_trig = {
    .on_spawn = trig_on_spawn,
    .on_update = NULL,
    .on_trigger = trig_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
