/* behavior_trophy.c - C3DVRTrophy (3TRO).
 *
 * The VR challenge-level reward (docs/decomp/C3DVRTrophy.md). Its owned visual
 * slot (vfunc_04_067) binds the HIDEFAULT trophy.ase + trophy.png + DEFAULT
 * animation once — the native visual resolver already draws trophy.ASE, so no
 * asset work is needed here. The gameplay slot (vfunc_01_010) waits on a flag
 * and, once set, fires the object's exit/deactivate action (slot 0x58): the
 * trophy is grabbed, deactivates itself, and the level objective is met.
 *
 * Native mapping: one trophy sits in each VR level; Jimmy's contact (player
 * overlap) collects it -> the trophy hides + stops triggering, and the win
 * condition is signalled via game_flow_level_objective_met(). That call is a
 * no-op flag set when campaign mode is off (direct --level launches, the audit
 * + screenshot harnesses), so rendering there is unchanged and the 2-tick
 * probes never overlap the trophy.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../game_flow.h"
#include <stddef.h>
#include <stdio.h>

/* Contact volume for the grab. The decomp gives no touch radius (the flag is
   set by the generic object-overlap path); 60u is a generous Jimmy-sized grab
   box for a free-standing trophy mesh. */
#define TROPHY_HALF 60.0f

enum { TROPHY_READY = 0, TROPHY_TAKEN = 1 };

static void trophy_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, TROPHY_HALF, TROPHY_HALF, TROPHY_HALF);
    e->user_flag = TROPHY_READY;
}

static void trophy_collect(Entity *e) {
    if (e->user_flag != TROPHY_READY) return;
    e->user_flag = TROPHY_TAKEN;
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    game_flow_level_objective_met();
    printf("[TROPHY] collected '%s' -> objective met\n", e->tag);
}

static void trophy_on_trigger(Entity *e, Entity *by) {
    if (by != g_player) return;
    trophy_collect(e);
}

static void trophy_on_update(Entity *e, World *w, float dt) {
    (void)behavior_animated_update_base(e, w, dt);
}

const EntityVTable vt_trophy = {
    .on_spawn   = trophy_on_spawn,
    .on_update  = trophy_on_update,
    .on_trigger = trophy_on_trigger,
    .flags      = ENTITY_FLAG_TRIGGER,
};
