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

/* --- CPickupType picture-flag economy (behavior_pickup_core.c) -----------
   The parts of C3DPickupItem::HandlePickupCollection (00435ce0) that both
   pickup-bearing vtables need: vt_item (3PIC) and vt_creature (3FIS/3GIR/3DIN,
   which descend from C3DPickupType and author PIC_NUMBER too).

   taken/mark     the DAT_004f8438[PickupIndex] read and write, keyed on
                  (level, PickupIndex) because indices collide across levels.
   spawn_gate     the load-time half (PostLoadPickupItem, 00436200): capture the
                  World for later tag dispatch, then disable a pickup that is
                  already collected or authored InitallyActive=0. Returns which
                  of the two it was (see PICKUP_SPAWN_*), because they differ
                  for the level item tally: an inactive product is still one of
                  the level's items, an already-collected one is not.
   set_state      SetPickupItemState (004360b0), the pickup family's state slot:
                  state 0 shows/enables, state 1 also clears the collected flag
                  (how a vending machine re-arms its product).
   gate_allows    CheckRequiredPicAndConsume (00436830): consume
                  ReqPicNumAmount of RequiredPicNum, or play NeedMoreSound and
                  refuse. Returns 1 when collection may proceed.
   award          the PIC_NUMBER award.
   dispatch_state ActivateObject then ToggleObject, each through the target's
                  state slot with the authored Toggle. Fires before the award.
   dispatch_next  NextTrigger, as a trigger-chain forward. Fires after it.
   sweep_collect  JN_TEST_PICTURES: force every authored pickup row in the world
                  through its own on_trigger; returns how many newly collected.
                  Call in a loop until it returns 0. */
/* behavior_pickup_spawn_gate results. */
enum {
    PICKUP_SPAWN_AVAILABLE = 0,  /* collectible right now */
    PICKUP_SPAWN_INACTIVE  = 1,  /* InitallyActive=0: a level item, not yet */
    PICKUP_SPAWN_TAKEN     = 2   /* collected on an earlier visit */
};

int  behavior_pickup_taken(const Entity *e);
void behavior_pickup_mark_taken(const Entity *e);
int  behavior_pickup_spawn_gate(Entity *e, World *w);
void behavior_pickup_set_state(Entity *e, int state);
int  behavior_pickup_gate_allows(const Entity *e);
void behavior_pickup_award_pictures(const Entity *e);
void behavior_pickup_dispatch_state(const Entity *e);
void behavior_pickup_dispatch_next(const Entity *e);
int  behavior_pickup_sweep_collect(World *w);

#endif
