#include "fixture.h"
#include "behaviors/behaviors.h"
#include "behaviors/behavior_projectile.h"
#include "../engine/glad.h"
#include "../engine/ground.h"
#include <stdio.h>
#include <string.h>

static unsigned int fixture_texture;

static void fixture_player_update(Entity *e, World *world, float dt) {
    (void)world;
    e->anim_time += dt;
}

static const EntityVTable fixture_player_vtable = {
    .on_spawn = NULL,
    .on_update = fixture_player_update,
    .on_trigger = NULL,
    .flags = ENTITY_FLAG_PHYSICS | ENTITY_FLAG_SOLID | ENTITY_FLAG_PLAYER,
};

static Entity *fixture_add(World *world, const char *type, const char *tag,
                           float x, float y, float z) {
    Entity *e = world_add(world);
    if (!e) return NULL;
    snprintf(e->type, sizeof(e->type), "%s", type);
    snprintf(e->tag, sizeof(e->tag), "%s", tag);
    e->x = x;
    e->y = y;
    e->z = z;
    return e;
}

int fixture_level_build(World *world) {
    Entity *player = fixture_add(world, "3JIM", "fixture_player", 0.0f, 60.0f, 0.0f);
    Entity *left = fixture_add(world, "3PAT", "fixture_patrol_left", -320.0f, 40.0f, 300.0f);
    Entity *right = fixture_add(world, "3PAT", "fixture_patrol_right", 320.0f, 40.0f, 300.0f);
    Entity *ai_left = fixture_add(world, "3CAR", "fixture_ai_left", -320.0f, 40.0f, 300.0f);
    Entity *ai_right = fixture_add(world, "3CAR", "fixture_ai_right", 320.0f, 40.0f, 300.0f);
    Entity *trigger = fixture_add(world, "TRIG", "fixture_trigger", 100.0f, 60.0f, 0.0f);
    if (!player || !left || !right || !ai_left || !ai_right || !trigger)
        return -1;

    snprintf(left->next_patrol, sizeof(left->next_patrol), "fixture_patrol_right");
    snprintf(right->next_patrol, sizeof(right->next_patrol), "fixture_patrol_left");
    snprintf(ai_left->patrol_point, sizeof(ai_left->patrol_point), "fixture_patrol_right");
    snprintf(ai_right->patrol_point, sizeof(ai_right->patrol_point), "fixture_patrol_left");

    player->vt = &fixture_player_vtable;
    player->runtime_flags = fixture_player_vtable.flags;
    player->half_extents[0] = 35.0f;
    player->half_extents[1] = 60.0f;
    player->half_extents[2] = 35.0f;
    g_player = player;

    left->vt = &vt_patrolpoint;
    right->vt = &vt_patrolpoint;
    ai_left->vt = &vt_walker;
    ai_right->vt = &vt_walker;
    trigger->vt = &vt_trig;
    Entity *bound[] = { left, right, ai_left, ai_right, trigger };
    for (size_t i = 0; i < sizeof(bound) / sizeof(bound[0]); i++) {
        bound[i]->runtime_flags = bound[i]->vt->flags;
        if (bound[i]->vt->on_spawn)
            bound[i]->vt->on_spawn(bound[i], world);
        if (bound[i]->half_extents[0] == 0.0f) {
            bound[i]->half_extents[0] = 40.0f;
            bound[i]->half_extents[1] = 40.0f;
            bound[i]->half_extents[2] = 40.0f;
        }
    }
    world->ground_y = 0.0f;
    printf("[fixture0] built player, floor, two patrol AI actors, and trigger\n");
    return world->count;
}

int fixture_level_start_runtime(World *world) {
    Entity *projectile = projectile_spawn(world,
        -420.0f, 90.0f, 180.0f, 1.0f, 0.0f, 0.0f,
        PROJ_TEAM_PLAYER, 180.0f, 1, 4.0f);
    if (!projectile) return 0;
    snprintf(projectile->tag, sizeof(projectile->tag), "fixture_projectile");
    return 1;
}

void fixture_level_configure_floor(World *world) {
    static const unsigned char checker[4 * 4 * 4] = {
        36, 88, 52,255,  65,132, 76,255,  36, 88, 52,255,  65,132, 76,255,
        65,132, 76,255,  36, 88, 52,255,  65,132, 76,255,  36, 88, 52,255,
        36, 88, 52,255,  65,132, 76,255,  36, 88, 52,255,  65,132, 76,255,
        65,132, 76,255,  36, 88, 52,255,  65,132, 76,255,  36, 88, 52,255,
    };
    glGenTextures(1, &fixture_texture);
    glBindTexture(GL_TEXTURE_2D, fixture_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, checker);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    world->safety_floor_enabled = 1;
    world->safety_floor_y = 0.0f;
    world->safety_floor_cx = 0.0f;
    world->safety_floor_cz = 0.0f;
    world->safety_floor_half_x = 900.0f;
    world->safety_floor_half_z = 900.0f;
    ground_init(fixture_texture, 900.0f, 900.0f, 0.0f, 0.0f, 18.0f, 0.0f);
}

void fixture_level_destroy(void) {
    if (fixture_texture) glDeleteTextures(1, &fixture_texture);
    fixture_texture = 0;
}
