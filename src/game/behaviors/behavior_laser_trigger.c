/* behavior_laser_trigger.c — C3DLaserTrigger (3LAS).
 *
 * The decomp describes an active/inactive laser tripwire with ItemActive,
 * Toggle, and Next. A crossed active beam hides/disables itself, flashes/damages
 * Jimmy, then relays Toggle to the object named by Next.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "behavior_tesla.h"
#include "../gamestate.h"
#include "../../engine/physics.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define LASER_DAMAGE          10
#define LASER_TINT_COOLDOWN   10.0f

static void laser_sync_active(Entity *e) {
    if (!e) return;
    if (e->user_flag) {
        e->visible = 1;
        e->runtime_flags |= ENTITY_FLAG_TRIGGER;
    } else {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    }
}

static void laser_set_active(Entity *e, int mode) {
    if (!e) return;
    if (mode == 0) {
        e->user_flag = 0;
    } else if (mode == 1) {
        e->user_flag = 1;
    } else {
        e->user_flag = !e->user_flag;
    }
    laser_sync_active(e);
    printf("[LASER] '%s' active=%d\n", e->tag, e->user_flag ? 1 : 0);
}

static void laser_resolve_target(Entity *e, World *w) {
    e->link_target = NULL;
    if (!e->activate_target[0]) return;
    for (Entity *o = w->head; o; o = o->next) {
        if (o == e || !o->tag[0]) continue;
        if (strcasecmp(o->tag, e->activate_target) == 0) {
            e->link_target = o;
            break;
        }
    }
    printf("[LASER] '%s' -> target '%s' %s\n", e->tag, e->activate_target,
           e->link_target ? "linked" : "(unresolved)");
}

static void laser_relay(Entity *e, Entity *by) {
    Entity *t = e->link_target;
    if (!t) return;
    int toggle = gam_prop_i(e, "Toggle", -1);
    if (strncmp(t->type, "3TES", 4) == 0) {
        behavior_tesla_set_active(t, toggle);
    } else if (strncmp(t->type, "3LAS", 4) == 0) {
        laser_set_active(t, toggle);
    } else if (t->vt && t->vt->on_trigger) {
        t->vt->on_trigger(t, by);
    } else {
        t->user_flag = (toggle == 0) ? 0 : (toggle == 1) ? 1 : !t->user_flag;
    }
}

static void laser_on_spawn(Entity *e, World *w) {
    behavior_trigger_spawn_base(e, 120.0f, 180.0f, 120.0f);
    e->user_flag = gam_prop_i(e, "ItemActive", 1) ? 1 : 0;
    e->user_float = 0.0f; /* damage tint/cooldown timer */
    laser_resolve_target(e, w);
    laser_sync_active(e);
}

static void laser_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (e->user_float > 0.0f) {
        e->user_float -= dt;
        if (e->user_float < 0.0f) e->user_float = 0.0f;
    }
    laser_sync_active(e);
}

static void laser_on_trigger(Entity *e, Entity *by) {
    if (!by) return;

    if (by != g_player || !physics_aabb_overlap(e, by)) {
        laser_set_active(e, -1);
        return;
    }

    if (!e->user_flag) return;
    laser_set_active(e, 0);
    if (e->user_float <= 0.0f) {
        e->user_float = LASER_TINT_COOLDOWN;
        gamestate_damage_player(LASER_DAMAGE);
        printf("[LASER] '%s' hit player\n", e->tag);
    }
    laser_relay(e, by);
}

const EntityVTable vt_laser_trigger = {
    .on_spawn   = laser_on_spawn,
    .on_update  = laser_on_update,
    .on_trigger = laser_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
