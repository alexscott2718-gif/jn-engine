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
 * SCENE sequencer (2026-06-24): aitrig_apply_story_progress ports
 * C3DAITrigger::ApplyAITriggerStoryProgress (FUN_0040caa0) — the hardcoded
 * ObjectTag x current-SCENE -> new-SCENE task-state patch table that drives the
 * campaign's story progression (docs/decomp/_scene_sequencer.md). The freed gates
 * (3FOW fowl windows, 3YCA LEV5 cargo, 3HUM clone reveal) read the now-mutable
 * CTaskList store via game_flow_entity_state.
 *
 * Deferred (need systems the native port hasn't reached): AIState/AISpeed on the
 * C3DAI state machine, AIAnim/ActivateByAnim animation selection, the
 * ActivateObject0..4 state-gated next-object machine, and the reward-flag /
 * counter-popup / story-screen side effects of the patch table (HUD/menu subsystems).
 *
 * Scratch layout (Entity):
 *   user_flag  = trigger_count
 *   user_float = armed (>=0.5 once the player has been outside the volume)
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../game_flow.h"
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

/* ApplyAITriggerStoryProgress (FUN_0040caa0): the SCENE sequencer's writer.
   A hardcoded ObjectTag x current-SCENE -> new-SCENE patch table, applied when
   the player trips this trigger (docs/decomp/_scene_sequencer.md). Only the SCENE
   / KITTY task-state writes are ported; the reward-flag / counter-popup / story-
   screen / Goddard / energy / bonus / NewGame-reload side effects depend on
   HUD/menu subsystems the native port hasn't landed and are documented-deferred.
   A no-op unless a CTaskList store is loaded (campaign mode); set_entity_state
   returns 0 otherwise, so direct `--level` launches are unaffected. */
static void aitrig_apply_story_progress(Entity *e) {
    const char *tag = e->tag;
    if (!tag[0]) return;
    long scene = game_flow_entity_state("SCENE");

#define AITRIG_SET_SCENE(newv)                                                  \
    do {                                                                        \
        if (game_flow_set_entity_state("SCENE", (newv)))                        \
            printf("[SCENE] '%s' advanced SCENE 0x%lx -> 0x%x\n",               \
                   tag, scene, (unsigned)(newv));                               \
    } while (0)

    if      (!strcasecmp(tag, "teleportexplanation") && (scene == 0x1e || scene == 0x1f)) AITRIG_SET_SCENE(0x23);
    else if (!strcasecmp(tag, "CARLOUT")      && scene == 0x46)  AITRIG_SET_SCENE(0x4b);
    else if (!strcasecmp(tag, "CALLFROMNICK") && scene == 0x5a)  AITRIG_SET_SCENE(0x64);
    else if (!strcasecmp(tag, "INVISPART")) {
        if      (scene == 0xa2)  AITRIG_SET_SCENE(0xa8);
        else if (scene == 0x1ea) AITRIG_SET_SCENE(500);
    }
    else if (!strcasecmp(tag, "GOGODDARD")    && scene == 0xa8)  AITRIG_SET_SCENE(0xaa);
    else if (!strcasecmp(tag, "ESCAPESHIP")   && scene == 0xac)  AITRIG_SET_SCENE(0xb2);
    else if (!strcasecmp(tag, "GOINGHOME")    && scene == 0xb2)  AITRIG_SET_SCENE(0xc8);
    else if (!strcasecmp(tag, "GIVEKEY")      && scene == 0xcd)  AITRIG_SET_SCENE(0xd2);
    else if (!strcasecmp(tag, "TICKETBOOTH")  && scene == 0x140) AITRIG_SET_SCENE(0x14a);
    else if (!strcasecmp(tag, "GIVEAUTO")     && scene == 0x14a) AITRIG_SET_SCENE(0x154);
    else if (!strcasecmp(tag, "CARLDIS")      && scene == 0x17c) AITRIG_SET_SCENE(0x186);
    else if (!strcasecmp(tag, "GETFUEL4")     && scene == 0x186) AITRIG_SET_SCENE(400);
    else if (!strcasecmp(tag, "BEAMOFF")      && scene == 0x19a) AITRIG_SET_SCENE(0x1a4);
    else if (!strcasecmp(tag, "FOWLINV")      && scene == 0x1cc) AITRIG_SET_SCENE(0x1d6);
    else if (!strcasecmp(tag, "RESCUECARL")   && scene == 0x1e0) AITRIG_SET_SCENE(0x1ea);
    else if (!strcasecmp(tag, "LANDSHIP")     && scene == 0x1fe) AITRIG_SET_SCENE(0x208);
    else if (!strcasecmp(tag, "SAVECARL")     && scene == 0x1fe) AITRIG_SET_SCENE(0x208);
    else if (!strcasecmp(tag, "SEECARL")      && scene == 0x208) AITRIG_SET_SCENE(0x212);
    else if (!strcasecmp(tag, "ABDUCTED")     && scene == 0x212) AITRIG_SET_SCENE(0x21c);
    else if (!strcasecmp(tag, "EVADEYOKES")   && scene == 0x21c) AITRIG_SET_SCENE(0x226);
    else if (!strcasecmp(tag, "KITEND1") && game_flow_entity_state("KITTY1") == 0) {
        if (game_flow_set_entity_state("KITTY1", 10)) printf("[SCENE] '%s' KITTY1=10\n", tag);
    }
    else if (!strcasecmp(tag, "KITEND2") && game_flow_entity_state("KITTY2") == 0) {
        if (game_flow_set_entity_state("KITTY2", 10)) printf("[SCENE] '%s' KITTY2=10\n", tag);
    }
    else if (!strcasecmp(tag, "KITEND3") && game_flow_entity_state("KITTY3") == 0) {
        if (game_flow_set_entity_state("KITTY3", 10)) printf("[SCENE] '%s' KITTY3=10\n", tag);
    }
    /* REMOTE / PUTGODDARD / JIMEND / RECHARGE / BONUSSCREEN / RESTARTGAME write
       no SCENE state (menu/Goddard/energy/bonus/reload effects) — deferred. */

#undef AITRIG_SET_SCENE
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

    /* ActivateAITrigger order: target mutations -> ToggleObject -> story-progress
       SCENE patch table -> NextTrigger dispatch. */
    aitrig_apply_story_progress(e);

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

/* Headless SCENE-sequencer test (JN_TEST_SCENE=<ObjectTag>): force the AITrigger
   with the given ObjectTag through its activate core (bypassing the touch/arm
   gates) so the story-progress SCENE write can be exercised in a campaign run.
   Returns the fired entity, or NULL. */
Entity *behavior_ai_trigger_fire_tag(World *w, const char *tag) {
    if (!w || !tag || !tag[0]) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || strncmp(e->type, "3AIT", 4) != 0) continue;
        if (strcasecmp(e->tag, tag) != 0) continue;
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
