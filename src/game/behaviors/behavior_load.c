#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include <stddef.h>
#include <stdio.h>

static void load_refresh_gate(Entity *e) {
    if (!e) return;
    if (behavior_required_task_gate_allows(e)) {
        e->visible = 1;
        e->runtime_flags |= ENTITY_FLAG_TRIGGER;
    } else {
        e->visible = 0;
        e->runtime_flags &= ~ENTITY_FLAG_TRIGGER;
    }
}

static void load_on_spawn(Entity *e, World *w) {
    (void)w;
    float radius = gam_prop_f(e, "Radius", 60.0f);
    if (radius <= 0.0f) radius = 60.0f;
    behavior_trigger_spawn_base(e, radius, radius, radius);
    load_refresh_gate(e);
}

static void load_on_update(Entity *e, World *w, float dt) {
    (void)w;
    (void)dt;
    load_refresh_gate(e);
}

static void load_on_trigger(Entity *e, Entity *by) {
    if (!behavior_required_task_gate_allows(e)) return;
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
    .on_update = load_on_update,
    .on_trigger = load_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
