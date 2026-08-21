/* behavior_pickup.c — Wave N3 ability / inventory pickups.
 *
 * One-touch sprite pickups that grant Jimmy an ability (a picture/inventory
 * flag in the original) then hide. Faithful to the per-class Jimmy-touch
 * handlers in docs/decomp/C3DBaseballPickup.md, C3DBubblePickup.md,
 * C3DHelmet.md, and C3DMetalPickup.md. Each gates on the player toucher
 * (on_trigger only fires on player overlap) and collects exactly once.
 *
 * Abilities are modelled as gamestate inventory tools (the native stand-in for
 * the original's picture/inventory flags) so the rest of the engine — e.g. the
 * F-key baseball throw in behavior_player.c — can gate on them. None of these
 * FourCCs is placed in the current .gam corpus (they are code-spawned in the
 * original); they are wired here so the family is complete and so JN_TEST hooks
 * can exercise them.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Shared collect: grant a tool (NULL = score-only), play a sound (<0 = none),
   award Points, hide. Idempotent via user_flag. */
static void pickup_collect(Entity *e, const char *tool, const char *icon, int sound) {
    if (e->user_flag) return;
    e->user_flag = 1;
    if (tool) gamestate_grant_tool(tool, icon);
    if (e->points) gamestate_add_points(e->points);
    if (sound >= 0) {
        const char *db = e->sound_database[0] ? e->sound_database : "soundeffects.omt";
        audio_play_db(db, sound, 0, 128);
    }
    e->visible = 0;
    e->alive = 0;
    gamestate_item_collected();
}

static void pickup_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, 30.0f, 30.0f, 30.0f);
    e->user_flag = 0;
    if (e->visible && (e->runtime_flags & ENTITY_FLAG_TRIGGER))
        gamestate_item_added();
}

/* C3DBaseballPickup (3BPU): grants the baseball (picture flag (0,6)); sound 0x12. */
static void baseball_pickup_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, "baseball", NULL, 0x12);
}
/* C3DBubblePickup (3BUP): grants the bubble ability (picture flag (0,0)). */
static void bubble_pickup_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, "bubble", NULL, -1);
}
/* C3DRocketFuel (3FUE): the plutonium rods in level4b -- seven of them, all on
   sprite 40, the canvas the artist named "plutrod". The decompiled Jimmy-touch
   handler deactivates the object and drives SCENE state; the deactivate half
   needs nothing we do not have, so it lands here, and the SCENE write stays
   deferred. That half is moot on this data anyway: every 3FUE row authors
   TaskName "none", so there is no mission tag for a SCENE write to advance.
   The rows author no PointValue and no SoundIndex either, so collecting is
   silent and scoreless -- it counts toward the level tally and disappears,
   which is what the original's handler does. */
static void rocket_fuel_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, NULL, NULL, -1);
}

/* C3DHelmet (3HEL): grants the helmet. The HELMET task-state visibility hook
   (slot 264) lands with the Wave N5 task system. */
static void helmet_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, "helmet", NULL, -1);
}
/* C3DMetalPickup (3MEP): metal-can collectible -> score. The Goddard fetch
   beacon requests C3DGoddard mode 5 within 1300 units, and releases mode 2 on
   collection or timeout. Scratch: user_float=scan_timer, move_speed=fetch
   request timeout, move_vert=post-release holdoff. */
static void metal_pickup_spawn(Entity *e, World *w) {
    pickup_spawn(e, w);
    snprintf(e->sprite_database, sizeof(e->sprite_database), "%s", "sprites.omt");
    e->sprite_index = 18;      /* cans64 */
    e->sprite_size = 50.0f;
    e->half_extents[0] = 50.0f;
    e->half_extents[1] = 50.0f;
    e->half_extents[2] = 50.0f;
    e->user_float = 0.0f;
    e->move_speed = 0.0f;
    e->move_vert = 0.0f;
}

static void metal_pickup_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive || !e->visible || e->user_flag) return;

    if (e->move_vert > 0.0f) {
        e->move_vert -= dt;
        if (e->move_vert < 0.0f) e->move_vert = 0.0f;
        return;
    }

    if (e->move_speed > 0.0f) {
        e->move_speed -= dt;
        if (e->move_speed <= 0.0f) {
            if (behavior_goddard_target_is(e)) {
                behavior_goddard_release_if_target(e, 2);
                e->move_vert = 6.0f;
            }
            e->move_speed = 0.0f;
        }
    }

    e->user_float += dt;
    if (e->user_float < 1.0f) return;
    e->user_float = 0.0f;

    Entity *g = behavior_goddard_get();
    if (!g || !g->visible) return;
    float dx = g->x - e->x, dy = g->y - e->y, dz = g->z - e->z;
    if (dx * dx + dy * dy + dz * dz <= 1300.0f * 1300.0f) {
        if (behavior_goddard_request_mode(e, 5))
            e->move_speed = 10.0f;
    }
}

static void metal_pickup_trigger(Entity *e, Entity *by) {
    if (by && by != g_player && strncmp(by->type, "3GOD", 4) != 0) return;
    pickup_collect(e, NULL, NULL, -1);
    behavior_goddard_release_if_target(e, 2);
    e->move_speed = 0.0f;
}

#define DEF_PICKUP_VT(name, trig)                                       \
    const EntityVTable name = { .on_spawn = pickup_spawn,               \
                                .on_update = NULL, .on_trigger = (trig), \
                                .flags = ENTITY_FLAG_TRIGGER }
DEF_PICKUP_VT(vt_baseball_pickup, baseball_pickup_trigger);
DEF_PICKUP_VT(vt_bubble_pickup,   bubble_pickup_trigger);
DEF_PICKUP_VT(vt_helmet,          helmet_trigger);
const EntityVTable vt_rocket_fuel = {
    .on_spawn   = pickup_spawn,
    .on_update  = NULL,
    .on_trigger = rocket_fuel_trigger,
    /* Without this the rods spawn as inert geometry: behavior_trigger_spawn_base
       deliberately does NOT set the trigger flag (see its comment), the vtable
       does, and pickup_spawn only counts an entity toward the level tally if
       it is already flagged. */
    .flags      = ENTITY_FLAG_TRIGGER
};

const EntityVTable vt_metal_pickup = {
    .on_spawn   = metal_pickup_spawn,
    .on_update  = metal_pickup_update,
    .on_trigger = metal_pickup_trigger,
    .flags      = ENTITY_FLAG_TRIGGER
};
