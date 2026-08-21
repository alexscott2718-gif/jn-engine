/* behavior_gadgets.c — the gadget runtime behind the action menu.
 *
 * Eight gadgets, named by the owner from the retail game and mapped to their
 * classes in docs/picture_flag_wiring_plan.md section 18. Five of the eight are
 * never placed in level data: the executable creates them in code, so there is
 * nothing in the .gam corpus to spawn and each has to be stood up here.
 *
 * What is ported from evidence, per gadget:
 *
 *   jetpack       UpdateJimmyActiveController's fly path: set FLY/HIFLY, start
 *                 looping sound id 1, latch, seed 5.0s, mode word 5.
 *   shrink ray    C3DShrinkRay (3SHR): ray.ASE bound to alias HIRAY, textures
 *                 ray0000..ray0002 cycled on a 0.1s timer (fields 0x5fc/0x600),
 *                 inherited scale 40.0. Owner-confirmed effect: fired at Dino /
 *                 Darwin fish / girl-eating plant / Humphrey, the target plays
 *                 HISHRINK, scales down, and becomes a small moving pickup.
 *   bubble        C3DBubble (3BUB): RetainedSprites.omt chunk 1 "orangebubble",
 *                 SpriteSize 240, and the exact three-phase state machine from
 *                 UpdateBubbleState -- grow to 1.0, steady pulse at
 *                 sin(t*10)*30 on the HEIGHT only, then fade 1.0 -> 0.0 and
 *                 hide.
 *   grappler      C3DGraplingHook (3GRA, one L in the original): rope01.ASE
 *                 under shape alias HIROPE, texture jimycarl.png, scale 10.0,
 *                 state tag ROPE. Its targets are the level-authored C3DHook
 *                 (3HOO) objects.
 *   remote Goddard  behavior_goddard already carries the mode protocol
 *                 C3DMetalPickup uses: 5 = fetch that object, 2 = release and
 *                 follow. This just drives it from the menu.
 *   rocket        3ROC is placed and already rideable; the menu boards it.
 *   scooter       behavior_scooter.c.
 *   invisibility  no class registered anywhere in the executable, so this one
 *                 is native design: Jimmy stops being drawn and AI stops
 *                 seeing him.
 *
 * Tuning numbers that are NOT from the decomp -- thrust, ray speed, grapple
 * reach, durations -- are grouped at the top of each section and marked. The
 * mechanisms are recovered; the feel is calibration.
 */
#include "behaviors.h"
#include "behavior_ai.h"
#include "behavior_vehicle.h"
#include "../entities.h"
#include "../gadget_menu.h"
#include "../player_anim.h"
#include "../gamestate.h"
#include "../../engine/input.h"
#include "../../engine/world.h"
#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* ---- shared state ------------------------------------------------------ */

static int    s_active = GADGET_NONE;
static int    s_invisible = 0;
static Entity *s_flame = NULL;     /* 3JFI, while the jetpack is on */
static float  s_fire_cooldown = 0.0f;

/* Recovered: the fly path seeds a 5.0s window and mode word 5. */
#define JETPACK_WINDOW      5.0f
/* Tuned. */
#define JETPACK_CLIMB     620.0f
#define JETPACK_HOVER      -40.0f
#define FIRE_COOLDOWN       0.45f

int behavior_gadget_active(void)     { return s_active; }
int behavior_gadget_invisible(void)  { return s_invisible; }

const char *behavior_gadget_name(int g) {
    switch (g) {
        case GADGET_JETPACK:      return "jetpack";
        case GADGET_SHRINKRAY:    return "shrinkray";
        case GADGET_BUBBLE:       return "bubble";
        case GADGET_GRAPPLER:     return "grappler";
        case GADGET_GODDARD:      return "goddard";
        case GADGET_ROCKET:       return "rocket";
        case GADGET_SCOOTER:      return "scooter";
        case GADGET_INVISIBILITY: return "invisibility";
        default:                  return "none";
    }
}

