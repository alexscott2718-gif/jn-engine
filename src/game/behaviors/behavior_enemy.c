/* behavior_enemy.c — C3DYokian humanoid enemy family (Wave N2).
 *
 * Covers the placeable Yokian leaves that already have visuals wired in
 * entity_visual.c: 3SOL (C3DYokianSoldier), 3GUA (C3DYokianGuard), and
 * 3SPY (C3DYokianSpy). They share the C3DYokian base behavior, which sits on
 * the C3DAI seek/scan/attack state machine (docs/decomp/C3DYokian.md,
 * C3DYokianSoldier.md, C3DAI.md).
 *
 * Native model (a faithful subset of C3DAI's state switch):
 *   - Acquire the player (C3DAI TargetName defaults to "JIM1") when within the
 *     authored VisibleRange and inside the FOV cone (C3DAI states 2/3/4:
 *     scan/move-toward-target).
 *   - CHASE: walk toward the player using the shared behavior_ai seek primitive.
 *   - ATTACK: in melee range, face the player and strike on a cooldown,
 *     damaging player health (C3DYokian::StartYokianAttackEffect selects ATTACK;
 *     contact-range reaction range default is 200). Soldiers are melee — they
 *     do not spawn projectiles in the decompiled body, so neither do we.
 *   - Defeat: the player's thrown baseball (C3DYokian::ReactToHitObject reacts
 *     to C3DBASEBALL) calls behavior_enemy_take_hit; at hp<=0 the Yokian enters
 *     a brief knockout, goes non-interactive, then is removed.
 *
 * Scratch layout (Entity):
 *   user_flag  = enemy mode (see EnemyMode)
 *   user_float = timer — attack cooldown while alive, knockout countdown after
 *   hp         = remaining health
 *   home[3]    = spawn position (idle anchor)
 *
 * The deeper C3DYokian visuals (shadow child, shield/bubble child, hit-recovery
 * state 5, randomized hit sounds) are deferred; this is the playable core.
 */
#include "behavior_enemy.h"
#include "behavior_ai.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../player_anim.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum EnemyMode {
    ENEMY_IDLE     = 0,
    ENEMY_CHASE    = 1,
    ENEMY_ATTACK   = 2,
    ENEMY_KNOCKOUT = 3,
};

#define ENEMY_HP_DEFAULT    100.0f
#define ENEMY_CHASE_SPEED   200.0f   /* units/sec — matches Yokian walk pace */
#define ENEMY_ATTACK_RANGE  260.0f   /* melee reach (C3DYokian contact range ~200) */
#define ENEMY_ATTACK_DAMAGE 10       /* health per landed strike */
#define ENEMY_ATTACK_COOLDN 1.2f     /* seconds between strikes */
#define ENEMY_KO_TIME       0.8f     /* knockout dwell before removal */
#define ENEMY_VISIBLE_DEF   2500.0f  /* C3DAI VisibleRange constructor default */
#define ENEMY_FOV_DEF       90.0f    /* C3DAI FOV constructor default (degrees) */

static int enemy_is_yokian(const char *type) {
    return strncmp(type, "3SOL", 4) == 0 ||
           strncmp(type, "3GUA", 4) == 0 ||
           strncmp(type, "3SPY", 4) == 0 ||
           strncmp(type, "3FLE", 4) == 0;  /* C3DFleetCommander inherits C3DYokian */
}

static int enemy_is_instant_ko(const char *type) {
    return strncmp(type, "3TUR", 4) == 0;
}

static float ang_wrap(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

int behavior_enemy_take_hit(Entity *e, int dmg) {
    if (!e || !e->alive || !e->visible) return 0;
    int yokian = enemy_is_yokian(e->type);
    if (!yokian && !enemy_is_instant_ko(e->type)) return 0;
    if (yokian && e->user_flag == ENEMY_KNOCKOUT) return 0;  /* already going down */

    e->hp -= (float)dmg;
    if (e->hp <= 0.0f) {
        e->hp = 0.0f;
        if (!yokian) {
            e->alive = 0;
            e->visible = 0;
            e->runtime_flags = 0;
            printf("[ENEMY] %s '%s' defeated\n", e->type, e->tag);
            return 1;
        }
        e->user_flag = ENEMY_KNOCKOUT;
        e->user_float = ENEMY_KO_TIME;
        e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
        behavior_ai_idle(e);
        printf("[ENEMY] %s '%s' defeated\n", e->type, e->tag);
    }
    return 1;
}

static void enemy_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->hp = ENEMY_HP_DEFAULT;
    e->user_flag = ENEMY_IDLE;
    e->user_float = 0.0f;
    e->home[0] = e->x;
    e->home[1] = e->y;
    e->home[2] = e->z;
}

static void enemy_on_update(Entity *e, World *w, float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return;

    /* Knockout: dwell, then remove. */
    if (e->user_flag == ENEMY_KNOCKOUT) {
        behavior_ai_idle(e);
        e->user_float -= dt;
        if (e->user_float <= 0.0f) {
            e->alive = 0;
            e->visible = 0;
        }
        return;
    }

    if (e->user_float > 0.0f) e->user_float -= dt;  /* attack cooldown */

    Entity *p = g_player;
    if (!p || !p->alive) {
        e->user_flag = ENEMY_IDLE;
        behavior_ai_idle(e);
        return;
    }

    float dx = p->x - e->x;
    float dz = p->z - e->z;
    float dist = sqrtf(dx * dx + dz * dz);

    float visible_range = gam_prop_f(e, "VisibleRange", ENEMY_VISIBLE_DEF);
    if (visible_range <= 0.0f) visible_range = ENEMY_VISIBLE_DEF;

    if (dist > visible_range) {
        e->user_flag = ENEMY_IDLE;
        behavior_ai_idle(e);
        return;
    }

    /* FOV gate only for first acquisition; once engaged the Yokian turns to
       track the player. Point-blank contact always acquires. */
    if (e->user_flag == ENEMY_IDLE && dist > ENEMY_ATTACK_RANGE) {
        float fov = gam_prop_f(e, "FOV", ENEMY_FOV_DEF);
        if (fov <= 0.0f) fov = ENEMY_FOV_DEF;
        float half = fov * 0.5f * (float)M_PI / 180.0f;
        float want = atan2f(-dx, -dz);
        if (fabsf(ang_wrap(want - e->ry)) > half) {
            behavior_ai_idle(e);
            return;  /* player not yet in view */
        }
    }

    if (dist <= ENEMY_ATTACK_RANGE) {
        e->user_flag = ENEMY_ATTACK;
        behavior_ai_idle(e);
        e->ry = atan2f(-dx, -dz);  /* face the player */
        if (e->user_float <= 0.0f) {
            gamestate_damage_player(ENEMY_ATTACK_DAMAGE);
            player_anim_react_to_damage(p, gamestate_player_is_down());
            e->user_float = ENEMY_ATTACK_COOLDN;
        }
    } else {
        e->user_flag = ENEMY_CHASE;
        behavior_ai_seek_position(e, p->x, p->y, p->z,
                                  ENEMY_CHASE_SPEED, ENEMY_ATTACK_RANGE, dt);
    }
}

/* flags=0: Yokians are not AABB-solid in this native subset (their contact is
   modelled by distance, and keeping them non-solid lets player projectiles
   reach them through world_query_segment instead of stopping at their box). */
const EntityVTable vt_yokian = {
    .on_spawn   = enemy_on_spawn,
    .on_update  = enemy_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
