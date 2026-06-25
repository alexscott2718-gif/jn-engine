/* behavior_neutron.c — C3DNeutron (3NEU) and C3DRedNeutron (3RED).
 *
 * These are C3DSprite-family billboard pickups, not inert sprite markers.
 * The original preloads sprites.omt frames 0..12, idles on frames 0..7, then
 * runs the burst frames after active-player contact. 3NEU later respawns;
 * 3RED stays collected and can dispatch its NextTrigger.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define NEUTRON_FRAME_STEP      0.08f
#define NEUTRON_RESPAWN_WAIT    5.0f
#define NEUTRON_MIN_HALF       35.0f
#define RED_NEUTRON_RADIUS    100.0f

enum {
    NEU_IDLE = 0,
    NEU_BURST = 1,
    NEU_RESPAWN_WAITING = 2,
    NEU_DONE = 3
};

static int neu_state(const Entity *e) { return e->user_flag / 100; }
static int neu_frame(const Entity *e) { return e->user_flag % 100; }
static void neu_set_state_frame(Entity *e, int state, int frame) {
    if (frame < 0) frame = 0;
    if (frame > 12) frame = 12;
    e->user_flag = state * 100 + frame;
    e->sprite_index = frame;
}

static void neutron_use_runtime_sprite(Entity *e) {
    snprintf(e->sprite_database, sizeof(e->sprite_database), "%s", "sprites.omt");
    if (e->sprite_size <= 1.0f)
        e->sprite_size = 100.0f;
}

static void neutron_set_trigger_size(Entity *e, float fallback_radius) {
    float half = e->sprite_size > 1.0f ? e->sprite_size * 0.5f : fallback_radius;
    if (half < NEUTRON_MIN_HALF) half = NEUTRON_MIN_HALF;
    e->half_extents[0] = half;
    e->half_extents[1] = half;
    e->half_extents[2] = half;
}

static Entity *find_tag(World *w, const char *tag) {
    if (!w || !tag || !tag[0] || strcasecmp(tag, "none") == 0) return NULL;
    for (Entity *o = w->head; o; o = o->next) {
        if (o->tag[0] && strcasecmp(o->tag, tag) == 0) return o;
    }
    return NULL;
}

static void neutron_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, 50.0f, 50.0f, 50.0f);
    neutron_use_runtime_sprite(e);
    neutron_set_trigger_size(e, 50.0f);
    e->user_float = 0.0f;
    e->anim_time = 0.0f;
    neu_set_state_frame(e, NEU_IDLE, 0);
}

static void red_neutron_spawn(Entity *e, World *w) {
    behavior_trigger_spawn_base(e, 50.0f, 50.0f, 50.0f);
    neutron_use_runtime_sprite(e);
    /* Red neutrons are headline collectibles. Level rows that don't author a
       SpriteSize fall to the 100 default — the low end of the measured
       100..1300 range (docs/decomp/C3DSprite.md) — which read too small from a
       distance (QA sandmanfan 2026-06-24 l5). Raise the floor for unauthored
       reds so they stay visible; authored larger sizes are kept. */
    if (e->sprite_size < 170.0f) e->sprite_size = 170.0f;
    float radius = gam_prop_f(e, "Radius", RED_NEUTRON_RADIUS);
    if (radius <= 0.0f) radius = RED_NEUTRON_RADIUS;
    e->half_extents[0] = radius;
    e->half_extents[1] = radius;
    e->half_extents[2] = radius;
    e->user_float = 0.0f;
    e->anim_time = 0.0f;
    e->link_target = find_tag(w, gam_str(e, "NextTrigger", "none"));
    neu_set_state_frame(e, NEU_IDLE, 0);
}

static void neutron_collect(Entity *e) {
    if (neu_state(e) != NEU_IDLE) return;
    audio_play_db("soundeffects.omt", 4, 0, 128);
    e->user_float = 0.0f;
    neu_set_state_frame(e, NEU_BURST, 8);
    printf("[NEUTRON] collected '%s'\n", e->tag);
}

static void red_neutron_collect(Entity *e, Entity *by, World *w) {
    if (neu_state(e) != NEU_IDLE) return;
    audio_play_db("soundeffects.omt", 0x0e, 0, 128);
    e->user_float = 0.0f;
    neu_set_state_frame(e, NEU_BURST, 8);

    const char *next = gam_str(e, "NextTrigger", "none");
    Entity *n = e->link_target ? e->link_target : find_tag(w, next);
    if (n && n->vt && n->vt->on_trigger)
        n->vt->on_trigger(n, by);
    printf("[REDNEUTRON] collected '%s' next='%s'\n", e->tag, next);
}

static void neutron_trigger(Entity *e, Entity *by) {
    if (by != g_player) return;
    neutron_collect(e);
}

static void red_neutron_trigger(Entity *e, Entity *by) {
    if (by != g_player) return;
    red_neutron_collect(e, by, NULL);
}

static void neutron_tick_common(Entity *e, float dt, int respawns) {
    if (!e->alive) return;
    e->anim_time += dt;
    e->user_float += dt;

    int state = neu_state(e);
    if (state == NEU_RESPAWN_WAITING) {
        if (e->user_float < NEUTRON_RESPAWN_WAIT) return;
        e->visible = 1;
        e->runtime_flags |= ENTITY_FLAG_TRIGGER;
        e->user_float = 0.0f;
        neu_set_state_frame(e, NEU_IDLE, 0);
        return;
    }

    if (e->user_float < NEUTRON_FRAME_STEP) return;
    e->user_float = 0.0f;

    int frame = neu_frame(e);
    if (state == NEU_BURST) {
        frame++;
        if (frame <= 12) {
            neu_set_state_frame(e, NEU_BURST, frame);
            return;
        }
        e->visible = 0;
        e->runtime_flags &= ~ENTITY_FLAG_TRIGGER;
        if (respawns) {
            e->user_float = 0.0f;
            neu_set_state_frame(e, NEU_RESPAWN_WAITING, 12);
        } else {
            neu_set_state_frame(e, NEU_DONE, 12);
            e->alive = 0;
        }
        return;
    }

    if (state == NEU_IDLE)
        neu_set_state_frame(e, NEU_IDLE, (frame + 1) & 7);
}

static void neutron_update(Entity *e, World *w, float dt) {
    (void)w;
    neutron_tick_common(e, dt, 1);
}

static void red_neutron_update(Entity *e, World *w, float dt) {
    if (neu_state(e) == NEU_IDLE && g_player) {
        /* physics.c will call on_trigger, but this keeps scripted direct
           dispatches and large Radius rows responsive even if flags changed. */
        float dx = e->x - g_player->x;
        float dy = e->y - g_player->y;
        float dz = e->z - g_player->z;
        float r = e->half_extents[0];
        if (dx * dx + dy * dy + dz * dz <= r * r)
            red_neutron_collect(e, g_player, w);
    }
    neutron_tick_common(e, dt, 0);
}

const EntityVTable vt_neutron = {
    .on_spawn   = neutron_spawn,
    .on_update  = neutron_update,
    .on_trigger = neutron_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};

const EntityVTable vt_red_neutron = {
    .on_spawn   = red_neutron_spawn,
    .on_update  = red_neutron_update,
    .on_trigger = red_neutron_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