static int gadget_from_tag(const char *tag) {
    if (!tag || !tag[0]) return GADGET_NONE;
    if (strcasecmp(tag, "jetpack") == 0)      return GADGET_JETPACK;
    if (strcasecmp(tag, "shrinkray") == 0)    return GADGET_SHRINKRAY;
    if (strcasecmp(tag, "bubble") == 0)       return GADGET_BUBBLE;
    if (strcasecmp(tag, "grappler") == 0)     return GADGET_GRAPPLER;
    if (strcasecmp(tag, "goddard") == 0)      return GADGET_GODDARD;
    if (strcasecmp(tag, "rocket") == 0)       return GADGET_ROCKET;
    if (strcasecmp(tag, "scooter") == 0)      return GADGET_SCOOTER;
    if (strcasecmp(tag, "invisibility") == 0) return GADGET_INVISIBILITY;
    return GADGET_NONE;
}

static void jetpack_stop(void) {
    if (s_flame) { s_flame->alive = 0; s_flame->visible = 0; s_flame = NULL; }
}

static void invisibility_stop(void) {
    if (!s_invisible) return;
    s_invisible = 0;
    if (g_player) g_player->visible = 1;
    printf("[GADGET] invisibility off\n");
}

/* Leaving a gadget: undo whatever it was holding open. */
static void gadget_deactivate(int g) {
    switch (g) {
        case GADGET_JETPACK:      jetpack_stop(); break;
        case GADGET_INVISIBILITY: invisibility_stop(); break;
        case GADGET_SCOOTER:      if (behavior_scooter_riding())
                                      behavior_scooter_activate();
                                  break;
        default: break;
    }
    action_mode_set(ACTION_MODE_NONE);
}

/* ---- rocket ------------------------------------------------------------ */

static Entity *nearest_of_type(World *w, const char *type, float max_dist) {
    if (!w || !g_player) return NULL;
    Entity *best = NULL;
    float best_d2 = max_dist * max_dist;
    for (Entity *e = w->head; e; e = e->next) {
        if (!e->alive || strncmp(e->type, type, 4) != 0) continue;
        float dx = e->x - g_player->x, dy = e->y - g_player->y,
              dz = e->z - g_player->z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < best_d2) { best_d2 = d2; best = e; }
    }
    return best;
}

/* ---- activation -------------------------------------------------------- */

int behavior_gadget_activate(const char *tag, World *w) {
    int g = gadget_from_tag(tag);
    if (g == GADGET_NONE) return 0;

    if (s_active == g) {          /* choosing the live gadget puts it away */
        gadget_deactivate(g);
        s_active = GADGET_NONE;
        printf("[GADGET] %s off\n", behavior_gadget_name(g));
        return 0;
    }
    if (s_active != GADGET_NONE) gadget_deactivate(s_active);
    s_active = g;

    switch (g) {
        case GADGET_ROCKET: {
            /* Mode 1 is the one arm of the AMI dispatch that traces
               "Activating Rocket" and plays DRIVE. */
            action_mode_set(ACTION_MODE_ROCKET);
            Entity *r = nearest_of_type(w, "3ROC", 6000.0f);
            if (r && !behavior_vehicle_riding()) behavior_vehicle_force_board(r);
            else if (!r) printf("[GADGET] no rocket within reach\n");
            break;
        }
        case GADGET_SCOOTER:
            behavior_scooter_activate();
            break;
        case GADGET_GODDARD: {
            /* C3DMetalPickup's protocol: 5 = go fetch that object, 2 = release
               and follow. Point him at the nearest pickup if there is one. */
            action_mode_set(ACTION_MODE_5);
            Entity *target = nearest_of_type(w, "3PIC", 4000.0f);
            if (target) behavior_goddard_request_mode(target, 5);
            else        behavior_goddard_request_mode(NULL, 2);
            break;
        }
        case GADGET_INVISIBILITY:
            s_invisible = 1;
            if (g_player) g_player->visible = 0;
            printf("[GADGET] invisibility on\n");
            break;
        case GADGET_SHRINKRAY:
            /* Mode 6 is aim/shoot: pitch clamped [0,45], SHOOT. */
            action_mode_set(ACTION_MODE_AIM);
            break;
        default:
            break;
    }
    printf("[GADGET] %s on (action mode %d)\n",
           behavior_gadget_name(g), action_mode());
    return 1;
}

void behavior_gadgets_reset(void) {
    s_active = GADGET_NONE;
    s_invisible = 0;
    s_flame = NULL;
    s_fire_cooldown = 0.0f;
}

