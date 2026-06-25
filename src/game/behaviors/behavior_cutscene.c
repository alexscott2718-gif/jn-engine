/* Wave N5 — scripted cutscene cameras.
 *
 * C3DCutSceneCamera (3CAM, docs/decomp/C3DCutSceneCamera.md) is the scripted
 * shot director: each placed 3CAM frames a CameraTarget, offsets the view
 * (TargOffsetX/Y/Z), holds it at a distance (InitialDist, clamped
 * MinDist..MaxDist), applies a vertical look offset (LookVoffset) and eases the
 * distance by ZoomSpeed. C3DMultiCutSceneCamera (3MCA,
 * docs/decomp/C3DMultiCutSceneCamera.md) is the sequencer that steps through up
 * to 8 shots in turn.
 *
 * Native port: each 3CAM registers its shot parameters on spawn; the runtime
 * plays the registered shots in sequence through renderer_set_camera_override.
 * 3CAM/3MCA carry no mesh (invisible in the audit), so the behaviors are inert
 * placeable objects until the sequence is requested. Playback is OFF by default
 * (cutscene_request_intro is opt-in) so the faithfulness audit and the
 * matched-camera validators — which drive this same render loop — are
 * unaffected. 3MCA's CameraType table is recovered from Neutron.exe 00430da0:
 * each step chooses a target-local camera offset, transforms it through the
 * current target, then looks at target.y + LookatVOffset - 60. */

#include "behaviors.h"
#include "behavior_base.h"
#include "../../engine/audio.h"
#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#define CUTSCENE_MAX_SHOTS 32
#define CUTSCENE_MAX_SEQUENCES 32
#define CUTSCENE_MAX_STEPS 8
#define CUTSCENE_DEFAULT_SHOT_SECONDS 3.0f
#define CUTSCENE_AUDIO_PAD_SECONDS 0.35f

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

typedef struct {
    char  target[24];        /* CameraTarget tag (resolved at play time) */
    char  sound_db[64];      /* SoundDatabase for the shot/step */
    int   sound_index;       /* SoundIndex within sound_db */
    float offset[3];         /* TargOffsetX/Y/Z */
    float initial_dist;      /* InitialDist */
    float min_dist, max_dist;/* distance clamp */
    float zoom_speed;        /* ZoomSpeed (distance ease rate) */
    float look_voffset;      /* LookVoffset */
    float hold_seconds;      /* per-step hold, usually audio length + pad */
    int   camera_type;       /* CameraType, currently used only as framing hint */
} CutSceneShot;

typedef struct {
    char target[24];
    char target_anim[32];
    int  sound_index;
    int  camera_type;
    float look_voffset;
} CutSceneStep;

typedef struct {
    char tag[64];
    char sound_db[64];
    CutSceneStep steps[CUTSCENE_MAX_STEPS];
    int count;
} CutSceneSequence;

static struct {
    CutSceneShot shots[CUTSCENE_MAX_SHOTS];
    CutSceneSequence seqs[CUTSCENE_MAX_SEQUENCES];
    CutSceneShot active[CUTSCENE_MAX_STEPS > CUTSCENE_MAX_SHOTS ? CUTSCENE_MAX_STEPS : CUTSCENE_MAX_SHOTS];
    int   count;
    int   seq_count;
    int   active_count;
    int   playing;
    int   cur;               /* active shot index */
    float shot_t;            /* time in the current shot */
    float dist;              /* eased distance toward the target */
    float shot_duration;     /* duration of the current active shot */
    int   audio_channel;      /* active per-shot audio channel, or -1 */
    int   playing_sequence_index;
} g_cut;

void cutscene_reset(void) {
    memset(&g_cut, 0, sizeof(g_cut));
    g_cut.audio_channel = -1;
    g_cut.playing_sequence_index = -1;
}

/* ---- 3CAM: register a shot on spawn ------------------------------------- */

static CutSceneShot *prepend_shot(void) {
    if (g_cut.count >= CUTSCENE_MAX_SHOTS) return NULL;
    memmove(&g_cut.shots[1], &g_cut.shots[0],
            sizeof(g_cut.shots[0]) * (size_t)g_cut.count);
    g_cut.count++;
    memset(&g_cut.shots[0], 0, sizeof(g_cut.shots[0]));
    return &g_cut.shots[0];
}

