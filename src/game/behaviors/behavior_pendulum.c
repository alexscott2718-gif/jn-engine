/* behavior_pendulum.c — native port of C3DPendulum (FourCC 3PEN).
 *
 * Faithful to docs/decomp/C3DPendulum.md: the only authored parameter is
 * SwingHeight (float). Validated against shipped .gam data: 3 instances,
 * SwingHeight 15..25.
 *
 * Native realisation: the pendulum swings sinusoidally about its pivot with an
 * angular amplitude derived from SwingHeight, applied as a roll (rz). A player
 * within reach near the low point of the swing is shoved horizontally in the
 * swing direction — a sweeping hazard.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <math.h>
#include <stddef.h>

#define PEN_FREQ     1.4f     /* swing rate (rad/sec) */
#define PEN_AMP_RAD  0.0349f  /* radians of roll per SwingHeight unit (~2 deg) */
#define PEN_RADIUS   140.0f   /* horizontal reach of the bob (units) */
#define PEN_SHOVE    320.0f   /* peak shove velocity (units/sec) */

static void pendulum_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->user_float = 0.0f;     /* swing phase clock */
}

static void pendulum_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!behavior_animated_update_base(e, w, dt)) return;
    float swing_height = gam_prop_f(e, "SwingHeight", 20.0f);
    float amp = swing_height * PEN_AMP_RAD;            /* radians */

    e->user_float += dt;
    float s  = sinf(e->user_float * PEN_FREQ);          /* -1..1 position */
    float ds = cosf(e->user_float * PEN_FREQ);          /* swing velocity sign */
    e->rz = amp * s;                                    /* roll, radians */

    /* Shove the player when the bob sweeps past them near the low point. */
    if (g_player && fabsf(s) < 0.6f) {                  /* near bottom of arc */
        float dx = g_player->x - e->x;
        float dz = g_player->z - e->z;
        if (dx*dx + dz*dz < PEN_RADIUS*PEN_RADIUS) {
            float dir = (ds >= 0.0f) ? 1.0f : -1.0f;    /* swing direction along X */
            float k = (1.0f - fabsf(s)) * PEN_SHOVE * dt;
            g_player->vx += dir * k;
        }
    }
}

const EntityVTable vt_pendulum = {
    .on_spawn   = pendulum_on_spawn,
    .on_update  = pendulum_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
