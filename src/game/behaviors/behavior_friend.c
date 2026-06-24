/* behavior_friend.c — C3DFriends / C3DAI idle NPC cast.
 *
 * Shared native behavior for the friend/NPC leaves that derive from C3DFriends
 * (Nick 3NIC, Sheen 3SHE, UltraLord 3ULT, Libby 3LIB, Hugh 3HUG, Benny 3BEN,
 * Judy 3MOM) or directly from C3DAI (Kitty 3KIT). They share the C3DAI movement
 * base plus a five-entry talk-trigger table (docs/decomp/C3DFriends.md,
 * C3DAI.md). Cindy (3CIN) keeps its own module (behavior_cindy.c) for its
 * character-specific SCENE visibility windows.
 *
 * Native model (a faithful subset of the C3DFriends base):
 *   - Spawn through the C3DAI patrol base (behavior_ai): idle in place, or
 *     patrol if the row authored a PatrolPoint. Most friends stand still; a few
 *     carry a PatrolPoint and wander between nav nodes.
 *   - Face the player when he is inside the authored VisibleRange — friends turn
 *     to look at Jimmy (C3DAI scans for TargetName, which defaults to JIM1).
 *   - Visibility follows the inherited C3DAnimated InitiallyVisible / level gate
 *     (behavior_animated_*_base), so InitiallyVisible=0 friends stay hidden.
 *   - Talk-trigger reward side effects are deferred until the task-state mutator
 *     path exists; the TalkTrigger0..4 / TaskName fields are preserved on load
 *     (see Entity.talk_trigger / Entity.task_name) for the future talk system.
 *
 * Kitty's effect/sound handle is deferred; this is the placed, visible,
 * idle-or-patrol core that clears the friend rows from the behavior lens and
 * gives them real C3DAI movement.
 *
 * Talk-reward path (2026-06-24): the friend half of the SCENE sequencer
 * (docs/decomp/_scene_sequencer.md). Each concrete friend leaf overrides
 * C3DFriends vtable slot 96 (StartFriendTalkPulse) with a
 * Handle<X>TalkProgressReward hook that re-reads SCENE and writes the next story
 * beat. friend_apply_talk_reward() is the shared FourCC x current-SCENE ->
 * new-SCENE table (mirrors aitrig_apply_story_progress); the talk itself is
 * driven from one centralized entrypoint (the T key in main.c, or JN_TEST_TALK),
 * so Cindy (vt_cindy) and Carl (vt_walker) — which keep their own modules — are
 * reached through the same table without duplicating it. The friend turns to
 * face the player on talk. Only the SCENE/task-state writes are ported; the
 * inventory-grid / counter-popup / story-screen side effects are the same
 * HUD/menu helpers the AITrigger patch table deferred.
 */
#include "behavior_ai.h"
#include "../game_flow.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FRIEND_WALK_SPEED   150.0f   /* patrol pace for the wandering few */
#define FRIEND_ARRIVE       60.0f
#define FRIEND_VISIBLE_DEF  2500.0f  /* C3DAI VisibleRange constructor default */
#define FRIEND_TURN_RATE    2.5f     /* rad/sec — limit the look-at-Jimmy turn */
#define FRIEND_HALF_X       90.0f
#define FRIEND_HALF_Y       170.0f
#define FRIEND_HALF_Z       90.0f

static float friend_ang_wrap(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

static void friend_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
    e->half_extents[0] = FRIEND_HALF_X;
    e->half_extents[1] = FRIEND_HALF_Y;
    e->half_extents[2] = FRIEND_HALF_Z;
}

/* Idle friends pivot to watch Jimmy when he is within VisibleRange. The turn is
   rate-limited so a friend placed near the player spawn doesn't snap on the
   first frame (and the screenshot/audit probes, which run two warmup ticks, see
   at most a fraction of a degree of rotation). */
