#ifndef GAME_BEHAVIOR_BASE_H
#define GAME_BEHAVIOR_BASE_H

#include "behaviors.h"

/* Shared C3DObject/C3DAnimated lifecycle subset used by native leaves.
   Returns 1 when the derived behavior should continue its update. */
void behavior_animated_spawn_base(Entity *e);
int  behavior_animated_update_base(Entity *e, World *w, float dt);

/* Shared C3DTriggerType/C3DSpriteType activation setup for overlap leaves. */
void behavior_trigger_spawn_base(Entity *e, float hx, float hy, float hz);

#endif
