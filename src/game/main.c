#include "../engine/window.h"
#include "../engine/glad.h"
#include "../engine/renderer.h"
#include "../engine/world.h"
#include "../engine/audio.h"
#include "../engine/input.h"
#include "../engine/physics.h"
#include "../engine/ground.h"
#include "../engine/canon_data.h"   /* Phase 12: measured ground footprint/topography */
#include "../engine/phase1_sky_tint.h"  /* Phase 1: measured keyframe-8881 sky + scene tint */
#include "../engine/phase4_capture_state.h"  /* Phase 4: measured alpha/blend/fog state */
#include "../engine/capture.h"
#include "../engine/replay.h"
#include "../engine/capture_scene.h"
#include "../engine/assets/gam_loader.h"
#include "../engine/assets/ase_loader.h"
#include "../engine/assets/tex_loader.h"
#include "../engine/assets/asset_cache.h"
#include "../engine/assets/placement_loader.h"
#include "../engine/assets/texture_overrides.h"
#include "../engine/assets/billboard_overrides.h"
#include "entities.h"
#include "entity_visual.h"
#include "camera.h"
#include "gamestate.h"
#include "player_anim.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <zlib.h>
#include <SDL.h>

static int env_enabled(const char *name) {
    const char *s = getenv(name);
    return (s && s[0] && strcmp(s, "0") != 0) ? 1 : 0;
}

static int env_enabled_default(const char *name, int default_value) {
    const char *s = getenv(name);
    if (!s || !s[0]) return default_value;
    return strcmp(s, "0") != 0;
}

static int load_camera_descriptor_file(const char *path, int *screen_w, int *screen_h) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[native_camera] cannot open %s\n", path);
        return 0;
    }
    float view[16], proj[16];
    int have_view = 0;
    int have_proj = 0;
    int sw = 1280;
    int sh = 720;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') continue;
        if (strncmp(p, "screen", 6) == 0) {
            sscanf(p + 6, "%d %d", &sw, &sh);
        } else if (strncmp(p, "view", 4) == 0) {
            float *m = view;
            have_view = (sscanf(p + 4,
                "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                &m[0],&m[1],&m[2],&m[3],&m[4],&m[5],&m[6],&m[7],
                &m[8],&m[9],&m[10],&m[11],&m[12],&m[13],&m[14],&m[15]) == 16);
        } else if (strncmp(p, "proj", 4) == 0) {
            float *m = proj;
            have_proj = (sscanf(p + 4,
                "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                &m[0],&m[1],&m[2],&m[3],&m[4],&m[5],&m[6],&m[7],
                &m[8],&m[9],&m[10],&m[11],&m[12],&m[13],&m[14],&m[15]) == 16);
        }
    }
    fclose(f);
    if (!have_view || !have_proj) {
        fprintf(stderr, "[native_camera] %s missing view/proj\n", path);
        return 0;
    }
    renderer_set_camera_override(view, proj);
    if (screen_w) *screen_w = sw;
    if (screen_h) *screen_h = sh;
    fprintf(stderr, "[native_camera] keyframe camera override from %s (%dx%d)\n",
            path, sw, sh);
    return 1;
}

