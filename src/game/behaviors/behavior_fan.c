/* behavior_fan.c — native port of C3DFan (FourCC 3FAN).
 *
 * Faithful to docs/decomp/C3DFan.md:
 *   - InitObject registers FanSpeed (float), FanRange (float), FanOn (int).
 *   - vfunc_01_010 each frame exposes a "fan output" = FanOn ? FanSpeed : 0,
 *     which drives the airflow within range.
 * Native realisation: when FanOn, the blades spin at FanSpeed (deg/s) about the
 * fan's facing axis, and the player is pushed along the fan's forward direction
 * with a linear falloff out to FanRange. All three properties are confirmed
 * present in shipped .gam data (gam_schema 3FAN; FanSpeed 800..2700,
 * FanRange 1000..3500, FanOn 1).
 */
#include "behaviors.h"
#include <math.h>
#include <stddef.h>

#define FAN_DEG2RAD 0.017453292519943295f

static void fan_on_spawn(Entity *e, World *w) {
    (void)w;
    e->user_float = 0.0f;   /* accumulated blade angle (deg) */
}

static void fan_on_update(Entity *e, World *w, float dt) {
    (void)w;
    int   fan_on    = gam_prop_i(e, "FanOn", 1);
    float fan_speed = gam_prop_f(e, "FanSpeed", 0.0f);
    float fan_range = gam_prop_f(e, "FanRange", 0.0f);
    if (!fan_on || fan_speed <= 0.0f) return;

    /* Spin the blades. The fan's facing is its authored RotationY; we advance a
       roll the renderer applies to the mesh. */
    e->user_float = fmodf(e->user_float + fan_speed * dt, 360.0f);
    e->rz = e->user_float;

    /* Airflow: push the player along the fan's forward (-Z rotated by ry) with a
       linear falloff to FanRange. */
    if (fan_range > 0.0f && g_player) {
        float dx = g_player->x - e->x;
        float dz = g_player->z - e->z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist < fan_range) {
            float yaw = e->ry * FAN_DEG2RAD;
            float fx = -sinf(yaw), fz = -cosf(yaw);     /* forward dir */
            float strength = (fan_range - dist) / fan_range;     /* 1 at fan, 0 at edge */
            /* gentle wind impulse, scaled from FanSpeed; integrates into velocity */
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
