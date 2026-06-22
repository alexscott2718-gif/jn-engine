#ifndef GAME_BEHAVIOR_FLYING_H
#define GAME_BEHAVIOR_FLYING_H

#include "behavior_base.h"
#include "../../engine/movement_base.h"

void behavior_flying_spawn_base(Entity *e);
int  behavior_flying_update_base(Entity *e, World *w,
                                 const MovementFlyingInput *input, float dt);

#endif
