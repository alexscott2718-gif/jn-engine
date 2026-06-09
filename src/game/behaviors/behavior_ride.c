/* behavior_ride.c — native ports of the rotating-ride classes:
 *   C3DFerris (3FER)   — Ferris wheel, turns slowly about its facing axis.
 *
 * Neither carries a per-instance .gam motion parameter (docs/decomp/C3DFerris.md:
 * owned methods are just InitObject + asset load; the rotation is driven by the
 * inherited base). The rotation rate here is engine-tuned to a believable ride
 * speed. Ride-along (carrying the player on a car) is a future refinement; for
 * now the wheel turns visibly.
 */
#include "behaviors.h"
#include <math.h>
#include <stddef.h>

#define FERRIS_DEG_PER_SEC  12.0f   /* slow, stately wheel */

static void ferris_on_update(Entity *e, World *w, float dt) {
    (void)w;
    e->rz = fmodf(e->rz + FERRIS_DEG_PER_SEC * dt, 360.0f);
}

const EntityVTable vt_ferris = {
    .on_spawn   = NULL,
    .on_update  = ferris_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
