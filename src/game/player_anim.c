#include "player_anim.h"
#include "../engine/assets/asset_cache.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static const char *POSE_PATHS[PA_COUNT] = {
    "assets/ase/jimstop.ase",
    "assets/ase/jimrun.ASE",
    "assets/ase/jimleft.ASE",
    "assets/ase/jimright.ASE",
    "assets/ase/jimbackpedal.ASE",
    "assets/ase/jimjump.ASE",
    "assets/ase/jimfall.ASE",
    "assets/ase/jimpickup.ASE",
    "assets/ase/jimswing.ASE",
    "assets/ase/jimladder.ASE",
};

static unsigned int g_shared_tex = 0;
static int          g_loaded[PA_COUNT];
static PlayerAnim   g_current_anim = PA_IDLE;
static float        g_clip_time = 0.0f;

static void copy_shared_jimmy_uvs(AseModel *dst, const AseModel *src) {
    if (!dst || !src || !dst->frames || !src->frames) return;
    if (dst->vertex_count != src->vertex_count) return;

    size_t verts = (size_t)dst->vertex_count;
    const float *uv_src = src->frames;
    for (int fr = 0; fr < dst->frame_count; fr++) {
        float *frame = dst->frames + (size_t)fr * verts * 8u;
        for (size_t v = 0; v < verts; v++) {
            frame[v * 8u + 3u] = uv_src[v * 8u + 3u];
            frame[v * 8u + 4u] = uv_src[v * 8u + 4u];
        }
    }
}

static void bind_shared_jimmy_texture(AseModel *m) {
    if (!m || !g_shared_tex) return;
    m->texture_id = g_shared_tex;
    for (int k = 0; k < m->material_count; k++) {
        if (!m->materials[k].texture_id)
            m->materials[k].texture_id = g_shared_tex;
        m->materials[k].diffuse[0] = 1.0f;
        m->materials[k].diffuse[1] = 1.0f;
        m->materials[k].diffuse[2] = 1.0f;
    }
}

static void ensure_shared_jimmy_clip_data(PlayerAnim a, AseModel *m) {
    bind_shared_jimmy_texture(m);
    if (!m || a == PA_IDLE) return;
    AseModel *idle = model_cache_get(POSE_PATHS[PA_IDLE]);
    bind_shared_jimmy_texture(idle);
    copy_shared_jimmy_uvs(m, idle);
}

static int anim_loops(PlayerAnim a) {
    return a == PA_IDLE || a == PA_RUN || a == PA_LEFT ||
           a == PA_RIGHT || a == PA_BACKPEDAL || a == PA_FALL ||
           a == PA_SWING || a == PA_LADDER;
}

int player_anim_init(unsigned int shared_texture_id) {
    g_shared_tex = shared_texture_id;
    int n = 0;
    for (int i = 0; i < PA_COUNT; i++) {
        AseModel *m = model_cache_get(POSE_PATHS[i]);
        g_loaded[i] = (m != NULL);
        if (m) {
            bind_shared_jimmy_texture(m);
            n++;
        } else {
            fprintf(stderr, "player_anim: failed to load %s\n", POSE_PATHS[i]);
        }
    }

    AseModel *idle = model_cache_get(POSE_PATHS[PA_IDLE]);
    if (idle) {
        for (int i = 0; i < PA_COUNT; i++) {
            if (i == PA_IDLE || !g_loaded[i]) continue;
            AseModel *m = model_cache_get(POSE_PATHS[i]);
            ensure_shared_jimmy_clip_data((PlayerAnim)i, m);
        }
    }
    return n;
}

void player_anim_destroy(void) {
    /* Models are owned by asset_cache now — asset_cache_destroy_all handles them. */
    for (int i = 0; i < PA_COUNT; i++) g_loaded[i] = 0;
    g_shared_tex = 0;
    g_current_anim = PA_IDLE;
    g_clip_time = 0.0f;
}

const AseModel *player_anim_model(PlayerAnim a) {
    if (a < 0 || a >= PA_COUNT) a = PA_IDLE;
    AseModel *m = model_cache_get(POSE_PATHS[g_loaded[a] ? a : PA_IDLE]);
    if (m) {
        ensure_shared_jimmy_clip_data(a, m);
        return m;
    }
    for (int i = 0; i < PA_COUNT; i++) {
        if (g_loaded[i]) {
            AseModel *fb = model_cache_get(POSE_PATHS[i]);
            if (fb) {
                ensure_shared_jimmy_clip_data((PlayerAnim)i, fb);
                return fb;
            }
        }
    }
    return NULL;
}

void player_anim_advance(PlayerAnim a, float dt) {
    if (a < 0 || a >= PA_COUNT) a = PA_IDLE;
    if (a != g_current_anim) {
        g_current_anim = a;
        g_clip_time = 0.0f;
    } else if (dt > 0.0f) {
        g_clip_time += dt;
    }
}

PlayerAnimSample player_anim_sample(PlayerAnim a) {
    PlayerAnimSample s;
    s.model = player_anim_model(a);
    s.frame_a = 0;
    s.frame_b = 0;
    s.lerp = 0.0f;
    if (!s.model || s.model->frame_count <= 1) return s;

    float fps = s.model->framespeed > 0.0f ? s.model->framespeed : 10.0f;
    float frame_pos = g_clip_time * fps;
    int base = (int)floorf(frame_pos);
    if (anim_loops(a)) {
        s.frame_a = base % s.model->frame_count;
        s.frame_b = (s.frame_a + 1) % s.model->frame_count;
        s.lerp = frame_pos - floorf(frame_pos);
    } else {
        int last = s.model->frame_count - 1;
        if (base >= last) {
            s.frame_a = last;
            s.frame_b = last;
            s.lerp = 0.0f;
        } else {
            s.frame_a = base;
            s.frame_b = base + 1;
            s.lerp = frame_pos - floorf(frame_pos);
        }
    }
    return s;
}
