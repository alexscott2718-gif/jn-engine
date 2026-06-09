/* behavior_geyser.c — native port of C3DGeyser (FourCC 3GEY).
 *
 * Faithful to docs/decomp/C3DGeyser.md: the only authored parameter is
 * SteamPeriod (float). Validated against shipped .gam data: 9 instances,
 * SteamPeriod 2..6 (seconds).
 *
 * Native realisation: the geyser erupts on a SteamPeriod cycle (erupting for the
 * first ~45% of each period). While erupting, a player standing over the vent
 * (within a horizontal radius) is launched upward — the classic steam-jet
 * hazard. The mesh bobs up during the eruption for a visible plume.
 */
#include "behaviors.h"
#include <math.h>
#include <stddef.h>

#define GEY_RADIUS      110.0f   /* horizontal reach of the jet (units) */
#define GEY_LAUNCH      900.0f   /* upward launch velocity (units/sec) */
#define GEY_ERUPT_FRAC  0.45f    /* fraction of the period spent erupting */
#define GEY_PLUME_RISE  120.0f   /* mesh rise at full eruption (units) */

static void geyser_on_spawn(Entity *e, World *w) {
    (void)w;
    e->user_float = 0.0f;        /* cycle clock */
    e->home[1] = e->y;           /* base Y for the plume bob */
}

static void geyser_on_update(Entity *e, World *w, float dt) {
    (void)w;
    float period = gam_prop_f(e, "SteamPeriod", 4.0f);
    if (period <= 0.0f) period = 4.0f;

    e->user_float = fmodf(e->user_float + dt, period);
    float phase = e->user_float / period;            /* 0..1 over the cycle */
    int erupting = phase < GEY_ERUPT_FRAC;

    /* Visible plume: rise on a half-sine during the eruption window. */
    float rise = 0.0f;
    if (erupting) rise = GEY_PLUME_RISE * sinf((phase / GEY_ERUPT_FRAC) * 3.14159265f);
    e->y = e->home[1] + rise;

    /* Launch the player if standing over the vent while erupting. */
    if (erupting && g_player) {
        float dx = g_player->x - e->x;
        float dz = g_player->z - e->z;
        if (dx*dx + dz*dz < GEY_RADIUS*GEY_RADIUS && g_player->y >= e->home[1] - 40.0f) {
            if (g_player->vy < GEY_LAUNCH) g_player->vy = GEY_LAUNCH;
        }
    }
}

const EntityVTable vt_geyser = {
    .on_spawn   = geyser_on_spawn,
    .on_update  = geyser_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
