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
 * The per-character special cases the leaves add on top of this base — Nick's
 * skateboard child + race checkpoints, Sheen/Libby/Judy/Benny/UltraLord one-shot
 * talk rewards, Kitty's effect/sound handle — are deferred; this is the placed,
 * visible, idle-or-patrol core that clears the friend rows from the behavior
 * lens and gives them real C3DAI movement.
 */
#include "behavior_ai.h"
#include <math.h>
#include <stddef.h>

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

const EntityVTable vt_friend = {
    .on_spawn   = friend_on_spawn,
    .on_update  = friend_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
