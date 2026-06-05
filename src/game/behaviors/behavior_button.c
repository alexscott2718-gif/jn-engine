#include "behaviors.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* C3DButton / C3DWaterButton (3BUT / 3WAB): a pressable switch that activates a
   remote entity. The GAM's `ActivateButton` names the target's ObjectTag; on
   press we forward a trigger to that target (e.g. a door named "numeydoor"),
   which is the engine's "level trigger -> world action" primitive. */

static void button_on_spawn(Entity *e, World *w) {
    e->half_extents[0] = 90.0f;
    e->half_extents[1] = 90.0f;
    e->half_extents[2] = 90.0f;
    e->user_flag = 0;
    e->link_target = NULL;
    if (e->activate_target[0]) {
        for (Entity *o = w->head; o; o = o->next) {
            if (o == e || !o->tag[0]) continue;
            if (strcasecmp(o->tag, e->activate_target) == 0) {
                e->link_target = o;
                break;
            }
        }
        printf("[BUTTON] '%s' -> target '%s' %s\n", e->tag, e->activate_target,
               e->link_target ? "linked" : "(unresolved)");
    }
}

static void button_on_trigger(Entity *e, Entity *by) {
    if (e->user_flag) return;        /* press once */
    e->user_flag = 1;
    printf("[BUTTON] pressed '%s'\n", e->tag);
    if (e->link_target && e->link_target->vt && e->link_target->vt->on_trigger)
        e->link_target->vt->on_trigger(e->link_target, by);
}

const EntityVTable vt_button = {
    .on_spawn   = button_on_spawn,
    .on_update  = NULL,
    .on_trigger = button_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
