/* Headless dumper for the C3DStartPoint linkage oracle
   (docs/linked_parity_plan.md L3). Loads a real shipped .gam with the real,
   unmodified gam_load(), then drives the real, unmodified place_player()
   (src/game/spawn.c -- extracted from main.c for exactly this harness) with
   the request tags given on the command line, and dumps every observable:
   the player's post-place pose bits, the velocity/on-ground reset, and the
   MusicDatabase/MusicIndex selection.

   Stubs audio_set_music_db (records the call instead of touching the mixer).
   world_add()/world_find_type() are reproduced byte-for-byte from
   src/engine/world.c, same reason as the earlier gam_load-based dumpers
   (world.c drags in renderer/physics deps).

   Request forms: a literal tag, or "@default" for place_player(w, NULL) (the
   player's own authored StartPoint request). Before each request the player
   pose is restored to its as-loaded values and the velocity/on-ground fields
   are poisoned, so requests are independent and the reset is observable.

   Build: cc tools/linkage_oracles/c3dstartpoint_dump.c src/game/spawn.c \
             src/engine/assets/gam_loader.c src/engine/player_physics.c -lm
   Run:   <out> <file.gam> <request>...  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/spawn.h"
#include "../../src/engine/assets/gam_loader.h"

static char g_music_db[128];
static int  g_music_handle = -999;
static int  g_music_called = 0;

int audio_set_music_db(const char *db, int handle) {
    snprintf(g_music_db, sizeof(g_music_db), "%s", db ? db : "");
    g_music_handle = handle;
    g_music_called++;
    return 0;
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

Entity *world_find_type(const World *w, const char *type) {
    for (Entity *e = w->head; e; e = e->next)
        if (strcmp(e->type, type) == 0) return e;
    return NULL;
}

static unsigned int f32_bits(float f) {
    unsigned int u;
    memcpy(&u, &f, 4);
    return u;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.gam> [request...]\n", argv[0]);
        return 2;
    }
    World w;
    memset(&w, 0, sizeof w);
    if (gam_load(&w, argv[1]) < 0) {
        fprintf(stderr, "gam_load failed on %s\n", argv[1]);
        return 1;
    }

    /* On-disk-order STRT table exactly as the real loader stored it. */
    Entity **arr = malloc(sizeof(Entity *) * (size_t)(w.count > 0 ? w.count : 1));
    int i = 0;
    for (Entity *e = w.head; e; e = e->next) arr[i++] = e;
    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        if (strncmp(e->type, "STRT", 4) != 0) continue;
        printf("T|%s|%08x|%08x|%08x|%08x|%08x|%08x|%s|%d\n",
               e->tag, f32_bits(e->x), f32_bits(e->y), f32_bits(e->z),
               f32_bits(e->rx), f32_bits(e->ry), f32_bits(e->rz),
               e->music_database, gam_prop_i(e, "MusicIndex", -1));
    }
    free(arr);

    Entity *jim = world_find_type(&w, "3JIM");
    if (!jim) {
        printf("J|none\n");
        return 0;
    }
    printf("J|%s|%s|%08x|%08x|%08x|%08x|%08x|%08x\n",
           jim->tag, jim->start_point,
           f32_bits(jim->x), f32_bits(jim->y), f32_bits(jim->z),
           f32_bits(jim->rx), f32_bits(jim->ry), f32_bits(jim->rz));

    float jx = jim->x, jy = jim->y, jz = jim->z;
    float jrx = jim->rx, jry = jim->ry, jrz = jim->rz;

    for (int a = 2; a < argc; a++) {
        const char *req = argv[a];
        jim->x = jx; jim->y = jy; jim->z = jz;
        jim->rx = jrx; jim->ry = jry; jim->rz = jrz;
        jim->vx = jim->vy = jim->vz = 123.5f;   /* poison: reset must clear */
        jim->on_ground = 7;
        g_music_called = 0;
        g_music_db[0] = '\0';
        g_music_handle = -999;

        Entity *ret = place_player(&w, strcmp(req, "@default") == 0 ? NULL : req);

        printf("R|%d|%s|%d|%08x|%08x|%08x|%08x|%08x|%08x|%08x|%08x|%08x|%d|%d|%s|%d\n",
               a - 2, req, ret == jim,
               f32_bits(jim->x), f32_bits(jim->y), f32_bits(jim->z),
               f32_bits(jim->rx), f32_bits(jim->ry), f32_bits(jim->rz),
               f32_bits(jim->vx), f32_bits(jim->vy), f32_bits(jim->vz),
               jim->on_ground, g_music_called, g_music_db, g_music_handle);
    }
    return 0;
}
