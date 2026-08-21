/* behavior_scooter.c — the scooter gadget (C3DJeep, FourCC 3JEE).
 *
 * The class is named Jeep and registers `3JEE`, but it is the scooter. Its
 * InitObject loads `omt\scooter.omt`, and its update switches an attached child
 * between the animations `SCOOT` and `SCOOTSTOP` (docs/decomp/C3DJeep.md). The
 * repo carries the matching assets: parsed `scooter.omt`, `jimscooter.ASE` and
 * `jimscooterstop.ASE` for the rider, `godscooter.ASE` for Goddard.
 *
 * Two things pin what the player actually sees. `scooter.omt` contains exactly
 * two textures, `scooterwheel2` and **`goddard128`**; and Goddard's own
 * animation table registers `HISCOOT -> godscooter.ASE` alongside
 * `HIROCK -> godrocket.ASE` and `HIFLY -> godfly.ASE`. Goddard *is* the
 * scooter — he transforms into it, which is why C3DJeep carries his texture and
 * why `jimscooter.ASE` contains only the node `01jimmy` with no vehicle mesh in
 * it. The Jeep is the physics body; Goddard is the bodywork; Jimmy is the pose
 * sitting on top.
 *
 * `3JEE` has zero rows in all 35 levels, because the original does not place
 * it. `JimmySetupOrReset` (00423610) spawns it in code at Jimmy field `0x970`
 * through the `3JEE` factory `FUN_004211a0` and immediately hides and disables
 * it — right beside the C3DGoddard it spawns at `0x95c`. This file mirrors
 * that: one hidden instance per level, revealed when the action menu selects
 * the scooter.
 *
 * Ported constants are the ones the decompiled bodies actually pin: the
 * constructor's `drive_state = 1` seed and the `0.3s` child-animation refresh
 * cadence from `UpdateJeep` (00421580). The speeds and turn rates are NOT
 * recovered — they come from vehicle-database entry 0 of `scooter.omt`, and our
 * parse of that file holds only its two textures. Those numbers are tuned, and
 * they are grouped here so they are easy to retune when the database is read.
 */
#include "behavior_vehicle.h"
#include "behavior_ai.h"
#include "../entities.h"
#include "../player_anim.h"
#include "../../engine/input.h"
#include <SDL.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Recovered. */
#define SCOOTER_DRIVE_STATE     1      /* ctor 004211a0 seeds inherited 0x590 */
#define SCOOTER_ANIM_REFRESH    0.3f   /* 0x5e0 resets at 0.3, UpdateJeep     */

/* Tuned, not recovered — see the header comment. */
#define SCOOTER_MAX_SPEED     900.0f
#define SCOOTER_ACCEL        1400.0f
#define SCOOTER_BRAKE        2200.0f
#define SCOOTER_TURN_RATE       2.6f   /* rad/s */
#define SCOOTER_HALF           60.0f
#define SCOOTER_SEAT_UP        34.0f
#define SCOOTER_MOVING_EPS     40.0f   /* below this it reads as stopped */

static Entity *s_scooter   = NULL;   /* the one hidden instance, Jimmy 0x970 */
static int     s_riding    = 0;
static float   s_anim_timer = 0.0f;
static int     s_anim_moving = -1;   /* last SCOOT/SCOOTSTOP choice, -1 = unset */
static unsigned int s_saved_player_flags = 0;

Entity *behavior_scooter_get(void)   { return s_scooter; }
int     behavior_scooter_riding(void) { return s_riding; }

void behavior_scooter_reset(void) {
    s_scooter = NULL;
    s_riding = 0;
    s_anim_timer = 0.0f;
    s_anim_moving = -1;
    s_saved_player_flags = 0;
}

/* HideJeepAndWheelParts (vtable 4 slot 93, 00421800) forces the Jeep and its
   six wheel/part children hidden and disabled. Native has no wheel children, so
   this is the same intent over one entity. */
static void scooter_hide(Entity *e) {
    if (!e) return;
    e->visible = 0;
    e->runtime_flags = 0;
}

/* Mirror of the JimmySetupOrReset spawn: one instance, hidden, per level. */
Entity *behavior_scooter_ensure(World *w) {
    if (s_scooter) return s_scooter;
    if (!w) return NULL;
    Entity *e = world_add(w);
    if (!e) return NULL;
    memcpy(e->type, "3JEE", 4);
    e->type[4] = '\0';
    snprintf(e->tag, sizeof(e->tag), "C3DJEEP");
    e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = SCOOTER_HALF;
    e->move_speed = 0.0f;
    e->user_flag = SCOOTER_DRIVE_STATE;
    scooter_hide(e);
    s_scooter = e;
    printf("[SCOOTER] spawned hidden (3JEE, drive_state=%d)\n",
           SCOOTER_DRIVE_STATE);
    return e;
}

static void scooter_mount(void) {
    Entity *e = s_scooter;
    if (!e || s_riding || !g_player) return;
    if (behavior_vehicle_riding()) return;   /* already on the rocket */

    /* Park it under Jimmy, facing where he faces, and reveal it. */
    e->x = g_player->x;
    e->y = g_player->y;
    e->z = g_player->z;
    e->ry = g_player->ry;
    e->move_speed = 0.0f;
    e->visible = 1;

    s_saved_player_flags = g_player->runtime_flags;
    g_player->runtime_flags = 0;   /* the scooter owns his position now */
    g_player->visible = 1;
    s_riding = 1;
    s_anim_timer = 0.0f;
    s_anim_moving = -1;

    /* Goddard becomes the bodywork. He carries the scooter's own texture
       (goddard128 in scooter.omt) and the HISCOOT clip. */
    Entity *g = behavior_goddard_get();
    if (g) {
        g->visible = 1;
        printf("[SCOOTER] Goddard -> HISCOOT (godscooter.ASE)\n");
    }
    printf("[SCOOTER] mounted\n");
}

