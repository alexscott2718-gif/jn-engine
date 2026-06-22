#ifndef GAME_BEHAVIOR_AI_H
#define GAME_BEHAVIOR_AI_H

#include "behavior_base.h"

typedef enum BehaviorAIResult {
    BEHAVIOR_AI_IDLE = 0,
    BEHAVIOR_AI_SEEKING,
    BEHAVIOR_AI_ARRIVED,
    BEHAVIOR_AI_WAITING,
} BehaviorAIResult;

typedef struct BehaviorAIPatrolConfig {
    float speed;
    float arrive_radius;
    int loop_to_start;
} BehaviorAIPatrolConfig;

void behavior_ai_idle(Entity *e);
BehaviorAIResult behavior_ai_seek_position(Entity *e, float x, float y, float z,
                                           float speed, float arrive_radius,
                                           float dt);
Entity *behavior_ai_find_patrol_point(World *w, const char *tag);
void behavior_ai_spawn_patrol(Entity *e);
BehaviorAIResult behavior_ai_update_patrol(Entity *e, World *w,
                                           const BehaviorAIPatrolConfig *cfg,
                                           float dt);

#endif