static void cam_on_spawn(Entity *e, World *w) {
    (void)w;
    /* Inert placeable: no mesh, no collision. The shot is data for the runtime. */
    behavior_animated_spawn_base(e);
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);

    CutSceneShot *s = prepend_shot();
    if (!s) return;
    /* CameraTarget is mapped onto activate_target by the .gam loader. */
    snprintf(s->target, sizeof(s->target), "%s", e->activate_target);
    snprintf(s->sound_db, sizeof(s->sound_db), "%s", e->sound_database);
    s->sound_index = gam_prop_i(e, "SoundIndex", -1);
    s->offset[0]    = gam_prop_f(e, "TargOffsetX", 0.0f);
    s->offset[1]    = gam_prop_f(e, "TargOffsetY", 0.0f);
    s->offset[2]    = gam_prop_f(e, "TargOffsetZ", 0.0f);
    s->initial_dist = gam_prop_f(e, "InitialDist", 400.0f);
    s->min_dist     = gam_prop_f(e, "MinDist", 50.0f);
    s->max_dist     = gam_prop_f(e, "MaxDist", 4000.0f);
    s->zoom_speed   = gam_prop_f(e, "ZoomSpeed", 1.0f);
    s->look_voffset = gam_prop_f(e, "LookVoffset", 0.0f);
    s->hold_seconds = CUTSCENE_DEFAULT_SHOT_SECONDS;
    s->camera_type  = gam_prop_i(e, "CameraType", 0);
}

/* ---- 3MCA: sequencer presence ------------------------------------------- */

static CutSceneSequence *prepend_sequence(void) {
    if (g_cut.seq_count >= CUTSCENE_MAX_SEQUENCES) return NULL;
    memmove(&g_cut.seqs[1], &g_cut.seqs[0],
            sizeof(g_cut.seqs[0]) * (size_t)g_cut.seq_count);
    g_cut.seq_count++;
    memset(&g_cut.seqs[0], 0, sizeof(g_cut.seqs[0]));
    return &g_cut.seqs[0];
}

static void mca_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);

    CutSceneSequence *seq = prepend_sequence();
    if (!seq) return;
    snprintf(seq->tag, sizeof(seq->tag), "%s", e->tag[0] ? e->tag : "3MCA");
    snprintf(seq->sound_db, sizeof(seq->sound_db), "%s", e->sound_database);

    for (int i = 0; i < CUTSCENE_MAX_STEPS; i++) {
        char key[32];
        snprintf(key, sizeof(key), "CameraTarget%d", i);
        const char *target = gam_str(e, key, NULL);
        snprintf(key, sizeof(key), "SoundIndex%d", i);
        int sound_index = gam_prop_i(e, key, -1);
        if ((!target || !target[0] || strcasecmp(target, "none") == 0 ||
             strcasecmp(target, "null") == 0) && sound_index < 0)
            continue;
        if (seq->count >= CUTSCENE_MAX_STEPS) break;

        CutSceneStep *step = &seq->steps[seq->count++];
        snprintf(step->target, sizeof(step->target), "%s", target ? target : "");
        snprintf(key, sizeof(key), "TargetAnim%d", i);
        snprintf(step->target_anim, sizeof(step->target_anim), "%s",
                 gam_str(e, key, ""));
        step->sound_index = sound_index;
        snprintf(key, sizeof(key), "CameraType%d", i);
        step->camera_type = gam_prop_i(e, key, 0);
        snprintf(key, sizeof(key), "LookatVOffset%d", i);
        step->look_voffset = gam_prop_f(e, key, 100.0f);
    }

    if (seq->count <= 0) {
        memmove(&g_cut.seqs[0], &g_cut.seqs[1],
                sizeof(g_cut.seqs[0]) * (size_t)(g_cut.seq_count - 1));
        g_cut.seq_count--;
    }
}

/* ---- Runtime ------------------------------------------------------------ */

static float cutscene_desired_dist(const CutSceneShot *s) {
    float want = s->initial_dist * 0.45f;
    if (want < 220.0f) want = 220.0f;
    if (want > 760.0f) want = 760.0f;
    if (s->min_dist > 0.0f && want < s->min_dist * 0.45f)
        want = s->min_dist * 0.45f;
    if (s->max_dist > 0.0f && want > s->max_dist * 0.65f)
        want = s->max_dist * 0.65f;
    return want;
}

