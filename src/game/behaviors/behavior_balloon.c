/* behavior_balloon.c — C3DBalloon (3BAL), Wave N3.
 *
 * Faithful to docs/decomp/C3DBalloon.md. A balloon is a two-stage prop:
 *   1. Jimmy's touch RELEASES it (HandleBalloonTouch, active-player branch) —
 *      it then drifts upward (UpdateReleasedBalloon).
 *   2. A thrown baseball / rocket POPS it (HandleBalloonTouch, C3DBASEBALL /
 *      C3DROCKET branch), awarding score and latching it popped.
 * Our shared projectile (FourCC "PROJ", PROJ_TEAM_PLAYER) is the C3DBASEBALL
 * the player throws — the balloon scans for it, mirroring the decomp's
 * is_a("C3DBASEBALL") test. Score uses the decomp's distance-bonus formula and
 * the 10 (resting) / 200 (released) base values.
 *
 *   user_flag : BAL_WAITING -> BAL_RELEASED -> BAL_POPPED
 */
#include "behaviors.h"
#include "behavior_projectile.h"
#include "../gamestate.h"
#include "../../engine/physics.h"
#include "../../engine/audio.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define BALLOON_HALF       100.0f   /* SpriteSize 200 in the decomp -> ~half */
#define BALLOON_RISE        70.0f   /* released drift toward the decomp's 70.0 clamp */
#define BALLOON_POP_SOUND   0xad    /* C3DBalloon pop effect id */
#define BALLOON_DIST_MIN  1000.0f   /* min player distance before a distance bonus */
#define BALLOON_SCORE_RESTING   10  /* base score if popped before Jimmy released it */
#define BALLOON_SCORE_RELEASED 200  /* base score if popped after release */

enum { BAL_WAITING = 0, BAL_RELEASED = 1, BAL_POPPED = 2 };

static void balloon_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = BALLOON_HALF;
    e->user_flag = BAL_WAITING;
}

/* Jimmy contact (player overlap) releases a resting balloon; it does not pop. */
static void balloon_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag == BAL_WAITING)
        e->user_flag = BAL_RELEASED;
}

static void balloon_pop(Entity *e) {
    int released = (e->user_flag == BAL_RELEASED);
    int bonus = 0;
    if (g_player) {
        float dx = e->x - g_player->x, dy = e->y - g_player->y, dz = e->z - g_player->z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist > BALLOON_DIST_MIN)
            bonus = (int)(dist * (1.0f / 30.0f) - 33.0f);   /* decomp formula */
    }
    int score = bonus + (released ? BALLOON_SCORE_RELEASED : BALLOON_SCORE_RESTING);
    if (score > 0) gamestate_add_points(score);
    audio_play_db("soundeffects.omt", BALLOON_POP_SOUND, 0, 128);
    e->user_flag = BAL_POPPED;
    e->visible = 0;
    e->alive = 0;
    printf("[BALLOON] popped '%s' (+%d pts, %s)\n",
           e->tag, score > 0 ? score : 0, released ? "released" : "resting");
}

static void balloon_on_update(Entity *e, World *w, float dt) {
    if (!e->alive || e->user_flag == BAL_POPPED) return;

    /* Pop on contact with a thrown baseball (player projectile). */
    for (Entity *p = w->head; p; p = p->next) {
        if (!p->alive || p == e) continue;
        if (strncmp(p->type, "PROJ", 4) != 0) continue;
        if (p->user_flag != PROJ_TEAM_PLAYER) continue;
        if (!physics_aabb_overlap(p, e)) continue;
        p->alive = 0; p->visible = 0;   /* the ball is consumed by the pop */
        balloon_pop(e);
        return;
    }

    /* Released balloons drift upward until popped. */
    if (e->user_flag == BAL_RELEASED)
        e->y += BALLOON_RISE * dt;
}

const EntityVTable vt_balloon = {
    .on_spawn   = balloon_on_spawn,
    .on_update  = balloon_on_update,
    .on_trigger = balloon_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
