#ifndef GAME_BEHAVIOR_BASE_H
#define GAME_BEHAVIOR_BASE_H

#include "behaviors.h"

/* Shared C3DObject/C3DAnimated lifecycle subset used by native leaves.
   Returns 1 when the derived behavior should continue its update. */
void behavior_animated_spawn_base(Entity *e);
int  behavior_animated_update_base(Entity *e, World *w, float dt);

/* Temporary task/progress bridge until the real task-state level gates are
   fully mapped. When JN_PROGRESS_LEVEL is unset, generic progress gates stay
   disabled for direct --level/audit runs. */
int  behavior_progress_gate_enabled(void);
int  behavior_progress_level(void);

/* CLoadLevel / C3DArrow author RequiredTask plus RequiredLevel/ExactLevel.
   When a task store is loaded (campaign / NewGame), evaluate the level window
   against that task state; otherwise preserve direct --level behavior unless
   JN_PROGRESS_LEVEL is explicitly supplied by a probe. */
int  behavior_required_task_gate_allows(const Entity *e);

/* Shared C3DTriggerType/C3DSpriteType activation setup for overlap leaves. */
void behavior_trigger_spawn_base(Entity *e, float hx, float hy, float hz);

#endif
