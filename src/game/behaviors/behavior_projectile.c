/* behavior_projectile.c — shared projectile path (Wave N2).
 *
 * Projectiles are spawned mid-frame by enemies (bolts at the player) and by
 * the player (thrown baseball that defeats Yokians). Each is a runtime Entity
 * of FourCC "PROJ" with vt_projectile; it carries:
 *   vx/vy/vz   = velocity (units/sec)
 *   user_flag  = team (PROJ_TEAM_ENEMY / PROJ_TEAM_PLAYER)
 *   user_float = time-to-live remaining (sec)
 *   points     = damage dealt on hit
 * On each tick it integrates, raycasts through SOLID AABBs (world_query_segment)
 * to stop at the first wall, then overlap-tests its valid targets. Spent
 * projectiles set alive=0/visible=0 (the world list is freed at level teardown).
 */
#include "behavior_projectile.h"
#include "behavior_enemy.h"
#include "../gamestate.h"
#include "../../engine/physics.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#define PROJ_HALF 12.0f  /* small AABB so collisions read as a point-ish bolt */

Entity *projectile_spawn(World *w, float ox, float oy, float oz,
                         float dx, float dy, float dz,
                         int team, float speed, int damage, float ttl) {
    if (!w || speed <= 0.0f) return NULL;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return NULL;
    float inv = 1.0f / len;

    Entity *p = world_add(w);
    if (!p) return NULL;
    memcpy(p->type, "PROJ", 4);
    p->type[4] = '\0';
    p->x = ox; p->y = oy; p->z = oz;
    p->vx = dx * inv * speed;
    p->vy = dy * inv * speed;
    p->vz = dz * inv * speed;
    p->half_extents[0] = p->half_extents[1] = p->half_extents[2] = PROJ_HALF;
    p->user_flag = team;
    p->user_float = (ttl > 0.0f) ? ttl : 3.0f;
    p->points = damage;
    p->visible = 1;
    p->alive = 1;
    p->vt = &vt_projectile;
    p->runtime_flags = vt_projectile.flags;
    return p;
}

static void projectile_despawn(Entity *e) {
    e->alive = 0;
    e->visible = 0;
}

static void projectile_on_update(Entity *e, World *w, float dt) {
    if (!e || !e->alive) return;

    float nx = e->x + e->vx * dt;
    float ny = e->y + e->vy * dt;
    float nz = e->z + e->vz * dt;

    /* Stop at the first SOLID wall along this step. */
    float t = world_query_segment(w, e, e->x, e->y, e->z, nx, ny, nz);
    if (t < 1.0f) {
        e->x += (nx - e->x) * t;
        e->y += (ny - e->y) * t;
        e->z += (nz - e->z) * t;
        projectile_despawn(e);
        return;
    }
    e->x = nx; e->y = ny; e->z = nz;

    e->user_float -= dt;
    if (e->user_float <= 0.0f) {
        projectile_despawn(e);
        return;
    }

    if (e->user_flag == PROJ_TEAM_ENEMY) {
        if (g_player && g_player->alive && physics_aabb_overlap(e, g_player)) {
            gamestate_damage_player(e->points);
            projectile_despawn(e);
        }
        return;
    }

    /* PROJ_TEAM_PLAYER: defeat the first enemy it touches. */
    for (Entity *it = w->head; it; it = it->next) {
        if (it == e || !it->alive) continue;
        if (!physics_aabb_overlap(e, it)) continue;
        if (behavior_enemy_take_hit(it, e->points)) {
            projectile_despawn(e);
            return;
        }
    }
}

const EntityVTable vt_projectile = {
    .on_spawn   = NULL,
    .on_update  = projectile_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
