/* behavior_escort.c — C3DSumo (3SUM) + C3DPirate (3PIR) exit-escort actors.
 *
 * Both are C3DAnimated leaves whose defining behavior is a Jimmy-only escort:
 * when Jimmy touches an eligible actor, it captures him, runs a short carried
 * sequence, resolves its serialized StartPoint marker, moves Jimmy there, then
 * releases control and starts a release cooldown
 * (docs/decomp/C3DSumo.md, C3DPirate.md). Despite the class name, Pirate's
 * concrete assets are viking.png/viking.ase.
 *
 * Native model (a faithful subset):
 *   - On player contact (trigger overlap), if the actor is off cooldown and its
 *     StartPoint resolves to a placed marker, teleport the player to that
 *     marker and start the 10s release cooldown. This is the observable result
 *     of the escort handoff (Jimmy ends up at StartPoint).
 *   - The carried/animated escort sequence and the eligibility counter (the
 *     decomp's inventory/picture counter 0x1b gate) are deferred until the
 *     inventory-counter system is ported; this leaf preserves the StartPoint
 *     teleport, the gate on a resolvable destination, and the cooldown so it
 *     can't repeatedly grab the player.
 *
 * Scratch layout (Entity):
 *   link_target = resolved StartPoint marker (NULL => inert)
 *   user_float  = release cooldown timer (seconds)
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/physics.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define ESCORT_COOLDOWN 10.0f   /* C3DSumo/C3DPirate release cooldown */

static void escort_resolve_startpoint(Entity *e, World *w) {
    e->link_target = NULL;
    if (!e->start_point[0]) return;
    for (Entity *o = w->head; o; o = o->next) {
        if (o == e || !o->tag[0]) continue;
        if (strcasecmp(o->tag, e->start_point) == 0) {
            e->link_target = o;
            break;
        }
    }
    printf("[ESCORT] %s '%s' -> StartPoint '%s' %s\n", e->type, e->tag,
           e->start_point, e->link_target ? "linked" : "(unresolved)");
}

static void escort_on_spawn(Entity *e, World *w) {
    behavior_animated_spawn_base(e);
    e->half_extents[0] = 110.0f;
    e->half_extents[1] = 170.0f;
    e->half_extents[2] = 110.0f;
    e->user_float = 0.0f;
    escort_resolve_startpoint(e, w);
}

static void escort_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (e->user_float > 0.0f) {
        e->user_float -= dt;
        if (e->user_float < 0.0f) e->user_float = 0.0f;
    }
}

static void escort_on_trigger(Entity *e, Entity *by) {
    if (by != g_player || !by->alive) return;
    if (e->user_float > 0.0f) return;          /* still on release cooldown */
    if (!e->link_target) return;               /* no destination => inert */
    if (!physics_aabb_overlap(e, by)) return;

    Entity *dest = e->link_target;
    by->x = dest->x;
    by->y = dest->y;
    by->z = dest->z;
    by->vx = by->vy = by->vz = 0.0f;
    e->user_float = ESCORT_COOLDOWN;
    printf("[ESCORT] %s '%s' carried player to '%s' (%.0f,%.0f,%.0f)\n",
           e->type, e->tag, e->start_point, dest->x, dest->y, dest->z);
}

const EntityVTable vt_escort = {
    .on_spawn   = escort_on_spawn,
    .on_update  = escort_on_update,
    .on_trigger = escort_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
