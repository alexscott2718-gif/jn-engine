/* behavior_tesla.c — C3DTesla (3TES).
 *
 * Decomp summary: an animated electric hazard with ItemActive, a 0.1s texture
 * frame cycle while active, a looping effect, and a Jimmy-only contact push.
 * Native preserves the gameplay contract: inactive Teslas are hidden and do
 * not trigger; active Teslas push/damage the player on overlap and can be
 * toggled by remote triggers/buttons.
 */
#include "behavior_tesla.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/physics.h"
#include <math.h>
#include <stdio.h>

#define TESLA_DAMAGE          10
#define TESLA_HIT_COOLDOWN     1.0f
#define TESLA_PUSH_SPEED     520.0f
#define TESLA_PUSH_NUDGE      35.0f

static void tesla_sync_active(Entity *e) {
    if (!e) return;
    if (e->user_flag) {
        e->visible = 1;
        e->runtime_flags |= ENTITY_FLAG_TRIGGER;
    } else {
        e->visible = 0;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    }
}

void behavior_tesla_set_active(Entity *e, int mode) {
    if (!e) return;
    if (mode == 0) {
        e->user_flag = 0;
    } else if (mode == 1) {
        e->user_flag = 1;
    } else {
        e->user_flag = !e->user_flag;
    }
    tesla_sync_active(e);
    printf("[TESLA] '%s' active=%d\n", e->tag, e->user_flag ? 1 : 0);
}

static void tesla_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->user_flag = gam_prop_i(e, "ItemActive", 1) ? 1 : 0;
    e->user_float = 0.0f; /* contact damage cooldown */
    e->half_extents[0] = 95.0f;
    e->half_extents[1] = 150.0f;
    e->half_extents[2] = 95.0f;
    tesla_sync_active(e);
}

static void tesla_contact_player(Entity *e, Entity *by) {
    if (!e->user_flag || by != g_player || e->user_float > 0.0f) return;

    float dx = by->x - e->x;
    float dz = by->z - e->z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len <= 1.0f) return;
    float ux = dx / len;
    float uz = dz / len;
    by->vx += ux * TESLA_PUSH_SPEED;
    by->vz += uz * TESLA_PUSH_SPEED;
    by->x += ux * TESLA_PUSH_NUDGE;
    by->z += uz * TESLA_PUSH_NUDGE;

    e->user_float = TESLA_HIT_COOLDOWN;
    gamestate_damage_player(TESLA_DAMAGE);
    printf("[TESLA] '%s' shocked player\n", e->tag);
}

static void tesla_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    if (!e->user_flag) {
        tesla_sync_active(e);
        return;
    }
    if (e->user_float > 0.0f) e->user_float -= dt;
    tesla_sync_active(e);
}

static void tesla_on_trigger(Entity *e, Entity *by) {
    if (!by) return;
    if (by == g_player && physics_aabb_overlap(e, by)) {
        tesla_contact_player(e, by);
        return;
    }

    /* Remote activation forwarded through the native button/trigger path. */
    behavior_tesla_set_active(e, -1);
}

const EntityVTable vt_tesla = {
    .on_spawn   = tesla_on_spawn,
    .on_update  = tesla_on_update,
    .on_trigger = tesla_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