static void friend_face_player(Entity *e) {
    Entity *p = g_player;
    if (!p || !p->alive) return;
    float dx = p->x - e->x;
    float dz = p->z - e->z;
    float dist2 = dx * dx + dz * dz;
    if (dist2 <= 1.0f) return;
    float vr = gam_prop_f(e, "VisibleRange", FRIEND_VISIBLE_DEF);
    if (vr <= 0.0f) vr = FRIEND_VISIBLE_DEF;
    if (dist2 > vr * vr) return;

    float want = atan2f(-dx, -dz);
    float d = friend_ang_wrap(want - e->ry);
    float maxstep = FRIEND_TURN_RATE * 0.016f;  /* per-tick cap (~DT) */
    if (d >  maxstep) d =  maxstep;
    if (d < -maxstep) d = -maxstep;
    e->ry += d;
}

static void friend_on_update(Entity *e, World *w, float dt) {
    if (e->patrol_point[0]) {
        const BehaviorAIPatrolConfig cfg = {
            .speed = FRIEND_WALK_SPEED,
            .arrive_radius = FRIEND_ARRIVE,
            .loop_to_start = 1,
        };
        (void)behavior_ai_update_patrol(e, w, &cfg, dt);
        return;
    }

    if (!behavior_animated_update_base(e, w, dt)) return;
    behavior_ai_idle(e);
    friend_face_player(e);
}

/* ---- Talk-reward path (the friend half of the SCENE sequencer) ---------- */

#define FRIEND_TALK_RANGE 300.0f   /* stand-next-to distance for "press T to talk" */

/* The C3DFriends-derived cast the player can talk to. 3KIT (Kitty) is a bare
   C3DAI (not a C3DFriends leaf) so it has no talk trigger and is excluded. */
static int friend_is_talk_fourcc(const char *t) {
    static const char *const cast[] = {
        "3CAR", "3CIN", "3NIC", "3SHE", "3ULT", "3LIB", "3HUG", "3BEN", "3MOM"
    };
    for (size_t i = 0; i < sizeof(cast) / sizeof(cast[0]); i++)
        if (strncmp(t, cast[i], 4) == 0) return 1;
    return 0;
}

/* Each friend leaf's Handle<X>TalkProgressReward (C3DFriends vtable slot 96),
   collapsed into one FourCC x current-SCENE -> new-SCENE table (mirrors
   aitrig_apply_story_progress in behavior_ai_trigger.c). The deferred reward
   side effects are noted inline. A no-op unless a CTaskList store is loaded
   (campaign mode): game_flow_set_entity_state returns 0 otherwise, so direct
   --level / audit launches are unaffected. Hugh (3HUG: slot 96 is a tail-jump
   to StartFriendTalkPulse only) and UltraLord (3ULT: only deferred inventory at
   SCENE==0x186) write no SCENE state and are intentionally absent. Returns 1 if
   a SCENE write landed. */
static int friend_apply_talk_reward(Entity *e) {
    const char *fc = e->type;
    long scene = game_flow_entity_state("SCENE");
    long newv = -1;

    if (strncmp(fc, "3CAR", 4) == 0) {            /* Carl */
        if      (scene == 0x32)  newv = 0x3c;
        else if (scene == 0x41)  newv = 0x46;
        else if (scene == 0x10e) newv = 0x118;
    } else if (strncmp(fc, "3CIN", 4) == 0) {     /* Cindy (+ FUN_004038c0(0,7,1) deferred) */
        if      (scene == 0x104) newv = 0x10e;
    } else if (strncmp(fc, "3BEN", 4) == 0) {     /* Benny */
        if      (scene == 0x6e)  newv = 0x73;
        else if (scene == 0x8c)  newv = 0x8d;
        else if (scene == 0x154) newv = 0x15e;
    } else if (strncmp(fc, "3LIB", 4) == 0) {     /* Libby */
        if      (scene == 0xff)  newv = 0x104;
        else if (scene == 0x12c) newv = 0x136;
        else if (scene == 0x15e) newv = 0x168;
        else if (scene == 0x18f) newv = 0x19a;
    } else if (strncmp(fc, "3NIC", 4) == 0) {     /* Nick */
        if      (scene == 0x73)  newv = 0x78;
        else if (scene == 0x7d)  newv = 0x78;     /* + nick_race_counter++ (deferred) */
        else if (scene == 0x82)  newv = 0x8c;     /* + FUN_004038c0(0,6,1) (deferred) */
        else if (scene == 0x8c || scene == 0x8d) {/* only off the LV2A race level */
            if (strcasecmp(game_flow_current_level(), "level2a") != 0) newv = 0x91;
        }
        else if (scene == 0x96)  newv = 0x91;
        else if (scene == 0xa0)  newv = 0xa2;     /* + FUN_004038c0/004061d0 (deferred) */
    } else if (strncmp(fc, "3MOM", 4) == 0) {     /* Judy */
        if      (scene == 0xc8)  newv = 0xcd;     /* + FUN_004038c0(1,3,1) (deferred) */
        else if (scene == 0xd2)  newv = 0xdc;
        else if (scene == 0xe6)  newv = 0xfa;     /* + counter consume (deferred) */
    } else if (strncmp(fc, "3SHE", 4) == 0) {     /* Sheen */
        if      (scene == 0x118) newv = 0x122;
        else if (scene == 0x136) newv = 0x140;    /* + FUN_004038c0(1,4,1) (deferred) */
        else if (scene == 0x168) newv = 0x17c;
    }

    if (newv < 0) return 0;
    if (game_flow_set_entity_state("SCENE", newv)) {
        printf("[TALK] '%s' (%.4s) advanced SCENE 0x%lx -> 0x%lx\n",
               e->tag, fc, scene, newv);
        return 1;
    }
    return 0;
}

