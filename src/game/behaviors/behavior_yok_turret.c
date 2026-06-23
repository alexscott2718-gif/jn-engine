/* behavior_yok_turret.c — C3DYokTurret (3TUR).
 *
 * The original turret is a C3DAI descendant that tracks JIM1, increments a
 * fire-cycle timer, and launches one of three pooled C3DMissile children every
 * 4.5 seconds. Native keeps the same placed-enemy contract but routes launches
 * through the shared PROJ path so enemy bolts and player baseballs share hit
 * handling.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "behavior_projectile.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TURRET_HP_DEFAULT      100.0f
#define TURRET_FIRE_PERIOD       4.5f
#define TURRET_VISIBLE_DEF   15000.0f
#define TURRET_FOV_DEF         359.0f
#define TURRET_PROJECTILE_SPEED 1200.0f
#define TURRET_PROJECTILE_TTL     5.0f
#define TURRET_DAMAGE            10

static float turret_wrap_angle(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

static int turret_player_in_cone(const Entity *e, const Entity *p, float dist) {
    float visible = gam_prop_f(e, "VisibleRange", TURRET_VISIBLE_DEF);
    if (visible <= 0.0f) visible = TURRET_VISIBLE_DEF;
    if (dist > visible) return 0;

    float fov = gam_prop_f(e, "FOV", TURRET_FOV_DEF);
    if (fov <= 0.0f || fov >= 360.0f) return 1;

    float dx = p->x - e->x;
    float dz = p->z - e->z;
    float want = atan2f(-dx, -dz);
    float half = fov * 0.5f * (float)M_PI / 180.0f;
    return fabsf(turret_wrap_angle(want - e->ry)) <= half;
}

static void yok_turret_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->hp = TURRET_HP_DEFAULT;
    e->user_flag = 0;     /* native missile-pool cursor, modulo 3 */
    e->user_float = 0.0f; /* fire_cycle_timer */
    e->home[0] = e->x;
    e->home[1] = e->y;
    e->home[2] = e->z;
    e->half_extents[0] = 90.0f;
    e->half_extents[1] = 120.0f;
    e->half_extents[2] = 90.0f;
}

static void yok_turret_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;
    Entity *p = g_player;
    if (!p || !p->alive) return;

    float dx = p->x - e->x;
    float dz = p->z - e->z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (!turret_player_in_cone(e, p, dist)) return;

    e->ry = atan2f(-dx, -dz);
    e->user_float += dt;
    if (e->user_float < TURRET_FIRE_PERIOD) return;
    e->user_float = 0.0f;

    float ox = e->x;
    float oy = e->y + 70.0f;
    float oz = e->z;
    float tx = p->x;
    float ty = p->y + 35.0f;
    float tz = p->z;
    Entity *bolt = projectile_spawn(w, ox, oy, oz,
                                    tx - ox, ty - oy, tz - oz,
                                    PROJ_TEAM_ENEMY,
                                    TURRET_PROJECTILE_SPEED,
                                    TURRET_DAMAGE,
                                    TURRET_PROJECTILE_TTL);
    if (bolt) {
        e->user_flag = (e->user_flag + 1) % 3;
        fprintf(stderr, "[TURRET] %s '%s' fired enemy projectile slot=%d\n",
                e->type, e->tag, e->user_flag);
    }
}

const EntityVTable vt_yok_turret = {
    .on_spawn   = yok_turret_on_spawn,
    .on_update  = yok_turret_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
