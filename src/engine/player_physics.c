#include "player_physics.h"

/* Initialised to the measured Level1.gam values so the player behaves even
   before (or without) a successful .gam parse. */
static PlayerPhysics g_pp = {
    .max_speed = 1400.0f,
    .accel_rate = 400.0f,
    .decel_rate = 1.0f,
    .max_height = 1500.0f,
    .up_rate = 650.0f,
    .down_rate = -650.0f,
    .max_vert_velocity = 650.0f,
    .accel_lean = 20.0f,
    .decel_lean = -20.0f,
    .loaded = 0,
};

void player_physics_reset_defaults(void) {
    PlayerPhysics d = {
        1400.0f, 400.0f, 1.0f, 1500.0f, 650.0f, -650.0f, 650.0f, 20.0f, -20.0f, 0
    };
    g_pp = d;
}

PlayerPhysics *player_physics_mutable(void) { return &g_pp; }
const PlayerPhysics *player_physics_get(void) { return &g_pp; }