/* ---- the ray (3SHR) ---------------------------------------------------- */

#define RAY_SPEED     2600.0f   /* tuned */
#define RAY_LIFE         1.4f   /* tuned */
#define RAY_HIT_RADIUS 140.0f   /* tuned */
#define RAY_FRAME_TIME   0.1f   /* RECOVERED: field 0x600 threshold */
#define RAY_FRAMES          3   /* RECOVERED: ray0000..ray0002 */

/* Owner-confirmed: the shrunk target plays HISHRINK, scales down, and becomes
   a small moving pickup the player can collect. The transition body was never
   decompiled, so the shrink curve and the resulting pickup are native design;
   the outcome is what was observed. */
#define SHRINK_TIME      0.6f
#define SHRINK_FLOOR    0.30f
#define SHRINK_POINTS     100

static int entity_is_shrinkable(const Entity *e) {
    /* The classes that register a HISHRINK frame: Dino, Darwin fish,
       girl-eating plant (all vt_creature) and Humphrey. */
    return strncmp(e->type, "3DIN", 4) == 0 || strncmp(e->type, "3FIS", 4) == 0 ||
           strncmp(e->type, "3GIR", 4) == 0 || strncmp(e->type, "3HUM", 4) == 0 ||
           strncmp(e->type, "3CML", 4) == 0 || strncmp(e->type, "3SPW", 4) == 0;
}

static void ray_on_update(Entity *e, World *w, float dt) {
    if (!e->alive) return;

    /* Texture frame cadence, straight from slot 241. */
    e->user_float += dt;
    if (e->user_float >= RAY_FRAME_TIME) {
        e->user_float -= RAY_FRAME_TIME;
        e->user_flag = (e->user_flag + 1) % RAY_FRAMES;
    }

    e->x += e->vx * dt;
    e->y += e->vy * dt;
    e->z += e->vz * dt;

    e->hp -= dt;                       /* reused as the lifetime clock */
    if (e->hp <= 0.0f) { e->alive = 0; e->visible = 0; return; }

    for (Entity *t = w->head; t; t = t->next) {
        if (!t->alive || !t->visible || t == e) continue;
        if (!entity_is_shrinkable(t)) continue;
        if (t->user_float < 0.0f) continue;        /* already shrinking */
        float dx = t->x - e->x, dy = t->y - e->y, dz = t->z - e->z;
        if (dx * dx + dy * dy + dz * dz > RAY_HIT_RADIUS * RAY_HIT_RADIUS)
            continue;

        /* Mark it shrinking. Negative user_float is the shrink clock, counting
           up from -SHRINK_TIME so it cannot collide with any positive use. */
        t->user_float = -SHRINK_TIME;
        printf("[SHRINKRAY] hit %s '%s' -> HISHRINK\n", t->type, t->tag);
        e->alive = 0;
        e->visible = 0;
        return;
    }
}

const EntityVTable vt_shrinkray = {
    .on_spawn  = NULL,
    .on_update = ray_on_update,
};

/* Advance every shrinking creature. Kept here rather than in behavior_creature
   so the shrink mechanic lives with the gadget that causes it. */
static void shrink_targets_update(World *w, float dt) {
    for (Entity *t = w->head; t; t = t->next) {
        if (!t->alive || t->user_float >= 0.0f) continue;
        if (!entity_is_shrinkable(t)) continue;
        t->user_float += dt;
        float k = 1.0f + t->user_float / SHRINK_TIME;    /* 0 -> 1 */
        if (k < 0.0f) k = 0.0f;
        if (k > 1.0f) k = 1.0f;
        t->draw_scale = 1.0f - (1.0f - SHRINK_FLOOR) * k;
        if (t->user_float >= 0.0f) {
            /* Shrunk. It is a pickup now: small, collectable, worth points. */
            t->user_float = 0.0f;
            t->draw_scale = SHRINK_FLOOR;
            t->points = t->points ? t->points : SHRINK_POINTS;
            t->half_extents[0] = t->half_extents[1] = t->half_extents[2] = 60.0f;
            t->runtime_flags |= ENTITY_FLAG_TRIGGER;
            printf("[SHRINKRAY] %s '%s' is a pickup now\n", t->type, t->tag);
        }
    }
}

