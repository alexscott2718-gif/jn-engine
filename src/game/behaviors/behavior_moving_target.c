/* behavior_moving_target.c — C3DMovingTarget (3TAR), the shooting-range target.
 *
 * `3TAR` is registered TWICE in the executable: at `FUN_00430220` by this class,
 * and at `FUN_004453b0` by `C3DShadow` (which is the one whose RTTI string the
 * class-id scan captured). The engine used to bind the id to `vt_shadow`,
 * "Shadow sprite decor" — the other registrar — so every target in the game
 * loaded as static scenery.
 *
 * The shipped data settles it: all 22 placed `3TAR` rows carry
 * `ObjectTag = C3DMOVINGTARGET`, 16 in level3c and 6 in vr07, and none are
 * shadows. vr07 is the VR shooting range and those six targets are the level.
 *
 * The whole mechanic is authored. Every field the decompiled hit handler
 * (`vfunc_01_016` @ `00430570`) touches has a name in the `.gam` rows:
 *
 *     StartPos{X,Y,Z} / DestPos{X,Y,Z} / Speed   the patrol — this is the
 *                                                "Moving" in MovingTarget
 *     RespawnTime      the cooldown the body seeds after a hit (2..15s)
 *     HitsRequired     the threshold it compares the hit count against (3..5)
 *     ActivateObject   the object it activates once that threshold is passed
 *     NumPoints        score per hit (10 everywhere)
 *
 * The handler reads, in order: the toucher must be a `C3DBASEBALL` and the
 * cooldown must have expired; then it deactivates itself, reseeds the cooldown,
 * and resolves its linked object. With no linked object it plays sound `0xc6`
 * (198). With one, it increments a hit counter held on that object and — once
 * the count passes `HitsRequired` — activates it, zeroes the counter, and plays
 * sound `199`.
 *
 * So only the baseball scores: the `C3DTROPHY` / `C3DPICKUPITEM` type checks in
 * that body are on the *linked* object, not on whatever hit the target. In
 * level3c the linked objects are the Retroland midway tickets — `martianticket`,
 * `showerticket`, `starticket` — which is the ticket-booth game.
 *
 * Native differences, deliberate: the hit counter lives here keyed by target
 * tag rather than on the linked entity, so activating a ticket cannot collide
 * with the pickup fields that entity already uses. And the linked object is
 * activated through the shared trigger state setter (state 1), which is the
 * native equivalent of the body's `slot_0x428(1)`.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include "../../engine/physics.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* Recovered sound ids from the hit handler. */
#define TARGET_SOUND_HIT      198   /* 0xc6: hit with no linked object */
#define TARGET_SOUND_AWARD    199   /* 0xc7: threshold passed, object activated */

#define TARGET_HALF_MIN        40.0f
#define TARGET_TAG_MAX         16

/* Per-linked-object hit counters. The original keeps this on the linked object
   itself; here it is keyed by that object's tag so a ticket's own pickup state
   is left alone. Cleared per level with the rest of the behavior state. */
static struct { char tag[64]; int hits; } s_counters[TARGET_TAG_MAX];
static int s_counter_n;

void behavior_moving_target_reset(void) {
    memset(s_counters, 0, sizeof s_counters);
    s_counter_n = 0;
}

static int *counter_for(const char *tag) {
    for (int i = 0; i < s_counter_n; i++)
        if (strcasecmp(s_counters[i].tag, tag) == 0) return &s_counters[i].hits;
    if (s_counter_n >= TARGET_TAG_MAX) return NULL;
    snprintf(s_counters[s_counter_n].tag, sizeof(s_counters[0].tag), "%s", tag);
    s_counters[s_counter_n].hits = 0;
    return &s_counters[s_counter_n++].hits;
}

static const char *link_tag(const Entity *e) {
    const char *t = gam_str(e, "ActivateObject", "");
    if (!t || !t[0] || strcasecmp(t, "none") == 0) return NULL;
    return t;
}

