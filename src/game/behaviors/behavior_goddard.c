/* behavior_goddard.c - C3DGoddard (3GOD), runtime companion/controller.
 *
 * C3DGoddard has a real FourCC constructor in Neutron.exe but no direct .gam
 * rows in the current corpus. It is referenced by camera/AI data as
 * C3DGODDARD, and C3DMetalPickup talks to the player's Goddard/controller via
 * mode requests (5 = fetch this can, 2 = release/follow).
 *
 * Native slice (2026-06-24):
 *   - Synthesize one C3DGODDARD entity for campaign runs, or explicit
 *     JN_TEST_GODDARD probes, when the level references Goddard.
 *   - Apply the documented PostLoad level disables (LV1C/LV1D/LV4A/LV6A/LEV6/
 *     LEV7/VR06) so hidden/direct-path behavior stays conservative.
 *   - Provide a small mode API for 3MEP and future AITrigger side effects.
 *   - Mode 2 follows Jimmy at a short offset; mode 5 seeks a target pickup and
 *     invokes its trigger when reached. The long raw orbit/effect helper cluster
 *     remains deferred until the structs/mode vectors are cleaned up.
 */
#include "behaviors.h"
#include "behavior_ai.h"
#include "../entities.h"
#include "../game_flow.h"
#include "../../engine/physics.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define GODDARD_MODE_FOLLOW       2
#define GODDARD_MODE_FETCH_CAN    5
#define GODDARD_MODE_HOLD         1   /* scripted sit/idle: hold the AITrigger pose */
#define GODDARD_MODE_PATROL       3   /* scripted walk: follow authored PatrolPoint chain */
#define GODDARD_FOLLOW_DIST     150.0f
#define GODDARD_FOLLOW_SPEED    220.0f
#define GODDARD_FETCH_SPEED     450.0f
#define GODDARD_SNAP_DIST      2200.0f

static Entity *s_goddard = NULL;

static int env_on(const char *name) {
    const char *s = getenv(name);
    return s && s[0] && strcmp(s, "0") != 0;
}

static int is_disabled_level(const char *level) {
    if (!level) return 0;
    return !strcasecmp(level, "level1c") ||
           !strcasecmp(level, "level1d") ||
           !strcasecmp(level, "level4a") ||
           !strcasecmp(level, "level6a") ||
           !strcasecmp(level, "level6")  ||
           !strcasecmp(level, "level7")  ||
           !strcasecmp(level, "vr06");
}

static int goddard_ref(const char *s) {
    if (!s || !s[0]) return 0;
    return !strcasecmp(s, "C3DGODDARD") ||
           !strcasecmp(s, "GOGODDARD")  ||
           !strcasecmp(s, "GODDARDDIS") ||
           !strcasecmp(s, "PUTGODDARD") ||
           !strcasecmp(s, "JIMEND")     ||
           !strcasecmp(s, "RECHARGE");
}

static int world_references_goddard(World *w) {
    if (!w) return 0;
    for (Entity *e = w->head; e; e = e->next) {
        if (goddard_ref(e->tag) || goddard_ref(e->activate_target))
            return 1;
        for (int i = 0; i < e->nstrs; i++)
            if (goddard_ref(e->strs[i].val)) return 1;
    }
    return 0;
}

static Entity *find_goddard(World *w) {
    if (!w) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive) continue;
        if (strncmp(e->type, "3GOD", 4) == 0) return e;
        if (strcasecmp(e->tag, "C3DGODDARD") == 0) return e;
    }
    return NULL;
}

static void set_mode(Entity *g, int mode, Entity *target) {
    if (!g) return;
    if (mode < 0) mode = GODDARD_MODE_FOLLOW;
    int changed = (g->user_flag != mode) || (target && g->link_target != target);
    g->user_flag = mode;
    if (target) g->link_target = target;
    if (changed) {
        printf("[GODDARD] mode=%d target='%s'\n",
               mode, g->link_target ? g->link_target->tag : "");
    }
}