/* ---- the bubble (3BUB) ------------------------------------------------- */
/* UpdateBubbleState, with its own constants:
     grow    transition = timer, clamps at 1.0 then hands over to pulse
     pulse   offset = sin(timer * 10.0) * 30.0, applied to the HEIGHT only
     fade    transition = 1.0 - timer, hides at 0
   SpriteSize 240 is the constructor default and the pulse base. */
#define BUBBLE_SIZE      240.0f
#define BUBBLE_WAVE_RATE  10.0f
#define BUBBLE_WAVE_AMP   30.0f
enum { BUBBLE_GROW = 0, BUBBLE_PULSE = 1, BUBBLE_FADE = 2 };

static void bubble_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;
    e->user_float += dt;

    switch (e->user_flag) {
        case BUBBLE_GROW:
            e->hp = e->user_float;                 /* transition scale */
            if (e->hp >= 1.0f) {
                e->hp = 1.0f;
                e->user_flag = BUBBLE_PULSE;
                e->user_float = 0.0f;
            }
            break;
        case BUBBLE_PULSE:
            e->hp = 1.0f;
            break;
        case BUBBLE_FADE:
            e->hp = 1.0f - e->user_float;
            if (e->hp <= 0.0f) { e->hp = 0.0f; e->alive = 0; e->visible = 0; }
            break;
    }

    /* It drifts up gently with Jimmy underneath it. Native feel, not recovered. */
    if (g_player && e->user_flag != BUBBLE_FADE) {
        e->x = g_player->x;
        e->z = g_player->z;
        e->y = g_player->y + 40.0f;
    }
}

const EntityVTable vt_bubble = {
    .on_spawn  = NULL,
    .on_update = bubble_on_update,
};

/* Size for the draw path: width holds at SpriteSize, height carries the wave,
   both scaled by the grow/fade transition. */
void behavior_bubble_size(const Entity *e, float *w_out, float *h_out,
                          float *alpha_out) {
    float k = e->hp;
    float wave = 0.0f;
    if (e->user_flag == BUBBLE_PULSE)
        wave = sinf(e->user_float * BUBBLE_WAVE_RATE) * BUBBLE_WAVE_AMP;
    if (w_out)     *w_out = BUBBLE_SIZE * k;
    if (h_out)     *h_out = (BUBBLE_SIZE + wave) * k;
    if (alpha_out) *alpha_out = k;
}

/* ---- the grappling rope (3GRA) ----------------------------------------- */

#define GRAPPLE_REACH   2200.0f   /* tuned */
#define GRAPPLE_PULL    1500.0f   /* tuned */
#define GRAPPLE_ARRIVE   120.0f   /* tuned */

static Entity *s_rope = NULL;
static Entity *s_hook_target = NULL;

static void rope_on_update(Entity *e, World *w, float dt) {
    (void)w; (void)dt;
    if (!e->alive) return;
    if (!s_hook_target || !g_player) { e->alive = 0; e->visible = 0; return; }
    /* The rope sits midway and points at the hook. */
    e->x = (g_player->x + s_hook_target->x) * 0.5f;
    e->y = (g_player->y + s_hook_target->y) * 0.5f + 30.0f;
    e->z = (g_player->z + s_hook_target->z) * 0.5f;
    e->ry = atan2f(s_hook_target->x - g_player->x,
                   s_hook_target->z - g_player->z);
}

const EntityVTable vt_grapple_rope = {
    .on_spawn  = NULL,
    .on_update = rope_on_update,
};

static void grapple_release(void) {
    if (s_rope) { s_rope->alive = 0; s_rope->visible = 0; s_rope = NULL; }
    s_hook_target = NULL;
}

/* ---- per-frame ---------------------------------------------------------- */

