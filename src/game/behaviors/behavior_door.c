#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <stdio.h>

#define DOOR_OPEN_RISE   180.0f   /* units */
#define DOOR_OPEN_SPEED  240.0f   /* units / sec */
#define DOOR_OPEN_SOUND  59       /* soundeffects.omt: Door opening loop */
#define DOOR_CLOSE_SOUND 58       /* soundeffects.omt: door closing */

static float door_open_rise(const Entity *e) {
    float rise = gam_prop_f(e, "OpenAmount", DOOR_OPEN_RISE);
    return rise > 0.0f ? rise : DOOR_OPEN_RISE;
}

static float door_open_speed(const Entity *e) {
    float speed = gam_prop_f(e, "DoorSpeed", 0.0f);
    return speed > 0.0f ? speed * 30.0f : DOOR_OPEN_SPEED;
}

static float door_open_time(const Entity *e) {
    float t = gam_prop_f(e, "OpenTime", 0.0f);
    return t > 0.0f ? t : 3.0f;
}

static void door_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->half_extents[0] = 70.0f;
    e->half_extents[1] = 90.0f;
    e->half_extents[2] = 70.0f;
    e->home[0] = 0.0f;         /* open wait timer */
    e->home[1] = e->y;         /* closed/base Y; update writes base + rise */
    e->user_flag = 0;          /* 0=closed, 1=opening, 2=open-wait, 3=closing */
    e->user_float = 0.0f;      /* current rise (additive to base y) */
    e->home[2] = -1.0f;        /* looping open-SFX channel (-1 = none) */
}

/* Stop the looping "door opening" SFX (soundeffects.omt[59]) once the door
   stops moving. QA 2026-06-24 #7 (l6a halldoor01): the open sound must loop
   while the door is in motion and cut out when it stops, not play one-shot. */
static void door_halt_open_loop(Entity *e) {
    if (e->home[2] >= 0.0f) {
        audio_channel_halt((int)e->home[2]);
        e->home[2] = -1.0f;
    }
}

static void door_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (e->user_flag == 1) {
        float rise = door_open_rise(e);
        e->user_float += door_open_speed(e) * dt;
        if (e->user_float >= rise) {
            e->user_float = rise;
            e->user_flag = 2;
            e->home[0] = 0.0f;
            door_halt_open_loop(e);   /* door reached fully-open -> stop loop */
            printf("[DOOR] open (tag='%s')\n", e->tag);
        }
        e->y = e->home[1] + e->user_float;
    } else if (e->user_flag == 2) {
        e->home[0] += dt;
        if (e->home[0] >= door_open_time(e)) {
            e->user_flag = 3;
            audio_play_db("soundeffects.omt", DOOR_CLOSE_SOUND, 0, 96);
            printf("[DOOR] closing (tag='%s')\n", e->tag);
        }
    } else if (e->user_flag == 3) {
        e->user_float -= door_open_speed(e) * dt;
        if (e->user_float <= 0.0f) {
            e->user_float = 0.0f;
            e->user_flag = 0;
            printf("[DOOR] closed (tag='%s')\n", e->tag);
        }
        e->y = e->home[1] + e->user_float;
    }
}

static void door_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag == 0) {
        e->user_flag = 1;
        /* Loop the open SFX (-1) for the duration of the opening motion; it is
           halted in door_on_update when the door latches fully-open. */
        door_halt_open_loop(e);   /* guard against a stale channel */
        e->home[2] = (float)audio_play_db("soundeffects.omt",
                                          DOOR_OPEN_SOUND, -1, 96);
        printf("[DOOR] opening (tag='%s')\n", e->tag);
    }
}

const EntityVTable vt_door = {
    .on_spawn = door_on_spawn,
    .on_update = door_on_update,
    .on_trigger = door_on_trigger,
    .flags = ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER,
};

/* Level door (3DOR/3DUD/3SCD): same rise-open animation but NOT solid, so
   binding it can't trap the player at a doorway (these were previously walked
   through). Opens on a direct touch or when a button forwards a trigger. */
const EntityVTable vt_leveldoor = {
    .on_spawn = door_on_spawn,
    .on_update = door_on_update,
    .on_trigger = door_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
