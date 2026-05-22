#include "player_anim.h"
#include "../engine/assets/asset_cache.h"
#include <stdio.h>
#include <string.h>

static const char *POSE_PATHS[PA_COUNT] = {
    "assets/ase/jimstop.ase",
    "assets/ase/jimrun.ASE",
    "assets/ase/jimjump.ASE",
    "assets/ase/jimfall.ASE",
    "assets/ase/jimpickup.ASE",
};

static unsigned int g_shared_tex = 0;
static int          g_loaded[PA_COUNT];

int player_anim_init(unsigned int shared_texture_id) {
    g_shared_tex = shared_texture_id;
    int n = 0;
    for (int i = 0; i < PA_COUNT; i++) {
        AseModel *m = model_cache_get(POSE_PATHS[i]);
        g_loaded[i] = (m != NULL);
        if (m) {
            m->texture_id = shared_texture_id;
            n++;
        } else {
            fprintf(stderr, "player_anim: failed to load %s\n", POSE_PATHS[i]);
        }
    }
    return n;
}

void player_anim_destroy(void) {
    /* Models are owned by asset_cache now — asset_cache_destroy_all handles them. */
    for (int i = 0; i < PA_COUNT; i++) g_loaded[i] = 0;
    g_shared_tex = 0;
}

const AseModel *player_anim_model(PlayerAnim a) {
    if (a < 0 || a >= PA_COUNT) a = PA_IDLE;
    AseModel *m = model_cache_get(POSE_PATHS[g_loaded[a] ? a : PA_IDLE]);
    if (m) {
        if (!m->texture_id && g_shared_tex) m->texture_id = g_shared_tex;
        return m;
    }
    for (int i = 0; i < PA_COUNT; i++) {
        if (g_loaded[i]) {
            AseModel *fb = model_cache_get(POSE_PATHS[i]);
            if (fb) return fb;
        }
    }
    return NULL;
}