static void friend_snap_face_player(Entity *e) {
    Entity *p = g_player;
    if (!p) return;
    float dx = p->x - e->x, dz = p->z - e->z;
    if (dx * dx + dz * dz <= 1.0f) return;
    e->ry = atan2f(-dx, -dz);
}

/* ActivateFriendTalkTrigger -> StartFriendTalkPulse: turn to the player and run
   the per-character talk-reward. Returns 1 if a SCENE write landed. */
static int friend_talk(Entity *e) {
    friend_snap_face_player(e);
    return friend_apply_talk_reward(e);
}

/* T-key path: talk to the nearest talkable friend. Returns it, or NULL. Always
   logs an outcome so the key is observable: who was talked to (the per-character
   reward logs separately if a SCENE beat advanced), or — when nobody is in range
   — the nearest friend and how far away it is, so the player knows to walk over. */
Entity *behavior_friend_talk_nearest(World *w) {
    Entity *p = g_player;
    if (!w || !p) return NULL;
    Entity *best = NULL, *nearest = NULL;
    float bestd2 = 1e30f, nearestd2 = 1e30f;
    const float range2 = FRIEND_TALK_RANGE * FRIEND_TALK_RANGE;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || !e->visible || !friend_is_talk_fourcc(e->type)) continue;
        float dx = p->x - e->x, dz = p->z - e->z;
        float d2 = dx * dx + dz * dz;
        if (d2 < nearestd2) { nearestd2 = d2; nearest = e; }
        if (d2 <= range2 && d2 < bestd2) { bestd2 = d2; best = e; }
    }
    if (best) { (void)friend_talk(best); return best; }
    if (nearest)
        printf("[TALK] no friend in range; nearest is '%s' (%.4s) %.0f units away "
               "(need <= %.0f)\n", nearest->tag, nearest->type,
               sqrtf(nearestd2), FRIEND_TALK_RANGE);
    else
        printf("[TALK] no talkable friends in this level\n");
    return NULL;
}

/* JN_TEST_TALK=<ObjectTag> headless hook: force the friend with this tag through
   its talk-reward (bypassing the proximity/visibility gate) so the per-character
   SCENE write can be exercised without piloting the player. Returns it, or NULL. */
Entity *behavior_friend_talk_tag(World *w, const char *tag) {
    if (!w || !tag || !tag[0]) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || !friend_is_talk_fourcc(e->type)) continue;
        if (strcasecmp(e->tag, tag) != 0) continue;
        (void)friend_talk(e);
        return e;
    }
    return NULL;
}

const EntityVTable vt_friend = {
    .on_spawn   = friend_on_spawn,
    .on_update  = friend_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