static void cutscene_mca_local_offset(int camera_type, float t, float out[3]) {
    float z;
    switch (camera_type) {
    case 1:
        z = 300.0f - 15.0f * t;
        if (z < 100.0f) z = 100.0f;
        out[0] = 0.0f; out[1] = 140.0f; out[2] = z;
        break;
    case 2:
        z = 500.0f - 35.0f * t;
        if (z < 100.0f) z = 100.0f;
        out[0] = 0.0f; out[1] = 240.0f; out[2] = z;
        break;
    case 3:
        z = 700.0f - 55.0f * t;
        if (z < 100.0f) z = 100.0f;
        out[0] = 200.0f; out[1] = 240.0f; out[2] = z;
        break;
    case 4:
        z = 700.0f - 55.0f * t;
        if (z < 100.0f) z = 100.0f;
        out[0] = -200.0f; out[1] = 190.0f; out[2] = z;
        break;
    case 0:
    default:
        out[0] = 0.0f; out[1] = 40.0f; out[2] = 200.0f;
        break;
    }
}

static void entity_local_to_world(const Entity *e, const float local[3], float out[3]) {
    float c = cosf(e->ry), s = sinf(e->ry);
    out[0] = e->x + local[0] * c + local[2] * s;
    out[1] = e->y + local[1];
    out[2] = e->z - local[0] * s + local[2] * c;
}

static void cutscene_halt_audio(void) {
    if (g_cut.audio_channel >= 0) {
        audio_channel_halt(g_cut.audio_channel);
        g_cut.audio_channel = -1;
    }
}

static void cutscene_play_current_audio(void) {
    cutscene_halt_audio();
    if (!g_cut.playing || g_cut.cur < 0 || g_cut.cur >= g_cut.active_count)
        return;
    CutSceneShot *s = &g_cut.active[g_cut.cur];
    float duration = 0.0f;
    if (s->sound_index >= 0) {
        duration = audio_duration_db(s->sound_db, s->sound_index);
        g_cut.audio_channel = audio_play_db(s->sound_db, s->sound_index, 0, 128);
        printf("[CUTSCENE] audio %s[%d] duration=%.2fs channel=%d\n",
               s->sound_db[0] ? s->sound_db : "soundeffects.omt",
               s->sound_index, duration, g_cut.audio_channel);
    }
    if (duration > 0.0f)
        s->hold_seconds = duration + CUTSCENE_AUDIO_PAD_SECONDS;
    if (s->hold_seconds < CUTSCENE_DEFAULT_SHOT_SECONDS)
        s->hold_seconds = CUTSCENE_DEFAULT_SHOT_SECONDS;
    g_cut.shot_duration = s->hold_seconds;
}

static void cutscene_start_active(int sequence_index, const char *label) {
    if (g_cut.active_count <= 0) {
        printf("[CUTSCENE] no playable shots for %s\n", label ? label : "request");
        return;
    }
    g_cut.playing = 1;
    g_cut.cur = 0;
    g_cut.shot_t = 0.0f;
    g_cut.dist = cutscene_desired_dist(&g_cut.active[0]);
    g_cut.shot_duration = g_cut.active[0].hold_seconds;
    g_cut.playing_sequence_index = sequence_index;
    printf("[CUTSCENE] play %s: %d shot(s)\n",
           label ? label : "sequence", g_cut.active_count);
    cutscene_play_current_audio();
}

void cutscene_stop(void) {
    cutscene_halt_audio();
    g_cut.playing = 0;
    g_cut.cur = 0;
    g_cut.shot_t = 0.0f;
    g_cut.playing_sequence_index = -1;
}

void cutscene_request_intro(void) {
    if (g_cut.count <= 0) {
        printf("[CUTSCENE] no shots registered; nothing to play\n");
        return;
    }
    g_cut.active_count = g_cut.count;
    for (int i = 0; i < g_cut.count; i++)
        g_cut.active[i] = g_cut.shots[i];
    cutscene_start_active(-1, "intro");
}

int cutscene_sequence_count(void) { return g_cut.seq_count; }

int cutscene_request_index(int index) {
    if (index < 0 || index >= g_cut.seq_count) {
        printf("[CUTSCENE] invalid sequence index %d (count=%d)\n", index, g_cut.seq_count);
        return 0;
    }

    CutSceneSequence *seq = &g_cut.seqs[index];
    g_cut.active_count = 0;
    for (int i = 0; i < seq->count && g_cut.active_count < CUTSCENE_MAX_STEPS; i++) {
        CutSceneStep *step = &seq->steps[i];
        CutSceneShot *dst = &g_cut.active[g_cut.active_count++];
        memset(dst, 0, sizeof(*dst));
        snprintf(dst->target, sizeof(dst->target), "%s", step->target);
        dst->offset[0] = 0.0f;
        dst->offset[1] = 0.0f;
        dst->offset[2] = 0.0f;
        dst->initial_dist = 520.0f;
        dst->min_dist = 220.0f;
        dst->max_dist = 900.0f;
        dst->zoom_speed = 8.0f;
        dst->hold_seconds = CUTSCENE_DEFAULT_SHOT_SECONDS;
        snprintf(dst->sound_db, sizeof(dst->sound_db), "%s", seq->sound_db);
        dst->sound_index = step->sound_index;
        dst->look_voffset = step->look_voffset;
        dst->camera_type = step->camera_type;
    }

    cutscene_start_active(index, seq->tag);
    return 1;
}

