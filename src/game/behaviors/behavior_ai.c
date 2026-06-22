/* behavior_ai.c — shared C3DAI patrol/seek/idle base.
 *
 * This is the native base for AI leaves that author PatrolPoint, TargetName,
 * VisibleRange, FOV, AIState, and WanderRange. It intentionally exposes small
 * primitives first; leaf classes still own attacks, animation selection, and
 * state-specific effects.
 */
#include "behavior_ai.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

#define AI_STATE_IDLE    0
#define AI_STATE_PATROL  1
#define AI_STATE_WAIT    2

static void copy_tag(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) src = "";
    size_t n = strlen(src);
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void behavior_ai_idle(Entity *e) {
    if (!e) return;
    e->vx = 0.0f;
    e->vz = 0.0f;
}

BehaviorAIResult behavior_ai_seek_position(Entity *e, float x, float y, float z,
                                           float speed, float arrive_radius,
                                           float dt) {
    (void)y;
    if (!e || dt <= 0.0f || speed <= 0.0f) return BEHAVIOR_AI_IDLE;
    float dx = x - e->x;
    float dz = z - e->z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist <= arrive_radius) {
        behavior_ai_idle(e);
        return BEHAVIOR_AI_ARRIVED;
    }

    float inv = 1.0f / dist;
    float step = speed * dt;
    if (step > dist) step = dist;
    float ox = e->x;
    float oz = e->z;
    e->x += dx * inv * step;
    e->z += dz * inv * step;
    e->vx = (e->x - ox) / dt;
    e->vz = (e->z - oz) / dt;
    e->ry = atan2f(-dx, -dz);
    return BEHAVIOR_AI_SEEKING;
}

Entity *behavior_ai_find_patrol_point(World *w, const char *tag) {
    if (!w || !tag || !tag[0]) return NULL;
    for (Entity *e = w->head; e; e = e->next) {
        if (strncmp(e->type, "3PAT", 4) != 0) continue;
        if (strcasecmp(e->tag, tag) == 0) return e;
    }
    return NULL;
}

void behavior_ai_spawn_patrol(Entity *e) {
    if (!e) return;
    behavior_animated_spawn_base(e);
    copy_tag(e->start_point, sizeof(e->start_point), e->patrol_point);
    e->user_flag = e->patrol_point[0] ? AI_STATE_PATROL : AI_STATE_IDLE;
    e->user_float = 0.0f;
}

BehaviorAIResult behavior_ai_update_patrol(Entity *e, World *w,
                                           const BehaviorAIPatrolConfig *cfg,
                                           float dt) {
    if (!behavior_animated_update_base(e, w, dt)) return BEHAVIOR_AI_IDLE;
    if (!e->patrol_point[0]) {
        behavior_ai_idle(e);
        e->user_flag = AI_STATE_IDLE;
        return BEHAVIOR_AI_IDLE;
    }

    if (e->user_flag == AI_STATE_WAIT) {
        e->user_float -= dt;
        if (e->user_float > 0.0f) {
            behavior_ai_idle(e);
            return BEHAVIOR_AI_WAITING;
        }
        e->user_flag = AI_STATE_PATROL;
    }

    Entity *wp = behavior_ai_find_patrol_point(w, e->patrol_point);
    if (!wp) {
        e->patrol_point[0] = '\0';
        e->user_flag = AI_STATE_IDLE;
        behavior_ai_idle(e);
        return BEHAVIOR_AI_IDLE;
    }

    float speed = cfg ? cfg->speed : 160.0f;
    float arrive = cfg ? cfg->arrive_radius : 60.0f;
    BehaviorAIResult seek = behavior_ai_seek_position(e, wp->x, wp->y, wp->z,
                                                      speed, arrive, dt);
    if (seek != BEHAVIOR_AI_ARRIVED) return seek;

    const char *next = wp->next_patrol[0] ? wp->next_patrol : "";
    if ((!next[0]) && cfg && cfg->loop_to_start)
        next = e->start_point;
    copy_tag(e->patrol_point, sizeof(e->patrol_point), next);
    if (!e->patrol_point[0]) {
        e->user_flag = AI_STATE_IDLE;
        return BEHAVIOR_AI_IDLE;
    }

    float wait = gam_prop_f(wp, "WaitTime", 0.0f);
    if (wait > 0.0f) {
        e->user_flag = AI_STATE_WAIT;
        e->user_float = wait;
        return BEHAVIOR_AI_WAITING;
    }
    e->user_flag = AI_STATE_PATROL;
    return BEHAVIOR_AI_ARRIVED;
}
