/* Player spawn placement -- native counterpart of C3DStartPoint::PlacePlayer
   (Neutron.exe @ 00442740, docs/decomp/C3DStartPoint.md). Extracted verbatim
   from main.c (2026-07-02, linked-parity pass) so the STRT-resolution logic
   is reachable by the headless L3 oracle
   (tools/linkage_oracles/C3DStartPoint.py) without dragging in the
   window/GL/audio init path. Behavior unchanged. */
#include "spawn.h"

#include "../engine/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Place player at the named STRT in the world (case-insensitive on tag).
   Falls back to the 3JIM spawn if start_point is empty or not found. */
Entity *place_player(World *world, const char *start_point) {
    Entity *jim = world_find_type(world, "3JIM");
    if (!jim) return NULL;
    const char *want = start_point;
    if (!want || !want[0]) want = jim->start_point;
    const char *explicit_spawn = getenv("JN_DEMO_SPAWN_XYZ");
    int preserve_position = (explicit_spawn && explicit_spawn[0] && (!start_point || !start_point[0]));
    Entity *selected_start = NULL;
    if (want && want[0]) {
        for (Entity *e = world->head; e; e = e->next) {
            if (strncmp(e->type, "STRT", 4) != 0) continue;
            if (strcasecmp(e->tag, want) == 0) {
                if (!preserve_position) {
                    jim->x = e->x; jim->y = e->y; jim->z = e->z;
                    jim->rx = e->rx; jim->ry = e->ry; jim->rz = e->rz;
                }
                selected_start = e;
                printf("[SPAWN] using STRT '%s' at (%.1f, %.1f, %.1f) ry=%.2f\n",
                       e->tag, e->x, e->y, e->z, e->ry);
                break;
            }
        }
    }
    if (selected_start && selected_start->music_database[0]) {
        int music_index = gam_prop_i(selected_start, "MusicIndex", -1);
        audio_set_music_db(selected_start->music_database, music_index);
    }
    jim->vx = jim->vy = jim->vz = 0.0f;
    jim->on_ground = 0;
    return jim;
}
