#include "behaviors.h"
#include "behavior_projectile.h"
#include "behavior_vehicle.h"
#include "../../engine/input.h"
#include "../../engine/movement_base.h"
#include "../gamestate.h"
#include "../player_anim.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

Entity *g_player = NULL;

/* Tuned for Level1 units (JIM spawns near y=31, world extents ~10000).
   NOTE: the data-driven C3DPlayer physics (accel/decel ramp, .gam speed/jump,
   body lean) is DEFERRED — it produced ice-skating / wrong turn+speed. This is
   the approved simple tank-turn movement (instant velocity). The .gam parsing
   (player_physics) stays in place, dormant, for when the physics track resumes. */
#define PLAYER_HALF_X     35.0f
#define PLAYER_HALF_Y     60.0f
#define PLAYER_HALF_Z     35.0f
#define PLAYER_MOVE_SPEED 600.0f   /* units / sec */
#define PLAYER_RUN_MULT   2.0f
#define PLAYER_TURBO_MULT 4.0f     /* sticky fast-explore boost (T / web button) */
#define PLAYER_JUMP_VEL   650.0f
#define PLAYER_NOCLIP_VERTICAL_SPEED 600.0f
#define PLAYER_TURN_RATE  2.8f     /* radians / sec (~160 deg/s) */
#define PICKUP_ANIM_TIME  0.45f    /* seconds the pickup pose overrides */

static int s_last_items_collected = 0;

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

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

/* Phase 4 contextual state: is the player standing near a playground swing
   (3SWN)? Level 1's only contextual prop with a player animation. */
#define SWING_RADIUS 400.0f
static int player_near_swing(World *w, const Entity *p) {
    if (!w) return 0;
    const float r2 = SWING_RADIUS * SWING_RADIUS;
    for (Entity *s = w->head; s; s = s->next) {
        if (!s->alive || s == p) continue;
        if (strncmp(s->type, "3SWN", 4) != 0) continue;
        float dx = s->x - p->x, dz = s->z - p->z;
        if (dx * dx + dz * dz <= r2) return 1;
    }
    return 0;
}

