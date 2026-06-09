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

#define FERRIS_RAD_PER_SEC  0.21f   /* ~12 deg/s — slow, stately wheel (radians) */

static void ferris_on_update(Entity *e, World *w, float dt) {
    (void)w;
    /* Roll about the wheel's local forward axis; renderer euler path applies rz. */
    e->rz = fmodf(e->rz + FERRIS_RAD_PER_SEC * dt, 6.2831853f);
}

const EntityVTable vt_ferris = {
    .on_spawn   = NULL,
    .on_update  = ferris_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