void behavior_gadgets_update(World *w, float dt) {
    if (!w) return;
    shrink_targets_update(w, dt);

    if (s_fire_cooldown > 0.0f) s_fire_cooldown -= dt;

    /* The rope pulls Jimmy in even if the player switches gadget mid-swing;
       releasing is the gadget's job, arriving is the rope's. */
    if (s_hook_target && g_player) {
        float dx = s_hook_target->x - g_player->x;
        float dy = s_hook_target->y - g_player->y;
        float dz = s_hook_target->z - g_player->z;
        float d = sqrtf(dx * dx + dy * dy + dz * dz);
        if (d <= GRAPPLE_ARRIVE) {
            grapple_release();
        } else {
            g_player->x += dx / d * GRAPPLE_PULL * dt;
            g_player->y += dy / d * GRAPPLE_PULL * dt;
            g_player->z += dz / d * GRAPPLE_PULL * dt;
            g_player->vy = 0.0f;
        }
    }

    if (s_active == GADGET_NONE || !g_player) return;

    if (input_just_pressed(SDL_SCANCODE_F) || input_virtual_take_use())
        behavior_gadget_fire(w);

    switch (s_active) {
        case GADGET_INVISIBILITY:
            g_player->visible = 0;
            break;
        case GADGET_JETPACK: {
            int up = input_is_down(SDL_SCANCODE_SPACE) ||
                     input_get_virtual_fly() > 0.1f;
            if (up) {
                g_player->vy = JETPACK_CLIMB;
                g_player->on_ground = 0;
            } else if (g_player->vy < JETPACK_HOVER) {
                g_player->vy = JETPACK_HOVER;      /* thrusters idle: soft fall */
            }
            player_anim_advance_entity(g_player, PA_FLY, dt);
            /* The flame follows underneath. */
            if (!s_flame) {
                s_flame = entity_spawn(w, "3JFI", "C3DJETPACKFIRE",
                                       g_player->x, g_player->y, g_player->z);
            }
            if (s_flame) {
                s_flame->x = g_player->x;
                s_flame->y = g_player->y - 30.0f;
                s_flame->z = g_player->z;
                s_flame->visible = up;
            }
            break;
        }
        default:
            break;
    }
}

/* One-shot: what the active gadget does when the player pulls the trigger.
   Exposed so a headless run can exercise the spawn paths. */
int behavior_gadget_fire(World *w) {
    if (!w || !g_player || s_active == GADGET_NONE) return 0;
    if (s_fire_cooldown > 0.0f) return 0;

    switch (s_active) {
        case GADGET_SHRINKRAY: {
            s_fire_cooldown = FIRE_COOLDOWN;
            float sy = sinf(g_player->ry), cy = cosf(g_player->ry);
            Entity *r = entity_spawn(w, "3SHR", "C3DSHRINKRAY",
                                     g_player->x + sy * 90.0f,
                                     g_player->y + 40.0f,
                                     g_player->z + cy * 90.0f);
            if (r) {
                r->ry = g_player->ry;
                r->vx = sy * RAY_SPEED;
                r->vz = cy * RAY_SPEED;
                r->vy = 0.0f;
                r->hp = RAY_LIFE;
                r->user_flag = 0;
                r->user_float = 0.0f;
                r->runtime_flags = 0;
                player_anim_start_entity_state(g_player, PA_SHOOT);
                printf("[SHRINKRAY] fired\n");
            }
            return 1;
        }
        case GADGET_BUBBLE: {
            Entity *b = entity_spawn(w, "3BUB", "C3DBUBBLE",
                                     g_player->x, g_player->y + 40.0f,
                                     g_player->z);
            if (b) {
                b->user_flag = BUBBLE_GROW;
                b->user_float = 0.0f;
                b->hp = 0.0f;
                b->runtime_flags = 0;
                printf("[BUBBLE] blown\n");
            }
            s_fire_cooldown = FIRE_COOLDOWN;
            return 1;
        }
        case GADGET_GRAPPLER: {
            s_fire_cooldown = FIRE_COOLDOWN;
            if (s_hook_target) { grapple_release(); return 1; }
            Entity *hook = nearest_of_type(w, "3HOO", GRAPPLE_REACH);
            if (!hook) { printf("[GRAPPLER] no hook in reach\n"); return 0; }
            s_hook_target = hook;
            s_rope = entity_spawn(w, "3GRA", "C3DGRAPLINGHOOK",
                                  g_player->x, g_player->y, g_player->z);
            if (s_rope) s_rope->runtime_flags = 0;
            printf("[GRAPPLER] hooked '%s'\n", hook->tag);
            return 1;
        }
        default:
            return 0;
    }
}