static void player_on_update(Entity *e, World *w, float dt) {
    /* Riding a vehicle: it drives and positions us at the seat (see
       behavior_vehicle.c). Suppress on-foot control; hold the idle pose so the
       visible, seated Jimmy reads as riding rather than running in place. */
    if (behavior_vehicle_riding()) {
        e->user_flag = (int)PA_IDLE;
        player_anim_advance(PA_IDLE, dt);
        return;
    }
    if (input_just_pressed(SDL_SCANCODE_N))
        input_toggle_noclip();
    if (input_just_pressed(SDL_SCANCODE_T))
        input_toggle_turbo();
    int noclip = input_noclip_enabled();
    /* Turbo (sticky) stacks with the held SHIFT run. */
    float speed = PLAYER_MOVE_SPEED;
    if (input_is_down(SDL_SCANCODE_LSHIFT)) speed *= PLAYER_RUN_MULT;
    if (input_turbo_enabled()) speed *= PLAYER_TURBO_MULT;

    /* Tank-turn controls (original JNBG): Left/Right (arrows + A/D) ROTATE
       Jimmy's heading; Up/Down (arrows + W/S) drive forward/backpedal along
       that heading. The follow camera tracks the heading from behind, so input
       is body-relative, not screen-relative. The virtual stick maps x->turn,
       y->forward. */
    float turn = 0.0f;   /* +1 = turn right */
    float fwd  = 0.0f;   /* +1 = forward, -1 = backpedal */
    if (input_is_down(SDL_SCANCODE_LEFT)  || input_is_down(SDL_SCANCODE_A)) turn -= 1.0f;
    if (input_is_down(SDL_SCANCODE_RIGHT) || input_is_down(SDL_SCANCODE_D)) turn += 1.0f;
    if (input_is_down(SDL_SCANCODE_UP)    || input_is_down(SDL_SCANCODE_W)) fwd  += 1.0f;
    if (input_is_down(SDL_SCANCODE_DOWN)  || input_is_down(SDL_SCANCODE_S)) fwd  -= 1.0f;
    float vjx, vjy;
    input_get_virtual_move(&vjx, &vjy);
    turn += vjx; fwd += vjy;
    MovementTankInput drive =
        movement_base_tank_drive(e, turn, fwd, speed, PLAYER_TURN_RATE, dt);
    turn = drive.turn;
    fwd = drive.forward;

    if (noclip) {
        float up = 0.0f;
        if (input_is_down(SDL_SCANCODE_SPACE)) up += 1.0f;
        if (input_is_down(SDL_SCANCODE_LCTRL) || input_is_down(SDL_SCANCODE_RCTRL) ||
            input_is_down(SDL_SCANCODE_Q)) up -= 1.0f;
        up += input_get_virtual_fly();
        up = clampf(up, -1.0f, 1.0f);
        e->vy = up * PLAYER_NOCLIP_VERTICAL_SPEED *
            (input_is_down(SDL_SCANCODE_LSHIFT) ? PLAYER_RUN_MULT : 1.0f) *
            (input_turbo_enabled() ? PLAYER_TURBO_MULT : 1.0f);
        e->on_ground = 1;
    }

    /* Jump (only when grounded). Keyboard SPACE edge OR a touch jump tap.
       just_pressed / take_jump prevent holding from auto-bunny-hopping. */
    int jump_pressed = input_just_pressed(SDL_SCANCODE_SPACE) || input_virtual_take_jump();
    if (!noclip && e->on_ground && jump_pressed) {
        e->vy = PLAYER_JUMP_VEL;
        e->on_ground = 0;
    }

    /* Use the active tool (F key, or the web "Use Tool" button). The active
       tool is selected via the inventory cycle (web Tool button); dispatch is
       by tag. Today only the baseball has an action — it throws via the shared
       projectile path (defeats Yokians, pops balloons). The baseball is granted
       by a C3DBaseballPickup (3BPU), a 3PIC awarding picture #6 (level1c /
       level2a / Level2b), or sandbox mode. */
    int use_tool = input_just_pressed(SDL_SCANCODE_F) || input_virtual_take_use();
    if (!noclip && use_tool) {
        const char *tool = gamestate_active_tool_tag();
        if (tool && strcmp(tool, "baseball") == 0 && gamestate_has_tool("baseball")) {
            float fx = sinf(e->ry), fz = cosf(e->ry);
            /* Spawn clear of the player's own SOLID AABB (half ~35) so the
               projectile's wall raycast doesn't immediately stop on the thrower. */
            projectile_spawn(w,
                             e->x + fx * 80.0f, e->y + 30.0f, e->z + fz * 80.0f,
                             fx, 0.0f, fz,
                             PROJ_TEAM_PLAYER, 800.0f, 100, 2.5f);
        }
    }

    /* Detect pickups: gamestate counter rising means an item triggered this tick. */
    int cur_items = gamestate_get()->items_collected;
    if (cur_items > s_last_items_collected) e->user_float = PICKUP_ANIM_TIME;
    s_last_items_collected = cur_items;
    if (e->user_float > 0.0f) e->user_float -= dt;

    /* Animation state machine. PICKUP wins while its countdown is active.
       jimleft/jimright are now the turn-in-place lean clips. */
    PlayerAnim anim;
    if (e->user_float > 0.0f) {
        anim = PA_PICKUP;
    } else if (!noclip && !e->on_ground) {
        anim = (e->vy > 0.0f) ? PA_JUMP : PA_FALL;
    } else if (fabsf(turn) > fabsf(fwd) && fabsf(turn) > 0.0001f) {
        /* Turn-dominant input plays the sidestep/strafe clip (LEFT->jimleft). */
        anim = (turn < 0.0f) ? PA_LEFT : PA_RIGHT;
    } else if (fwd > 0.0001f) {
        anim = PA_RUN;
    } else if (fwd < -0.0001f) {
        anim = PA_BACKPEDAL;
    } else if (player_near_swing(w, e)) {
        anim = PA_SWING;   /* contextual: idling by a swing */
    } else {
        anim = PA_IDLE;
    }
    e->user_flag = (int)anim;
    player_anim_advance(anim, dt);
}

const EntityVTable vt_player = {
    .on_spawn = player_on_spawn,
    .on_update = player_on_update,
    .on_trigger = NULL,
    .flags = ENTITY_FLAG_PHYSICS | ENTITY_FLAG_SOLID | ENTITY_FLAG_PLAYER,
};
