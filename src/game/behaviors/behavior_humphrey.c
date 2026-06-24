/* behavior_humphrey.c — C3DHumphrey (3HUM), the clone-controller enemy leaf.
 *
 * C3DHumphrey derives from C3DEnemy -> C3DPickupType -> C3DAI. Its defining,
 * confirmed behavior (docs/decomp/C3DHumphrey.md) is NOT a normal walking
 * enemy — it is a scripted clone controller:
 *
 *   - PostLoadHideHumphrey (slot 259): reads SCENE, then unconditionally HIDES
 *     itself through the inherited enable/visibility slot 0x110(false). Every
 *     placed Humphrey (the "preclone"/"C3DHUMPHREY" controller AND the
 *     "clone1".."clone7" rows) starts hidden after load.
 *   - ActivateHumphreyClonesScene90 (slot 265) / PrepareHumphreyClonesScene90
 *     (vtable-4 slot 90): when the global task value SCENE == 0x5a (90), find
 *     CLONE1..CLONE7 by object tag, show each, and toggle clone slot 0x124.
 *
 * Native model (the safe, faithful core the handoff calls for):
 *   - on_spawn HIDES the Humphrey (matches PostLoadHideHumphrey) — strictly more
 *     faithful than the prior fall-through, which drew an idle humpstop mesh that
 *     the original never shows at boot.
 *   - The clone-reveal gate is wired exactly as decompiled: when SCENE == 0x5a,
 *     reveal self + CLONE1..CLONE7 (case-insensitive tag match — the rows author
 *     lowercase "clone1" while the hooks look up uppercase "CLONE1"). This is
 *     DORMANT in the current runtime: the only SCENE source is the CTaskList
 *     initial table (SCENE=30) and there is no ported SCENE sequencer, so SCENE
 *     never reaches 90 and the Humphreys stay hidden — which is the correct
 *     boot-state for the clone-boss scene.
 *
 * Deferred: the clone slot-0x124 toggle, Humphrey's C3DEnemy chase/attack combat,
 * and the shrink/grow animation set (HISHRINK/HIGROW/...) — none are reachable
 * until the SCENE sequencer is ported, so they are intentionally not wired here.
 *
 * Shrink note (game-owner ground truth 2026-06-23): like the C3DAI creature leaves,
 * Humphrey is a shrink-ray target — the shrink ray makes him small and he becomes a
 * moving pickup (hence C3DPickupType in the chain + the HISHRINK frame). The active
 * mechanic is deferred for the same reason (un-decompiled transition; 3SHR unplaced);
 * recorded in docs/decomp/C3DShrinkRay.md + behavior_creature.c.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "behavior_ai.h"
#include "../game_flow.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define HUMPHREY_SCENE_CLONES 0x5a   /* SCENE value that activates the clones */

static void humphrey_hide(Entity *e) {
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
}

static void humphrey_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    /* PostLoadHideHumphrey: Humphrey always hides itself after post-load. */
    humphrey_hide(e);
    e->user_flag = 0;   /* 0 = hidden/dormant, 1 = activated (SCENE==0x5a) */
}

/* ActivateHumphreyClonesScene90: reveal the CLONE1..CLONE7 actors by tag. */
static void humphrey_reveal_clones(World *w) {
    static const char *clone_tags[] = {
        "CLONE1", "CLONE2", "CLONE3", "CLONE4", "CLONE5", "CLONE6", "CLONE7"
    };
    for (int i = 0; i < 7; i++) {
        for (Entity *c = w->head; c; c = c->next) {
            if (!c->alive || !c->tag[0]) continue;
            if (strcasecmp(c->tag, clone_tags[i]) != 0) continue;
            c->visible = 1;             /* clone.enable_or_visibility(true) */
            c->user_flag = 1;
            /* clone slot 0x124(true/false) toggle is deferred (unnamed). */
        }
    }
}

static void humphrey_on_update(Entity *e, World *w, float dt) {
    (void)dt;
    /* The clone scene gate. SCENE never reaches 0x5a in the current runtime
       (no ported SCENE sequencer), so this stays dormant and Humphrey hidden. */
    long scene = game_flow_entity_state("SCENE");
    if (scene != HUMPHREY_SCENE_CLONES) {
        humphrey_hide(e);
        return;
    }

    /* Dormant active branch: the clone boss scene is live. Reveal self + clones
       and hold position (combat/animation deferred — see file header). */
    if (!e->user_flag) {
        e->user_flag = 1;
        e->visible = 1;
        humphrey_reveal_clones(w);
        printf("[HUMPHREY] '%s' activated (SCENE==0x5a)\n", e->tag);
    }
    behavior_ai_idle(e);
}

const EntityVTable vt_humphrey = {
    .on_spawn   = humphrey_on_spawn,
    .on_update  = humphrey_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