static void scooter_dismount(void) {
    Entity *e = s_scooter;
    if (!e || !s_riding) return;
    s_riding = 0;
    if (g_player) {
        g_player->runtime_flags = s_saved_player_flags;
        /* Step off to the side, as the rocket does behind itself. */
        float sx = sinf(e->ry), cx = cosf(e->ry);
        g_player->x = e->x - cx * (SCOOTER_HALF + 50.0f);
        g_player->z = e->z + sx * (SCOOTER_HALF + 50.0f);
        g_player->y = e->y;
        g_player->vx = g_player->vy = g_player->vz = 0.0f;
        g_player->on_ground = 0;
        g_player->visible = 1;
    }
    scooter_hide(e);
    printf("[SCOOTER] dismounted\n");
}

/* What the action menu calls when the scooter is chosen. Toggles, so choosing
   it again puts it away. */
int behavior_scooter_activate(void) {
    if (!s_scooter) return 0;
    if (s_riding) { scooter_dismount(); return 0; }
    scooter_mount();
    return s_riding;
}

/* UpdateJeep (00421580) advances the attached child, copies the Jeep transform
   into it, and refreshes its animation string every 0.3s, choosing between
   SCOOT and SCOOTSTOP. The refresh really is on a timer rather than on a state
   edge, so a stop/start inside one interval does not restart the clip. */
static void scooter_sync_child(Entity *e, float dt) {
    int moving = e->move_speed > SCOOTER_MOVING_EPS;

    s_anim_timer += dt;
    if (s_anim_timer >= SCOOTER_ANIM_REFRESH || s_anim_moving < 0) {
        s_anim_timer = 0.0f;
        if (moving != s_anim_moving) {
            s_anim_moving = moving;
            if (g_player)
                player_anim_start_entity_state(
                    g_player, moving ? PA_SCOOT : PA_SCOOTSTOP);
        }
    }

    if (g_player) {
        g_player->x  = e->x;
        g_player->y  = e->y + SCOOTER_SEAT_UP;
        g_player->z  = e->z;
        g_player->ry = e->ry;
        g_player->vx = g_player->vy = g_player->vz = 0.0f;
        g_player->visible = 1;
        player_anim_advance_entity(g_player,
                                   moving ? PA_SCOOT : PA_SCOOTSTOP, dt);
    }

    /* Goddard rides underneath as the bodywork, on the same transform. */
    Entity *g = behavior_goddard_get();
    if (g && g->visible) {
        g->x  = e->x;
        g->y  = e->y;
        g->z  = e->z;
        g->ry = e->ry;
    }
}

static void scooter_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    if (e->half_extents[0] == 0.0f)
        e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = SCOOTER_HALF;
}

static void scooter_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!s_riding || e != s_scooter) return;

    /* E dismounts, matching the rocket's board/leave key. */
    if (input_just_pressed(SDL_SCANCODE_E) || input_virtual_board_peek()) {
        input_virtual_board_consume();
        scooter_dismount();
        return;
    }

    float turn = 0.0f, throttle = 0.0f;
    if (input_is_down(SDL_SCANCODE_LEFT)  || input_is_down(SDL_SCANCODE_A)) turn     -= 1.0f;
    if (input_is_down(SDL_SCANCODE_RIGHT) || input_is_down(SDL_SCANCODE_D)) turn     += 1.0f;
    if (input_is_down(SDL_SCANCODE_UP)    || input_is_down(SDL_SCANCODE_W)) throttle += 1.0f;
    if (input_is_down(SDL_SCANCODE_DOWN)  || input_is_down(SDL_SCANCODE_S)) throttle -= 1.0f;
    float vjx, vjy; input_get_virtual_move(&vjx, &vjy);
    turn += vjx;
    throttle += vjy;

    e->ry -= turn * SCOOTER_TURN_RATE * dt;

    if (throttle > 0.0f) {
        e->move_speed += SCOOTER_ACCEL * dt;
        if (e->move_speed > SCOOTER_MAX_SPEED) e->move_speed = SCOOTER_MAX_SPEED;
    } else if (throttle < 0.0f) {
        e->move_speed -= SCOOTER_BRAKE * dt;
        if (e->move_speed < -SCOOTER_MAX_SPEED * 0.35f)
            e->move_speed = -SCOOTER_MAX_SPEED * 0.35f;
    } else {
        /* Coast down rather than stopping dead. */
        float drag = SCOOTER_BRAKE * 0.45f * dt;
        if (e->move_speed > drag)       e->move_speed -= drag;
        else if (e->move_speed < -drag) e->move_speed += drag;
        else                            e->move_speed = 0.0f;
    }

    float sy = sinf(e->ry), cy = cosf(e->ry);
    e->vx = sy * e->move_speed;
    e->vz = cy * e->move_speed;
    e->x += e->vx * dt;
    e->z += e->vz * dt;

    scooter_sync_child(e, dt);
}

const EntityVTable vt_scooter = {
    .on_spawn   = scooter_on_spawn,
    .on_update  = scooter_on_update,
    .on_trigger = NULL,   /* mounted from the action menu, not by walking into it */
};