static void target_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);

    /* StartPos/DestPos come straight out of the prop bag, so they carry the
       raw .gam handedness. PositionZ is negated at load (gam_loader.c) to put
       the whole world in one convention; these have to follow it or the patrol
       runs mirrored through the level. */
    e->home[0]      = gam_prop_f(e, "StartPosX", e->x);
    e->home[1]      = gam_prop_f(e, "StartPosY", e->y);
    e->home[2]      = -gam_prop_f(e, "StartPosZ", -e->z);
    e->patrol_to[0] = gam_prop_f(e, "DestPosX", e->home[0]);
    e->patrol_to[1] = gam_prop_f(e, "DestPosY", e->home[1]);
    e->patrol_to[2] = -gam_prop_f(e, "DestPosZ", -e->home[2]);

    float half = e->sprite_size > 0.0f ? e->sprite_size * 0.5f : TARGET_HALF_MIN;
    if (half < TARGET_HALF_MIN) half = TARGET_HALF_MIN;
    e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = half;

    e->x = e->home[0];
    e->y = e->home[1];
    e->z = e->home[2];
    e->user_float = 0.0f;   /* respawn cooldown, 0 = live */
    e->move_lean  = 0.0f;   /* patrol phase, 0..1 along home -> dest */
    e->user_flag  = 0;      /* 0 = up, 1 = knocked down */

    float dx = e->patrol_to[0] - e->home[0];
    float dy = e->patrol_to[1] - e->home[1];
    float dz = e->patrol_to[2] - e->home[2];
    printf("[TARGET] '%s' travel=%.0f speed=%.0f hits=%d respawn=%.0f link='%s'\n",
           e->tag, sqrtf(dx * dx + dy * dy + dz * dz),
           gam_prop_f(e, "Speed", 0.0f), gam_prop_i(e, "HitsRequired", 0),
           gam_prop_f(e, "RespawnTime", 0.0f),
           link_tag(e) ? link_tag(e) : "none");
}

static void target_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;

    /* Knocked down: count the cooldown out, then stand back up. */
    if (e->user_flag) {
        e->user_float -= dt;
        if (e->user_float > 0.0f) return;
        e->user_flag = 0;
        e->visible = 1;
        return;
    }

    float dx = e->patrol_to[0] - e->home[0];
    float dy = e->patrol_to[1] - e->home[1];
    float dz = e->patrol_to[2] - e->home[2];
    float span = sqrtf(dx * dx + dy * dy + dz * dz);
    float speed = gam_prop_f(e, "Speed", 0.0f);
    if (span > 1.0f && speed > 0.0f) {
        /* Ping-pong: phase runs 0..2, folded so 1..2 walks back. */
        e->move_lean += (speed / span) * dt;
        while (e->move_lean >= 2.0f) e->move_lean -= 2.0f;
        float t = e->move_lean <= 1.0f ? e->move_lean : 2.0f - e->move_lean;
        e->x = e->home[0] + dx * t;
        e->y = e->home[1] + dy * t;
        e->z = e->home[2] + dz * t;
    }
}

/* The baseball's hit, from behavior_projectile's player-team sweep. Returns 1
   when this entity consumed the hit, so the projectile despawns on it. */
int behavior_moving_target_take_hit(Entity *e, World *w) {
    if (!e || !e->alive || e->vt != &vt_moving_target) return 0;
    if (e->user_flag || !e->visible) return 0;      /* already down */

    e->user_flag  = 1;
    e->visible    = 0;
    e->user_float = gam_prop_f(e, "RespawnTime", 5.0f);

    int pts = gam_prop_i(e, "NumPoints", 0);
    if (pts) gamestate_add_points(pts);

    const char *link = link_tag(e);
    if (!link) {
        audio_play_db("soundeffects.omt", TARGET_SOUND_HIT, 0, 128);
        printf("[TARGET] '%s' hit (+%d)\n", e->tag, pts);
        return 1;
    }

    int *hits = counter_for(link);
    int need = gam_prop_i(e, "HitsRequired", 3);
    if (hits) (*hits)++;
    printf("[TARGET] '%s' hit (+%d) -> '%s' %d/%d\n",
           e->tag, pts, link, hits ? *hits : 0, need);

    if (hits && *hits >= need) {
        *hits = 0;
        behavior_trigger_set_state_tag(w, link, 1);
        audio_play_db("soundeffects.omt", TARGET_SOUND_AWARD, 0, 128);
        printf("[TARGET] '%s' activated after %d hits\n", link, need);
    } else {
        audio_play_db("soundeffects.omt", TARGET_SOUND_HIT, 0, 128);
    }
    return 1;
}

const EntityVTable vt_moving_target = {
    .on_spawn   = target_on_spawn,
    .on_update  = target_on_update,
    .on_trigger = NULL,     /* hit by projectile, not by walking into it */
};