static void goddard_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    e->half_extents[0] = 45.0f;
    e->half_extents[1] = 65.0f;
    e->half_extents[2] = 45.0f;
    e->move_speed = !strcasecmp(game_flow_current_level(), "vr06") ? 3500.0f : 1500.0f;
    e->move_vert = GODDARD_FETCH_SPEED;
    e->user_float = 0.0f;
    e->link_target = g_player;
    e->user_flag = GODDARD_MODE_FOLLOW;
    if (is_disabled_level(game_flow_current_level())) {
        e->visible = 0;
        e->runtime_flags = 0;
    }
    s_goddard = e;
}

static void follow_player(Entity *e, Entity *p, float dt) {
    if (!p) return;
    float want_x = p->x - sinf(p->ry) * GODDARD_FOLLOW_DIST;
    float want_y = p->y + 25.0f;
    float want_z = p->z - cosf(p->ry) * GODDARD_FOLLOW_DIST;
    float dx = want_x - e->x;
    float dy = want_y - e->y;
    float dz = want_z - e->z;
    float d2 = dx * dx + dy * dy + dz * dz;
    if (d2 > GODDARD_SNAP_DIST * GODDARD_SNAP_DIST) {
        e->x = want_x; e->y = want_y; e->z = want_z;
        e->vx = e->vy = e->vz = 0.0f;
        return;
    }
    (void)behavior_ai_seek_position(e, want_x, want_y, want_z,
                                    GODDARD_FOLLOW_SPEED, 35.0f, dt);
    e->y += (want_y - e->y) * fminf(dt * 4.0f, 1.0f);
}

static void fetch_target(Entity *e, float dt) {
    Entity *t = e->link_target;
    if (!t || !t->alive || !t->visible) {
        set_mode(e, GODDARD_MODE_FOLLOW, g_player);
        return;
    }

    (void)behavior_ai_seek_position(e, t->x, t->y, t->z,
                                    GODDARD_FETCH_SPEED, 35.0f, dt);
    e->y += (t->y - e->y) * fminf(dt * 8.0f, 1.0f);
    if (physics_aabb_overlap(e, t) && t->vt && t->vt->on_trigger) {
        t->vt->on_trigger(t, e);
        set_mode(e, GODDARD_MODE_FOLLOW, g_player);
    }
}

/* Scripted sit/idle (AIState 1/2): hold the pose the AITrigger placed Goddard in
   (position + AINewRotY already applied by the generic mutation path). */
static void goddard_hold(Entity *e) {
    e->vx = e->vy = e->vz = 0.0f;
}

/* Scripted walk (AIState 3, AIPatrol set): seek the authored PatrolPoint chain
   at the AISpeed tuning, advancing on arrival. Mirrors behavior_ai's patrol step
   but keeps Goddard's own field layout (user_flag = mode, user_float = elapsed)
   instead of borrowing the shared patrol-state slots. */
static void goddard_patrol(Entity *e, World *w, float dt) {
    if (!e->patrol_point[0] || !strcasecmp(e->patrol_point, "none")) {
        goddard_hold(e);
        return;
    }
    Entity *wp = behavior_ai_find_patrol_point(w, e->patrol_point);
    if (!wp) { goddard_hold(e); return; }

    float speed = e->move_speed > 0.0f ? e->move_speed : GODDARD_FOLLOW_SPEED;
    BehaviorAIResult r = behavior_ai_seek_position(e, wp->x, wp->y, wp->z,
                                                   speed, 50.0f, dt);
    e->y += (wp->y - e->y) * fminf(dt * 4.0f, 1.0f);
    if (r == BEHAVIOR_AI_ARRIVED) {
        const char *next = wp->next_patrol[0] ? wp->next_patrol : "";
        snprintf(e->patrol_point, sizeof(e->patrol_point), "%s", next);
    }
}

static void goddard_on_update(Entity *e, World *w, float dt) {
    if (is_disabled_level(game_flow_current_level())) {
        e->visible = 0;
        e->runtime_flags = 0;
        return;
    }
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (!e->visible) return;

    e->user_float += dt;  /* goddard_state_elapsed */
    switch (e->user_flag) {
        case GODDARD_MODE_FETCH_CAN: fetch_target(e, dt);     break;
        case GODDARD_MODE_PATROL:    goddard_patrol(e, w, dt); break;
        case GODDARD_MODE_HOLD:      goddard_hold(e);          break;
        default:                     follow_player(e, g_player, dt); break;
    }
}

