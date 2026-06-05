#include "behaviors.h"
#include <stddef.h>
#include <stdio.h>

#define DOOR_OPEN_RISE   180.0f   /* units */
#define DOOR_OPEN_SPEED  240.0f   /* units / sec */

static void door_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = 70.0f;
    e->half_extents[1] = 90.0f;
    e->half_extents[2] = 70.0f;
    e->user_flag = 0;          /* 0 = closed, 1 = opening, 2 = open */
    e->user_float = 0.0f;      /* current rise (additive to base y) */
}

static void door_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (e->user_flag == 1) {
        e->user_float += DOOR_OPEN_SPEED * dt;
        e->y += DOOR_OPEN_SPEED * dt;
        if (e->user_float >= DOOR_OPEN_RISE) {
            e->user_flag = 2;
            printf("[DOOR] open (tag='%s')\n", e->tag);
        }
    }
}

static void door_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag == 0) {
        e->user_flag = 1;
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
