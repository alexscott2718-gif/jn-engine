/* CLoadLevel (FourCC LOAD) -- the in-world level-transition portal.
 *
 * Ported from the recovered contact/gate body at 00457ec0
 * (docs/decomp/evidence/cloadlevel_gate_00457ec0.md), with the field map and
 * ActivateLoad (00458370) in docs/decomp/CLoadLevel.md. What was here before
 * was a functional bridge: it forwarded LevelName/StartPoint and applied a
 * RequiredLevel/ExactLevel window the original does not use. The recovered
 * order is:
 *
 *     if !toucher.IsA("C3DJIMMY"):            return
 *     if RequiredTask != "none":
 *         state = task_state(RequiredTask)     # -1 when the task is missing
 *         if state != -1:
 *             if state < RequiredLevel:                    return
 *             if ExactLevel != -1 and state != ExactLevel: return
 *         else:
 *             log "ERROR: Task %s not found in in %s"      # and CONTINUE
 *     if LevelName == "RETURN":  return_loadpoint_path();  return
 *     if LevelName == "none":                              return
 *     hide_this_portal()                                   # slot 0xd8
 *     jimmy_load_handoff(LevelName, StartPoint, <departure level>, <departure spawn>)
 *
 * Three of those branches are new here, and each has shipped rows behind it
 * (counted over the 35 shipped levels in assets/gam -- 97 LOAD rows):
 *
 *   RETURN (10 rows, every one of them in VR01..VR08) sends Jimmy back where
 *   he came from instead of to a named level. Nothing in the corpus loads a VR
 *   level through a portal, so the departure point those rows read is recorded
 *   by whatever route entered the VR level -- the main menu. See
 *   gamestate_request_level_swap().
 *
 *   LevelName == "none" (1 row: level4.gam's C3DLOADLEVEL, which authors
 *   StartPoint="level1.gam" -- the designer filled the two fields in the wrong
 *   order) does nothing at all. Native used to request a swap to a level named
 *   "none" and log a resolve failure.
 *
 *   RequiredLevel and ExactLevel are BOTH checked, and only inside the
 *   RequiredTask branch. The shared native window helper (behavior_base.c
 *   level_window_allows) gives ExactLevel precedence over RequiredLevel and
 *   treats RequiredLevel == -1 as blocked; the recovered body does neither,
 *   and applies neither test when RequiredTask is "none". Rows this separates:
 *   Level3.gam's carlcapt authors RequiredLevel 380 AND ExactLevel 0, which
 *   under the recovered body can never both hold -- that portal is dead in the
 *   original and live under the window helper. level1c.gam's yokian2 authors
 *   ExactLevel 0 with RequiredTask "none", so the original never reads it.
 *
 * Deliberately NOT ported, all of it unrecovered or absent natively:
 *   - the DAT_004f0588 game-mode switch (cases 0/1/4/7) that calls player slot
 *     0x178 before the hide, and the trailing player slots 0x11c / 0x2c4;
 *   - SoundIndex (this+0x614) and FadeType/FadeTime (this+0x61c/+0x620): the
 *     engine has no fade, and 95 of 97 rows author SoundIndex -1.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../game_flow.h"
#include "../gamestate.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* The story counter the gate compares against. FUN_0045fea0 looks the task up
   in the CTaskList store and returns -1 when it is not there.

   JN_PROGRESS_LEVEL has no original counterpart: it is the native probe seam
   standing in for the store on a direct --level run, kept so the existing
   progress probes still reach this gate. It is consulted only when no store
   answers -- the same case the original logs and passes through -- so with the
   seam unset (every normal run, every check) this is the recovered rule. */
static long load_task_state(const char *task) {
    long state = game_flow_entity_state(task);
    if (state >= 0) return state;
    if (behavior_progress_gate_enabled()) return behavior_progress_level();
    return -1;
}

/* The 00457ec0 gate. 1 = the touch proceeds. */
int behavior_load_gate_allows(const Entity *e) {
    if (!e) return 0;
    /* gam_loader filters an authored "none" out of the string bag, so an
       absent RequiredTask here IS the original __strcmpi(...,"none") skip.
       The explicit compare stays for a value that survives the filter. */
    const char *task = gam_str(e, "RequiredTask", NULL);
    if (!task || !task[0] || strcasecmp(task, "none") == 0) return 1;

    long state = load_task_state(task);
    if (state < 0) {
        /* Missing task: the original logs and falls through to the
           transition. A cold --level entry has no store, so every gated
           portal is live -- unchanged from the previous native behavior. */
        return 1;
    }
    if (state < (long)gam_prop_i(e, "RequiredLevel", -1)) return 0;
    int exact = gam_prop_i(e, "ExactLevel", -1);
    if (exact != -1 && state != (long)exact) return 0;
    return 1;
}

static void load_on_spawn(Entity *e, World *w) {
    (void)w;
    /* Radius sizes the inherited contact volume. The previous code replaced an
       authored radius <= 0 with 60; nothing in the decomp supports that, and
       it changes exactly one shipped row (Level2.gam rockspaceb, authored 0.0,
       whose neighbours in the corpus author 1.0 -- these read as designer-
       disabled portals, not as unset). An unauthored Radius still defaults. */
    float radius = gam_prop_f(e, "Radius", 60.0f);
    behavior_trigger_spawn_base(e, radius, radius, radius);
    /* No visibility gate here. The recovered body evaluates the prerequisites
       on contact and never touches the portal until it fires; the old
       load_refresh_gate hid the portal and dropped ENTITY_FLAG_TRIGGER while
       the gate was shut. LOAD has no visual (entity_visual.c TYPE_TABLE), so
       what that actually decided was contact -- at the wrong time and by the
       wrong rule. */
}

/* ActivateLoad (00458370) hides the portal via slot 0xd8 before handing the
   request to the loader; that hide is the original's own re-entry guard, and
   user_flag is the native latch that goes with it. */
static void load_hide(Entity *e) {
    e->user_flag = 1;
    e->visible = 0;
    e->runtime_flags &= ~ENTITY_FLAG_TRIGGER;
}

static void load_on_trigger(Entity *e, Entity *by) {
    if (!e || !by) return;
    /* IsA("C3DJIMMY"): the engine's trigger dispatch (physics.c) only ever
       passes the player, so the identity test is satisfied upstream and is not
       duplicated here. */
    if (e->user_flag) return;
    if (!behavior_load_gate_allows(e)) return;

    const char *level = e->target_level;

    if (strcasecmp(level, "RETURN") == 0) {
        if (gamestate_request_level_return())
            printf("[LOAD] RETURN portal %s -> %s (spawn %s)\n",
                   e->tag, gamestate_return_level(), gamestate_return_spawn());
        else
            /* No departure point recorded: this level was entered directly
               (--level VRxx) rather than through a swap. Native-defined; the
               original would hand the loader whatever its player fields held. */
            printf("[LOAD] RETURN portal %s: no recorded departure point\n", e->tag);
        e->user_flag = 1;
        return;
    }

    if (!level[0] || strcasecmp(level, "none") == 0) {
        printf("[LOAD] portal %s authors LevelName '%s' -- inert\n", e->tag, level);
        e->user_flag = 1;
        return;
    }

    printf("[LOAD] level change requested by %s (tag=%s -> %s spawn=%s)\n",
           by->type, e->tag, level, e->start_point);
    load_hide(e);
    gamestate_request_level_swap(level, e->start_point);
}

const EntityVTable vt_load = {
    .on_spawn = load_on_spawn,
    .on_trigger = load_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
