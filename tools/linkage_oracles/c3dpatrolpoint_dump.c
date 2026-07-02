/* Headless dumper for the C3DPatrolPoint linkage oracle
   (docs/linked_parity_plan.md L3). Loads a real shipped .gam file with the
   real, unmodified gam_load() + behavior_ai_find_patrol_point()/gam_prop_f()
   and dumps, for every placed 3PAT entity, its resolved NextPatrolPoint edge
   (case-insensitive same-level tag lookup) and WaitTime -- the "next-select"
   half of C3DPatrolPoint::OnArrive (00434ea0).

   Stubs behavior_animated_spawn_base (no-op) and behavior_animated_update_base
   (always-enabled) -- both are level-gate/animation bookkeeping unrelated to
   patrol graph resolution, same spirit as the CLoadLevel oracle's local
   world_add(). world_add() itself is reproduced byte-for-byte from
   src/engine/world.c for the same headless-without-renderer reason. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behavior_ai.h"
#include "../../src/engine/assets/gam_loader.h"

void behavior_animated_spawn_base(Entity *e) { (void)e; }
int behavior_animated_update_base(Entity *e, World *w, float dt) {
    (void)e; (void)w; (void)dt;
    return 1;
}

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

static unsigned int f32_bits(float f) {
    unsigned int u;
    memcpy(&u, &f, 4);
    return u;
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

    /* w.head is push-front order; collect + reverse to on-disk object order,
       same convention as the CLoadLevel oracle. */
    Entity **arr = malloc(sizeof(Entity *) * (size_t)(w.count > 0 ? w.count : 1));
    int i = 0;
    for (Entity *e = w.head; e; e = e->next) arr[i++] = e;

    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        int file_idx = w.count - 1 - k;
        if (strncmp(e->type, "3PAT", 4) != 0) continue;

        Entity *next = behavior_ai_find_patrol_point(&w, e->next_patrol);
        float wait = gam_prop_f(e, "WaitTime", 0.0f);
        printf("P|%d|%s|%s|%s|%08x\n", file_idx, e->tag, e->next_patrol,
               next ? next->tag : "", f32_bits(wait));
    }

    free(arr);
    return 0;
}