const EntityVTable vt_goddard = {
    .on_spawn   = goddard_on_spawn,
    .on_update  = goddard_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};

void behavior_goddard_reset(void) {
    s_goddard = NULL;
}

Entity *behavior_goddard_get(void) {
    return (s_goddard && s_goddard->alive) ? s_goddard : NULL;
}

Entity *behavior_goddard_ensure(World *w, const char *level_name) {
    s_goddard = find_goddard(w);
    if (s_goddard) return s_goddard;

    if (!game_flow_campaign_active() && !env_on("JN_TEST_GODDARD"))
        return NULL;
    if (!world_references_goddard(w) && !env_on("JN_TEST_GODDARD"))
        return NULL;

    Entity *p = g_player ? g_player : world_find_type(w, "3JIM");
    float x = p ? p->x - sinf(p->ry) * GODDARD_FOLLOW_DIST : 0.0f;
    float y = p ? p->y + 25.0f : 0.0f;
    float z = p ? p->z - cosf(p->ry) * GODDARD_FOLLOW_DIST : 0.0f;
    Entity *g = entity_spawn(w, "3GOD", "C3DGODDARD", x, y, z);
    if (!g) return NULL;
    if (level_name && is_disabled_level(level_name)) {
        g->visible = 0;
        g->runtime_flags = 0;
    }
    s_goddard = g;
    printf("[GODDARD] spawned companion for level '%s' visible=%d at (%.0f, %.0f, %.0f)\n",
           level_name ? level_name : "", g->visible, g->x, g->y, g->z);
    return g;
}

int behavior_goddard_request_mode(Entity *target, int mode) {
    Entity *g = behavior_goddard_get();
    if (!g || !g->visible) return 0;
    set_mode(g, mode, target ? target : g_player);
    return 1;
}

int behavior_goddard_release_if_target(Entity *target, int mode) {
    Entity *g = behavior_goddard_get();
    if (!g || (target && g->link_target != target)) return 0;
    set_mode(g, mode, g_player);
    return 1;
}

int behavior_goddard_target_is(Entity *target) {
    Entity *g = behavior_goddard_get();
    return g && target && g->link_target == target;
}

int behavior_goddard_mode(void) {
    Entity *g = behavior_goddard_get();
    return g ? g->user_flag : -1;
}

/* The C3DAI-target branch of C3DAITrigger::ActivateAITrigger, scoped to the
   Goddard companion. The generic AITrigger path already applied AINewPos
   (teleport), AINewRotY (rotation), and AIPatrol (-> patrol_point); this routes
   the authored AIState / AISpeed into Goddard's mode machine so a scripted
   pose/patrol isn't immediately overridden by follow mode. Faithful to the
   decompiled body (set_ai_state(AIState) + speed_tuning = AISpeed at 0x604):
     - AIPatrol set       -> PATROL the authored chain at the AISpeed tuning
     - AIState == 4       -> FOLLOW (Goddard's constructor-default attached state)
     - otherwise (1/2)    -> HOLD (sit/idle at the placed pose)
   AIAnim (SIT/walk/sit) selection stays deferred with the rest of the
   AITrigger animation wiring. A no-op for any non-Goddard target. Returns 1 if
   it took effect. */
int behavior_goddard_apply_ai_state(Entity *g, int ai_state, int ai_speed) {
    if (!g || behavior_goddard_get() != g) return 0;
    if (ai_speed != -1) g->move_speed = (float)ai_speed;  /* speed_tuning (0x604) */
    int mode;
    if (g->patrol_point[0] && strcasecmp(g->patrol_point, "none") != 0)
        mode = GODDARD_MODE_PATROL;
    else if (ai_state == 4)
        mode = GODDARD_MODE_FOLLOW;
    else
        mode = GODDARD_MODE_HOLD;
    set_mode(g, mode, NULL);
    return 1;
}
