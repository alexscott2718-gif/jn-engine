/* behavior_ai_trigger.c — C3DAITrigger (3AIT), the broadest placed behavior.
 *
 * 174 placements across 24 levels. An invisible C3DTriggerType volume that,
 * when its activator enters, mutates a named AI/sprite/animated target and then
 * dispatches a follow-up trigger (docs/decomp/C3DAITrigger.md). It is the
 * game's AI/script mission-wiring primitive: hide/show actors, teleport them to
 * a marker, repoint a patrol, rotate, toggle an object, chain the next trigger.
 *
 * Native model (a faithful, conservatively-gated subset of ActivateAITrigger):
 *   - Invisible self-detecting volume sized by the authored Radius. We run our
 *     own player-overlap test in on_update (flags=0) so we don't depend on the
 *     engine trigger path and can arm/disarm precisely.
 *   - ARM-ON-EXIT: the trigger only fires after the player has been OUTSIDE the
 *     volume at least once, then re-enters. This matches "touch to enter" and,
 *     critically, means a trigger whose volume happens to cover the level spawn
 *     never fires from a standing start — so the 2-tick audit/screenshot probes
 *     are unaffected.
 *   - TouchActivated gate: only rows with TouchActivated!=0 respond to the
 *     player walking in. The rest are dispatched by scripted trigger chains the
 *     native runtime doesn't model yet, so they stay inert (faithful + safe).
 *   - Activator gate (ActivateBy / IsA): the player activates when ActivateBy is
 *     the player tag JIM1, names no existing object (generic allow), or IsA is
 *     C3DJIMMY. If ActivateBy names a different, existing object (C3DCARL,
 *     C3DSUV, ...) the player can't trip it.
 *   - TimesToTrigger limit (-1 = unlimited).
 *   - Target mutations on AITarget: AIHideObj show/hide, AINewPos teleport to a
 *     marker, AINewRotY Y-rotation, AIPatrol patrol-repoint. Then ToggleObject
 *     and NextTrigger are dispatched by forwarding on_trigger.
 *
 * Deferred (need systems the native port hasn't reached): AIState/AISpeed on the
 * C3DAI state machine, AIAnim/ActivateByAnim animation selection, the
 * ActivateObject0..4 state-gated next-object machine, and the hardcoded
 * story-progress patch table (RESTARTGAME/GIVEKEY/GOGODDARD/...).
 *
 * Scratch layout (Entity):
 *   user_flag  = trigger_count
 *   user_float = armed (>=0.5 once the player has been outside the volume)
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/physics.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define AITRIG_DEFAULT_RADIUS 100.0f

static Entity *aitrig_find_by_tag(World *w, const char *tag) {
    if (!w || !tag || !tag[0] || strcasecmp(tag, "none") == 0) return NULL;
    for (Entity *o = w->head; o; o = o->next) {
        if (!o->tag[0]) continue;
        if (strcasecmp(o->tag, tag) == 0) return o;
    }
    return NULL;
}

/* The activator-gate slice of ActivateAITrigger: can the player trip this? */
static int aitrig_player_allowed(Entity *e, World *w) {
    const char *activate_by = gam_str(e, "ActivateBy", "JIM1");
    const char *isa = gam_str(e, "IsA", "none");
    if (strcasecmp(isa, "C3DJIMMY") == 0) return 1;
    if (strcasecmp(activate_by, "JIM1") == 0) return 1;
    /* An ActivateBy that resolves to a concrete, non-player object means some
       other actor is the activator; the player can't trip it. An unresolved
       non-"none" tag is a generic allow (matches the decomp's else branch). */
    if (strcasecmp(activate_by, "none") == 0) return 0;
    return aitrig_find_by_tag(w, activate_by) == NULL;
}

/* The mutate-and-dispatch core (everything after the activator/limit gates).
   Shared by the natural overlap path and the headless test hook. */
