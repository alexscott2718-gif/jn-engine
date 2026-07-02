/* Headless dumper for the CLoadLevel linkage oracle (docs/linked_parity_plan.md L3).
   Compiles the real gam_loader.c into a tiny CLI that loads one .gam file with
   gam_load() (unmodified) and prints every object's (type,tag) plus, for LOAD
   rows, CLoadLevel's 9 registered properties (docs/decomp/CLoadLevel.md field
   map) exactly as gam_loader.c stored them -- via the same gam_str/gam_prop_f/
   gam_prop_i accessors a ported behavior would use.

   world_add() below is a byte-for-byte copy of src/engine/world.c's real
   implementation (calloc + alive/visible/omt_index defaults + push-front list)
   -- reproduced locally so this dumper stays headless and does not need to
   link world.c's renderer/collision dependencies (glad.h, collision.c), which
   gam_load()'s parse path never touches. */
#include "../../src/engine/assets/gam_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if (argc < 2) {
        fprintf(stderr, "usage: %s <gam-file>\n", argv[0]);
        return 1;
    }
    World w;
    memset(&w, 0, sizeof w);

    int n = gam_load(&w, argv[1]);
    if (n < 0) {
        fprintf(stderr, "gam_load failed on %s\n", argv[1]);
        return 1;
    }

    Entity **arr = malloc(sizeof(Entity *) * (size_t)(w.count > 0 ? w.count : 1));
    int i = 0;
    for (Entity *e = w.head; e; e = e->next) arr[i++] = e;

    /* arr[0] is the most-recently-loaded (push-front list); walk it back to
       front so file_idx ascends 0..count-1 in on-disk object order. */
    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        int file_idx = w.count - 1 - k;
        printf("E|%d|%s|%s\n", file_idx, e->type, e->tag);
        if (strcmp(e->type, "LOAD") == 0) {
            float radius = gam_prop_f(e, "Radius", 0.0f);
            float fadetime = gam_prop_f(e, "FadeTime", 0.0f);
            unsigned int radius_bits, fadetime_bits;
            memcpy(&radius_bits, &radius, 4);
            memcpy(&fadetime_bits, &fadetime, 4);
            printf("L|%d|%s|%s|%s|%d|%d|%08x|%d|%d|%08x\n",
                   file_idx,
                   e->target_level,
                   e->start_point,
                   gam_str(e, "RequiredTask", "none"),
                   gam_prop_i(e, "RequiredLevel", -1),
                   gam_prop_i(e, "ExactLevel", -1),
                   radius_bits,
                   gam_prop_i(e, "SoundIndex", -1),
                   gam_prop_i(e, "FadeType", -1),
                   fadetime_bits);
        }
    }

    free(arr);
    return 0;
}
