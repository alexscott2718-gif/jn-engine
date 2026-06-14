/* behavior_music.c — native port of C3DMusicTrigger (FourCC 3MUS).
 *
 * From docs/decomp/C3DMusicTrigger.md + shipped .gam data (11/11 props): a
 * proximity music switch. When the player enters `Radius` it changes the
 * background music to one of up to five indexed tracks (`MusicIndex0..4`) from
 * `MusicDatabase`. `TouchActivated` is the arm/fire state; RequiredLevel /
 * ExactLevel / RemoveLevel gate whether the trigger is live.
 *
 * Native realisation: faithful proximity edge-trigger. MusicIndex values are
 * OMT Wave handles in MusicDatabase. Which of MusicIndex0..4 is selected, and
 * on what condition, is still an open decomp question; until the condition is
 * recovered we pick the first authored non-negative slot, which matches the
 * Level5a musicship trigger rows where slot 2 is the only live choice.
 * The level gates are read but not enforced (no global level-progress counter
 * yet).
 *
 * State: user_float = inside-edge (0/1).
 */
#include "behaviors.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <math.h>
#include <stdio.h>

static int selected_music_index(const Entity *e) {
    for (int i = 0; i < 5; i++) {
        char name[16];
        snprintf(name, sizeof(name), "MusicIndex%d", i);
        int v = gam_prop_i(e, name, -1);
        if (v >= 0) return v;
    }
    return -1;
}

static void music_on_spawn(Entity *e, World *w) {
    (void)w;
    e->user_float = 0.0f;   /* outside */
}

static void music_on_update(Entity *e, World *w, float dt) {
    (void)w; (void)dt;
    if (!g_player) return;

    float radius = gam_prop_f(e, "Radius", 200.0f);
    if (radius <= 0.0f) return;

    float dx = g_player->x - e->x;
    float dz = g_player->z - e->z;
    int inside = (dx*dx + dz*dz) < radius*radius;
    int was_inside = e->user_float > 0.5f;

    if (inside && !was_inside) {
        int track = selected_music_index(e);
        if (track >= 0)
            audio_set_music_db(e->music_database, track);
    }
    e->user_float = inside ? 1.0f : 0.0f;
}

const EntityVTable vt_music = {
    .on_spawn   = music_on_spawn,
    .on_update  = music_on_update,
    .on_trigger = NULL,
    .flags      = 0,
};
