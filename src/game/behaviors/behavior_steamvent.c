/* behavior_steamvent.c — native port of C3DSteamVent (FourCC 3STE).
 *
 * From docs/decomp/C3DSteamVent.md + shipped .gam data (17 instances): the vent
 * carries a lit colour (Red=1, Green/Blue=0.1 -> red glow), ButtonAvailable
 * (0/1 — whether the vent is currently emitting), AvailSound=81, and a
 * NextButton chain ("VENT10","VENT11",...). It is a button-derived, sequenced
 * steam jet.
 *
 * Native realisation: while ButtonAvailable, the vent steadily emits steam and
 * launches a player standing over it upward (a steady-jet hazard/lift). The
 * NextButton chain and lit colour are cosmetic here. Motion strength is
 * engine-tuned (the original drives it through the inherited steam base, with no
 * per-instance .gam parameter).
 */
#include "behaviors.h"
#include <stddef.h>

#define STE_RADIUS  100.0f
#define STE_LAUNCH  820.0f

static void steamvent_on_spawn(Entity *e, World *w) {
    (void)w;
    e->home[1] = e->y;
    /* ButtonAvailable seeds the runtime emit-state; a control could toggle it. */
    e->user_flag = gam_prop_i(e, "ButtonAvailable", 1);
}

static void steamvent_on_update(Entity *e, World *w, float dt) {
    (void)w; (void)dt;
    if (!e->user_flag || !g_player) return;
    float dx = g_player->x - e->x;
    float dz = g_player->z - e->z;
    if (dx*dx + dz*dz < STE_RADIUS*STE_RADIUS && g_player->y >= e->home[1] - 40.0f) {
        if (g_player->vy < STE_LAUNCH) g_player->vy = STE_LAUNCH;
    }
}

const EntityVTable vt_steamvent = {
    .on_spawn   = steamvent_on_spawn,
    .on_update  = steamvent_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
