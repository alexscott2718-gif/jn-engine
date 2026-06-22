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
#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#define CUTSCENE_MAX_SHOTS 32
#define CUTSCENE_SHOT_SECONDS 2.5f   /* per-shot hold; sequencer cadence */

typedef struct {
    char  target[24];        /* CameraTarget tag (resolved at play time) */
    float offset[3];         /* TargOffsetX/Y/Z */
    float initial_dist;      /* InitialDist */
    float min_dist, max_dist;/* distance clamp */
    float zoom_speed;        /* ZoomSpeed (distance ease rate) */
    float look_voffset;      /* LookVoffset */
} CutSceneShot;

static struct {
    CutSceneShot shots[CUTSCENE_MAX_SHOTS];
    int   count;
    int   playing;
    int   cur;               /* active shot index */
    float shot_t;            /* time in the current shot */
    float dist;              /* eased distance toward the target */
} g_cut;

void cutscene_reset(void) {
    memset(&g_cut, 0, sizeof(g_cut));
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
    s->offset[0]    = gam_prop_f(e, "TargOffsetX", 0.0f);
    s->offset[1]    = gam_prop_f(e, "TargOffsetY", 0.0f);
    s->offset[2]    = gam_prop_f(e, "TargOffsetZ", 0.0f);
    s->initial_dist = gam_prop_f(e, "InitialDist", 400.0f);
    s->min_dist     = gam_prop_f(e, "MinDist", 50.0f);
    s->max_dist     = gam_prop_f(e, "MaxDist", 4000.0f);
    s->zoom_speed   = gam_prop_f(e, "ZoomSpeed", 1.0f);
    s->look_voffset = gam_prop_f(e, "LookVoffset", 0.0f);
    g_cut.count++;
}

/* ---- 3MCA: sequencer presence ------------------------------------------- */

static void mca_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_animated_spawn_base(e);
    e->visible = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    /* The sequencer itself holds no extra runtime state here — it drives the
       registered 3CAM shots, which the runtime plays in placement order. */
}

/* ---- Runtime ------------------------------------------------------------ */

void cutscene_request_intro(void) {
    if (g_cut.count <= 0) {
        printf("[CUTSCENE] no shots registered; nothing to play\n");
        return;
    }
    g_cut.playing = 1;
    g_cut.cur     = 0;
    g_cut.shot_t  = 0.0f;
    g_cut.dist    = g_cut.shots[0].initial_dist;
    printf("[CUTSCENE] play %d shot(s)\n", g_cut.count);
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

    CutSceneShot *s = &g_cut.shots[g_cut.cur];
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
        float bearing = 0.6f;  /* slight 3/4 angle, stable across shots */
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
        if (g_cut.cur >= g_cut.count) {
            g_cut.playing = 0;
            printf("[CUTSCENE] sequence complete -> return to gameplay camera\n");
            return;
        }
        g_cut.dist = g_cut.shots[g_cut.cur].initial_dist;
        printf("[CUTSCENE] shot %d/%d target='%s'\n",
               g_cut.cur + 1, g_cut.count, g_cut.shots[g_cut.cur].target);
    }
}

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
