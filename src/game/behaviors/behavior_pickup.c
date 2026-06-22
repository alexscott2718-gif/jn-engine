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
/* C3DHelmet (3HEL): grants the helmet. The HELMET task-state visibility hook
   (slot 264) lands with the Wave N5 task system. */
static void helmet_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, "helmet", NULL, -1);
}
/* C3DMetalPickup (3MEP): metal-can collectible -> score. The Goddard fetch
   beacon (controller mode 5/2 within 1300 units) is deferred until C3DGoddard
   is ported in a later wave. */
static void metal_pickup_trigger(Entity *e, Entity *by) {
    (void)by; pickup_collect(e, NULL, NULL, -1);
}

#define DEF_PICKUP_VT(name, trig)                                       \
    const EntityVTable name = { .on_spawn = pickup_spawn,               \
                                .on_update = NULL, .on_trigger = (trig), \
                                .flags = ENTITY_FLAG_TRIGGER }
DEF_PICKUP_VT(vt_baseball_pickup, baseball_pickup_trigger);
DEF_PICKUP_VT(vt_bubble_pickup,   bubble_pickup_trigger);
DEF_PICKUP_VT(vt_helmet,          helmet_trigger);
DEF_PICKUP_VT(vt_metal_pickup,    metal_pickup_trigger);
