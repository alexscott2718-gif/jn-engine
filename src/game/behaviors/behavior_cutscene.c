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
 * unaffected. The per-shot framing math matches the spec's interpreted
 * on-activate behavior; full trigger-graph activation (CTrigger -> 3MCA -> 3CAM
 * messaging) and the CameraType/ViewFromCamera enums are deferred open
 * questions in the spec. */

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
#define CUTSCENE_SHOT_SECONDS 2.5f   /* per-shot hold; sequencer cadence */

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
    int   audio_channel;      /* active per-shot audio channel, or -1 */
    int   playing_sequence_index;
} g_cut;

void cutscene_reset(void) {
    memset(&g_cut, 0, sizeof(g_cut));
    g_cut.audio_channel = -1;
    g_cut.playing_sequence_index = -1;
}

/* ---- 3CAM: register a shot on spawn ------------------------------------- */

static void cam_on_spawn(Entity *e, World *w) {
    (void)w;
    /* Inert placeable: no mesh, no collision. The shot is data for the runtime. */
    behavior_animated_spawn_base(e);
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);

    if (g_cut.count >= CUTSCENE_MAX_SHOTS) return;
    CutSceneShot *s = &g_cut.shots[g_cut.count];
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
    s->camera_type  = gam_prop_i(e, "CameraType", 0);
    g_cut.count++;
}

/* ---- 3MCA: sequencer presence ------------------------------------------- */

static void mca_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);

    if (g_cut.seq_count >= CUTSCENE_MAX_SEQUENCES) return;
    CutSceneSequence *seq = &g_cut.seqs[g_cut.seq_count];
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

    if (seq->count > 0)
        g_cut.seq_count++;
}

/* ---- Runtime ------------------------------------------------------------ */

static int is_real_target(const char *target) {
    return target && target[0] &&
           strcasecmp(target, "none") != 0 &&
           strcasecmp(target, "null") != 0;
}

static const CutSceneShot *find_shot_template(const char *target) {
    if (!is_real_target(target)) return NULL;
    for (int i = 0; i < g_cut.count; i++)
        if (strcasecmp(g_cut.shots[i].target, target) == 0)
            return &g_cut.shots[i];
    return NULL;
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
    if (s->sound_index >= 0) {
        g_cut.audio_channel = audio_play_db(s->sound_db, s->sound_index, 0, 128);
        printf("[CUTSCENE] audio %s[%d] channel=%d\n",
               s->sound_db[0] ? s->sound_db : "soundeffects.omt",
               s->sound_index, g_cut.audio_channel);
    }
}

static void cutscene_start_active(int sequence_index, const char *label) {
    if (g_cut.active_count <= 0) {
        printf("[CUTSCENE] no playable shots for %s\n", label ? label : "request");
        return;
    }
    g_cut.playing = 1;
    g_cut.cur = 0;
    g_cut.shot_t = 0.0f;
    g_cut.dist = g_cut.active[0].initial_dist;
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
        const CutSceneShot *tmpl = find_shot_template(step->target);
        memset(dst, 0, sizeof(*dst));
        if (tmpl) {
            *dst = *tmpl;
        } else {
            snprintf(dst->target, sizeof(dst->target), "%s", step->target);
            dst->offset[0] = 0.0f;
            dst->offset[1] = 0.0f;
            dst->offset[2] = 0.0f;
            dst->initial_dist = 900.0f;
            dst->min_dist = 250.0f;
            dst->max_dist = 1800.0f;
            dst->zoom_speed = 8.0f;
        }
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
        float tx = target->x + s->offset[0];
        float ty = target->y + s->offset[1];
        float tz = target->z + s->offset[2];

        /* Ease the framing distance toward InitialDist at ZoomSpeed, clamped. */
        float want = s->initial_dist;
        if (want < s->min_dist) want = s->min_dist;
        if (want > s->max_dist) want = s->max_dist;
        g_cut.dist += (want - g_cut.dist) * (s->zoom_speed * dt);

        /* Look point: the target with the vertical look offset applied. */
        float lx = tx, ly = ty + s->look_voffset, lz = tz;

        /* Place the camera back from the look point along a fixed framing
           bearing and derive yaw/pitch to look at it (Camera has no look-at). */
        float bearing = 0.6f + 0.45f * (float)(s->camera_type & 3);
        float cx = lx - sinf(bearing) * g_cut.dist;
        float cz = lz - cosf(bearing) * g_cut.dist;
        float cy = ly + g_cut.dist * 0.35f;

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
    if (g_cut.shot_t >= CUTSCENE_SHOT_SECONDS) {
        g_cut.cur++;
        g_cut.shot_t = 0.0f;
        if (g_cut.cur >= g_cut.active_count) {
            cutscene_stop();
            printf("[CUTSCENE] sequence complete -> return to gameplay camera\n");
            return;
        }
        g_cut.dist = g_cut.active[g_cut.cur].initial_dist;
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
