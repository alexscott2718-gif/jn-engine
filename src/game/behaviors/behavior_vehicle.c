/* behavior_vehicle.c — Wave N4 vehicles.
 *
 * Two flavors, both reusing N1 movement/AI bases:
 *
 *  - vt_rocket (3ROC, C3DRocketShip — a C3DFlyingObject leaf): the player walks
 *    into it and presses **E** to BOARD, flies it with the normal move keys
 *    (forward/turn + SPACE up / CTRL|Q down), and presses **E** to dismount.
 *    While ridden the rocket flies via the N1 `behavior_flying_update_base`
 *    (authored 3ROC flight params), and the player entity is snapped onto the
 *    rocket so the follow camera tracks the ride. The player's runtime flags are
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

static Entity      *s_ride = NULL;          /* vehicle the player rides, or NULL */
static unsigned int s_saved_player_flags = 0;

int     behavior_vehicle_riding(void)  { return s_ride != NULL; }
Entity *behavior_vehicle_current(void) { return s_ride; }

static void rocket_mount(Entity *e) {
    if (s_ride || !e) return;
    s_ride = e;
    if (g_player) {
        /* Make the rider inert: no gravity, collision, or trigger firing while
           the rocket owns its position (restored on dismount). */
        s_saved_player_flags = g_player->runtime_flags;
        g_player->runtime_flags = 0;
        g_player->visible = 0;
    }
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

/* Board on player contact + E (only when not already riding something). */
static void rocket_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (!s_ride && input_just_pressed(SDL_SCANCODE_E))
        rocket_mount(e);
}

static void rocket_on_update(Entity *e, World *w, float dt) {
    if (s_ride != e) {
        behavior_animated_update_base(e, w, dt);  /* idle: visible, not moving */
        return;
    }
    if (input_just_pressed(SDL_SCANCODE_E)) { rocket_dismount(e); return; }

    MovementFlyingInput in = {0};
    if (input_is_down(SDL_SCANCODE_UP)   || input_is_down(SDL_SCANCODE_W)) in.forward = 1.0f;
    if (input_is_down(SDL_SCANCODE_DOWN) || input_is_down(SDL_SCANCODE_S)) in.brake   = 1.0f;
    if (input_is_down(SDL_SCANCODE_LEFT) || input_is_down(SDL_SCANCODE_A)) in.turn   -= 1.0f;
    if (input_is_down(SDL_SCANCODE_RIGHT)|| input_is_down(SDL_SCANCODE_D)) in.turn   += 1.0f;
    if (input_is_down(SDL_SCANCODE_SPACE)) in.up = 1.0f;
    if (input_is_down(SDL_SCANCODE_LCTRL)|| input_is_down(SDL_SCANCODE_RCTRL) ||
        input_is_down(SDL_SCANCODE_Q)) in.down = 1.0f;
    /* Touch / headless: virtual stick drives turn+forward, virtual fly drives up. */
    float vjx, vjy; input_get_virtual_move(&vjx, &vjy);
    in.turn += vjx;
    if (vjy > 0.0f) in.forward = vjy; else if (vjy < 0.0f) in.brake = -vjy;
    float vf = input_get_virtual_fly();
    if (vf > 0.0f) in.up = vf; else if (vf < 0.0f) in.down = -vf;

    behavior_flying_update_base(e, w, &in, dt);
    /* The flying base sets vx/vy/vz; the rocket isn't a PHYSICS entity (no
       double gravity), so integrate its own position here. */
    e->x += e->vx * dt;
    e->y += e->vy * dt;
    e->z += e->vz * dt;

    /* Slave the (inert, hidden) player to the rocket so the camera follows. */
    if (g_player) {
        g_player->x = e->x; g_player->y = e->y; g_player->z = e->z;
        g_player->ry = e->ry;
        g_player->vx = g_player->vy = g_player->vz = 0.0f;
    }
}

const EntityVTable vt_rocket = {
    .on_spawn   = rocket_on_spawn,
    .on_update  = rocket_on_update,
    .on_trigger = rocket_on_trigger,
    .flags      = ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER,
};
