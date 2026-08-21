/* Headless dumper for the CLoadLevel contact-gate linkage oracle.
   Loads a real shipped .gam with the real, unmodified gam_load(), then asks
   the real, unmodified behavior_load_gate_allows() (src/game/behaviors/
   behavior_load.c, ported from the recovered contact body at 00457ec0) for a
   verdict on every placed LOAD row at each story state named on the command
   line.

   The story state comes from the real CTaskList store: `none` unloads it
   (game_flow_end_campaign), any integer seeds the SCENE entry through
   game_flow_test_seed_state, which is the same path a campaign write takes.
   Tasks that are not in the store -- level4.gam authors RequiredTask
   "tunneldt" -- therefore exercise the missing-task branch for real.

   JN_PROGRESS_LEVEL, the native-only probe seam behavior_load.c consults when
   no store answers, is deliberately left unset here: it has no counterpart in
   the recovered body, and every run this oracle certifies is a run without it.

   Emits, in on-disk object order:
     T|<file-index>|<ObjectTag>
     G|<file-index>|<state>|<0 blocked | 1 allowed>

   Stubs: behavior_trigger_spawn_base (spawn is not part of this aspect and
   the gate reads only .gam properties), the audio/visual leaves the linked
   translation units reach for, and world_add, reproduced the same
   headless-without-renderer way the other .gam-driven dumpers do it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/behaviors/behavior_base.h"
#include "../../src/game/game_flow.h"
#include "../../src/game/gamestate.h"
#include "../../src/engine/assets/gam_loader.h"

Entity *g_player = NULL;

/* --- stubs --------------------------------------------------------------
   behavior_base.c is not linked: it would drag the animation dispatcher and
   the whole animated lifecycle in for two accessors and a setter that this
   aspect does not certify. The two progress-seam accessors report the seam
   OFF, which is what an unset JN_PROGRESS_LEVEL means in the real binary. */
void behavior_trigger_spawn_base(Entity *e, float hx, float hy, float hz) {
    e->half_extents[0] = hx;
    e->half_extents[1] = hy;
    e->half_extents[2] = hz;
    e->runtime_flags = ENTITY_FLAG_TRIGGER;
}
int behavior_progress_gate_enabled(void) { return 0; }
int behavior_progress_level(void) { return 0; }

/* gamestate.c is linked for the swap/return bookkeeping the gate does
   not touch; its sandbox toggle reaches the visual layer, which no
   headless dumper links. */
int  entity_visual_sandbox_enabled(void) { return 0; }
void entity_visual_set_sandbox(int on) { (void)on; }

Entity *world_add(World *w) {
    Entity *e = calloc(1, sizeof(Entity));
    if (!e) return NULL;
    e->alive = 1;
    e->visible = 1;
    e->omt_index = -1;
    e->next = w->head;
    w->head = e;
    w->count++;
    return e;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <gam-file> <state|none> ...\n", argv[0]);
        return 1;
    }
    World w;
    memset(&w, 0, sizeof w);
    gamestate_init();

    int n = gam_load(&w, argv[1]);
    if (n < 0) {
        fprintf(stderr, "gam_load failed on %s\n", argv[1]);
        return 1;
    }

    /* w.head is push-front order; walk it backwards for on-disk order. */
    Entity **arr = malloc(sizeof(Entity *) * (size_t)(w.count > 0 ? w.count : 1));
    int i = 0;
    for (Entity *e = w.head; e; e = e->next) arr[i++] = e;

    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        if (strncmp(e->type, "LOAD", 4) != 0) continue;
        printf("T|%d|%s\n", w.count - 1 - k, e->tag);
    }

    for (int a = 2; a < argc; a++) {
        game_flow_end_campaign();          /* store unloaded: every task missing */
        if (strcmp(argv[a], "none") != 0)
            game_flow_test_seed_state("SCENE", strtol(argv[a], NULL, 0));
        for (int k = w.count - 1; k >= 0; k--) {
            Entity *e = arr[k];
            if (strncmp(e->type, "LOAD", 4) != 0) continue;
            printf("G|%d|%s|%d\n", w.count - 1 - k, argv[a],
                   behavior_load_gate_allows(e) ? 1 : 0);
        }
    }
    free(arr);
    return 0;
}
