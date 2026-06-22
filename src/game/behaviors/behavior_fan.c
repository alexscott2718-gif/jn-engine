/* behavior_fan.c — native port of C3DFan (FourCC 3FAN).
 *
 * Faithful to docs/decomp/C3DFan.md:
 *   - InitObject registers FanSpeed (float), FanRange (float), FanOn (int).
 *   - vfunc_01_010 each frame exposes a "fan output" = FanOn ? FanSpeed : 0,
 *     which drives the airflow within range.
 * All three properties are confirmed present in shipped .gam data (gam_schema
 * 3FAN; FanSpeed 800..2700, FanRange 1000..3500, FanOn 1).
 *
 * Native realisation: fan.ASE is a flat disc (large in X/Z, thin in Y). ase_load
 * remaps ASE Z -> GL Y, so the loaded disc stands vertical in GL (large in X/Y,
 * thin in Z) facing the Z axis. A pinwheel rotates in its own plane, i.e. about
 * its normal = GL Z, so we drive e->rz (roll) with the blade angle and route
 * 3FAN through the euler render path. (Driving e->ry/yaw instead spins it edge-on
 * like a coin on its side.) The authored facing stays in home[0] for airflow.
 * The player within FanRange is pushed along that facing.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include <math.h>
#include <stddef.h>

#define FAN_DEG2RAD 0.017453292519943295f
/* FanSpeed (~800..2700) is the airflow magnitude, not a screen-friendly blade
   RPM; scale it to a clear, non-strobing visual spin (deg/s ~= 80..270). */
#define FAN_VIS_SCALE 0.10f

static void fan_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->home[0]   = e->ry;                       /* authored facing -> airflow dir */
    e->user_float = 0.0f;                       /* accumulated blade angle (rad) */
    e->user_flag  = gam_prop_i(e, "FanOn", 1);  /* runtime on-state; a C3DSwitch
                                                   targeting this fan toggles it */
}

static void fan_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!behavior_animated_update_base(e, w, dt)) return;
    float fan_speed = gam_prop_f(e, "FanSpeed", 0.0f);
    float fan_range = gam_prop_f(e, "FanRange", 0.0f);
    if (!e->user_flag || fan_speed <= 0.0f) return;

    /* Spin the blade about its normal (GL Z, roll) so it turns like a pinwheel
       in its own plane. Radians; the euler render path applies e->rz as roll. */
    e->user_float = fmodf(e->user_float + fan_speed * FAN_VIS_SCALE * FAN_DEG2RAD * dt,
                          6.2831853f);
    e->rz = e->user_float;

    /* Airflow: push the player along the authored facing (home[0]) with a linear
       falloff to FanRange. */
    if (fan_range > 0.0f && g_player) {
        float dx = g_player->x - e->x;
        float dz = g_player->z - e->z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist < fan_range) {
            /* home[0] is radians (gam_loader converts authored degrees). */
            float yaw = e->home[0];
            float fx = -sinf(yaw), fz = -cosf(yaw);     /* forward dir */
            float strength = (fan_range - dist) / fan_range;     /* 1 at fan, 0 at edge */
            float gust = (fan_speed * 0.02f) * strength * dt;
            g_player->vx += fx * gust;
            g_player->vz += fz * gust;
        }
    }
}

const EntityVTable vt_fan = {
    .on_spawn = fan_on_spawn,
    .on_update = fan_on_update,
    .on_trigger = NULL,
    .flags = 0,
};