static void aitrig_activate(Entity *e, World *w) {
    Entity *target = aitrig_find_by_tag(w, gam_str(e, "AITarget", "none"));
    if (target) {
        int hide = gam_prop_i(e, "AIHideObj", -1);
        if (hide == 0) {
            target->visible = 0;
            target->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
        } else if (hide == 1) {
            target->visible = 1;
        }

        const char *newpos = gam_str(e, "AINewPos", "none");
        if (strcasecmp(newpos, "none") != 0) {
            Entity *src = aitrig_find_by_tag(w, newpos);
            if (src) { target->x = src->x; target->y = src->y; target->z = src->z; }
        }

        float rot_y = gam_prop_f(e, "AINewRotY", -1.0f);
        if (rot_y != -1.0f) target->ry = rot_y * (float)(M_PI / 180.0);

        const char *patrol = gam_str(e, "AIPatrol", "none");
        if (strcasecmp(patrol, "none") != 0) {
            snprintf(target->patrol_point, sizeof(target->patrol_point), "%s", patrol);
            snprintf(target->start_point, sizeof(target->start_point), "%s", patrol);
        }
    }

    Entity *toggle = aitrig_find_by_tag(w, gam_str(e, "ToggleObject", "none"));
    if (toggle && toggle->vt && toggle->vt->on_trigger)
        toggle->vt->on_trigger(toggle, g_player);

    Entity *next = aitrig_find_by_tag(w, gam_str(e, "NextTrigger", "none"));
    if (next && next->vt && next->vt->on_trigger)
        next->vt->on_trigger(next, g_player);

    e->user_flag++;  /* trigger_count */
    printf("[AITRIG] '%s' fired (#%d) target='%s' next='%s'\n", e->tag,
           e->user_flag, gam_str(e, "AITarget", "none"),
           gam_str(e, "NextTrigger", "none"));
}

static void aitrig_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);   /* respect level/InitiallyVisible gating */
    float radius = gam_prop_f(e, "Radius", AITRIG_DEFAULT_RADIUS);
    if (radius <= 0.0f) radius = AITRIG_DEFAULT_RADIUS;
    e->half_extents[0] = radius;
    e->half_extents[1] = radius;
    e->half_extents[2] = radius;
    e->visible = 0;                    /* invisible trigger; nothing to draw */
    e->runtime_flags = 0;              /* we self-detect overlap in on_update */
    e->user_flag = 0;                  /* trigger_count */
    e->user_float = 0.0f;              /* not armed until the player leaves */
}

static void aitrig_on_update(Entity *e, World *w, float dt) {
    (void)dt;
    if (!e->alive) return;
    Entity *p = g_player;
    if (!p || !p->alive) return;

    int overlapping = physics_aabb_overlap(e, p);
    if (!overlapping) {
        e->user_float = 1.0f;          /* armed: player is outside the volume */
        return;
    }
    if (e->user_float < 0.5f) return;  /* entered without ever leaving — wait */

    /* Player is inside an armed volume — disarm and try to fire. */
    e->user_float = 0.0f;

    if (gam_prop_i(e, "TouchActivated", 0) == 0) return;  /* not a touch trigger */
    if (!aitrig_player_allowed(e, w)) return;

    int limit = gam_prop_i(e, "TimesToTrigger", -1);
    if (limit != -1 && e->user_flag >= limit) return;

    aitrig_activate(e, w);
}

/* Headless test (JN_TEST_AITRIG): force the first TouchActivated 3AIT with a
   resolvable AITarget through the gate+mutation core so the wiring can be
   exercised without walking the player through a volume. Returns the fired
   entity, or NULL. */
Entity *behavior_ai_trigger_test_fire(World *w) {
    if (!w) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || strncmp(e->type, "3AIT", 4) != 0) continue;
        if (gam_prop_i(e, "TouchActivated", 0) == 0) continue;
        if (!aitrig_player_allowed(e, w)) continue;
        if (!aitrig_find_by_tag(w, gam_str(e, "AITarget", "none"))) continue;
        int limit = gam_prop_i(e, "TimesToTrigger", -1);
        if (limit != -1 && e->user_flag >= limit) continue;
        aitrig_activate(e, w);
        return e;
    }
    return NULL;
}

const EntityVTable vt_ai_trigger = {
    .on_spawn   = aitrig_on_spawn,
    .on_update  = aitrig_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
