/* behavior_tractor.c — native port of C3DTractorBeam (FourCC 3TRC).
 *
 * From docs/decomp/C3DTractorBeam.md + shipped .gam data (6 instances): no
 * per-instance motion parameter — InitObject just sets up the beam visual
 * (scale 10.0) and the pull is driven by the inherited base. The classic Jimmy
 * Neutron UFO/abduction beam.
 *
 * Native realisation: a player within the beam's horizontal radius is pulled
 * inward toward the beam axis and lifted upward (overcoming gravity), as if
 * caught in the tractor beam. Strengths are engine-tuned.
 */
#include "behaviors.h"
#include <math.h>
#include <stddef.h>

#define TRC_RADIUS   150.0f   /* horizontal capture radius (units) */
#define TRC_LIFT     520.0f   /* upward pull velocity (units/sec) */
#define TRC_PULL     3.0f     /* inward centering rate (per sec) */

static void tractor_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!g_player) return;
    float dx = g_player->x - e->x;
    float dz = g_player->z - e->z;
    float d2 = dx*dx + dz*dz;
    if (d2 >= TRC_RADIUS*TRC_RADIUS) return;

    /* lift toward/into the beam */
    if (g_player->vy < TRC_LIFT) g_player->vy += (TRC_LIFT - g_player->vy) * fminf(1.0f, dt * 2.0f);
    /* draw horizontally toward the beam axis */
    g_player->x -= dx * fminf(1.0f, TRC_PULL * dt);
    g_player->z -= dz * fminf(1.0f, TRC_PULL * dt);
}

const EntityVTable vt_tractor = {
    .on_spawn   = NULL,
    .on_update  = tractor_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