int cutscene_active(void) { return g_cut.playing; }

static Entity *find_by_tag(World *w, const char *tag) {
    if (!tag || !tag[0]) return NULL;
    for (Entity *e = w->head; e; e = e->next)
        if (e->alive && strcasecmp(e->tag, tag) == 0)
            return e;
    return NULL;
}

void cutscene_update(Camera *cam, World *w, float dt) {
    if (!g_cut.playing || !cam || !w) return;

    CutSceneShot *s = &g_cut.active[g_cut.cur];
    Entity *target = find_by_tag(w, s->target);

    if (target) {
        float cx, cy, cz, lx, ly, lz;
        if (g_cut.playing_sequence_index >= 0) {
            float local[3], world[3];
            cutscene_mca_local_offset(s->camera_type, g_cut.shot_t, local);
            entity_local_to_world(target, local, world);
            cx = world[0]; cy = world[1]; cz = world[2];
            lx = target->x;
            ly = target->y + s->look_voffset - 60.0f;
            lz = target->z;
        } else {
            float tx = target->x + s->offset[0];
            float ty = target->y + s->offset[1];
            float tz = target->z + s->offset[2];

            /* 3CAM fallback for intro-shot playback until ViewFromCamera is
               decoded. 3MCA selector playback uses the recovered table above. */
            float want = cutscene_desired_dist(s);
            g_cut.dist += (want - g_cut.dist) * (s->zoom_speed * dt);
            lx = tx; ly = ty + s->look_voffset; lz = tz;

            float front = target->ry;
            float shoulder = ((s->camera_type & 1) ? 0.45f : -0.45f);
            float sx = sinf(front + 1.5707963f) * shoulder * g_cut.dist;
            float sz = -cosf(front + 1.5707963f) * shoulder * g_cut.dist;
            cx = lx + sinf(front) * g_cut.dist + sx;
            cz = lz - cosf(front) * g_cut.dist + sz;
            cy = ly + g_cut.dist * 0.18f;
        }

        cam->pos[0] = cx;
        cam->pos[1] = cy;
        cam->pos[2] = cz;

        float dx = lx - cx, dy = ly - cy, dz = lz - cz;
        float horiz = sqrtf(dx * dx + dz * dz);
        /* Renderer forward is (sin y·cos p, sin p, -cos y·cos p), so a look
           direction (dx,dy,dz) inverts to yaw=atan2(dx,-dz), pitch=atan2(dy,horiz). */
        cam->yaw   = atan2f(dx, -dz);
        cam->pitch = atan2f(dy, horiz);
    }

    g_cut.shot_t += dt;
    if (g_cut.shot_t >= g_cut.shot_duration) {
        g_cut.cur++;
        g_cut.shot_t = 0.0f;
        if (g_cut.cur >= g_cut.active_count) {
            cutscene_stop();
            printf("[CUTSCENE] sequence complete -> return to gameplay camera\n");
            return;
        }
        g_cut.dist = cutscene_desired_dist(&g_cut.active[g_cut.cur]);
        printf("[CUTSCENE] shot %d/%d target='%s'\n",
               g_cut.cur + 1, g_cut.active_count, g_cut.active[g_cut.cur].target);
        cutscene_play_current_audio();
    }
}

EMSCRIPTEN_KEEPALIVE
int cutscene_request_web(int index) { return cutscene_request_index(index); }

EMSCRIPTEN_KEEPALIVE
int cutscene_count_web(void) { return cutscene_sequence_count(); }

EMSCRIPTEN_KEEPALIVE
int cutscene_active_web(void) { return cutscene_active(); }

EMSCRIPTEN_KEEPALIVE
void cutscene_stop_web(void) { cutscene_stop(); }

const EntityVTable vt_cutscene_camera = {
    .on_spawn  = cam_on_spawn,
    .on_update = NULL,
    .on_trigger = NULL,
    .flags = 0,
};

const EntityVTable vt_multi_cutscene = {
    .on_spawn  = mca_on_spawn,
    .on_update = NULL,
    .on_trigger = NULL,
    .flags = 0,
};
