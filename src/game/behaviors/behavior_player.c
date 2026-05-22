#include "behaviors.h"
#include "../../engine/input.h"
#include "../../engine/renderer.h"
#include "../gamestate.h"
#include "../player_anim.h"
#include <math.h>
#include <stddef.h>

Entity *g_player = NULL;

/* Tuned for Level1 units (JIM spawns near y=31, world extents ~10000). */
#define PLAYER_HALF_X     35.0f
#define PLAYER_HALF_Y     60.0f
#define PLAYER_HALF_Z     35.0f
#define PLAYER_MOVE_SPEED 600.0f   /* units / sec */
#define PLAYER_RUN_MULT   2.0f
#define PLAYER_JUMP_VEL   650.0f
#define PICKUP_ANIM_TIME  0.45f    /* seconds the pickup pose overrides */

static int s_last_items_collected = 0;

static void player_on_spawn(Entity *e, World *w) {
    (void)w;
    e->half_extents[0] = PLAYER_HALF_X;
    e->half_extents[1] = PLAYER_HALF_Y;
    e->half_extents[2] = PLAYER_HALF_Z;
    e->user_flag = PA_IDLE;
    e->user_float = 0.0f;       /* pickup-animation countdown (sec) */
    s_last_items_collected = 0;
    if (!g_player) g_player = e;
}

static void player_on_update(Entity *e, World *w, float dt) {
    (void)w;
    Camera *cam = renderer_camera();

    /* Movement is relative to the camera yaw so WASD is screen-space. */
    float cy = cosf(cam->yaw), sy = sinf(cam->yaw);
    float speed = PLAYER_MOVE_SPEED;
    if (input_is_down(SDL_SCANCODE_LSHIFT)) speed *= PLAYER_RUN_MULT;

    float mx = 0.0f, mz = 0.0f;
    if (input_is_down(SDL_SCANCODE_W)) { mx += sy;  mz += -cy; }
    if (input_is_down(SDL_SCANCODE_S)) { mx += -sy; mz += cy;  }
    if (input_is_down(SDL_SCANCODE_A)) { mx += -cy; mz += -sy; }
    if (input_is_down(SDL_SCANCODE_D)) { mx += cy;  mz += sy;  }

    float len = sqrtf(mx * mx + mz * mz);
    if (len > 0.0001f) {
        mx /= len; mz /= len;
        e->vx = mx * speed;
        e->vz = mz * speed;
        /* Face direction of travel. */
        e->ry = atan2f(mx, mz);
    } else {
        e->vx = 0.0f;
        e->vz = 0.0f;
    }

    /* Jump (only when grounded). Use just_pressed so holding doesn't auto-bunny-hop. */
    if (e->on_ground && input_just_pressed(SDL_SCANCODE_SPACE)) {
        e->vy = PLAYER_JUMP_VEL;
        e->on_ground = 0;
    }

    /* Detect pickups: gamestate counter rising means an item triggered this tick. */
    int cur_items = gamestate_get()->items_collected;
    if (cur_items > s_last_items_collected) e->user_float = PICKUP_ANIM_TIME;
    s_last_items_collected = cur_items;
    if (e->user_float > 0.0f) e->user_float -= dt;

    /* Animation state machine. PICKUP wins while its countdown is active. */
    PlayerAnim anim;
    if (e->user_float > 0.0f) {
        anim = PA_PICKUP;
    } else if (!e->on_ground) {
        anim = (e->vy > 0.0f) ? PA_JUMP : PA_FALL;
    } else if (len > 0.0001f) {
        anim = PA_RUN;
    } else {
        anim = PA_IDLE;
    }
    e->user_flag = (int)anim;
}

const EntityVTable vt_player = {
    .on_spawn = player_on_spawn,
    .on_update = player_on_update,
    .on_trigger = NULL,
    .flags = ENTITY_FLAG_PHYSICS | ENTITY_FLAG_SOLID | ENTITY_FLAG_PLAYER,
};
