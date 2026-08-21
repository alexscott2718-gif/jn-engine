#include "player_anim.h"
#include "animated_dispatch.h"
#include "../engine/assets/asset_cache.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
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
    "assets/ase/jimtalk.ASE",
    "assets/ase/jimbuttons.ASE",
    "assets/ase/jimheadshrink.ASE",
    "assets/ase/jimdrive.ASE",
    "assets/ase/jimfence.ASE",
    "assets/ase/jimsplat.ASE",
    "assets/ase/jimhit.ASE",
    "assets/ase/jimscooter.ASE",
    "assets/ase/jimscooterstop.ASE",
    "assets/ase/jimfly.ASE",
    "assets/ase/jimshoot.ASE",
};

static const char *POSE_DISPATCH_NAMES[PA_COUNT] = {
    "STOP",
    "RUN",
    "LEFT",
    "RIGHT",
    "BACK",
    "JUMP",
    "FALL",
    "PICKUP",
    "SWING",
    "LADDER",
    "TALK",
    "BUTTONS",
    "PLAY",
    "DRIVE",
    "FENCE",
    "SPLAT",
    "HIT",
    "SCOOT",
    "SCOOTSTOP",
    "FLY",
    "SHOOT",
};

static unsigned int g_shared_tex = 0;
static int          g_loaded[PA_COUNT];
static PlayerAnim   g_current_anim = PA_IDLE;
static float        g_clip_time = 0.0f;
static AnimatedClip g_dispatch_clips[PA_COUNT];
static int          g_dispatch_clip_ready[PA_COUNT];

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
           a == PA_SWING || a == PA_TALK ||
           a == PA_DRIVE || a == PA_SCOOT || a == PA_SCOOTSTOP ||
           a == PA_FLY;
}

int player_anim_is_special(PlayerAnim a) {
    return a == PA_FENCE || a == PA_LADDER || a == PA_SPLAT || a == PA_HIT;
}

static const char *dispatch_name(PlayerAnim a) {
    if (a < 0 || a >= PA_COUNT) a = PA_IDLE;
    return POSE_DISPATCH_NAMES[a];
}

static void dispatch_record_name(PlayerAnim a, char *dst, size_t dst_size) {
    snprintf(dst, dst_size, "HI%s", dispatch_name(a));
}

static int refresh_dispatch_clip(PlayerAnim a) {
    if (a < 0 || a >= PA_COUNT) return 0;
    const AseModel *m = player_anim_model(a);
    if (!m) return 0;
    animated_dispatch_clip_from_ase(&g_dispatch_clips[a], m);
    g_dispatch_clip_ready[a] = 1;
    return 1;
}

static int completion_alias(const char *alias) {
    return alias &&
           (strcasecmp(alias, "FENCE") == 0 ||
            strcasecmp(alias, "LADDER") == 0 ||
            strcasecmp(alias, "SPLAT") == 0 ||
            strcasecmp(alias, "HIT") == 0);
}

static void player_anim_dispatch_ended(Entity *e) {
    const char *alias = animated_dispatch_active_alias(e);
    if (completion_alias(alias)) {
        e->user_flag = (int)PA_IDLE;
        (void)animated_dispatch_set_by_name(e, "STOP", 1);
    }
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
    for (int i = 0; i < PA_COUNT; i++) g_dispatch_clip_ready[i] = 0;
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

void player_anim_bind_entity(Entity *e) {
    if (!e) return;

    animated_dispatch_set_key_strings("HI", "HI", "");
    (void)animated_dispatch_init_entity(e);
    animated_dispatch_set_anim_ended_hook(e, player_anim_dispatch_ended);

    for (int i = 0; i < PA_COUNT; i++) {
        char rec_name[64];
        dispatch_record_name((PlayerAnim)i, rec_name, sizeof rec_name);
        if (animated_dispatch_find_record(e, rec_name))
            continue;
        if (!g_dispatch_clip_ready[i] && !refresh_dispatch_clip((PlayerAnim)i))
            continue;
        (void)animated_dispatch_create_record(e, rec_name, &g_dispatch_clips[i]);
    }
}

int player_anim_entity_special_active(const Entity *e) {
    return e && player_anim_is_special((PlayerAnim)e->user_flag);
}

int player_anim_start_entity_state(Entity *e, PlayerAnim a) {
    if (!e || a < 0 || a >= PA_COUNT) return 0;
    PlayerAnim cur = (PlayerAnim)e->user_flag;
    if (player_anim_is_special(cur) && cur != a)
        return 0;
    e->user_flag = (int)a;
    player_anim_bind_entity(e);
    AnimatedDispatchResult r =
        animated_dispatch_set_by_name(e, dispatch_name(a), anim_loops(a));
    return r != ANIMATED_DISPATCH_NOT_FOUND;
}

void player_anim_advance_entity(Entity *e, PlayerAnim a, float dt) {
    if (!e) {
        player_anim_advance(a, dt);
        return;
    }
    if (a < 0 || a >= PA_COUNT) a = PA_IDLE;
    if (player_anim_entity_special_active(e) && !player_anim_is_special(a))
        a = (PlayerAnim)e->user_flag;

    player_anim_bind_entity(e);
    const char *name = dispatch_name(a);
    const char *active = animated_dispatch_active_alias(e);
    int changed = !active || strcasecmp(active, name) != 0;

    if (changed) {
        AnimatedDispatchResult r =
            animated_dispatch_set_by_name(e, name, anim_loops(a));
        if (r == ANIMATED_DISPATCH_NOT_FOUND) {
            player_anim_advance(a, dt);
        }
        return;
    }

    if (dt > 0.0f) {
        animated_dispatch_update(e, dt);
    }
}

void player_anim_react_to_damage(Entity *e, int is_down) {
    (void)player_anim_start_entity_state(e, is_down ? PA_SPLAT : PA_HIT);
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

PlayerAnimSample player_anim_sample_entity(const Entity *e, PlayerAnim a) {
    PlayerAnimSample s;
    s.model = player_anim_model(a);
    s.frame_a = 0;
    s.frame_b = 0;
    s.lerp = 0.0f;
    if (!s.model || s.model->frame_count <= 1)
        return s;

    const char *active = animated_dispatch_active_alias(e);
    if (active && strcasecmp(active, dispatch_name(a)) == 0) {
        AnimatedDispatchSample ds;
        if (animated_dispatch_sample(e, &ds)) {
            s.frame_a = ds.frame_a;
            s.frame_b = ds.frame_b;
            s.lerp = ds.lerp;
            return s;
        }
    }

    return player_anim_sample(a);
}
