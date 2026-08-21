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

/* The matched start point's StartTrigger, for the caller to fire once the
   level's entities are bound. See spawn.h for why this is only recorded here
   and not acted on. */
static char g_start_trigger[64];

const char *spawn_start_trigger(void) { return g_start_trigger; }

/* Place player at the named STRT in the world (case-insensitive on tag).
   Falls back to the 3JIM spawn if start_point is empty or not found. */
Entity *place_player(World *world, const char *start_point) {
    g_start_trigger[0] = '\0';
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
    if (selected_start) {
        /* gam_loader filters an authored "none" out of the string bag, so a
           start point that authors one leaves this NULL. Of the corpus's 100
           STRT rows, 67 author "none" and 2 omit the property; the other 31
           name something. */
        const char *st = gam_str(selected_start, "StartTrigger", NULL);
        if (st && st[0])
            snprintf(g_start_trigger, sizeof g_start_trigger, "%s", st);
    }
    jim->vx = jim->vy = jim->vz = 0.0f;
    jim->on_ground = 0;
    return jim;
}
