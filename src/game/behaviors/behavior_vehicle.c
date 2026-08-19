/* behavior_vehicle.c — Wave N4 vehicles.
 *
 * Two flavors, both reusing N1 movement/AI bases:
 *
 *  - vt_rocket (3ROC, C3DRocketShip — a C3DFlyingObject leaf): the player walks
 *    into it and presses **E** to BOARD, flies it with auto-forward cruise,
 *    SHIFT boost, Up/Down pitch, Left/Right turn, CTRL|Q brake, and presses
 *    **E** to dismount.
 *    While ridden the rocket flies from the authored 3ROC flight params, and the
 *    player entity is snapped onto the rocket so the follow camera tracks the
 *    ride. The player's runtime flags are
 *    cleared while riding so `physics_step` doesn't fight the snap with gravity,
 *    and restored on dismount. Placed in every level — the player-driven vehicle
 *    validated on real levels.
 *
 *  - vt_ai_vehicle (3SUV C3DAISuv, 3SBU C3DBus, 3SAI C3DSailBoat — all C3DAI
 *    consumers): self-driving traffic that patrols its authored PatrolPoint
 *    chain via the N1 `behavior_ai` patrol primitive, faithful to the C3DAI
 *    patrol these classes inherit. (The SUV light-cone child, AICar horn-contact
 *    response, and SailBoat sine-bob are deferred — they need the C3DLightCone /
 *    effect / Goddard subsystems not yet ported.)
 */
#include "behavior_vehicle.h"
#include "behavior_ai.h"
#include "behavior_flying.h"
#include "../../engine/input.h"
#include <stddef.h>
#include <math.h>
#include <stdio.h>

/* ---- AI vehicles (self-driving) -------------------------------------- */
#define VEHICLE_AI_SPEED    400.0f   /* C3DAICar/SUV tuning sits ~400-500 */
#define VEHICLE_AI_ARRIVE   120.0f   /* vehicles are bigger than a walker */

static void ai_vehicle_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_ai_spawn_patrol(e);
}

static void ai_vehicle_on_update(Entity *e, World *w, float dt) {
    const BehaviorAIPatrolConfig cfg = {
        .speed = VEHICLE_AI_SPEED,
        .arrive_radius = VEHICLE_AI_ARRIVE,
        .loop_to_start = 1,
    };
    (void)behavior_ai_update_patrol(e, w, &cfg, dt);
}

const EntityVTable vt_ai_vehicle = {
    .on_spawn   = ai_vehicle_on_spawn,
    .on_update  = ai_vehicle_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};

/* ---- Player-rideable rocket ------------------------------------------ */
#define ROCKET_HALF   120.0f
#define ROCKET_SEAT_UP  40.0f  /* Jimmy rides this far above the hull center */
#define ROCKET_TURN_RATE  2.0f /* yaw rad/sec */
#define ROCKET_PITCH_RATE 1.4f /* pitch rad/sec */
#define ROCKET_PITCH_MAX  1.10f/* ~63 deg nose up/down clamp */
#define ROCKET_BRAKE_DECEL 1800.0f /* speed bled per sec while braking */

static Entity      *s_ride = NULL;          /* vehicle the player rides, or NULL */
static unsigned int s_saved_player_flags = 0;

int     behavior_vehicle_riding(void)  { return s_ride != NULL; }
Entity *behavior_vehicle_current(void) { return s_ride; }

/* Forget the ridden vehicle across a world teardown; the saved player flags
   describe an entity that no longer exists. */
void behavior_vehicle_reset(void) { s_ride = NULL; s_saved_player_flags = 0; }

static void rocket_mount(Entity *e) {
    if (s_ride || !e) return;
    s_ride = e;
    if (g_player) {
        /* Make the rider inert: no gravity, collision, or trigger firing while
           the rocket owns its position (restored on dismount). */
        s_saved_player_flags = g_player->runtime_flags;
        g_player->runtime_flags = 0;
        /* Jimmy stays VISIBLE and is parked at the rocket seat each frame
           (rocket_on_update) so he's seen riding it, as in the game. */
        g_player->visible = 1;
    }
    /* Start level + stationary; it cruises up to speed once boarded. */
    e->rx = 0.0f;
    e->move_speed = 0.0f;
    printf("[VEHICLE] boarded rocket '%s'\n", e->tag);
}

void behavior_vehicle_force_board(Entity *vehicle) { rocket_mount(vehicle); }

static void rocket_dismount(Entity *e) {
    if (s_ride != e) return;
    s_ride = NULL;
    if (g_player) {
        g_player->runtime_flags = s_saved_player_flags;
        /* Step off behind the rocket and let physics resettle the player. */
        float sx = sinf(e->ry), cx = cosf(e->ry);
        g_player->x = e->x - sx * (ROCKET_HALF + 60.0f);
        g_player->z = e->z - cx * (ROCKET_HALF + 60.0f);
        g_player->y = e->y;
        g_player->vx = g_player->vy = g_player->vz = 0.0f;
        g_player->on_ground = 0;
        g_player->visible = 1;
    }
    printf("[VEHICLE] left rocket '%s'\n", e->tag);
}

static void rocket_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_flying_spawn_base(e);
    if (e->half_extents[0] == 0.0f && e->half_extents[1] == 0.0f &&
        e->half_extents[2] == 0.0f)
        e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = ROCKET_HALF;
}

/* Boarding is proximity-based, not overlap-based: the rocket is SOLID, so the
   player can never enter its AABB to fire a trigger — they stop at the surface.
   Boarding in on_update (which runs before physics) when the player is within a
   generous horizontal radius + E also avoids any dismount->reboard race. */
