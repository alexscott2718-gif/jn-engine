#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include <stddef.h>
#include <stdio.h>

/* C3DCheckPoint (3CHK): when the player passes through, the level's respawn
   point moves here. Last-touched wins, which matches the original's
   checkpoint-progression feel (CheckpointNum orders save slots; for respawn the
   most-recently-reached checkpoint is the one we restore to). */

static void checkpoint_on_spawn(Entity *e, World *w) {
    (void)w;
    /* Generous trigger volume so the player reliably crosses it. */
    behavior_trigger_spawn_base(e, 120.0f, 160.0f, 120.0f);
    e->user_flag = 0;   /* 0 = not yet reached */
}

static void checkpoint_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag) return;   /* announce once */
    e->user_flag = 1;
    /* Respawn slightly above the marker so the player doesn't re-trigger or
       clip into the ground on restore. */
    gamestate_set_spawn(e->x, e->y + 40.0f, e->z);
    printf("[CHECKPOINT] reached (tag='%s') -> respawn (%.0f, %.0f, %.0f)\n",
           e->tag, e->x, e->y + 40.0f, e->z);
}

const EntityVTable vt_checkpoint = {
    .on_spawn   = checkpoint_on_spawn,
    .on_update  = NULL,
    .on_trigger = checkpoint_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
