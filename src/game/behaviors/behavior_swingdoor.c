/* behavior_swingdoor.c — C3DSwingDoor (3SWN).
 *
 * A placeable swinging door: a timed yaw-rotation mechanism. Activation (a
 * player touch when TouchActivated, or a forwarded trigger) sets is_moving and
 * seeds a countdown from TimeToOpen; each frame the door rotates about its yaw
 * by OpenSpeed*dt — forward while opening, backward while closing — until the
 * timer expires, then it latches the opposite direction so the *next*
 * activation swings it back (docs/decomp/C3DSwingDoor.md, UpdateDoor).
 *
 * The decompiled two-phase machine (is_moving + closing latch) is modeled here
 * as a 4-state phase so the "next activation reverses" latch is explicit:
 *   CLOSED_IDLE -> OPENING -> OPEN_IDLE -> CLOSING -> CLOSED_IDLE
 * A short re-trigger cooldown after each swing keeps a player standing in the
 * (non-solid) doorway from toggling it twice per overlap — physics calls
 * on_trigger every frame of contact, not just on enter (cf. behavior_escort.c).
 *
 * Collision: the original's Reset enables collision (HasCollision is authored
 * on), but this is non-solid like vt_leveldoor — our AABB does not rotate with
 * the yaw swing (a "swung-open" solid door would still block), and the
 * TouchActivated=0 instances (mummydoor, the Level3D retro doors) have no
 * ported opener (no ActivateObject is authored; the scripted-trigger chain is
 * deferred), so a solid door there would trap the player.
 *
 * Scratch: user_flag=phase, home[0]=motion_timer, home[1]=retrigger cooldown.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <stdio.h>

#define SWINGDOOR_TIME_TO_OPEN 1.8f   /* authored on every instance */
#define SWINGDOOR_OPEN_SPEED   50.0f  /* deg/sec; *1.8s = a 90-degree swing */
#define SWINGDOOR_COOLDOWN     1.5f   /* re-trigger lockout after a swing */
#define SWINGDOOR_OPEN_SOUND   59     /* soundeffects.omt: door opening */
#define SWINGDOOR_CLOSE_SOUND  58     /* soundeffects.omt: door closing */

enum {
    SWING_CLOSED_IDLE = 0,
    SWING_OPENING     = 1,
    SWING_OPEN_IDLE   = 2,
    SWING_CLOSING     = 3
};

static float swingdoor_time_to_open(const Entity *e) {
    float t = gam_prop_f(e, "TimeToOpen", SWINGDOOR_TIME_TO_OPEN);
    return t > 0.0f ? t : SWINGDOOR_TIME_TO_OPEN;
}

static float swingdoor_open_speed(const Entity *e) {
    float s = gam_prop_f(e, "OpenSpeed", SWINGDOOR_OPEN_SPEED);
    return s > 0.0f ? s : SWINGDOOR_OPEN_SPEED;
}

static void swingdoor_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->half_extents[0] = 80.0f;
    e->half_extents[1] = 100.0f;
    e->half_extents[2] = 80.0f;
    e->user_flag = SWING_CLOSED_IDLE;
    e->home[0] = 0.0f;   /* motion_timer */
    e->home[1] = 0.0f;   /* cooldown */
    e->runtime_flags &= ~ENTITY_FLAG_SOLID;   /* non-solid swing (see header) */
}

static void swingdoor_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    e->runtime_flags &= ~ENTITY_FLAG_SOLID;

    if (e->home[1] > 0.0f) {
        e->home[1] -= dt;
        if (e->home[1] < 0.0f) e->home[1] = 0.0f;
    }

    if (e->user_flag != SWING_OPENING && e->user_flag != SWING_CLOSING) return;

    float speed = swingdoor_open_speed(e);
    e->home[0] -= dt;
    if (e->user_flag == SWING_OPENING) {
        e->ry += speed * dt;
        if (e->home[0] <= 0.0f) {
            e->home[0] = 0.0f;
            e->user_flag = SWING_OPEN_IDLE;
            e->home[1] = SWINGDOOR_COOLDOWN;
            printf("[SWINGDOOR] open (tag='%s' ry=%.1f)\n", e->tag, e->ry);
        }
    } else {  /* SWING_CLOSING */
        e->ry -= speed * dt;
        if (e->home[0] <= 0.0f) {
            e->home[0] = 0.0f;
            e->user_flag = SWING_CLOSED_IDLE;
            e->home[1] = SWINGDOOR_COOLDOWN;
            printf("[SWINGDOOR] closed (tag='%s' ry=%.1f)\n", e->tag, e->ry);
        }
    }
}

static void swingdoor_on_trigger(Entity *e, Entity *by) {
    if (by != g_player || !by->alive) return;
    if (e->home[1] > 0.0f) return;            /* still on re-trigger cooldown */
    if (e->user_flag == SWING_OPENING || e->user_flag == SWING_CLOSING) return;

    e->home[0] = swingdoor_time_to_open(e);   /* seed motion_timer */
    if (e->user_flag == SWING_CLOSED_IDLE) {
        e->user_flag = SWING_OPENING;
        audio_play_db("soundeffects.omt", SWINGDOOR_OPEN_SOUND, 0, 96);
        printf("[SWINGDOOR] opening (tag='%s')\n", e->tag);
    } else {  /* SWING_OPEN_IDLE */
        e->user_flag = SWING_CLOSING;
        audio_play_db("soundeffects.omt", SWINGDOOR_CLOSE_SOUND, 0, 96);
        printf("[SWINGDOOR] closing (tag='%s')\n", e->tag);
    }
}

/* Non-solid (TRIGGER only) so the swing never traps the player at a doorway,
   matching vt_leveldoor's collision posture. */
const EntityVTable vt_swingdoor = {
    .on_spawn   = swingdoor_on_spawn,
    .on_update  = swingdoor_on_update,
    .on_trigger = swingdoor_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