#define ROCKET_BOARD_RADIUS  (ROCKET_HALF + 220.0f)

static int rocket_player_in_range(const Entity *e) {
    if (!g_player) return 0;
    float dx = g_player->x - e->x, dz = g_player->z - e->z;
    return dx * dx + dz * dz <= ROCKET_BOARD_RADIUS * ROCKET_BOARD_RADIUS;
}

static void rocket_on_update(Entity *e, World *w, float dt) {
    /* Board/leave trigger: E key (edge) OR the web Enter-Vehicle button. The
       button flag is peeked (not consumed) here so the one rocket that actually
       (dis)mounts clears it; the main loop force-clears any leftover at end of
       frame. */
    int board_trig = input_just_pressed(SDL_SCANCODE_E) || input_virtual_board_peek();
    if (s_ride != e) {
        behavior_animated_update_base(e, w, dt);  /* idle: visible, not moving */
        /* Board when the player is near and presses the board trigger. */
        if (!s_ride && board_trig && rocket_player_in_range(e)) {
            rocket_mount(e);
            input_virtual_board_consume();
        }
        return;
    }
    if (board_trig) { rocket_dismount(e); input_virtual_board_consume(); return; }

    /* Flight feel per the original C3DRocketShip (player recollection + the
       C3DFlyingObject .gam params, docs/decomp/C3DRocketShip.md): the rocket
       CRUISES forward on its own; SHIFT boosts to full MaxSpeed; the brake cuts
       the engine and bleeds speed to a stop. The nose uses direct controls:
       Up/W = nose up, Down/S = nose down, Left/A = turn left, Right/D = turn
       right; velocity follows the nose. (Brake also cuts the engine sound +
       stops the C3DFireStrato exhaust -- both deferred, see docs/native_port_plan.md
       section 6.) */
    MovementFlyingParams fp; movement_base_flying_params_from_gam(e, &fp);
    int boost = input_is_down(SDL_SCANCODE_LSHIFT) || input_is_down(SDL_SCANCODE_RSHIFT);
    int brake = input_is_down(SDL_SCANCODE_LCTRL)  || input_is_down(SDL_SCANCODE_RCTRL) ||
                input_is_down(SDL_SCANCODE_Q);
    float turn = 0.0f, pitch_in = 0.0f;
    if (input_is_down(SDL_SCANCODE_LEFT)  || input_is_down(SDL_SCANCODE_A)) turn     -= 1.0f;
    if (input_is_down(SDL_SCANCODE_RIGHT) || input_is_down(SDL_SCANCODE_D)) turn     += 1.0f;
    if (input_is_down(SDL_SCANCODE_UP)    || input_is_down(SDL_SCANCODE_W)) pitch_in -= 1.0f; /* nose up */
    if (input_is_down(SDL_SCANCODE_DOWN)  || input_is_down(SDL_SCANCODE_S)) pitch_in += 1.0f; /* nose down */
    /* Touch / headless: stick turns + pitches (stick up = nose up), fly axis pitches,
       full stick-back brakes. Boost is keyboard-only for now. */
    float vjx, vjy; input_get_virtual_move(&vjx, &vjy);
    turn += vjx;
    pitch_in -= vjy;                       /* stick up = nose up, back = nose down */
    pitch_in -= input_get_virtual_fly();   /* fly-up = nose up */
    if (vjy < -0.85f) brake = 1;

    e->ry -= turn * ROCKET_TURN_RATE * dt;
    e->rx += pitch_in * ROCKET_PITCH_RATE * dt;
    if (e->rx >  ROCKET_PITCH_MAX) e->rx =  ROCKET_PITCH_MAX;
    if (e->rx < -ROCKET_PITCH_MAX) e->rx = -ROCKET_PITCH_MAX;

    /* Cruise unless braking; SHIFT lifts the target to full MaxSpeed. */
    float target = brake ? 0.0f : (boost ? fp.max_speed : fp.max_speed * 0.5f);
    if (brake) {
        e->move_speed -= ROCKET_BRAKE_DECEL * dt;
        if (e->move_speed < 0.0f) e->move_speed = 0.0f;
    } else if (e->move_speed < target) {
        e->move_speed += fp.accel_rate * dt;
        if (e->move_speed > target) e->move_speed = target;
    } else if (e->move_speed > target) {
        e->move_speed -= fp.accel_rate * dt;
        if (e->move_speed < target) e->move_speed = target;
    }

    float cx = cosf(e->rx), sx = sinf(e->rx);
    float sy = sinf(e->ry), cy = cosf(e->ry);
    e->vx = sy * cx * e->move_speed;
    e->vy = -sx * e->move_speed;          /* nose up (rx<0) -> climb */
    e->vz = cy * cx * e->move_speed;
    e->x += e->vx * dt;
    e->y += e->vy * dt;
    e->z += e->vz * dt;

    /* Slave the (inert, VISIBLE) player to the rocket seat so Jimmy rides it and
       the follow camera tracks the ride. Kept visible (see rocket_mount). */
    if (g_player) {
        g_player->x = e->x;
        g_player->y = e->y + ROCKET_SEAT_UP;
        g_player->z = e->z;
        g_player->ry = e->ry;
        g_player->vx = g_player->vy = g_player->vz = 0.0f;
        g_player->visible = 1;
    }
}

const EntityVTable vt_rocket = {
    .on_spawn   = rocket_on_spawn,
    .on_update  = rocket_on_update,
    .on_trigger = NULL,   /* boarding is proximity-based in on_update */
    .flags      = ENTITY_FLAG_SOLID,
};