static const float CAPTURE_LEVEL1_VIEW_IDENTITY[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

/* D3D capture projection from frame_v4_hudfix with the capture_scene z fix
   baked into row 2, so native model depth matches the pre-transformed scene. */
static const float CAPTURE_LEVEL1_PROJ_GL[16] = {
    1.29942167f, 0.0f,       0.0f,        0.0f,
    0.0f,       1.73256218f, 0.0f,        0.0f,
    0.0f,       0.0f,        1.00142956f, 1.0f,
    0.0f,       0.0f,      -41.0285912f,  1.0f,
};

static const float CAPTURE_LEVEL1_JIMMY_MODEL[16] = {
    1.0f,          7.4505806e-09f,  2.98023224e-08f, 0.0f,
    0.0f,          0.974252999f,   -0.225457549f,    0.0f,
    0.0f,          0.225457549f,    0.974253058f,    0.0f,
   -0.009765625f, -96.4539566f,   381.575134f,      1.0f,
};

#define CAPTURE_LIVE_MIN_Z_DELTA  (-90.0f)
#define CAPTURE_LIVE_MAX_Z_DELTA  (240.0f)
#define CAPTURE_LIVE_MIN_Y_DELTA  (-50.0f)
#define CAPTURE_LIVE_MAX_Y_DELTA  (220.0f)
#define CAPTURE_LIVE_NEAR_X_LIMIT (140.0f)
#define CAPTURE_LIVE_FAR_X_LIMIT  (220.0f)

static float clampf_local(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void capture_live_visual_delta(const Entity *jim, const float spawn[3],
                                      int bounded, float out_delta[3]) {
    out_delta[0] = jim->x - spawn[0];
    out_delta[1] = jim->y - spawn[1];
    out_delta[2] = jim->z - spawn[2];

    /* Headless QA helper: add a visual-only movement delta without mutating
       gameplay state. Format: x,y,z in native Level 1 units. */
    const char *test_delta = getenv("JN_CAPTURE_BACKED_TEST_JIMMY_DELTA");
    if (test_delta && test_delta[0]) {
        float tx = 0.0f, ty = 0.0f, tz = 0.0f;
        if (sscanf(test_delta, "%f,%f,%f", &tx, &ty, &tz) == 3) {
            out_delta[0] += tx;
            out_delta[1] += ty;
            out_delta[2] += tz;
        }
    }

    if (bounded) {
        /* Fixed-camera bounded QA: keep the accepted captured world/camera
           static and only bound the live actor's visual delta so normal
           movement remains visible. The simulation position, collision,
           triggers, and camera state remain untouched. */
        out_delta[2] = clampf_local(out_delta[2],
                                    CAPTURE_LIVE_MIN_Z_DELTA,
                                    CAPTURE_LIVE_MAX_Z_DELTA);
        float z_t = (out_delta[2] - CAPTURE_LIVE_MIN_Z_DELTA) /
            (CAPTURE_LIVE_MAX_Z_DELTA - CAPTURE_LIVE_MIN_Z_DELTA);
        float x_limit = CAPTURE_LIVE_NEAR_X_LIMIT +
            (CAPTURE_LIVE_FAR_X_LIMIT - CAPTURE_LIVE_NEAR_X_LIMIT) * z_t;
        out_delta[0] = clampf_local(out_delta[0], -x_limit, x_limit);
        out_delta[1] = clampf_local(out_delta[1],
                                    CAPTURE_LIVE_MIN_Y_DELTA,
                                    CAPTURE_LIVE_MAX_Y_DELTA);
    }
}

/* True if the model resolves at least one real texture (model-level or any
   material). Used to skip untextured meshes for faithfulness: the original
   game renders no untextured geometry (audit D1/D2). */
static int model_has_texture(const AseModel *m) {
    if (!m) return 0;
    if (m->texture_id) return 1;
    for (int i = 0; i < m->material_count; i++)
        if (m->materials[i].texture_id) return 1;
    return 0;
}

/* Save current GL framebuffer as a PNG using only stdlib + zlib */
static void save_screenshot(const char *path, int w, int h) {
    unsigned char *pixels = malloc((size_t)w * h * 3);
    if (!pixels) return;
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    /* Flip rows (GL origin = bottom-left, PNG = top-left) */
    for (int y = 0; y < h / 2; y++) {
        unsigned char *a = pixels + (size_t)y       * w * 3;
        unsigned char *b = pixels + (size_t)(h-1-y) * w * 3;
        for (int x = 0; x < w * 3; x++) { unsigned char t=a[x]; a[x]=b[x]; b[x]=t; }
    }

    /* Build raw PNG data: filter byte 0x00 before each row */
    size_t raw_len = (size_t)(w * 3 + 1) * h;
    unsigned char *raw = malloc(raw_len);
    if (!raw) { free(pixels); return; }
    for (int y = 0; y < h; y++) {
        raw[y * (w * 3 + 1)] = 0; /* None filter */
        memcpy(raw + y * (w * 3 + 1) + 1, pixels + (size_t)y * w * 3, (size_t)w * 3);
    }
    free(pixels);

    uLongf comp_len = compressBound(raw_len);
    unsigned char *comp = malloc(comp_len);
    if (!comp) { free(raw); return; }
    compress2(comp, &comp_len, raw, raw_len, 6);
    free(raw);

    FILE *f = fopen(path, "wb");
    if (!f) { free(comp); return; }

    /* PNG helpers */
    unsigned int crc = 0;
    #define W1(v) fputc((v), f)
    #define W4(v) do { unsigned int _v=(v); W1(_v>>24); W1(_v>>16); W1(_v>>8); W1(_v); } while(0)
    #define CRC_RESET()   crc = crc32(0, Z_NULL, 0)
    #define CRC_UPDATE(b,n) crc = crc32(crc, b, n)

    /* Signature */
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);

    /* IHDR */
    { unsigned char hdr[13];
      hdr[0]=w>>24;hdr[1]=w>>16;hdr[2]=w>>8;hdr[3]=w;
      hdr[4]=h>>24;hdr[5]=h>>16;hdr[6]=h>>8;hdr[7]=h;
      hdr[8]=8; hdr[9]=2; hdr[10]=0; hdr[11]=0; hdr[12]=0; /* 8-bit RGB */
      W4(13); CRC_RESET(); CRC_UPDATE((unsigned char*)"IHDR",4); fwrite("IHDR",1,4,f);
      CRC_UPDATE(hdr,13); fwrite(hdr,1,13,f); W4(crc); }

    /* IDAT */
    W4((unsigned int)comp_len);
    CRC_RESET(); CRC_UPDATE((unsigned char*)"IDAT",4); fwrite("IDAT",1,4,f);
    CRC_UPDATE(comp,(unsigned int)comp_len); fwrite(comp,1,comp_len,f); W4(crc);

    /* IEND */
    W4(0); CRC_RESET(); CRC_UPDATE((unsigned char*)"IEND",4); fwrite("IEND",1,4,f); W4(crc);
    #undef W1
    #undef W4
    #undef CRC_RESET
    #undef CRC_UPDATE

    fclose(f);
    free(comp);
    printf("Screenshot saved: %s (%dx%d)\n", path, w, h);
}

/* Strip directory; try direct open then case-insensitive scan of assets/gam.
   Writes the resolved relative path into out. Returns 1 on success. */
#include <dirent.h>
#include <strings.h>
static int resolve_gam_path(const char *name, char *out, size_t out_size) {
    if (!name || !name[0]) return 0;
    /* Try as-is first. */
    snprintf(out, out_size, "assets/gam/%s", name);
    FILE *f = fopen(out, "rb");
    if (f) { fclose(f); return 1; }
    /* Case-insensitive scan. */
    DIR *d = opendir("assets/gam");
    if (!d) return 0;
    struct dirent *de;
    int found = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcasecmp(de->d_name, name) == 0) {
            snprintf(out, out_size, "assets/gam/%s", de->d_name);
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

/* Place player at the named STRT in the world (case-insensitive on tag).
   Falls back to the 3JIM spawn if start_point is empty or not found. */
static Entity *place_player(World *world, const char *start_point) {
    Entity *jim = world_find_type(world, "3JIM");
    if (!jim) return NULL;
    if (start_point && start_point[0]) {
        for (Entity *e = world->head; e; e = e->next) {
            if (strncmp(e->type, "STRT", 4) != 0) continue;
            if (strcasecmp(e->tag, start_point) == 0) {
                jim->x = e->x; jim->y = e->y; jim->z = e->z;
                printf("[SPAWN] using STRT '%s' at (%.1f, %.1f, %.1f)\n",
                       e->tag, e->x, e->y, e->z);
                break;
            }
        }
    }
    jim->vx = jim->vy = jim->vz = 0.0f;
    jim->on_ground = 0;
    return jim;
}

/* Entity type color coding */
static void entity_color(const char *type, float *r, float *g, float *b) {
    if      (strcmp(type, "3JIM") == 0) { *r=1.0f; *g=1.0f; *b=0.0f; }  /* yellow = player */
    else if (strcmp(type, "3ROC") == 0) { *r=0.2f; *g=0.6f; *b=1.0f; }  /* blue   = rocket */
    else if (strcmp(type, "3TRE") == 0) { *r=0.1f; *g=0.8f; *b=0.2f; }  /* green  = tree   */
    else if (strcmp(type, "LOAD") == 0) { *r=1.0f; *g=0.4f; *b=0.0f; }  /* orange = loader */
    else if (strcmp(type, "STRT") == 0) { *r=1.0f; *g=0.0f; *b=1.0f; }  /* purple = start  */
    else if (strcmp(type, "DOOR") == 0) { *r=0.6f; *g=0.3f; *b=0.0f; }  /* brown  = door   */
    else if (strcmp(type, "PLAT") == 0) { *r=0.7f; *g=0.7f; *b=0.9f; }  /* lilac  = platform */
    else if (strcmp(type, "ITEM") == 0) { *r=1.0f; *g=0.9f; *b=0.2f; }  /* gold   = item   */
    else if (strcmp(type, "TRIG") == 0) { *r=0.0f; *g=1.0f; *b=1.0f; }  /* cyan   = trigger */
    else if (strcmp(type, "CRAT") == 0) { *r=0.7f; *g=0.5f; *b=0.3f; }  /* tan    = crate  */
    else                                { *r=0.5f; *g=0.5f; *b=0.5f; }  /* grey   = other  */
}

static void render_capture_live_jimmy(Entity *jim, int jim_model_ok,
                                      const float spawn[3], int bounded,
                                      int camera_pan,
                                      int viewport_w, int viewport_h) {
    if (!jim || !jim_model_ok) return;
    const AseModel *pose = player_anim_model((PlayerAnim)jim->user_flag);
    if (!pose) return;

    float model[16];
    memcpy(model, CAPTURE_LEVEL1_JIMMY_MODEL, sizeof(model));
    /* The native ASE faces opposite the captured Jimmy mesh in this camera. */
    for (int i = 0; i < 4; i++) {
        model[i] = -model[i];
        model[8 + i] = -model[8 + i];
    }
    float visual_delta[3];
    if (camera_pan) {
        visual_delta[0] = 0.0f;
        visual_delta[1] = 0.0f;
        visual_delta[2] = 0.0f;
    } else {
        capture_live_visual_delta(jim, spawn, bounded, visual_delta);
    }
    model[12] += visual_delta[0];
    model[13] += visual_delta[1];
    model[14] += visual_delta[2];

    renderer_set_camera_override(CAPTURE_LEVEL1_VIEW_IDENTITY,
                                 CAPTURE_LEVEL1_PROJ_GL);
    renderer_begin_overlay(viewport_w, viewport_h);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    renderer_draw_model_matrix(pose, 0, model);
    renderer_end_frame();
    renderer_set_camera_override(NULL, NULL);
}

static void draw_hud_segment(int viewport_w, int viewport_h,
                             float x, float y, float scale,
                             int horizontal) {
    float w = horizontal ? 18.0f : 4.0f;
    float h = horizontal ? 4.0f : 18.0f;
    renderer_draw_screen_rect(viewport_w, viewport_h, x, y, w * scale, h * scale,
                              1.0f, 0.96f, 0.70f, 1.0f);
}

static void draw_hud_digit(int viewport_w, int viewport_h,
                           int digit, float x, float y, float scale) {
    static const unsigned char SEGMENTS[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F,
    };
    if (digit < 0 || digit > 9) return;
    unsigned char bits = SEGMENTS[digit];
    if (bits & 0x01) draw_hud_segment(viewport_w, viewport_h, x + 4.0f * scale,  y,                 scale, 1);
    if (bits & 0x02) draw_hud_segment(viewport_w, viewport_h, x + 22.0f * scale, y + 4.0f * scale,  scale, 0);
    if (bits & 0x04) draw_hud_segment(viewport_w, viewport_h, x + 22.0f * scale, y + 26.0f * scale, scale, 0);
    if (bits & 0x08) draw_hud_segment(viewport_w, viewport_h, x + 4.0f * scale,  y + 44.0f * scale, scale, 1);
    if (bits & 0x10) draw_hud_segment(viewport_w, viewport_h, x,                y + 26.0f * scale, scale, 0);
    if (bits & 0x20) draw_hud_segment(viewport_w, viewport_h, x,                y + 4.0f * scale,  scale, 0);
    if (bits & 0x40) draw_hud_segment(viewport_w, viewport_h, x + 4.0f * scale,  y + 22.0f * scale, scale, 1);
}

static void draw_hud_slash(int viewport_w, int viewport_h,
                           float x, float y, float scale) {
    for (int i = 0; i < 7; i++) {
        renderer_draw_screen_rect(viewport_w, viewport_h,
                                  x + (float)(6 - i) * 3.0f * scale,
                                  y + (float)i * 7.0f * scale,
                                  4.0f * scale, 6.0f * scale,
                                  1.0f, 0.96f, 0.70f, 1.0f);
    }
}

static void render_capture_live_hud(int viewport_w, int viewport_h) {
    const GameState *gs = gamestate_get();
    int collected = gs->items_collected;
    int total = gs->items_total;
    if (collected < 0) collected = 0;
    if (total < 0) total = 0;
    if (collected > 99) collected = 99;
    if (total > 99) total = 99;

    float scale = (float)viewport_h / 720.0f;
    if (scale < 0.5f) scale = 0.5f;
    float x = 22.0f * scale;
    float y = 16.0f * scale;

    renderer_draw_screen_rect(viewport_w, viewport_h,
                              x, y, 48.0f * scale, 48.0f * scale,
                              0.06f, 0.09f, 0.12f, 0.72f);
    renderer_draw_screen_rect(viewport_w, viewport_h,
                              x + 10.0f * scale, y + 10.0f * scale,
                              28.0f * scale, 28.0f * scale,
                              0.95f, 0.78f, 0.24f, 1.0f);
    renderer_draw_screen_rect(viewport_w, viewport_h,
                              x + 17.0f * scale, y + 17.0f * scale,
                              14.0f * scale, 14.0f * scale,
                              0.18f, 0.22f, 0.28f, 1.0f);

    float tx = x + 64.0f * scale;
    int col_tens = collected / 10;
    int col_ones = collected % 10;
    if (col_tens > 0) {
        draw_hud_digit(viewport_w, viewport_h, col_tens, tx, y + 2.0f * scale, scale);
        tx += 34.0f * scale;
    }
    draw_hud_digit(viewport_w, viewport_h, col_ones, tx, y + 2.0f * scale, scale);
    tx += 34.0f * scale;
    draw_hud_slash(viewport_w, viewport_h, tx, y + 1.0f * scale, scale);
    tx += 28.0f * scale;
    if (total >= 10) {
        draw_hud_digit(viewport_w, viewport_h, total / 10, tx, y + 2.0f * scale, scale);
        tx += 34.0f * scale;
    }
    draw_hud_digit(viewport_w, viewport_h, total % 10, tx, y + 2.0f * scale, scale);
}

int main(void) {
    Window w;
    if (!window_init(&w, "JN Engine - Step 4: Textured Scene", 1280, 720))
        return 1;

    int native_level1 = env_enabled("JN_NATIVE_LEVEL1");
    int hybrid_level1 = env_enabled("JN_HYBRID_LEVEL1");
    int capture_backed_level1 = env_enabled("JN_CAPTURE_BACKED_LEVEL1");
    if (native_level1 && (hybrid_level1 || capture_backed_level1)) {
        fprintf(stderr,
                "[native_level1] clean native map runtime supersedes hybrid/capture-backed modes\n");
        hybrid_level1 = 0;
        capture_backed_level1 = 0;
    } else if (hybrid_level1 && capture_backed_level1) {
        fprintf(stderr,
                "[hybrid_level1] native hybrid mode supersedes JN_CAPTURE_BACKED_LEVEL1\n");
        capture_backed_level1 = 0;
    }
    int capture_live_jimmy = capture_backed_level1 &&
        env_enabled("JN_CAPTURE_BACKED_LIVE_JIMMY");
    int capture_live_jimmy_bounded = capture_live_jimmy &&
        env_enabled_default("JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS", 1);
    int capture_live_world_pan = capture_live_jimmy &&
        env_enabled("JN_CAPTURE_BACKED_WORLD_PAN");
    int capture_live_hud = capture_backed_level1 &&
        env_enabled("JN_CAPTURE_BACKED_LIVE_HUD");
    int capture_multiframe = capture_backed_level1 &&
        env_enabled("JN_CAPTURE_BACKED_MULTIFRAME");
    if (capture_multiframe) {
        /* Multi-frame world reproject supersedes the single-frame world pan
           because both want to drive the static world group through a
           runtime view*proj. */
        capture_live_world_pan = 0;
    }
    if (capture_backed_level1 || hybrid_level1 || native_level1) {
        SDL_SetWindowSize(w.sdl_win, 1280, 720);
        w.width = 1280;
        w.height = 720;
    }
    if (native_level1) {
        fprintf(stderr,
                "[native_level1] clean native map runtime enabled: full OMT geometry + native camera\n");
    } else if (hybrid_level1) {
        fprintf(stderr,
                "[hybrid_level1] native runtime enabled: GAM simulation + OMT placements + native camera\n");
    }

    if (native_level1) {
        const char *cam_path = getenv("JN_NATIVE_LEVEL1_CAMERA");
        char keyframe_path[192];
        const char *keyframe = getenv("JN_NATIVE_LEVEL1_KEYFRAME");
        if ((!cam_path || !cam_path[0]) && keyframe && keyframe[0]) {
            snprintf(keyframe_path, sizeof(keyframe_path),
                     "assets/native/keyframe_cameras/%s.txt", keyframe);
            cam_path = keyframe_path;
        }
        if (cam_path && cam_path[0]) {
            int cam_sw = w.width;
            int cam_sh = w.height;
            if (load_camera_descriptor_file(cam_path, &cam_sw, &cam_sh)) {
                SDL_SetWindowSize(w.sdl_win, cam_sw, cam_sh);
                w.width = cam_sw;
                w.height = cam_sh;
            }
        }
    }

    if (!renderer_init(w.width, w.height)) {
        window_destroy(&w);
        return 1;
    }

    if (native_level1) {
        /* Phase 1 of docs/native_vs_capture_8881_plan.md: lift sky gradient
           + scene tint from the captured keyframe-8881 reference so the
           overall colour cast matches the original.  Header is regenerated
           by `make phase1-sky-tint`. */
        renderer_set_sky(PHASE1_SKY_TOP_R, PHASE1_SKY_TOP_G, PHASE1_SKY_TOP_B,
                         PHASE1_SKY_BOT_R, PHASE1_SKY_BOT_G, PHASE1_SKY_BOT_B);
        renderer_set_scene_tint(PHASE1_SCENE_TINT_R,
                                PHASE1_SCENE_TINT_G,
                                PHASE1_SCENE_TINT_B);
        /* Hide multi-material slots whose canvas chain didn't resolve so
           SCHOOL's missing slots, foliage tree trunks with debug_flat
           branches, etc. don't render as dark slabs. */
        renderer_set_hide_untextured_groups(1);

        /* Phase 4: enable shader-side alpha cutout for capture-derived
           billboards (foliage, playground sand). The original game runs
           with ALPHATESTENABLE=1 on >89% of draws -- we approximate by
           discarding fragments whose texture alpha is below the threshold,
           giving clean leaf cutouts without replicating the original's
           color-key machinery. */
        renderer_set_alpha_cutout(PHASE4_ALPHA_CUTOUT_ENABLED,
                                  PHASE4_ALPHA_CUTOUT_THRESHOLD);
        fprintf(stderr,
                "[native_level1] phase 1 sky/tint applied: sky_top=(%.3f,%.3f,%.3f) "
                "scene_tint=(%.3f,%.3f,%.3f)\n",
                PHASE1_SKY_TOP_R, PHASE1_SKY_TOP_G, PHASE1_SKY_TOP_B,
                PHASE1_SCENE_TINT_R, PHASE1_SCENE_TINT_G, PHASE1_SCENE_TINT_B);

        /* Phase 2: ground/water texture overrides for slots whose
           level1.omt canvas chain resolves to nothing. Provenance and
           per-entry rationale live in
           assets/native/level1_texture_overrides.json. */
        texture_overrides_load("assets/native/level1_texture_overrides.txt");
        fprintf(stderr,
                "[native_level1] phase 2 texture overrides loaded: %d\n",
                texture_overrides_count());

        /* Phase 5: replace per-mesh native tree geometry with the camera-
           facing quad the original game draws. Sizes + textures come from
           4-vertex FVF=0x152 records in the captured frame -- never guesses.
           Provenance: assets/native/level1_billboard_overrides.json. */
        billboard_overrides_load("assets/native/level1_billboard_overrides.txt");
        fprintf(stderr,
                "[native_level1] phase 5 billboard overrides loaded: %d\n",
                billboard_overrides_count());
    }

    if (!audio_init()) {
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }

    /* M6 render-stream capture. No-op unless built with -DJN_CAPTURE *and*
       JN_CAPTURE=<path> is set; see src/engine/capture.h. */
    capture_init();

    /* M7a: if a matched-camera descriptor was loaded, size the window to its
       screen so the rendered aspect matches the original capture. */
    {
        int cam_sw, cam_sh;
        if (capture_camera_screen(&cam_sw, &cam_sh)) {
            SDL_SetWindowSize(w.sdl_win, cam_sw, cam_sh);
            w.width = cam_sw; w.height = cam_sh;
        }
    }

    if (!input_init()) {
        audio_destroy();
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }

    int capture_scene_ready = 0;
    if (capture_backed_level1) {
        const char *scene_path;
        if (capture_multiframe)
            scene_path = "assets/capture/level1_hudfix/scene_world.bin";
        else if (capture_live_world_pan)
            scene_path = "assets/capture/level1_hudfix/scene_reproject.bin";
        else
            scene_path = "assets/capture/level1_hudfix/scene.bin";
        capture_scene_ready =
            capture_scene_init(scene_path);
        if (!capture_scene_ready && capture_multiframe) {
            fprintf(stderr,
                    "[capture_level1] multi-frame world fixture unavailable; falling back to single-frame scene\n");
            capture_multiframe = 0;
            capture_scene_ready =
                capture_scene_init("assets/capture/level1_hudfix/scene.bin");
        }
        if (!capture_scene_ready && capture_live_world_pan) {
            fprintf(stderr,
                    "[capture_level1] reproject scene unavailable; retrying stable capture scene\n");
            capture_live_world_pan = 0;
            capture_scene_ready =
                capture_scene_init("assets/capture/level1_hudfix/scene.bin");
        }
        if (!capture_scene_ready)
            fprintf(stderr, "[capture_level1] capture scene unavailable; using old renderer\n");
        if (capture_scene_ready && capture_multiframe) {
            /* In multi-frame mode the static_world group is always world-space
               and driven by the runtime view*proj uniform. */
            capture_scene_set_group_use_world(CAPTURE_SCENE_GROUP_STATIC_WORLD, 1);
            fprintf(stderr,
                    "[capture_level1] multi-frame world reproject enabled\n");
        }
        if (capture_scene_ready && capture_live_jimmy) {
            capture_scene_set_group_visible(CAPTURE_SCENE_GROUP_PLAYER_JIMMY, 0);
            fprintf(stderr,
                    "[capture_level1] %s live Jimmy overlay enabled%s; captured Jimmy group hidden\n",
                    capture_live_jimmy_bounded ? "bounded" : "unbounded",
                    capture_live_world_pan ? " with static-world pan" : "");
        }
        if (capture_scene_ready && capture_live_hud) {
            capture_scene_set_group_visible(CAPTURE_SCENE_GROUP_HUD, 0);
            fprintf(stderr,
                    "[capture_level1] live HUD overlay enabled; captured HUD group hidden\n");
        }
    }

    /* Faithful .omtc replay path: when JN_REPLAY=<path> is set, skip game
       setup entirely and render the captured D3D7 command stream. Phase-12
       pivot proof (docs/faithful_engine_rethink.md). */
    if (replay_active()) {
        if (!replay_init(w.width, w.height)) {
            input_destroy(); audio_destroy(); renderer_destroy();
            window_destroy(&w); return 1;
        }
        int screenshot_mode = getenv("JN_SCREENSHOT") != NULL;
        const char *screenshot_path = getenv("JN_SCREENSHOT_PATH");
        if (!screenshot_path || !screenshot_path[0])
            screenshot_path = "screenshot.png";
        int screenshot_taken = 0;
        while (!w.should_quit) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) w.should_quit = 1;
                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                    w.should_quit = 1;
                if (ev.type == SDL_WINDOWEVENT &&
                    ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    w.width = ev.window.data1; w.height = ev.window.data2;
                }
            }
            replay_render_frame();
            window_swap(&w);
            if (screenshot_mode && !screenshot_taken) {
                glFinish();
                save_screenshot(screenshot_path, w.width, w.height);
                screenshot_taken = 1;
                w.should_quit = 1;
            }
            SDL_Delay(16);
        }
        replay_destroy();
        capture_scene_destroy();
        input_destroy(); audio_destroy(); renderer_destroy();
        window_destroy(&w);
        return 0;
    }

    World world;
    world_init(&world);
    world_box_init();
    gamestate_init();

    /* Load all player poses with the shared real-Jimmy texture. All jim*.ASE
       reference jimmylast.bmp (which doesn't exist); jimycarl.png is the
       atlas that matches the model UVs (red shirt, atom emblem, backpack). */
    asset_cache_begin_level();
    int need_native_jim_visual = !capture_scene_ready || capture_live_jimmy;
    int jim_poses_loaded = 0;
    if (need_native_jim_visual) {
        unsigned int jim_tex = tex_cache_get("assets/png/jimycarl.png");
        jim_poses_loaded = player_anim_init(jim_tex);
    } else {
        printf("[capture_level1] skipping native Jimmy visual assets\n");
    }
    int jim_model_ok = (jim_poses_loaded > 0);

    int n = gam_load(&world, "assets/gam/Level1.gam");
    if (n < 0) {
        fprintf(stderr, "Failed to load Level1.gam\n");
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }
    if (!capture_scene_ready) {
        /* Static city geometry: OMT chunk centers from level1.omt → world placements. */
        placements_load(&world, "assets/ase/omt/level1_placements.txt");
        {
            int missing = 0;
            for (int i = 0; i < world.placement_count; i++)
                if (!model_cache_get(world.placements[i].ase_path)) missing++;
            printf("placements_loaded=%d, missing_mesh=%d\n", world.placement_count, missing);
        }
    } else {
        printf("[capture_level1] skipping old visual-only OMT placements\n");
    }

    /* Level1 has no ITEM entities; synthesize a small ring around the player
       spawn so the Phase 4 pickup/win loop is exercisable. */
    {
        Entity *jim_pre = world_find_type(&world, "3JIM");
        if (jim_pre && !native_level1) {
            const int N = 5;
            const float R = 600.0f;
            for (int i = 0; i < N; i++) {
                float ang = (6.28318f * i) / N;
                Entity *it = world_add(&world);
                if (!it) break;
                memcpy(it->type, "ITEM", 4); it->type[4] = '\0';
                snprintf(it->tag, sizeof(it->tag), "test_item_%d", i);
                it->x = jim_pre->x + R * cosf(ang);
                it->y = jim_pre->y + 40.0f;
                it->z = jim_pre->z + R * sinf(ang);
            }
        }
    }

    /* JN_TEST_SPRITES=1 spawns one of each billboard FourCC in a ring around
       the player so the Phase 10 sprite renderer can be visually QA'd from
       the default spawn (Level1 has no 3NEU/3BAL/3CON within camera reach of
       PHONEBOOTH). */
    if (getenv("JN_TEST_CHARS")) {
        Entity *jp = world_find_type(&world, "3JIM");
        if (jp) {
            const char *kinds[] = { "3CAR","3SHE","3LIB","3MOM","3HUM","3BEN","3GIR","3NIC" };
            const int N = (int)(sizeof(kinds) / sizeof(kinds[0]));
            const float R = 200.0f;
            for (int i = 0; i < N; i++) {
                float ang = (6.28318f * i) / N;
                Entity *e = world_add(&world);
                if (!e) break;
                memcpy(e->type, kinds[i], 4); e->type[4] = '\0';
                snprintf(e->tag, sizeof(e->tag), "test_char_%s", kinds[i]);
                e->x = jp->x + R * cosf(ang);
                e->y = jp->y;
                e->z = jp->z + R * sinf(ang);
                e->ry = ang + 3.14159f;  /* face toward center */
            }
        }
    }

    if (getenv("JN_TEST_SPRITES")) {
        Entity *jp = world_find_type(&world, "3JIM");
        if (jp) {
            const char *kinds[] = { "3NEU", "3RED", "3BAL", "3CON", "3LEA" };
            const int N = (int)(sizeof(kinds) / sizeof(kinds[0]));
            const float R = 250.0f;
            for (int i = 0; i < N; i++) {
                float ang = (6.28318f * i) / N;
                Entity *e = world_add(&world);
                if (!e) break;
                memcpy(e->type, kinds[i], 4); e->type[4] = '\0';
                snprintf(e->tag, sizeof(e->tag), "test_sprite_%s", kinds[i]);
                e->x = jp->x + R * cosf(ang);
                e->y = jp->y + 60.0f;
                e->z = jp->z + R * sinf(ang);
            }
        }
    }

    /* Boot rollup: log any FourCCs that will still render as placeholder boxes.
       Each distinct unresolved FourCC is listed once; per-entity spam is avoided. */
    {
        char seen[64][5];
        int  nseen = 0;
        int  nbox  = 0;
        for (Entity *e = world.head; e; e = e->next) {
            if (!e->alive) continue;
            EntityVisual v;
            if (entity_visual_resolve(e, &v)) continue;
            /* Check if we already noted this FourCC. */
            int found = 0;
            for (int k = 0; k < nseen; k++) {
                if (strncmp(seen[k], e->type, 4) == 0) { found = 1; break; }
            }
            if (!found && nseen < 64) {
                memcpy(seen[nseen], e->type, 4); seen[nseen][4] = '\0'; nseen++;
            }
            nbox++;
        }
        if (nbox > 0) {
            fprintf(stderr, "[entity_visual] %d placeholder boxes; unresolved FourCCs:", nbox);
            for (int k = 0; k < nseen; k++) fprintf(stderr, " %s", seen[k]);
            fprintf(stderr, "\n");
        } else {
            printf("[entity_visual] all entities resolved\n");
        }
    }

    /* Bind vtables + run on_spawn for every entity (must come after gam_load). */
    entity_bind_vtables(&world);

    /* Find JIM and frame the camera on him. JIM's spawn Y becomes the ground plane. */
    Camera *cam = renderer_camera();
    Entity *jim  = world_find_type(&world, "3JIM");
    float capture_live_spawn[3] = {0.0f, 0.0f, 0.0f};
    if (jim) {
        world.ground_y = jim->y - jim->half_extents[1];
        gamestate_set_spawn(jim->x, jim->y, jim->z);
        capture_live_spawn[0] = jim->x;
        capture_live_spawn[1] = jim->y;
        capture_live_spawn[2] = jim->z;
        printf("Player at (%.1f, %.1f, %.1f), ground_y=%.1f\n",
               jim->x, jim->y, jim->z, world.ground_y);
    }
    cam->fov    = 1.0472f;
    cam->near_z = 1.0f;
    cam->far_z  = 80000.0f;
    if (capture_backed_level1 || hybrid_level1 || native_level1) {
        cam->fov = 1.047215f;
        cam->near_z = 20.0f;
        cam->far_z = 28000.0f;
        if (capture_backed_level1)
            printf("[capture_level1] capture-scene viewport=1280x720 fov_y=60.001 near=20 far=28000\n");
        else if (native_level1)
            printf("[native_level1] viewport=1280x720 fov_y=60.001 near=20 far=28000\n");
        else
            printf("[hybrid_level1] viewport=1280x720 fov_y=60.001 near=20 far=28000\n");
    }

    /* Ground: a real level (placements present) supplies its own ground/street/
       terrain and water meshes (GROUND.ASE, ncwater*, etc.) at their authored
       world positions -- the faithful representation. We do NOT lay a synthetic
       ground over them (an earlier heightfield was a stand-in that buried the
       real geometry, including the stream). The synthetic flat floor is kept
       ONLY for empty test scenes with no level geometry. */
    if (world.placement_count == 0 && !capture_scene_ready) {
        unsigned int ground_tex = tex_cache_get(CANON_GROUND_TEXTURE);
        ground_init(ground_tex, 20000.0f, 20000.0f,
                    jim ? jim->x : 0.0f, jim ? jim->z : 0.0f,
                    80.0f, 0.0f);
    } else if (native_level1) {
        /* Phase 2 of docs/native_vs_capture_8881_plan.md: the capture's
           dominant ground texture (tex_05e10d68, 133 draws at world
           translation (15,-545,-26)) corresponds to a large terrain mesh
           that has no single counterpart in level1.omt placements. Use the
           ground.c flat tile (y_amplitude=0) so the bottom of every native
           Level 1 view matches the captured grass tint. */
        unsigned int grass_tex = tex_cache_get(
            "assets/native/level1_capture_overrides/GROUND_mat0_grass.png");
        ground_init(grass_tex, 20000.0f, 20000.0f,
                    jim ? jim->x : 0.0f, jim ? jim->z : 0.0f,
                    80.0f, 0.0f);
    }

    FollowCam fcam;
    follow_cam_init(&fcam);
    follow_cam_snap(&fcam, cam, jim);

    int mouse_down = 0, last_mx = 0, last_my = 0;

    printf("Controls: WASD = move  |  SPACE = jump  |  SHIFT = run  |  R = respawn  |  LMB drag = orbit  |  ESC = quit\n");
    printf("Entities: %d   Items: %d\n", world.count, gamestate_get()->items_total);
    int screenshot_taken = 0;
    int screenshot_mode  = getenv("JN_SCREENSHOT") != NULL;
    const char *screenshot_path = getenv("JN_SCREENSHOT_PATH");
    if (!screenshot_path || !screenshot_path[0])
        screenshot_path = "screenshot.png";
    /* In screenshot mode let a few physics ticks fire first so pending state
       (level swaps, animation transitions) actually flushes before capture. */
    int screenshot_warmup_ticks = 0;
    const int SCREENSHOT_WARMUP_GOAL = 4;

    /* Debug: pre-queue a level swap so the swap path can be exercised
       without walking to a LOAD trigger. Format: "level1c.gam:FRONTDOOR" */
    {
        const char *test_swap = getenv("JN_TEST_SWAP");
        if (test_swap && *test_swap) {
            char lvl[64] = {0}, spawn[32] = {0};
            const char *colon = strchr(test_swap, ':');
            if (colon) {
                size_t llen = (size_t)(colon - test_swap);
                if (llen >= sizeof(lvl)) llen = sizeof(lvl) - 1;
                memcpy(lvl, test_swap, llen); lvl[llen] = '\0';
                snprintf(spawn, sizeof(spawn), "%s", colon + 1);
            } else {
                snprintf(lvl, sizeof(lvl), "%s", test_swap);
            }
            gamestate_request_level_swap(lvl, spawn);
        }
    }

    /* Fixed timestep: 60 Hz updates, uncapped rendering */
    const float DT = 1.0f / 60.0f;
    Uint32 last_time = SDL_GetTicks();
    float accumulator = 0.0f;
    int frame_count = 0;
    unsigned int cap_seq = 0;
    Uint32 fps_time = last_time;

    while (!w.should_quit) {
        Uint32 now = SDL_GetTicks();
        float frame_time = (now - last_time) / 1000.0f;
        last_time = now;

        /* Cap frame time to prevent spiral of death (lag spike) */
        if (frame_time > 0.1f) frame_time = 0.1f;

        accumulator += frame_time;
        if (screenshot_mode && screenshot_warmup_ticks < SCREENSHOT_WARMUP_GOAL) {
            accumulator = DT;   /* force one tick per render frame during warmup */
        }

        /* Update phase: fixed timestep */
        while (accumulator >= DT) {
            accumulator -= DT;

            input_update();

            /* Per-entity behavior tick (player reads input, platforms move, etc.) */
            for (Entity *e = world.head; e; e = e->next) {
                entity_update(e, &world, DT);
            }

            /* Physics: gravity, AABB collision, trigger detection. */
            physics_step(&world, DT);

            /* Drain a pending level swap. Done outside the entity-update
               iteration above so we don't mutate the list mid-walk. */
            const GameState *gs_pre = gamestate_get();
            if (gs_pre->swap_level[0] != '\0') {
                char path[160];
                char level_buf[64];
                char spawn_buf[32];
                snprintf(level_buf, sizeof(level_buf), "%s", gs_pre->swap_level);
                snprintf(spawn_buf, sizeof(spawn_buf), "%s", gs_pre->swap_spawn);
                if (resolve_gam_path(level_buf, path, sizeof(path))) {
                    printf("[SWAP] loading %s (cache: %d tex, %d models)\n",
                           path, asset_cache_tex_count(), asset_cache_model_count());
                    world_destroy(&world);
                    world_init(&world);
                    gamestate_reset_for_new_level();
                    asset_cache_begin_level();
                    if (gam_load(&world, path) >= 0) {
                        /* Re-load static placements when staying within Retroville
                           (Level1*.gam shares level1.omt geometry). Other levels
                           will get their own *_placements.txt once those OMTs are
                           processed. */
                        if (strncasecmp(level_buf, "level1", 6) == 0 &&
                            !capture_scene_ready)
                            placements_load(&world,
                                            "assets/ase/omt/level1_placements.txt");
                        entity_bind_vtables(&world);
                        /* Player textures + models + ground stay live across levels. */
                        if (need_native_jim_visual) {
                            tex_cache_get("assets/png/jimycarl.png");
                            tex_cache_get("assets/png/mud.png");
                            for (int pa = 0; pa < PA_COUNT; pa++)
                                (void)player_anim_model((PlayerAnim)pa);
                        }
                        asset_cache_purge_stale();
                        printf("[SWAP] post-purge cache: %d tex, %d models\n",
                               asset_cache_tex_count(), asset_cache_model_count());
                        jim = place_player(&world, spawn_buf);
                        if (jim) {
                            world.ground_y = jim->y - jim->half_extents[1];
                            gamestate_set_spawn(jim->x, jim->y, jim->z);
                            capture_live_spawn[0] = jim->x;
                            capture_live_spawn[1] = jim->y;
                            capture_live_spawn[2] = jim->z;
                        }
                        follow_cam_snap(&fcam, cam, jim);
                    } else {
                        fprintf(stderr, "[SWAP] gam_load failed for %s\n", path);
                    }
                } else {
                    fprintf(stderr, "[SWAP] could not find %s under assets/gam\n", level_buf);
                    gamestate_reset_for_new_level();
                }
                /* skip respawn/cam steps this tick — world just rebuilt */
                continue;
            }

            /* Lifecycle: manual respawn (R) or death plane below ground. */
            if (jim) {
                int want_respawn = input_just_pressed(SDL_SCANCODE_R);
                if (!want_respawn && jim->y < world.ground_y - 2000.0f) {
                    printf("[DEATH] below kill plane (y=%.1f)\n", jim->y);
                    want_respawn = 1;
                }
                if (want_respawn) {
                    gamestate_respawn_player(jim);
                    follow_cam_snap(&fcam, cam, jim);
                }
            }

            /* Follow camera tracks the player — unless a matched-camera
               override is installed (M7a), in which case the camera pose is
               fixed by the descriptor and begin_frame ignores the follow cam. */
            if (!renderer_camera_override_active())
                follow_cam_update(&fcam, cam, jim, &world, DT);
        }

        /* Event handling */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) w.should_quit = 1;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) w.should_quit = 1;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_1) audio_play(0);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_2) audio_play(1);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_3) audio_play(2);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_4) audio_play(3);
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                w.width = ev.window.data1; w.height = ev.window.data2;
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                mouse_down = 1; last_mx = ev.button.x; last_my = ev.button.y;
            }
            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) mouse_down = 0;
            if (ev.type == SDL_MOUSEMOTION && mouse_down) {
                int dx = ev.motion.x - last_mx, dy = ev.motion.y - last_my;
                last_mx = ev.motion.x; last_my = ev.motion.y;
                fcam.yaw   -= dx * 0.004f;
                fcam.pitch -= dy * 0.004f;
                if (fcam.pitch >  0.6f) fcam.pitch =  0.6f;
                if (fcam.pitch < -1.2f) fcam.pitch = -1.2f;
            }
        }

        /* Render phase: uncapped */
        capture_begin_frame(cap_seq, SDL_GetTicks());
        if (capture_scene_ready) {
            float view[16] = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            if (capture_multiframe && jim) {
                /* Camera-follow Jimmy by translating the captured view origin
                   by -visual_delta. The captured projection has the anchor
                   frame's camera baked in; this adds a per-frame offset on
                   top so movement reveals geometry contributed by other
                   keyframes in scene_world.bin. */
                float visual_delta[3];
                capture_live_visual_delta(jim, capture_live_spawn,
                                          capture_live_jimmy_bounded,
                                          visual_delta);
                view[12] = -visual_delta[0];
                view[13] = -visual_delta[1];
                view[14] = -visual_delta[2];
            }
            capture_scene_set_world_view_proj(view, CAPTURE_LEVEL1_PROJ_GL);
            capture_scene_set_group_ndc_offset(CAPTURE_SCENE_GROUP_STATIC_WORLD,
                                               0.0f, 0.0f);
            capture_scene_set_group_world_offset(CAPTURE_SCENE_GROUP_STATIC_WORLD,
                                                 0.0f, 0.0f, 0.0f);
            if (capture_multiframe) {
                /* The multi-frame fixture uses a zero group offset at spawn,
                   but still needs the static-world group rendered through the
                   runtime view*proj. capture_scene_set_group_world_offset()
                   disables world mode for zero offsets for the older pan path,
                   so restore the explicit world-mode flag here. */
                capture_scene_set_group_use_world(CAPTURE_SCENE_GROUP_STATIC_WORLD, 1);
            }
            if (capture_live_world_pan && jim) {
                float visual_delta[3];
                capture_live_visual_delta(jim, capture_live_spawn,
                                          capture_live_jimmy_bounded,
                                          visual_delta);
                capture_scene_set_group_world_offset(CAPTURE_SCENE_GROUP_STATIC_WORLD,
                                                     -visual_delta[0],
                                                     -visual_delta[1],
                                                     -visual_delta[2]);
            }
            capture_scene_render(w.width, w.height);
            if (capture_live_jimmy)
                render_capture_live_jimmy(jim, jim_model_ok, capture_live_spawn,
                                          capture_live_jimmy_bounded,
                                          capture_live_world_pan,
                                          w.width, w.height);
            if (capture_live_hud)
                render_capture_live_hud(w.width, w.height);
        } else {
        renderer_begin_frame(w.width, w.height);

        ground_draw(world.ground_y);

        unsigned int box_vao = world_box_vao();
        int          box_idx = world_box_index_count();

        for (Entity *e = world.head; e; e = e->next) {
            if (!e->alive) continue;

            /* Player: dedicated anim path. */
            if (jim_model_ok && strcmp(e->type, "3JIM") == 0) {
                const AseModel *pose = player_anim_model((PlayerAnim)e->user_flag);
                if (pose) renderer_draw_model(pose, 0, e->x, e->y, e->z, e->ry, 1.0f);
                continue;
            }

            /* Pre-bound model from entity vtable (legacy path). */
            if (e->model) {
                renderer_draw_model(e->model, 0, e->x, e->y, e->z, 0.0f, 1.0f);
                continue;
            }

            /* Resolver path: tag override → FourCC default → invisible/box. */
            EntityVisual v;
            if (entity_visual_resolve(e, &v)) {
                if (v.invisible) continue;
                if (v.sprite_path) {
                    unsigned int tex = tex_cache_get(v.sprite_path);
                    if (tex) {
                        float sz = v.sprite_size > 0.0f ? v.sprite_size : 64.0f;
                        renderer_draw_billboard(tex, e->x, e->y, e->z, sz, sz,
                                                v.tint_r, v.tint_g, v.tint_b, v.tint_a);
                        continue;
                    }
                }
                if (v.model_path) {
                    AseModel *m = model_cache_get(v.model_path);
                    if (m) {
                        unsigned int tex = v.texture_path ? tex_cache_get(v.texture_path) : 0;
                        float sc = v.scale > 0.0f ? v.scale : 1.0f;
                        renderer_draw_model(m, tex, e->x, e->y, e->z, e->ry, sc);
                        continue;
                    }
                }
            }

            /* Fallback: colored placeholder box. */
            float r, g, b;
            entity_color(e->type, &r, &g, &b);
            float scale = 50.0f;
            if      (strcmp(e->type, "3TRE") == 0) scale = 100.0f;
            else if (strcmp(e->type, "LOAD") == 0) scale = 30.0f;
            else if (strcmp(e->type, "DOOR") == 0) scale = 80.0f;
            else if (strcmp(e->type, "PLAT") == 0) scale = 120.0f;
            else if (strcmp(e->type, "ITEM") == 0) scale = 28.0f;
            else if (strcmp(e->type, "TRIG") == 0) scale = 60.0f;
            renderer_draw_box(box_vao, box_idx, e->x, e->y, e->z, scale, r, g, b);
        }

        /* Static world geometry from level1.omt (Phase 8). OMT-exported ASEs
           localize X/Z around the chunk center; ase_load maps Max Y to GL -Z.
           Native Level 1 therefore draws at (center_x, 0, -center_z) so the
           map shares the same basis as solved keyframe cameras. Keep the old
           +Z placement for legacy/hybrid validation paths. */
        for (int pi = 0; pi < world.placement_count; pi++) {
            const WorldPlacement *pl = &world.placements[pi];
            if (native_level1) {
                /* Phase 5: trees and similar 2D-billboard meshes render as a
                   single camera-facing quad with measured capture geometry.
                   This precedes the AseModel path so we don't pay the load
                   cost for a mesh we never draw. */
                float bb_size = 0.0f;
                const char *bb_tex_path = NULL;
                if (billboard_overrides_lookup(pl->name, &bb_size, &bb_tex_path)) {
                    unsigned int bb_tex = tex_cache_get(bb_tex_path);
                    if (bb_tex) {
                        /* Phase 5: anchor the leaf-cluster quad to the
                           native trunk's top. The leaf texture is naturally
                           oriented (transparent above canopy, dark shadow
                           below) — after stbi_set_flip_vertically_on_load,
                           the canopy lands at v ~ 0.25..0.55 of the quad's
                           lower-mid. Centring the quad at the trunk top
                           moves that canopy band up to crown height while
                           the trunk fills the gap to ground. */
                        AseModel *trunk = model_cache_get(pl->ase_path);
                        float trunk_top = trunk ? trunk->max[1] : (bb_size * 0.5f);
                        renderer_draw_billboard(
                            bb_tex,
                            pl->x, trunk_top, -pl->z,
                            bb_size, bb_size,
                            1.0f, 1.0f, 1.0f, 1.0f);
                    }
                    continue;
                }
            }
            AseModel *pm = model_cache_get(pl->ase_path);
            if (!pm) continue;
            if (native_level1) {
                /* Phase 2: apply capture-derived texture overrides for
                   meshes whose OMT canvas chain didn't resolve. No-op for
                   meshes without an override entry. */
                texture_overrides_apply(pm, pl->name);
            }
            /* Faithfulness (audit D1/D2): the original game renders ZERO
               untextured geometry. A placement whose OMT material resolved no
               texture is either collision/blocking (BLOCKING_*, GROUND base
               slab) or a mesh whose canvas couldn't be resolved -- neither
               appears as a visible surface in the game. Skip them rather than
               draw dark filler slabs. native_level1 used to render them as
               debug_flat for audit, but the resulting huge gray BLOCKING_06
               (radius 12697) and SCHOOL slab were dominating the view; the
               coverage manifest tracks them, the renderer no longer needs to. */
            if (!model_has_texture(pm)) continue;
            float draw_z = native_level1 ? -pl->z : pl->z;
            renderer_draw_model(pm, 0, pl->x, 0.0f, draw_z, 0.0f, 1.0f);
        }

        renderer_end_frame();
        }
        capture_end_frame();
        cap_seq++;

        /* Capture frame budget exhausted: break so the normal shutdown path
           (input/SDL teardown) runs. No-op in non-capture builds. */
        if (capture_should_exit()) break;

        if (screenshot_mode && !screenshot_taken) {
            if (screenshot_warmup_ticks >= SCREENSHOT_WARMUP_GOAL) {
                glFinish();
                save_screenshot(screenshot_path, w.width, w.height);
                screenshot_taken = 1;
                w.should_quit = 1;
            } else {
                screenshot_warmup_ticks++;
            }
        }

        /* HUD: update window title with FPS, items, and player state. */
        frame_count++;
        Uint32 now_fps = SDL_GetTicks();
        if (now_fps - fps_time >= 500) {
            int fps = frame_count * 1000 / (now_fps - fps_time);
            const GameState *gs = gamestate_get();
            char title[256];
            snprintf(title, sizeof(title),
                     "JN Engine | FPS: %d | Items: %d/%d%s | Pos: (%.0f, %.0f, %.0f)%s",
                     fps,
                     gs->items_collected, gs->items_total,
                     gs->level_done ? " (CLEARED)" : "",
                     jim ? jim->x : 0.0f,
                     jim ? jim->y : 0.0f,
                     jim ? jim->z : 0.0f,
                     (jim && jim->on_ground) ? " G" : "");
            SDL_SetWindowTitle(w.sdl_win, title);
            frame_count = 0;
            fps_time = now_fps;
        }

        window_swap(&w);
    }

    capture_shutdown();
    capture_scene_destroy();
    player_anim_destroy();
    world_box_destroy();
    ground_destroy();
    world_destroy(&world);
    asset_cache_destroy_all();
    input_destroy();
    audio_destroy();
    renderer_destroy();
    window_destroy(&w);
    return 0;
}
