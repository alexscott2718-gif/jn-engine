/* behavior_fan.c — native port of C3DFan (FourCC 3FAN).
 *
 * Faithful to docs/decomp/C3DFan.md:
 *   - InitObject registers FanSpeed (float), FanRange (float), FanOn (int).
 *   - vfunc_01_010 each frame exposes a "fan output" = FanOn ? FanSpeed : 0,
 *     which drives the airflow within range.
 * All three properties are confirmed present in shipped .gam data (gam_schema
 * 3FAN; FanSpeed 800..2700, FanRange 1000..3500, FanOn 1).
 *
 * Native realisation: the fan.ASE mesh is a flat horizontal blade slab (large in
 * X/Z, thin in Y), i.e. a ceiling-fan blade, so it spins about the vertical Y
 * axis (yaw). We drive e->ry with the blade angle (the existing yaw render path
 * applies it) and keep the authored facing in home[0] for the airflow direction.
 * The player within FanRange is pushed along that facing.
 */
#include "behaviors.h"
#include <math.h>
#include <stddef.h>

#define FAN_DEG2RAD 0.017453292519943295f
/* FanSpeed (~800..2700) is the airflow magnitude, not a screen-friendly blade
   RPM; scale it to a clear, non-strobing visual spin (deg/s ~= 80..270). */
#define FAN_VIS_SCALE 0.10f

static void fan_on_spawn(Entity *e, World *w) {
    (void)w;
    e->home[0]   = e->ry;                       /* authored facing -> airflow dir */
    e->user_float = 0.0f;                       /* accumulated blade angle (rad) */
    e->user_flag  = gam_prop_i(e, "FanOn", 1);  /* runtime on-state; a C3DSwitch
                                                   targeting this fan toggles it */
}

static void fan_on_update(Entity *e, World *w, float dt) {
    (void)w;
    float fan_speed = gam_prop_f(e, "FanSpeed", 0.0f);
    float fan_range = gam_prop_f(e, "FanRange", 0.0f);
    if (!e->user_flag || fan_speed <= 0.0f) return;

    /* Spin the blade about vertical Y (yaw). Radians; the yaw render path uses
       e->ry directly. */
    e->user_float = fmodf(e->user_float + fan_speed * FAN_VIS_SCALE * FAN_DEG2RAD * dt,
                          6.2831853f);
    e->ry = e->user_float;

    /* Airflow: push the player along the authored facing (home[0]) with a linear
       falloff to FanRange. */
    if (fan_range > 0.0f && g_player) {
        float dx = g_player->x - e->x;
        float dz = g_player->z - e->z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist < fan_range) {
            float yaw = e->home[0] * FAN_DEG2RAD;
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
