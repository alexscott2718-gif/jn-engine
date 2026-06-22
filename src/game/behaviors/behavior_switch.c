/* behavior_switch.c — native port of C3DSwitch (FourCC 3SWI).
 *
 * Faithful to docs/decomp/C3DSwitch.md: registers MyState (int, the persisted
 * on/off position), SwitchObject (the driven target's ObjectTag), Toggle (int).
 * Validated against shipped .gam data: 8 instances, SwitchObject names fans /
 * wires ("FAN1","FAN2","WIRE01"), MyState 0..1.
 *
 * Native realisation: on player contact the switch flips state and drives the
 * target's runtime on-flag (user_flag) — e.g. flipping a C3DFan on/off. The
 * SwitchObject tag is resolved to a target entity at spawn (shares the loader's
 * activate_target field with C3DButton).
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <stddef.h>
#include <stdio.h>
#include <strings.h>

static void switch_on_spawn(Entity *e, World *w) {
    behavior_trigger_spawn_base(e, 60.0f, 80.0f, 60.0f);
    e->user_flag = gam_prop_i(e, "MyState", 0);   /* current switch position */
    e->user_float = 0.0f;                          /* contact debounce timer */
    e->link_target = NULL;
    if (e->activate_target[0]) {
        for (Entity *o = w->head; o; o = o->next) {
            if (o == e || !o->tag[0]) continue;
            if (strcasecmp(o->tag, e->activate_target) == 0) { e->link_target = o; break; }
        }
        printf("[SWITCH] '%s' -> '%s' %s\n", e->tag, e->activate_target,
               e->link_target ? "linked" : "(unresolved)");
        /* push the initial MyState onto the target */
        if (e->link_target) e->link_target->user_flag = e->user_flag;
    }
}

static void switch_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (e->user_float > 0.0f) e->user_float -= dt;   /* cool down the debounce */
}

static void switch_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_float > 0.0f) return;                /* one flip per contact */
    e->user_float = 0.75f;
    e->user_flag = !e->user_flag;                    /* flip switch position */
    printf("[SWITCH] '%s' -> %d\n", e->tag, e->user_flag);
    if (e->link_target) e->link_target->user_flag = e->user_flag;
}

const EntityVTable vt_switch = {
    .on_spawn   = switch_on_spawn,
    .on_update  = switch_on_update,
    .on_trigger = switch_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
