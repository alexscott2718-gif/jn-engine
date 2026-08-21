#include "../engine/window.h"
#include "../engine/glad.h"
#include "../engine/renderer.h"
#include "../engine/world.h"
#include "../engine/audio.h"
#include "../engine/input.h"
#include "../engine/physics.h"
#include "../engine/collision.h"
#include "../engine/ground.h"
#include "../engine/canon_data.h"   /* Phase 12: measured ground footprint/topography */
#include "../engine/phase1_sky_tint.h"  /* Phase 1: measured keyframe-8881 sky + scene tint */
#include "../engine/phase4_capture_state.h"  /* Phase 4: measured alpha/blend/fog state */
#include "../engine/capture.h"
#include "../engine/replay.h"
#include "../engine/assets/gam_loader.h"
#include "../engine/assets/ase_loader.h"
#include "../engine/assets/tex_loader.h"
#include "../engine/assets/asset_cache.h"
#include "../engine/assets/placement_loader.h"
#include "../engine/assets/billboard_overrides.h"
#include "../engine/assets/asset_paths.h"
#include "entities.h"
#include "entity_visual.h"
#include "behaviors/behaviors.h"
#include "behaviors/behavior_base.h"
#include "behaviors/behavior_projectile.h"
#include "behaviors/behavior_enemy.h"
#include "behaviors/behavior_vehicle.h"
#include "camera.h"
#include "camera_record.h"
#include "gamestate.h"
#include "game_flow.h"
#include "spawn.h"
#include "fixture.h"
#include "menu.h"
#include "help_overlay.h"
#include "gadget_menu.h"
#include "level_select.h"

/* The controls card greets the player at every level boot, then fades. */
#define HELP_BOOT_SECONDS 10.0f
#include "hud.h"
#include "animated_dispatch.h"
#include "player_anim.h"
#include "qa.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <zlib.h>
#include <SDL.h>

static int env_enabled(const char *name) {
    const char *s = getenv(name);
    return (s && s[0] && strcmp(s, "0") != 0) ? 1 : 0;
}

static float env_float_default(const char *name, float default_value) {
    const char *s = getenv(name);
    return (s && s[0]) ? (float)atof(s) : default_value;
}

static int env_int_default(const char *name, int default_value) {
    const char *s = getenv(name);
    return (s && s[0]) ? atoi(s) : default_value;
}

static void dump_entity_positions(const World *world) {
    const char *filter = getenv("JN_DUMP_ENTITY_TAG");
    if (!world || !filter || !filter[0]) return;
    int all = (strcmp(filter, "*") == 0 || strcmp(filter, "1") == 0);
    for (const Entity *e = world->head; e; e = e->next) {
        if (!e->alive) continue;
        if (!all && strcmp(e->tag, filter) != 0 && strcmp(e->type, filter) != 0)
            continue;
        printf("[entity_pos] type=%s tag=%s pos=(%.3f,%.3f,%.3f) "
               "vel=(%.3f,%.3f,%.3f) ry=%.3f patrol=%s state=%d timer=%.3f hp=%.1f\n",
               e->type, e->tag, e->x, e->y, e->z, e->vx, e->vy, e->vz,
               e->ry, e->patrol_point, e->user_flag, e->user_float, e->hp);
    }
}

/* ---- Debug coordinate overlay (QA) -------------------------------------
   A tiny self-contained 3x5 bitmap font drawn as screen-space quads, so it
   needs no font texture/asset and works in the WASM deploy. Shows the
   player's live draw-space position (X/Y/Z) + facing (F, degrees). Toggle
   with 'C'; default on. The values are in GL draw space -- the same space
   the placement scenery is drawn in -- so a position read here can be
   compared directly against placement coords (gam-authored Z = -Z shown). */
static int g_show_coords = 1;

/* 3x5 glyphs, 5 rows top->bottom; bits 4/2/1 = left/mid/right pixel. */
static const unsigned char *dbg_glyph(char c) {
    static const unsigned char
      g0[5]={7,5,5,5,7}, g1[5]={2,6,2,2,7}, g2[5]={7,1,7,4,7},
      g3[5]={7,1,7,1,7}, g4[5]={5,5,7,1,1}, g5[5]={7,4,7,1,7},
      g6[5]={7,4,7,5,7}, g7[5]={7,1,2,2,2}, g8[5]={7,5,7,5,7},
      g9[5]={7,5,7,1,7},
      gmin[5]={0,0,7,0,0}, gspc[5]={0,0,0,0,0}, gdot[5]={0,0,0,0,2},
      gcol[5]={0,2,0,2,0},
      gX[5]={5,5,2,5,5}, gY[5]={5,5,2,2,2}, gZ[5]={7,1,2,4,7},
      gF[5]={7,4,7,4,4};
    switch (c) {
      case '0':return g0; case '1':return g1; case '2':return g2;
      case '3':return g3; case '4':return g4; case '5':return g5;
      case '6':return g6; case '7':return g7; case '8':return g8;
      case '9':return g9; case '-':return gmin; case '.':return gdot;
      case ':':return gcol; case 'X':return gX; case 'Y':return gY;
      case 'Z':return gZ; case 'F':return gF; default:return gspc;
    }
}

static void dbg_text(int vw, int vh, float x, float y, float px,
                     const char *s, float r, float g, float b) {
    for (; *s; s++) {
        const unsigned char *gl = dbg_glyph(*s);
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 3; col++)
                if (gl[row] & (4 >> col))
                    renderer_draw_screen_rect(vw, vh,
                        x + col * px, y + row * px, px, px, r, g, b, 1.0f);
        x += 4.0f * px;   /* 3 wide + 1 column gap */
    }
}

/* Walking-camera world ray (camera_record_set_ray_probe): the native
   stand-in for the original's FUN_0047c210 -- smallest hit across the
   solid-entity AABBs and the baked collider soup. The world/player
   pointers are re-bound every FOLLOW frame so level swaps and respawns
   (which reallocate jim) can never dangle them. */
static const World  *g_camrec_world  = NULL;
static const Entity *g_camrec_player = NULL;

static int camrec_world_probe(const float src[3], const float eye[3],
                              float hit[3]) {
    if (!g_camrec_world) return 0;
    float t = world_query_segment(g_camrec_world, g_camrec_player,
                                  src[0], src[1], src[2],
                                  eye[0], eye[1], eye[2]);
    float tm = collision_segment(g_camrec_world->collision, src, eye, NULL);
    if (tm < t) t = tm;
    if (t >= 1.0f) return 0;
    hit[0] = src[0] + (eye[0] - src[0]) * t;
    hit[1] = src[1] + (eye[1] - src[1]) * t;
    hit[2] = src[2] + (eye[2] - src[2]) * t;
    return 1;
}

static void dbg_coords_overlay(int vw, int vh, const Entity *jim) {
    if (!g_show_coords || !jim) return;
    float deg = fmodf(jim->ry * 57.29578f, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    char l[4][24];
    snprintf(l[0], sizeof l[0], "X: %.0f", jim->x);
    snprintf(l[1], sizeof l[1], "Y: %.0f", jim->y);
    snprintf(l[2], sizeof l[2], "Z: %.0f", jim->z);
    snprintf(l[3], sizeof l[3], "F: %.0f", deg);
    const float px = 3.0f, lh = 5.0f * px + 4.0f, pad = 8.0f;
    const float ox = 12.0f, oy = 96.0f;   /* below the top-left HUD cluster */
    renderer_draw_screen_rect(vw, vh, ox - pad, oy - pad,
                              150.0f, lh * 4.0f + pad, 0.0f, 0.0f, 0.0f, 0.72f);
    for (int i = 0; i < 4; i++) {
        float g = (i == 3) ? 0.9f : 1.0f, b = (i == 3) ? 0.5f : 0.6f;
        dbg_text(vw, vh, ox, oy + lh * i, px, l[i], 0.6f, g, b);
    }
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

/* JN_AUDIT=1: faithfulness sweep mode. Every entity/placement draw decision
   emits one machine-readable line through the REAL resolution code path (no
   parallel reimplementation that could drift). tools/audit_faithfulness.py
   runs this across every level and asserts the invariants the QA tickets
   established (no placeholder boxes, no invisible textured-mesh draws, no
   missing assets, no visible BLOCK collision meshes, no stub meshes...).
   Lines repeat per frame; the harness dedups. */
static int audit_active(void) {
    static int v = -1;
    if (v == -1) v = env_enabled("JN_AUDIT") ? 1 : 0;
    return v;
}
static void audit_line(const char *cat, const char *name, const char *tag,
                       const char *kind, const char *path,
                       const AseModel *m, unsigned int tex_override) {
    if (!audit_active()) return;
    fprintf(stderr, "[audit] cat=%s name=%s tag=%s kind=%s path=%s "
            "verts=%d textured=%d\n",
            cat, name && name[0] ? name : "-", tag && tag[0] ? tag : "-",
            kind, path && path[0] ? path : "-",
            m ? m->vertex_count : -1,
            m ? (model_has_texture(m) || tex_override != 0)
              : (tex_override != 0));
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

/* mingw-w64 declares mkdir(const char *) with no mode argument, unlike POSIX.
   Everything else this file relies on -- access(), getpid(), <unistd.h> --
   compiles unchanged under zig's mingw headers; mkdir is the one exception. */
#ifdef _WIN32
#define JN_MKDIR(path) mkdir(path)
#else
#define JN_MKDIR(path) mkdir((path), 0777)
#endif

static int make_directory_tree(const char *path) {
    char buf[PATH_MAX];
    size_t len;
    if (!path || !path[0] || snprintf(buf, sizeof(buf), "%s", path) >= (int)sizeof(buf))
        return 0;
    len = strlen(buf);
    while (len > 1 && buf[len - 1] == '/') buf[--len] = '\0';
    for (char *p = buf + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (JN_MKDIR(buf) != 0 && errno != EEXIST) return 0;
        *p = '/';
    }
    return JN_MKDIR(buf) == 0 || errno == EEXIST;
}

static void dump_deterministic_state(FILE *f, unsigned int frame,
                                     const World *world) {
    const GameState *gs = gamestate_get();
    fprintf(f, "frame=%u game items=%d collected=%d gems=%d points=%d "
               "health=%d/%d done=%d inventory=%d active_tool=%d\n",
            frame, gs->items_total, gs->items_collected, gs->gems_collected,
            gs->points, gs->health, gs->health_max, gs->level_done,
            gs->inventory_count, gs->active_tool);
    unsigned int index = 0;
    for (const Entity *e = world->head; e; e = e->next, index++) {
        fprintf(f, "entity=%u type=%.4s tag=%s alive=%d visible=%d flags=%u "
                   "pos=%a,%a,%a rot=%a,%a,%a vel=%a,%a,%a "
                   "ground=%d state=%d timer=%a hp=%a anim=%a\n",
                index, e->type, e->tag[0] ? e->tag : "-", e->alive,
                e->visible, e->runtime_flags, (double)e->x, (double)e->y,
                (double)e->z, (double)e->rx, (double)e->ry, (double)e->rz,
                (double)e->vx, (double)e->vy, (double)e->vz, e->on_ground,
                e->user_flag, (double)e->user_float, (double)e->hp,
                (double)e->anim_time);
    }
}

/* Strip directory; try direct open then case-insensitive scan of the configured
   GAM root. Default is $JN_ASSET_ROOT/gam; JN_GAM_ROOT is a legacy override. */
#include <dirent.h>
#include <strings.h>
static int resolve_gam_path(const char *name, char *out, size_t out_size) {
    if (!name || !name[0]) return 0;
    const char *root = asset_root(JN_ASSET_GAM);
    /* Try as-is first. */
    asset_path_join(out, out_size, root, name);
    FILE *f = fopen(out, "rb");
    if (f) { fclose(f); return 1; }
    /* Case-insensitive scan. */
    DIR *d = opendir(root);
    if (!d) return 0;
    struct dirent *de;
    int found = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcasecmp(de->d_name, name) == 0) {
            asset_path_join(out, out_size, root, de->d_name);
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

typedef struct {
    char name[32];
    char gam_path[160];
    char placements_path[160];
    char billboard_overrides[160];
    char sky_type[32];
} LevelDesc;

static int file_exists_local(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void normalize_level_name(const char *name, char *out, size_t out_size) {
    if (!out_size) return;
    out[0] = '\0';
    if (!name) return;

    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;
    size_t n = strlen(base);
    if (n > 4 && strcasecmp(base + n - 4, ".gam") == 0)
        n -= 4;
    if (n >= out_size) n = out_size - 1;
    for (size_t i = 0; i < n; i++)
        out[i] = (char)tolower((unsigned char)base[i]);
    out[n] = '\0';
}

static int level_desc_for(const char *name, LevelDesc *desc) {
    char gam_name[64];
    if (!desc) return 0;
    memset(desc, 0, sizeof(*desc));
    normalize_level_name(name, desc->name, sizeof(desc->name));
    if (!desc->name[0]) return 0;
    if (strcmp(desc->name, "fixture0") == 0)
        return 1;

    snprintf(gam_name, sizeof(gam_name), "%s.gam", desc->name);
    if (!resolve_gam_path(gam_name, desc->gam_path, sizeof(desc->gam_path)))
        return 0;

    const char *placements_root = asset_root(JN_ASSET_PLACEMENTS);
    char placements_name[80];
    snprintf(placements_name, sizeof(placements_name), "%s_placements.txt", desc->name);
    asset_path_join(desc->placements_path, sizeof(desc->placements_path),
                    placements_root, placements_name);
    if (!file_exists_local(desc->placements_path))
        desc->placements_path[0] = '\0';

    const char *native_root = asset_root(JN_ASSET_NATIVE);
    char overrides_name[96];
    snprintf(overrides_name, sizeof(overrides_name), "%s_billboard_overrides.txt",
             desc->name);
    asset_path_join(desc->billboard_overrides, sizeof(desc->billboard_overrides),
                    native_root, overrides_name);
    if (!file_exists_local(desc->billboard_overrides))
        desc->billboard_overrides[0] = '\0';

    if (strncmp(desc->name, "level1", 6) == 0 ||
        strncmp(desc->name, "level2", 6) == 0) {
        snprintf(desc->sky_type, sizeof(desc->sky_type), "bluesky3");
    } else if (strncmp(desc->name, "level5", 6) == 0) {
        snprintf(desc->sky_type, sizeof(desc->sky_type), "dusksky");
    }
    return 1;
}

/* Headless AMI test (JN_TEST_AMI=1): drive every action-menu request id
   through the real dispatch and print the mode and VR route it produces, so
   the transcribed 00428d50 switch can be diffed against the evidence rather
   than trusted. Runs once at startup and does not touch the world. */
static void ami_dump_if_requested(void) {
    if (!env_enabled("JN_TEST_AMI")) return;
    for (int id = 0; id <= AMI_ID_MAX; id++) {
        int before = action_mode();
        int mode = ami_dispatch(id);
        const char *vr = ami_vr_level(id);
        printf("[AMITABLE] id=%d mode=%d changed=%d vr=%s\n",
               id, mode, mode != before, vr ? vr : "-");
        action_mode_set(ACTION_MODE_NONE);
    }
    printf("[AMITABLE] done\n");
}

/* Headless picture-economy test (JN_TEST_PICTURES=1): force every authored
   pickup row in the loaded level through its own on_trigger, repeatedly until
   no further collection happens. Two of the three award paths are otherwise
   unreachable headlessly -- 3PIC needs a physical player overlap, and the
   3FIS/3GIR/3DIN creatures are deliberately non-trigger -- and a single pass is
   not a fixpoint, because collecting a picture can open a gate the sweep
   already walked past. Runs on the launch level and again after every swap, so
   JN_TEST_SWAP=<same level> proves re-entry awards nothing a second time.
   tools/verify_picture_economy.py drives this. */
static void picture_sweep_if_requested(World *world) {
    ami_dump_if_requested();
    if (!env_enabled("JN_TEST_PICTURES")) return;
    int pass = 0, took = 0, total = 0;
    while ((took = behavior_pickup_sweep_collect(world)) > 0) {
        total += took;
        printf("[PICSWEEP] pass %d collected %d\n", ++pass, took);
        if (pass > 64) break;   /* cannot happen: every pass consumes rows */
    }
    printf("[PICSWEEP] level=%s done: %d collected in %d pass(es)\n",
           gamestate_level(), total, pass);

    /* JN_TEST_SCOOTER=1: exercise the action-menu selection headlessly --
       mount, drive one step, dismount -- since the menu itself needs a window. */
    /* JN_TEST_GADGETRUN=1: activate each gadget in turn and report what it
       did, since the menu itself needs a window. */
    /* JN_TEST_TARGETS=1: hit the linked shooting-range target until its
       threshold trips, waiting out RespawnTime between hits, and report
       whether the linked object was activated. */
    if (env_enabled("JN_TEST_TARGETS")) {
        Entity *tgt = NULL;
        for (Entity *e = world->head; e; e = e->next) {
            if (!e->alive || strncmp(e->type, "3TAR", 4) != 0) continue;
            const char *lk = gam_str(e, "ActivateObject", "");
            if (lk && lk[0] && strcasecmp(lk, "none") != 0) { tgt = e; break; }
        }
        if (!tgt) {
            printf("[TARGETTEST] no linked target in this level\n");
        } else {
            const char *link = gam_str(tgt, "ActivateObject", "");
            int need = gam_prop_i(tgt, "HitsRequired", 3);
            float respawn = gam_prop_f(tgt, "RespawnTime", 5.0f);
            int before = gamestate_get()->points;
            printf("[TARGETTEST] target '%s' link='%s' need=%d respawn=%.0f\n",
                   tgt->tag, link, need, respawn);
            for (int h = 0; h < need; h++) {
                int took = behavior_moving_target_take_hit(tgt, world);
                printf("[TARGETTEST]   hit %d/%d took=%d visible=%d\n",
                       h + 1, need, took, tgt->visible);
                /* Wait out the cooldown so it stands back up for the next one. */
                for (float t = 0.0f; t <= respawn + 0.1f; t += 1.0f / 60.0f)
                    entity_update(tgt, world, 1.0f / 60.0f);
            }
            printf("[TARGETTEST] points %d -> %d (+%d), expected +%d\n",
                   before, gamestate_get()->points, gamestate_get()->points - before,
                   need * gam_prop_i(tgt, "NumPoints", 0));
        }
    }

    if (env_enabled("JN_TEST_GADGETRUN")) {
        /* Each gadget, and the FourCC it is supposed to put in the world.
           "----" means the gadget acts on Jimmy or a companion and spawns
           nothing of its own. */
        static const struct { const char *tag; const char *fourcc; } ALL[] = {
            { "jetpack",      "3JFI" },   /* C3DJetpackFire, the flame     */
            { "shrinkray",    "3SHR" },   /* C3DShrinkRay, the fired ray   */
            { "bubble",       "3BUB" },   /* C3DBubble                     */
            { "grappler",     "3GRA" },   /* C3DGraplingHook rope          */
            { "goddard",      "----" },   /* drives the companion's mode   */
            { "rocket",       "----" },   /* boards a placed 3ROC          */
            { "scooter",      "----" },   /* reveals the hidden 3JEE       */
            { "invisibility", "----" },   /* hides Jimmy                   */
        };
        for (size_t i = 0; i < sizeof(ALL) / sizeof(ALL[0]); i++) {
            /* Settle first: the shared fire cooldown and anything still in
               flight from the previous gadget would otherwise be counted
               here, or would block this one from firing at all. */
            for (int f = 0; f < 120; f++) {
                behavior_gadgets_update(world, 1.0f / 60.0f);
                for (Entity *e = world->head; e; e = e->next)
                    entity_update(e, world, 1.0f / 60.0f);
            }
            /* level4b's single hook sits ~10600 units from the start, so a
               grapple fired at spawn correctly finds nothing. Stand Jimmy next
               to it first, or this step only ever proves the reach check. */
            if (strcmp(ALL[i].tag, "grappler") == 0 && g_player) {
                for (Entity *h = world->head; h; h = h->next)
                    if (h->alive && strncmp(h->type, "3HOO", 4) == 0) {
                        g_player->x = h->x + 300.0f;
                        g_player->y = h->y;
                        g_player->z = h->z;
                        break;
                    }
            }
            int on = behavior_gadget_activate(ALL[i].tag, world);
            int fired = behavior_gadget_fire(world);
            behavior_gadgets_update(world, 1.0f / 60.0f);

            int n = 0;
            if (strcmp(ALL[i].fourcc, "----") != 0)
                for (Entity *e = world->head; e; e = e->next)
                    if (e->alive && strncmp(e->type, ALL[i].fourcc, 4) == 0) n++;
            printf("[GADGETRUN] %-13s on=%d fired=%d mode=%2d %s=%d\n",
                   ALL[i].tag, on, fired, action_mode(), ALL[i].fourcc, n);
            behavior_gadget_activate(ALL[i].tag, world);
        }
        /* The shrink ray end to end: stand Jimmy at a shrinkable creature,
           fire, and watch the target scale down and turn into a pickup. This
           is the half the decomp could not give us -- the hit/contact body was
           never recovered -- so it is worth proving rather than assuming. */
        {
            Entity *target = NULL;
            for (Entity *e = world->head; e; e = e->next)
                /* NOT filtered on visible: every shrinkable creature in the
                   corpus authors a RequiredLevel between 10 and 260, so in a
                   cold --level run they are all story-gated off and the ray
                   has nothing to shoot. The test un-gates one to exercise the
                   hit path; in a real playthrough the story does that. */
                if (e->alive &&
                    (strncmp(e->type, "3DIN", 4) == 0 ||
                     strncmp(e->type, "3FIS", 4) == 0 ||
                     strncmp(e->type, "3GIR", 4) == 0 ||
                     strncmp(e->type, "3HUM", 4) == 0)) { target = e; break; }
            if (!target) {
                printf("[SHRINKTEST] no shrinkable creature in this level\n");
            } else if (g_player) {
                target->visible = 1;              /* un-gate for the test */
                unsigned int saved = g_player->runtime_flags;
                g_player->runtime_flags = 0;      /* no gravity while parked */
                g_player->x = target->x;
                g_player->y = target->y;
                g_player->z = target->z - 200.0f;
                g_player->ry = 0.0f;
                g_player->vx = g_player->vy = g_player->vz = 0.0f;
                behavior_gadget_activate("shrinkray", world);
                for (int f = 0; f < 120; f++)
                    behavior_gadgets_update(world, 1.0f / 60.0f);
                int fired = behavior_gadget_fire(world);
                for (int f = 0; f < 90; f++) {
                    behavior_gadgets_update(world, 1.0f / 60.0f);
                    for (Entity *e = world->head; e; e = e->next)
                        entity_update(e, world, 1.0f / 60.0f);
                }
                printf("[SHRINKTEST] target alive=%d visible=%d at (%.0f,%.0f,%.0f) player (%.0f,%.0f,%.0f)\n",
                       target->alive, target->visible, target->x, target->y, target->z,
                       g_player->x, g_player->y, g_player->z);
                printf("[SHRINKTEST] %s '%s' fired=%d scale=%.2f trigger=%d pts=%d\n",
                       target->type, target->tag, fired, target->draw_scale,
                       (target->runtime_flags & ENTITY_FLAG_TRIGGER) != 0,
                       target->points);
                behavior_gadget_activate("shrinkray", world);
                g_player->runtime_flags = saved;
            }
        }
        printf("[GADGETRUN] done\n");
    }

    if (env_enabled("JN_TEST_SCOOTER")) {
        Entity *sc = behavior_scooter_get();
        printf("[SCOOTERTEST] spawned=%d visible=%d\n",
               sc != NULL, sc ? sc->visible : -1);
        int on = behavior_scooter_activate();
        printf("[SCOOTERTEST] activate -> riding=%d visible=%d\n",
               on, sc ? sc->visible : -1);
        if (sc) entity_update(sc, world, 1.0f / 60.0f);
        behavior_scooter_activate();
        printf("[SCOOTERTEST] deactivate -> riding=%d visible=%d\n",
               behavior_scooter_riding(), sc ? sc->visible : -1);
    }

    /* Both names, so a mismatch between what the designer called a row and
       what the artist drew is visible instead of silent. */
    for (int i = 0; ; i++) {
        const InventorySlot *g = gamestate_gadget_at(i);
        if (!g) break;
        const char *art = g->sprite > 0 ? sprite_chunk_name(g->sprite) : "";
        printf("[INVDUMP] slot=%d tag='%s' sprite=%d art='%s'\n",
               i, g->tag, g->sprite, art);
    }
}

static int load_level(const LevelDesc *desc, World *world) {
    if (strcmp(desc->name, "fixture0") == 0)
        return fixture_level_build(world);
    int n = gam_load(world, desc->gam_path);
    if (n < 0) return -1;
    if (desc->placements_path[0])
        placements_load(world, desc->placements_path);
    if (desc->billboard_overrides[0])
        billboard_overrides_load(desc->billboard_overrides);
    else
        billboard_overrides_load("");
    /* Build the mesh collision world from the collider placements (GROUND +
       BLOCKING_* / BLOCK_*). Needs the collider meshes in the cache, which a
       live GL context provides — same requirement as the render path. */
    collision_free(world->collision);
    world->collision = collision_build(world);
    return n;
}

static void configure_level_sky(const LevelDesc *desc,
                                AseModel **sky_model,
                                unsigned int *clouds_tex) {
    renderer_set_scene_tint(1.0f, 1.0f, 1.0f);
    *sky_model = NULL;
    *clouds_tex = 0;

    if (strcmp(desc->sky_type, "bluesky3") == 0) {
        renderer_set_sky(0.09f, 0.40f, 0.62f,
                         0.24f, 0.50f, 0.68f);
        *sky_model = model_cache_get("assets/glb/sky/bluesky3.glb");
        *clouds_tex = tex_cache_get("assets/glb/sky/clouds.png");
        fprintf(stderr, "[native_level] faithful bluesky3 dome %s; neutral scene tint\n",
                *sky_model ? "loaded" : "MISSING");
    } else if (strcmp(desc->sky_type, "dusksky") == 0) {
        renderer_set_sky(0.24f, 0.18f, 0.33f,
                         0.78f, 0.40f, 0.24f);
    } else {
        renderer_set_sky(0.10f, 0.32f, 0.48f,
                         0.34f, 0.50f, 0.58f);
    }
}

static void configure_safety_floor(World *world, const Entity *jim) {
    float min_x = jim ? jim->x : 0.0f;
    float max_x = min_x;
    float min_z = jim ? jim->z : 0.0f;
    float max_z = min_z;
    int have_bounds = jim != NULL;

    for (int i = 0; i < world->placement_count; i++) {
        const WorldPlacement *pl = &world->placements[i];
        float x = pl->x;
        float z = -pl->z;
        if (!have_bounds) {
            min_x = max_x = x;
            min_z = max_z = z;
            have_bounds = 1;
        } else {
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (z < min_z) min_z = z;
            if (z > max_z) max_z = z;
        }
    }

    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive) continue;
        if (!have_bounds) {
            min_x = max_x = e->x;
            min_z = max_z = e->z;
            have_bounds = 1;
        } else {
            if (e->x < min_x) min_x = e->x;
            if (e->x > max_x) max_x = e->x;
            if (e->z < min_z) min_z = e->z;
            if (e->z > max_z) max_z = e->z;
        }
    }

    if (!have_bounds) {
        min_x = min_z = -20000.0f;
        max_x = max_z =  20000.0f;
    }

    int excluded = 0;          /* level opts out of the safety floor entirely */
    const float margin = 12000.0f;
    float cx = (min_x + max_x) * 0.5f;
    float cz = (min_z + max_z) * 0.5f;
    float half_x = (max_x - min_x) * 0.5f + margin;
    float half_z = (max_z - min_z) * 0.5f + margin;
    if (half_x < 25000.0f) half_x = 25000.0f;
    if (half_z < 25000.0f) half_z = 25000.0f;

    /* Collision overhaul (Phase 1): the mesh CollisionWorld is now the
       authoritative floor. The safety floor is demoted to a backstop that only
       activates where there is no collider geometry under the entity. It stays
       ON by default because most levels' walkable surface is still a plain
       visible mesh (not a GROUND/BLOCK collider) — only level1/level2/level3a
       ship a GROUND floor mesh, so disabling it here would drop the player
       through every other level. Set JN_SAFETY_FLOOR=0 to disable it and test
       mesh-floor sufficiency per level (the path toward deleting it once the
       remaining levels gain real collider floors). safety_floor_y/geometry are
       computed unconditionally so the visible procedural ground keeps drawing
       at the same height regardless of the gate. */
    {
        /* Per-level opt-out. The backstop stays on by default for the reason
           above, but Area 51 (level5) is authored to be fallen out of and the
           flat plane 1000 units under the ground gets in the way of that, so
           it is excluded (owner, 2026-08-21).

           Note this only disables the *collision* backstop. The procedural
           ground below still draws, because ground_init runs unconditionally
           so the visible floor keeps its height regardless of the gate --
           removing the visual too is a separate call. */
        static const char *NO_SAFETY_FLOOR[] = { "level5" };
        const char *lvl = game_flow_current_level();
        excluded = 0;
        for (size_t i = 0; lvl && i < sizeof(NO_SAFETY_FLOOR) /
                                     sizeof(NO_SAFETY_FLOOR[0]); i++)
            if (strcasecmp(lvl, NO_SAFETY_FLOOR[i]) == 0) { excluded = 1; break; }

        const char *s = getenv("JN_SAFETY_FLOOR");
        if (s && strcmp(s, "0") == 0)      world->safety_floor_enabled = 0;
        else if (s && strcmp(s, "1") == 0) world->safety_floor_enabled = 1;
        else                               world->safety_floor_enabled = !excluded;
        if (excluded)
            fprintf(stderr, "[safety_floor] disabled for '%s'\n", lvl);
    }
    world->safety_floor_y = world->ground_y - 1000.0f;
    world->safety_floor_cx = cx;
    world->safety_floor_cz = cz;
    world->safety_floor_half_x = half_x;
    world->safety_floor_half_z = half_z;

    /* The collision backstop and the visible plane are two separate things:
       the gate above only turns off the former, and ground_init used to run
       unconditionally so the procedural ground kept drawing regardless. For a
       level that opts out, neither should be there -- so tear it down instead
       of rebuilding it. (Owner, 2026-08-21: the first pass removed the floor
       you could stand on and left the floor you could see.) */
    if (!world->safety_floor_enabled && excluded) {
        ground_destroy();
    } else {
        unsigned int ground_tex = tex_cache_get(CANON_GROUND_TEXTURE);
        ground_init(ground_tex, half_x, half_z, cx, cz, 120.0f, 0.0f);
    }
    fprintf(stderr,
            "[safety_floor] %s y=%.1f center=(%.0f, %.0f) half=(%.0f, %.0f)\n",
            world->safety_floor_enabled ? "on" : "off",
            world->safety_floor_y, cx, cz, half_x, half_z);
}

/* Headless CLoadLevel portal probe (JN_TEST_LOAD=1). For every LOAD row in the
   loaded level: print what it authors, what the recovered gate (00457ec0)
   decides at the current story state, and then -- one portal at a time, with
   the pending request drained in between -- what firing it actually asks the
   loader for. Firing is a direct on_trigger call, so this observes the ported
   body itself rather than a flag.

   Pair it with JN_TEST_SET_SCENE=<n> to move the story counter, and with
   JN_TEST_LOAD_RETURN=<level>[:<spawn>] to seed a departure point so the VR
   levels' RETURN portals have somewhere to go.

   Returns 0 always: it is a probe, not an assertion. What it prints is the
   evidence -- compare it against the authored .gam rows. */
static int load_portal_probe(World *world, Entity *jim) {
    if (!jim) { printf("[JN_TEST_LOAD] FAIL: no player\n"); return 1; }
    int portals = 0;
    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive || strcmp(e->type, "LOAD") != 0) continue;
        portals++;
        const char *task = gam_str(e, "RequiredTask", "none");
        printf("[JN_TEST_LOAD] portal tag=%-20s level=%-14s spawn=%-14s "
               "task=%-8s req=%-5d exact=%-5d radius=%.0f gate=%s\n",
               e->tag, e->target_level[0] ? e->target_level : "(empty)",
               e->start_point[0] ? e->start_point : "(empty)",
               task, gam_prop_i(e, "RequiredLevel", -1),
               gam_prop_i(e, "ExactLevel", -1), e->half_extents[0],
               behavior_load_gate_allows(e) ? "OPEN" : "SHUT");
    }
    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive || strcmp(e->type, "LOAD") != 0) continue;
        if (!e->vt || !e->vt->on_trigger) continue;
        e->vt->on_trigger(e, jim);
        const GameState *gs = gamestate_get();
        printf("[JN_TEST_LOAD] fire tag=%-20s -> request=%s spawn=%s\n",
               e->tag, gs->swap_level[0] ? gs->swap_level : "(none)",
               gs->swap_spawn[0] ? gs->swap_spawn : "(none)");
        /* Drain the request so the next portal is observed on its own. This
           also clears level_done/spawn_set, which is why the probe exits
           straight afterwards. */
        gamestate_reset_for_new_level();
    }
    printf("[JN_TEST_LOAD] %s: %d portal(s)\n", gamestate_level(), portals);
    return 0;
}

/* Headless collision self-test (JN_TEST_COLLIDE=1). Validates the mesh
   CollisionWorld end-to-end without a windowed session:
     (1) Ground-follow: from spawn, run physics ticks with the safety floor
         forced OFF; a pass proves the MESH floor (not a safety plane) caught
         the player and that on_ground latched.
     (2) Wall clamp: place the player penetrating the nearest BLOCKING wall and
         drive into it; assert collision_resolve_horizontal pushes it back out
         (signed distance >= player radius) with no tunnelling.
   Prints PASS/FAIL/SKIP lines; returns 0 on all-pass, 1 on any failure. */
static int collide_self_test(World *world, Entity *jim) {
    const float DT = 1.0f / 60.0f;
    int fails = 0;
    if (!jim) { printf("[JN_TEST_COLLIDE] FAIL: no player\n"); return 1; }

    int saved_safety = world->safety_floor_enabled;
    world->safety_floor_enabled = 0;   /* a landing now can only be the mesh */

    /* (1) Land from spawn. SKIP (not FAIL) where the spawn has no collider
       floor under it — most levels' walkable surface is still a plain visible
       mesh, not a GROUND/BLOCK collider, so they legitimately rely on the
       (here-disabled) safety floor. Where a mesh floor exists, a pass proves it
       (not a safety plane) caught the player and on_ground latched. */
    float spawn_feet = jim->y - jim->half_extents[1];
    float spawn_mesh = 0.0f;
    int spawn_has_floor = world->collision &&
        collision_ground_height(world->collision, jim->x, jim->z,
                                spawn_feet, &spawn_mesh, NULL);
    if (!spawn_has_floor) {
        printf("[JN_TEST_COLLIDE] SKIP land: no collider floor under spawn "
               "(%.0f,%.0f) — level relies on the safety floor\n", jim->x, jim->z);
    } else {
        input_set_virtual_move(0.0f, 0.0f);
        for (int i = 0; i < 90; i++) physics_step(world, DT);
        float feet = jim->y - jim->half_extents[1];
        if (jim->on_ground && fabsf(feet - spawn_mesh) < 5.0f) {
            printf("[JN_TEST_COLLIDE] PASS land: feet=%.1f mesh=%.1f on_ground=1\n",
                   feet, spawn_mesh);
        } else {
            printf("[JN_TEST_COLLIDE] FAIL land: feet=%.1f mesh=%.1f on_ground=%d\n",
                   feet, spawn_mesh, jim->on_ground);
            fails++;
        }
    }

    /* (2) Wall clamp against the nearest TALL BLOCKING wall. Replicate
       gameplay: stand the player with its feet at the wall's base and drive
       straight into the face. (A closest-point push only recovers a cylinder
       touching a face from the front — the only way the player meets a wall in
       play — and seating the feet at the base keeps the wall well above the
       step cap so it blocks rather than reading as a steppable curb.) The wall
       should clamp the player near +radius in front of the surface; a tunnel
       shows up as the signed distance going negative. Y is pinned to the base
       so the test isolates the horizontal resolve from the ground sample (which
       would otherwise find the wall's own roof beside it). */
    float p[3] = { jim->x, jim->y, jim->z };
    float wpt[3], wn[3], wymin = 0.0f, wymax = 0.0f;
    if (world->collision &&
        collision_nearest_wall(world->collision, p, 4000.0f, wpt, wn,
                               &wymin, &wymax)) {
        float radius = fmaxf(jim->half_extents[0], jim->half_extents[2]);
        float seat_y = wymin + jim->half_extents[1];   /* feet at the wall base */
        jim->x = wpt[0] + wn[0] * (radius + 30.0f);     /* just outside the face */
        jim->z = wpt[2] + wn[2] * (radius + 30.0f);
        jim->y = seat_y;
        jim->vx = jim->vy = jim->vz = 0.0f;
        float min_sd = 1.0e30f;   /* deepest signed distance from the surface */
        for (int i = 0; i < 120; i++) {
            jim->vx = -wn[0] * 600.0f;   /* drive straight into the wall */
            jim->vz = -wn[2] * 600.0f;
            physics_step(world, DT);
            jim->y = seat_y; jim->vy = 0.0f;   /* pin height: keep the Y overlap */
            float sd = (jim->x - wpt[0]) * wn[0] + (jim->z - wpt[2]) * wn[2];
            if (sd < min_sd) min_sd = sd;
        }
        if (min_sd > -10.0f) {
            printf("[JN_TEST_COLLIDE] PASS wall: min_signed_dist=%.1f radius=%.1f "
                   "wall_h=%.0f\n", min_sd, radius, wymax - wymin);
        } else if (!spawn_has_floor) {
            /* No floor at spawn => this is a level the player can't stand in yet
               (relies on the safety floor), and the spawn-nearest wall is often a
               short/edge segment the synthetic head-on drive rounds the end of
               (legitimately). Report, but don't hard-fail: wall clamping is only
               a hard assertion where the player can actually stand. */
            printf("[JN_TEST_COLLIDE] WARN wall: min_signed_dist=%.1f radius=%.1f "
                   "(rounded a finite wall; no spawn floor — not asserted)\n",
                   min_sd, radius);
        } else {
            printf("[JN_TEST_COLLIDE] FAIL wall: min_signed_dist=%.1f radius=%.1f "
                   "(penetrated/tunnelled)\n", min_sd, radius);
            fails++;
        }
    } else {
        printf("[JN_TEST_COLLIDE] SKIP wall: no tall BLOCKING wall near spawn\n");
    }

    world->safety_floor_enabled = saved_safety;
    printf("[JN_TEST_COLLIDE] %s (%d failure%s)\n",
           fails ? "FAIL" : "PASS", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
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

static int entity_visual_foot_anchors(const Entity *e) {
    static const char *types[] = {
        "3BEN", "3CAR", "3HUM", "3LIB", "3MOM", "3NIC",
        "3SHE", "3ULT", "3FOW", "3SPK",
        /* stop-pose NPCs omitted from the original list -> sank/floated by
           their negative local-Y bounds (QA 2026-06-14: 3HUG l4a, 3CIN l3d). */
        "3HUG", "3CIN"
    };
    if (!e) return 0;
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (strncmp(e->type, types[i], 4) == 0) return 1;
    }
    return 0;
}

static int water_surface_at(const World *world, float x, float z, float *out_y) {
    if (!world || !out_y) return 0;
    int found = 0;
    float best_y = -1.0e30f;

    for (int i = 0; i < world->placement_count; i++) {
        const WorldPlacement *pl = &world->placements[i];
        if (strncasecmp(pl->name, "ncwater", 7) != 0)
            continue;

        AseModel *m = model_cache_get(pl->ase_path);
        if (!m) continue;

        /* Static world placements draw at authored X/Y and flipped Z; water
           meshes bake their surface height in local vertices. */
        float min_x = pl->x + m->min[0];
        float max_x = pl->x + m->max[0];
        float draw_z = -pl->z;
        float min_z = draw_z + m->min[2];
        float max_z = draw_z + m->max[2];
        if (x < min_x || x > max_x || z < min_z || z > max_z)
            continue;

        if (!found || m->max[1] > best_y) {
            best_y = m->max[1];
            found = 1;
        }
    }

    if (!found) return 0;
    *out_y = best_y;
    return 1;
}

static int entity_visual_water_anchors(const Entity *e) {
    return e && strncmp(e->type, "3SAI", 4) == 0;
}

/* Per-entity uniform mesh scale. 0 means unset, which is 1.0; the shrink ray
   drives this down on its targets. */
static float ent_draw_scale(const Entity *e) {
    return e->draw_scale > 0.0f ? e->draw_scale : 1.0f;
}

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static int draw_authored_button(const Entity *e) {
    if (!e || (strcmp(e->type, "3BUT") != 0 && strcmp(e->type, "3WAB") != 0))
        return 0;

    const char *fallback_up = strcmp(e->type, "3WAB") == 0
        ? "buttondown.ASE" : "buttonup.ASE";
    const char *mesh_name = e->user_flag
        ? gam_str(e, "Down.ase", "buttondown.ASE")
        : gam_str(e, "Up.ase", fallback_up);

    char mesh_path[160];
    if (!mesh_name || !mesh_name[0] ||
        !asset_path_ci(mesh_path, sizeof(mesh_path), "assets/ase", mesh_name)) {
        audit_line("entity", e->type, e->tag, "mesh-missing",
                   mesh_name ? mesh_name : "(button)", NULL, 0);
        return 0;
    }

    AseModel *m = model_cache_get(mesh_path);
    if (!m) {
        audit_line("entity", e->type, e->tag, "mesh-missing",
                   mesh_path, NULL, 0);
        return 0;
    }

    unsigned int tex = 0;
    const char *png_name = gam_str(e, "UpDown.Png", NULL);
    char tex_path[160];
    if (png_name && png_name[0] &&
        asset_path_ci(tex_path, sizeof(tex_path), "assets/png", png_name)) {
        tex = tex_cache_get(tex_path);
    }

    float red   = clamp01(gam_prop_f(e, "Red",   1.0f));
    float green = clamp01(gam_prop_f(e, "Green", 1.0f));
    float blue  = clamp01(gam_prop_f(e, "Blue",  1.0f));
    float pulse = 0.5f + 0.5f * sinf(e->anim_time * 6.2831853f * 2.0f);
    renderer_set_model_tint(red * pulse, green * pulse, blue * pulse);
    renderer_set_hide_untextured_groups(0);
    renderer_draw_model(m, tex, e->x, e->y, e->z, e->ry, ent_draw_scale(e));
    renderer_set_hide_untextured_groups(1);
    renderer_set_model_tint(1.0f, 1.0f, 1.0f);

    audit_line("entity", e->type, e->tag, "mesh", mesh_path, m, tex);
    return 1;
}

/* C3DPickupItem ShowArrow (0x6a0, ctor default 1). Observed in the original
   2026-08-20: a red arrow spins clockwise above the pickup and points down at
   it. 353 rows author the flag and every one is a 3PIC; 347 are 1. The asset is
   assets/ase/3Darrow.ASE, whose own export path reads
   "D:\Jimmy (ken)\3D Items (pick up)\3D ARROW\3Darrow.bmp".

   Measurements are from the owner's description at the Level 1A candy machine:
   the arrow's lower tip sits a little below the top of Jimmy's head and it is
   about the size of his torso. Jimmy's mesh spans 209 units with his head
   145 above his origin; the arrow mesh is 103 tall, so a 0.70 scale gives ~72
   units (a torso) and a 110-unit rise puts it in that band above a low pickup.

   Drawn for authored-inactive pickups too: the only rows that switch the flag
   off (gcan, refill, rescue) are themselves InitallyActive=0, which would be
   pointless if inactive pickups never carried one. Not drawn once collected.

   NOT translucent yet -- the model path has no alpha parameter and the texture
   has no alpha channel, so that needs a renderer change (see the plan, 15.3). */
/* pointarrow.ASE, not 3Darrow.ASE. 3Darrow is the flat source spline
   (node "Line04", zero depth); pointarrow is the real 3D arrow -- 11 verts,
   55 x 13 x 76 -- and the artist shipped a Jimmy reference model in the same
   file, which is how the scale and height below are measured rather than
   guessed. Its own *BITMAP is D:
eutron
un\pngDarrow.png, the red
   swatch, so both files share that texture and the catalog name. */
#define PICKUP_ARROW_MODEL "assets/ase/pointarrow.ASE"
#define PICKUP_ARROW_TEX   "assets/png/3Darrow.png"
#define PICKUP_ARROW_RISE  0.0f     /* none needed: in the authoring scene the
                                      arrow spans 39.7..115.2 above the origin
                                      while Jimmy spans 0.6..215.3, so the mesh
                                      is built with the ITEM at the origin and
                                      the arrow already floating over it. */
#define PICKUP_ARROW_SCALE 1.0f     /* authored size: 75.5 tall against a 214.7
                                      Jimmy in the same file -- 35% of him, i.e.
                                      a torso, exactly as observed. */
#define PICKUP_ARROW_SPIN  2.0f     /* rad/s; negated below for clockwise */
#define PICKUP_ARROW_ALPHA 0.35f     /* observed translucent; exact value unmeasured */

/* C3DArrow (3ARR), the yellow navigation billboard on sprites.omt canvas 33.
   Observed 2026-08-20: "a vertical pulse scale wave with the red tint glow
   pulse". So the arrow stretches and squashes along its HEIGHT only -- the
   width holds -- and the red tint rides the same wave as a glow rather than
   being painted on constantly. One phase drives both, so the stretch and the
   glow peak together. Numbers are calibration guesses; the mechanism is what
   was observed. */
#define NAV_ARROW_PULSE_AMP  0.38f   /* +/- fraction of the authored HEIGHT */
#define NAV_ARROW_PULSE_RATE 6.5f    /* rad/s, shared by stretch and glow */
#define NAV_ARROW_GLOW_R     1.00f   /* tint at the red end of the pulse; */
#define NAV_ARROW_GLOW_G     0.45f   /* the other end is untinted yellow */
#define NAV_ARROW_GLOW_B     0.35f

/* Advanced once per frame by the main loop, never per entity -- draw_scene runs
   twice a frame when the QA pick pass is armed. */
static float s_arrow_clock = 0.0f;

static void draw_pickup_arrow(const Entity *e) {
    if (strncmp(e->type, "3PIC", 4) != 0) return;
    if (gam_prop_i(e, "ShowArrow", -1) != 1) return;
    /* The arrow marks what you can take right now. user_flag is latched by
       collection AND by the InitallyActive spawn gate, and both should stop
       the arrow: a collected pickup is gone, and an undispensed one is not
       yours yet. Observed at the vending machine -- before paying the arrow
       sits mid-machine (that is the machine trigger's own arrow) and only
       after dispensing does one appear above the product. */
    if (e->user_flag) return;
    AseModel *m = model_cache_get(PICKUP_ARROW_MODEL);
    if (!m) return;
    unsigned int tex = tex_cache_get(PICKUP_ARROW_TEX);
    renderer_set_model_alpha(PICKUP_ARROW_ALPHA);
    renderer_draw_model_euler(m, tex, e->x, e->y + PICKUP_ARROW_RISE, e->z,
                              -s_arrow_clock * PICKUP_ARROW_SPIN, 0.0f, 0.0f,
                              PICKUP_ARROW_SCALE);
    renderer_set_model_alpha(1.0f);
    audit_line("entity", e->type, e->tag, "pickup-arrow",
               PICKUP_ARROW_MODEL, m, tex);
}


/* Draw every pickable scene object: entities, then static OMT placements.
   This is the single enumeration path shared by the main render pass and the
   QA pick pass (docs/qa_annotate_plan.md) -- object N here must be object N
   there, so the pick pass must never grow its own loop. Sky, clouds, ground
   and HUD are not pickable and stay outside. */
static void draw_scene(World *world, int jim_model_ok)
{
    unsigned int box_vao = world_box_vao();
    int          box_idx = world_box_index_count();

    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive) continue;
        if (!e->visible) {
            audit_line("entity", e->type, e->tag, "gated", NULL, NULL, 0);
            continue;
        }

        /* Authored InitiallyVisible=0: the original hides these at boot
           until scripting shows them (phone booth, hydrant, teleport FX,
           rockets...). No native show-scripting exists yet, so they stay
           hidden — strictly more faithful than drawing them.
           Exception: C3DKitty's own per-frame update force-enables
           visibility while its task state is below 10 (cat not yet
           rescued) — decomp C3DKitty.md — so the original shows the cat
           from the first frame despite IV=0 (2026-06-12 QA #4: the level2
           marquee cat was invisible). */
        if (e->has_initially_visible && e->initially_visible == 0 &&
            strcmp(e->type, "3KIT") != 0) {
            audit_line("entity", e->type, e->tag, "gated", NULL, NULL, 0);
            continue;
        }

        /* Same shape for pickups: InitallyActive=0 (sic — the original's
           registrar typo) marks quest-spawned pickups (level1 egg2b nest
           egg, ...) that scripting activates later; the original never
           draws them at boot (2026-06-11 QA). Read the runtime flag, not the
           authored property: behavior_pickup_spawn_gate raises it from that
           property at spawn (so boot is unchanged) and SetPickupItemState
           lowers it, which is how a vending machine's product becomes visible
           in the tray once the machine has been paid. */
        if (e->pickup_inactive) {
            audit_line("entity", e->type, e->tag, "gated", NULL, NULL, 0);
            continue;
        }

        /* QA annotation: every drawable entity registers here so the pick
           pass and the main pass assign identical IDs. */
        qa_register_entity(e);

        /* fixture0 is deliberately asset-free: render every synthetic object
           through the engine's primitive cube instead of asking the authored
           visual resolver for meshes or sprites. Patrol actors pulse from the
           animation clock advanced by behavior_ai's shared animated base. */
        if (strncmp(e->tag, "fixture_", 8) == 0) {
            float r, g, b;
            entity_color(e->type, &r, &g, &b);
            float scale = e->half_extents[1] > 0.0f ? e->half_extents[1] : 35.0f;
            if (strcmp(e->type, "3CAR") == 0) {
                r = 0.95f; g = 0.25f; b = 0.20f;
                scale *= 1.0f + 0.08f * sinf(e->anim_time * 6.2831853f);
            } else if (strcmp(e->type, "PROJ") == 0) {
                r = 1.0f; g = 0.75f; b = 0.1f;
                scale = 12.0f;
            }
            renderer_draw_box(box_vao, box_idx, e->x, e->y, e->z,
                              scale, r, g, b);
            continue;
        }

        if (draw_authored_button(e))
            continue;

        /* Player: dedicated anim path. */
        if (jim_model_ok && strcmp(e->type, "3JIM") == 0) {
            PlayerAnimSample sample =
                player_anim_sample_entity(e, (PlayerAnim)e->user_flag);
            if (sample.model) {
                renderer_draw_model_anim(sample.model, 0, e->x, e->y, e->z,
                                         e->ry + 3.14159265f, 1.0f,
                                         sample.frame_a, sample.frame_b, sample.lerp);
            }
            audit_line("entity", e->type, e->tag, "player", NULL,
                       sample.model, 0);
            continue;
        }

        /* Cutscene target animation dispatch for non-player C3DANIMATED actors.
           The original calls the target's play-animation vfunc by alias; the
           native cutscene behavior records the resolved shipped ASE clip here. */
        if (e->cutscene_model[0]) {
            AseModel *m = model_cache_get(e->cutscene_model);
            unsigned int tex = e->cutscene_texture[0]
                ? tex_cache_get(e->cutscene_texture) : 0;
            if (m) {
                renderer_set_hide_untextured_groups(0);
                if (m->frame_count > 1) {
                    AnimatedDispatchSample ds;
                    int a, b;
                    float lerp;
                    if (animated_dispatch_sample(e, &ds)) {
                        a = ds.frame_a;
                        b = ds.frame_b;
                        lerp = ds.lerp;
                    } else {
                        float fps = m->framespeed > 0.0f ? m->framespeed : 10.0f;
                        float frame_pos = e->anim_time * fps;
                        int last = m->frame_count - 1;
                        int base = (int)floorf(frame_pos);
                        lerp = frame_pos - floorf(frame_pos);
                        if (e->cutscene_anim_loop) {
                            a = base % m->frame_count;
                            b = (a + 1) % m->frame_count;
                        } else if (base >= last) {
                            a = last;
                            b = last;
                            lerp = 0.0f;
                        } else {
                            a = base;
                            b = base + 1;
                        }
                    }
                    renderer_draw_model_anim(m, tex, e->x, e->y, e->z,
                                             e->ry, 1.0f, a, b, lerp);
                } else {
                    renderer_draw_model(m, tex, e->x, e->y, e->z, e->ry, ent_draw_scale(e));
                }
                renderer_set_hide_untextured_groups(1);
                audit_line("entity", e->type, e->tag, "cutscene-anim",
                           e->cutscene_model, m, tex);
                continue;
            }
            audit_line("entity", e->type, e->tag, "cutscene-anim-missing",
                       e->cutscene_model, NULL, tex);
        }

        /* 3ASE (C3DASEObj): generic per-object mesh named by the GAM's
           ASEStop/ASEWalk, textured by PNGFile. Meshes live under
           assets/ase/jnvsjn/ (lowercased on import). */
        if (strcmp(e->type, "3ASE") == 0 && e->ase_file[0]) {
            char lower[64]; size_t li = 0;
            for (const char *p = e->ase_file; *p && li < sizeof(lower) - 1; p++)
                lower[li++] = (char)tolower((unsigned char)*p);
            lower[li] = '\0';
            char path[160];
            snprintf(path, sizeof(path), "assets/ase/jnvsjn/%s", lower);
            AseModel *m = model_cache_get(path);
            if (m) {
                unsigned int tex = e->png_file[0] ? tex_cache_resolve_bmp(e->png_file) : 0;
                renderer_set_hide_untextured_groups(0);
                renderer_draw_model(m, tex, e->x, e->y, e->z, e->ry, ent_draw_scale(e));
                renderer_set_hide_untextured_groups(1);
                audit_line("entity", e->type, e->tag, "mesh", path, m, tex);
                continue;
            }
            audit_line("entity", e->type, e->tag, "mesh-missing", path, NULL, 0);
            /* mesh missing -> fall through to placeholder box */
        }

        /* Door classes with per-instance authored ASEFile/PNGFile:
           C3DSwingDoor (3SWN), C3DSchoolDoor (3SCD), and C3DDoorUpDown
           (3DUD). Every level's door can be a different mesh/texture pair
           (level1 blocksdoor, level1b bars+chain, level1c firedoor, level3
           doorretro, level2a doorfowl...). The files ship with the original
           install under assets/ase + assets/png with mixed case, so resolve
           case-insensitively. Several door ASEs are degenerate OMT->ASE
           stubs (firedoor.ASE is 8 verts); prefer the textured
           assets/glb/ase/<stem>.glb twin when one exists. */
        if ((strcmp(e->type, "3SWN") == 0 || strcmp(e->type, "3SCD") == 0 ||
             strcmp(e->type, "3DUD") == 0)
            && e->ase_file[0]) {
            char path[160];
            AseModel *m = NULL;
            unsigned int tex = 0;
            int from_glb = 0;
            char glb_name[96];
            const char *dot = strrchr(e->ase_file, '.');
            size_t stem_n = dot ? (size_t)(dot - e->ase_file)
                                : strlen(e->ase_file);
            if (stem_n < sizeof(glb_name) - 5) {
                memcpy(glb_name, e->ase_file, stem_n);
                memcpy(glb_name + stem_n, ".glb", 5);
                if (asset_path_ci(path, sizeof(path), "assets/glb/ase", glb_name)) {
                    m = model_cache_get(path);   /* real geometry (ASE may be a stub) */
                    from_glb = (m != NULL);
                }
            }
            if (!m && asset_path_ci(path, sizeof(path), "assets/ase", e->ase_file))
                m = model_cache_get(path);
            /* The authored PNGFile is the per-instance texture truth and
               applies to EITHER mesh source — level3 reuses one doorretro
               mesh with exit.png vs retrodoor.png, and the glb twins are
               geometry-only (firedoor.glb embeds no images; drawing it
               bare left the school fire door invisible — 2026-06-12 QA #3
               follow-up from sandmanfan). glb twins bake DX-convention
               UVs (the omt-gltf 1-v flip), so the standalone PNG must load
               with the opposite vertical orientation there or it renders
               upside down (2026-06-12 QA #4: the Retroland EXIT doors). */
            char tpath[160];
            if (e->png_file[0] &&
                asset_path_ci(tpath, sizeof(tpath), "assets/png", e->png_file))
                tex = from_glb ? tex_cache_get_vflip(tpath)
                               : tex_cache_get(tpath);
            if (m) {
                renderer_draw_model(m, tex, e->x, e->y, e->z, e->ry, ent_draw_scale(e));
                audit_line("entity", e->type, e->tag, "mesh", path, m, tex);
                continue;
            }
            audit_line("entity", e->type, e->tag, "mesh-missing",
                       e->ase_file, NULL, 0);
            /* mesh missing -> fall through to placeholder box */
        }

        /* Sprite-indexed objects: 3PIC pickups and sprite objects are 2D
           sprites from sprites.omt (chunk id == SpriteIndex), NOT meshes.
           Render the real sprite as a camera-facing billboard instead of a
           placeholder. (sprite_index 0 falls through so game-1 3PIC, which
           predates SpriteIndex, keep their ASE props.) Each game ships its
           own sprites.omt: JNBG resolves through the generated chunk map,
           JNvsJN through its spr_<id> extraction — previously JNBG levels
           wrongly loaded the sequel's sprites where indices collide. */
        if ((strcmp(e->type, "3PIC") == 0 || strcmp(e->type, "3SRO") == 0 ||
             strcmp(e->type, "3SPR") == 0 || strcmp(e->type, "3ANI") == 0)
            && e->sprite_index > 0
            /* Curated per-tag visuals win over the sprite shortcut. */
            && !entity_visual_tag_override(e, NULL)) {
            /* sprites.omt chunk 106 = the "hidden" canvas: an invisible
               pickup trigger whose visible referent is level geometry or a
               separate entity (2026-06-12 QA #3 — hydrant/nests/boat). */
            if (sprite_ref_hidden(e)) {
                /* The hardcoded level1b gdish arrow used to live here, drawing
                   the 2D canvas-33 billboard for one specific row. The general
                   ShowArrow path above covers it (and every other pickup), so
                   the special case is gone. */
                audit_line("entity", e->type, e->tag, "hidden", NULL, NULL, 0);
                continue;
            }
            char spath[96];
            unsigned int stex = 0;
            const char *sprite_src = NULL;
            if (entity_visual_is_jnbg()) {
                const char *cp = sprite_chunk_path(e->sprite_index);
                if (cp) { stex = tex_cache_get(cp); sprite_src = cp; }
            } else {
                snprintf(spath, sizeof(spath),
                         "assets/parsed/sprites/jnvsjn/spr_%d.png", e->sprite_index);
                stex = tex_cache_get(spath);
                sprite_src = spath;
            }
            if (!stex)
                audit_line("entity", e->type, e->tag, "sprite-unresolved",
                           sprite_src, NULL, 0);
            if (stex) {
                float sz = e->sprite_size > 1.0f ? e->sprite_size : 110.0f;
                /* JNBG: centre the quad at the authored Y — the original's
                   OMediaCanvasElement places the canvas AT the element
                   position. The old +sz/2 "ground lift" floated every
                   sprite pickup half a quad too high (2026-06-12 QA #3:
                   floating fishbowls, jetpack off-centre in its doorway).
                   JNvsJN is a different engine (Granny, not OMT) whose
                   anchoring hasn't been measured; keep its QA'd lift until
                   a ticket says otherwise. */
                float by = entity_visual_is_jnbg() ? e->y : e->y + sz * 0.5f;
                renderer_draw_billboard(stex, e->x, by, e->z,
                                        sz, sz, 0.0f, 0.0f, 0.0f, 0.0f);
                audit_line("entity", e->type, e->tag, "sprite",
                           sprite_src, NULL, stex);
                continue;
            }
        }

        /* Pre-bound model from entity vtable (legacy path). */
        if (e->model) {
            renderer_draw_model(e->model, 0, e->x, e->y, e->z, 0.0f, 1.0f);
            audit_line("entity", e->type, e->tag, "mesh-prebound", NULL,
                       e->model, 0);
            continue;
        }

        /* Resolver path: tag override → FourCC default → invisible/box. */
        EntityVisual v;
        if (entity_visual_resolve(e, &v)) {
            if (v.invisible) {
                audit_line("entity", e->type, e->tag, "invisible", NULL, NULL, 0);
                continue;
            }
            if (v.sprite_path) {
                unsigned int tex = tex_cache_get(v.sprite_path);
                if (tex) {
                    float sz = v.sprite_size > 0.0f ? v.sprite_size : 64.0f;
                    float tr = v.tint_r, tg = v.tint_g, tb = v.tint_b, ta = v.tint_a;
                    float sw = sz, sh = sz;
                    if (strncmp(e->type, "3BUB", 4) == 0) {
                        /* C3DBubble::UpdateBubbleState: the width holds at
                           SpriteSize while the height carries
                           sin(t*10)*30, and the grow/fade transition scales
                           both and drives alpha. */
                        float bw, bh, ba;
                        behavior_bubble_size(e, &bw, &bh, &ba);
                        sw = bw; sh = bh;
                        tr = tg = tb = 1.0f;
                        ta = ba;
                    } else if (strncmp(e->type, "3ARR", 4) == 0) {
                        float wave = sinf(s_arrow_clock * NAV_ARROW_PULSE_RATE);
                        sh *= 1.0f + NAV_ARROW_PULSE_AMP * wave;   /* height only */
                        float glow = 0.5f + 0.5f * wave;           /* 0..1 */
                        tr = 1.0f + (NAV_ARROW_GLOW_R - 1.0f) * glow;
                        tg = 1.0f + (NAV_ARROW_GLOW_G - 1.0f) * glow;
                        tb = 1.0f + (NAV_ARROW_GLOW_B - 1.0f) * glow;
                        ta = 1.0f;   /* tint_a == 0 would mean "no tint" */
                    }
                    renderer_draw_billboard(tex, e->x, e->y, e->z, sw, sh,
                                            tr, tg, tb, ta);
                    audit_line("entity", e->type, e->tag, "sprite",
                               v.sprite_path, NULL, tex);
                    continue;
                }
                audit_line("entity", e->type, e->tag, "sprite-unresolved",
                           v.sprite_path, NULL, 0);
            }
            if (v.model_path) {
                AseModel *m = model_cache_get(v.model_path);
                if (!m)
                    audit_line("entity", e->type, e->tag, "mesh-missing",
                               v.model_path, NULL, 0);
                if (m) {
                    unsigned int tex = v.texture_path ? tex_cache_get(v.texture_path) : 0;
                    float sc = (v.scale > 0.0f ? v.scale : 1.0f) *
                               ent_draw_scale(e);
                    /* Absolute-positioned placement meshes used as entity
                       meshes bake their level world position into vertices;
                       offset by the mesh bbox center so the mesh sits at the
                       entity (x,y,z) instead of its baked level position. */
                    float dx = e->x, dy = e->y, dz = e->z;
                    if (v.recenter) {
                        dx -= (m->min[0] + m->max[0]) * 0.5f * sc;
                        dy -= (m->min[1] + m->max[1]) * 0.5f * sc;
                        dz -= (m->min[2] + m->max[2]) * 0.5f * sc;
                    }
                    if (entity_visual_water_anchors(e)) {
                        float water_y = 0.0f;
                        if (water_surface_at(world, e->x, e->z, &water_y))
                            dy = water_y - m->min[1] * sc;
                    }
                    if (entity_visual_foot_anchors(e) && m->min[1] < 0.0f)
                        dy -= m->min[1] * sc;
                    /* Entity props are deliberately-chosen meshes; some
                       (gems, converted GRN props) are untextured flat-color
                       meshes. The global hide-untextured flag is for OMT
                       collision slabs, not these — render them regardless. */
                    renderer_set_hide_untextured_groups(0);
                    /* Props whose behavior drives a roll (Ferris wheel 3FER,
                       pendulum 3PEN spin in a vertical plane; the fan 3FAN
                       spins its blade disc about GL Z like a pinwheel) author
                       rz in radians at runtime; draw them with full euler so
                       the motion shows. Only these types opt in — authored
                       .gam RotationX/Z (degrees, ~1% of objects) stays on the
                       cheap yaw-only path to avoid wrong-unit tilts. */
                    int spins = strncmp(e->type, "3FER", 4) == 0 ||
                                strncmp(e->type, "3PEN", 4) == 0 ||
                                strncmp(e->type, "3FAN", 4) == 0;
                    /* The rideable rocket pitches its nose (rx) as it's aimed
                       (behavior_vehicle.c), so draw it with full euler too. */
                    int rocket = strncmp(e->type, "3ROC", 4) == 0;
                    if (rocket)
                        renderer_draw_model_euler(m, tex, dx, dy, dz,
                                                  e->ry, e->rx, e->rz, sc);
                    else if (spins)
                        renderer_draw_model_euler(m, tex, dx, dy, dz,
                                                  e->ry, 0.0f, e->rz, sc);
                    else
                        renderer_draw_model(m, tex, dx, dy, dz, e->ry, sc);
                    renderer_set_hide_untextured_groups(1);
                    audit_line("entity", e->type, e->tag, "mesh",
                               v.model_path, m, tex);
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
        audit_line("entity", e->type, e->tag, "box", NULL, NULL, 0);
    }

    /* Static world geometry from level1.omt (Phase 8). OMT-exported ASEs
       localize X/Z around the chunk center; ase_load maps Max Y to GL -Z.
       Native Level 1 therefore draws at (center_x, 0, -center_z) so the
       map shares the same basis as solved keyframe cameras. Keep the old
       +Z placement for legacy/hybrid validation paths. */
    for (int pi = 0; pi < world->placement_count; pi++) {
        const WorldPlacement *pl = &world->placements[pi];
        /* BLOCK*-prefixed meshes are the original's COLLISION volumes
           (BLOCKbench, BLOCKING_road, BLOCK_Rocket03, the level-perimeter
           BLOCK_HOODFAR fence shell...) — never visible; each has a separate
           visible twin (bench05, the road, Rocketa; the perimeter's visible
           fence is the chroma-keyed 2D_Trees foliage). Reporter-confirmed
           2026-06-12 QA #3 follow-up: "all Block meshes should be invisible".
           EXCEPTION (QA #4): the prefix over-matches level1's playground
           climbing toy, whose visible outer/inner shells are literally named
           Blocks_Out / Blocks_In ("the model BoxsOut is now gone"). Those two
           are the only BLOCK-named meshes that are real visible geometry
           rather than a collider (verified across all 35 levels by the
           faithfulness sweep) — let them draw; skip everything else.
           Skipped before QA registration so an invisible collider can't
           swallow a pick aimed at its visible twin. The invisible-collider set
           is now the shared collision_is_invisible() predicate (engine/
           collision.h), keeping the renderer and the CollisionWorld agreed on
           which placements are collision-only. */
        if (collision_is_invisible(pl->name)) {  /* level1c authors "Blocking01" */
            audit_line("placement", pl->name, NULL, "collision-skip", NULL, NULL, 0);
            continue;
        }
        /* QA annotation: one ID per placement; a tree's trunk mesh and crown
           billboard share it, so clicking either picks the tree. */
        qa_register_placement(pl, pl->x, 0.0f, -pl->z);
        /* The trunk+crown billboard is a LEVEL1 construction: level1.omt bark
           canvas, the level1 capture-measured crown (tex_052e7090) and size
           table. Other levels ship proper per-level tree glbs
           (assets/glb/omt/<level>/tree*.glb) with their own canvas textures, so
           forcing level1's crown onto them was wrong (QA 2026-06-24 l3c tree04
           "should be level3c/0000_128x128d32"). Restrict the billboard to the
           level1 family; everyone else falls through to the textured glb mesh. */
        if (!env_enabled("JN_DISABLE_TREE_BILLBOARDS") &&
            strncmp(pl->name, "tree", 4) == 0 &&
            strncmp(game_flow_current_level(), "level1", 6) == 0) {
            /* Full scattered tree = bark TRUNK mesh (base at ground) + a
               camera-facing green-crown CANOPY billboard sitting on the
               trunk top. The trunk mesh's own canvas mis-resolves to a fence
               texture, so we override it with the real brown bark canvas
               (level1.omt "tree"). The canopy (tex_052e7090) is alpha-keyed
               so the sky shows through; sized from the measured override
               table. */
            float bb_size = 0.0f;
            const char *unused_tex = NULL;
            if (!billboard_overrides_lookup(pl->name, &bb_size, &unused_tex)
                || bb_size <= 0.0f) {
                /* Not in the capture-measured table (only ~12 trees were in
                   frame 8881). Fall back to the measured median for the
                   category so untabled trees match the others rather than
                   defaulting small. Exact per-tree sizes would come from a
                   full-capture scan of build/level1_session.omtc. */
                bb_size = (strncmp(pl->name, "treebranch", 10) == 0)
                          ? 500.0f : 600.0f;
            }

            AseModel *trunk = model_cache_get(pl->ase_path);
            float trunk_top = trunk ? trunk->max[1] : 300.0f;
            if (trunk) {
                unsigned int bark = tex_cache_get("assets/native/tree_bark.png");
                renderer_draw_model(trunk, bark, pl->x, 0.0f, -pl->z, 0.0f, 1.0f);
            }
            unsigned int crown = tex_cache_get(
                "assets/native/billboard_textures/tex_052e7090_128x128.png");
            if (crown) {
                /* The crown's opaque pixels sit in the lower ~57% of the
                   texture, so raise the quad centre above the trunk top to
                   seat the crown ON the trunk (overlapping it slightly)
                   instead of hanging below it. */
                float canopy_y = trunk_top + bb_size * 0.40f;
                renderer_draw_billboard(crown, pl->x, canopy_y, -pl->z,
                                        bb_size, bb_size,
                                        1.0f, 1.0f, 1.0f, 1.0f);
            }
            audit_line("placement", pl->name, NULL, "tree-billboard",
                       pl->ase_path, NULL, crown);
            continue;
        }
        AseModel *pm = model_cache_get(pl->ase_path);
        if (!pm) {
            audit_line("placement", pl->name, NULL, "mesh-missing",
                       pl->ase_path, NULL, 0);
            continue;
        }
        /* Phase C: capture-derived texture overrides retired. The glTF
           meshes carry breakthrough-correct OMT-canvas textures, so the
           30-entry override layer is redundant (verified: keyframe 8881
           changes <1% without it). See docs/gltf_export_plan.md. */
        /* Faithfulness (audit D1/D2): the original game renders ZERO
           untextured geometry. A placement whose OMT material resolved no
           texture is either collision/blocking (BLOCKING_*, GROUND base
           slab) or a mesh whose canvas couldn't be resolved -- neither
           appears as a visible surface in the game. Skip them rather than
           draw dark filler slabs. The old audit mode rendered them as
           debug_flat, but the resulting huge gray BLOCKING_06
           (radius 12697) and SCHOOL slab were dominating the view; the
           coverage manifest tracks them, the renderer no longer needs to. */
        if (getenv("JN_DEBUG_DRAW") && pi < 6)
            fprintf(stderr, "[draw] %s mats=%d m0.tex=%u m.tex=%u hastex=%d "
                    "bbox=(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f) at(%.0f,%.0f)\n",
                    pl->name, pm->material_count,
                    pm->material_count>0?pm->materials[0].texture_id:0,
                    pm->texture_id, model_has_texture(pm),
                    pm->min[0],pm->min[1],pm->min[2],
                    pm->max[0],pm->max[1],pm->max[2], pl->x, pl->z);
        if (!model_has_texture(pm)) {
            audit_line("placement", pl->name, NULL, "untextured-skip",
                       pl->ase_path, NULL, 0);
            continue;
        }
        float draw_z = -pl->z;
        /* 2D_Trees boundary walls bake the sky behind the tree-line as an
           opaque chroma blue (RGB 57,148,198); key it out so the real sky
           shows through (Retroville feel pass, issue 5). */
        int foliage_wall = (strncmp(pl->name, "2D_Trees", 8) == 0);
        if (foliage_wall)
            renderer_set_color_key(1, 57.0f/255.0f, 148.0f/255.0f, 198.0f/255.0f, 0.08f);
        renderer_draw_model(pm, 0, pl->x, 0.0f, draw_z, 0.0f, 1.0f);
        if (foliage_wall)
            renderer_set_color_key(0, 0, 0, 0, 0.08f);
        audit_line("placement", pl->name, NULL, "mesh", pl->ase_path, pm, 0);
    }

    /* Translucent markers last, after every opaque draw. They blend with
       depth writes masked, so anything drawn afterwards would paint straight
       over them -- which is exactly what the vending machines did while this
       lived in the entity loop above. */
    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive) continue;
        draw_pickup_arrow(e);
    }
}

/* Keep a findable rideable rocket in sync with the sandbox flag each tick.
   Sandbox only *reveals* an authored 3ROC (entity_visual draws it), and where a
   level does have one it's often far away on a distant pad (level2) — so rather
   than rely on hunting it down, drop a tagged stand-in right next to the player
   whenever sandbox is on and ours isn't already present. When sandbox is
   switched off, retire that stand-in (unless the player is riding it). Authored
   rockets are never moved or removed. */
/* Sandbox/verification convenience for the rideable rocket. Every level ships
   exactly one authored C3DRocketShip (3ROC), but it's often far away and parked
   on an elevated pad (level2: ~590u off, y=158) — hard to find, and its SOLID
   box nudges an approaching player downward. So when sandbox is ON we bring that
   one rocket to the player ONCE, seat it on the ground, and clear its SOLID flag
   (boarding is proximity-based, so it's still rideable). This is strictly a test
   aid: we never duplicate the rocket, never touch it outside sandbox, and never
   move one the player is currently riding. Flight stays faithful (the authored
   .gam params / C3DFlyingObject defaults are unchanged). A stand-in is spawned
   only in the hypothetical case of a level with no authored rocket. */
static Entity *s_sandbox_prepped_rocket = NULL;

static void sandbox_reconcile(World *w, Entity *jim) {
    Entity *rk = NULL;
    for (Entity *e = w->head; e; e = e->next)
        if (e->alive && strncmp(e->type, "3ROC", 4) == 0) { rk = e; break; }

    if (!entity_visual_sandbox_enabled()) {
        /* Sandbox off: drop any stand-in we spawned; forget the prepped rocket. */
        if (rk && strcmp(rk->tag, "sbxrocket") == 0 && behavior_vehicle_current() != rk)
            rk->alive = 0;
        s_sandbox_prepped_rocket = NULL;
        return;
    }
    if (!jim) return;
    if (!rk) {  /* no authored rocket in this level — spawn a stand-in */
        float fx = sinf(jim->ry), fz = cosf(jim->ry);
        rk = entity_spawn(w, "3ROC", "sbxrocket",
                          jim->x + fx * 300.0f, jim->y, jim->z + fz * 300.0f);
        if (!rk) return;
    }
    /* Bring the rocket to the player exactly once per level (until ridden). */
    if (rk != s_sandbox_prepped_rocket && behavior_vehicle_current() != rk) {
        float fx = sinf(jim->ry), fz = cosf(jim->ry);
        rk->x = jim->x + fx * 300.0f;
        rk->z = jim->z + fz * 300.0f;
        rk->y = (jim->y - jim->half_extents[1]) + rk->half_extents[1]; /* base on ground */
        rk->ry = jim->ry;
        rk->runtime_flags &= ~ENTITY_FLAG_SOLID;   /* no downward eject on approach */
        s_sandbox_prepped_rocket = rk;
        printf("[SANDBOX] rideable rocket brought to player at (%.0f, %.0f, %.0f)\n",
               rk->x, rk->y, rk->z);
    }
}

static void goddard_reconcile_after_level_load(World *w, const char *level_name) {
    /* JimmySetupOrReset spawns Goddard at 0x95c and the scooter at 0x970,
       one after the other, and hides the scooter immediately. Same here. */
    behavior_scooter_ensure(w);
    gadget_menu_set_world(w);
    Entity *g = behavior_goddard_ensure(w, level_name);
    if (!g || !env_enabled("JN_TEST_GODDARD")) return;
    if (g->visible) {
        Entity *can = entity_spawn(w, "3MEP", "test_metal_can",
                                   g->x + 55.0f, g->y, g->z);
        if (can) {
            fprintf(stderr,
                    "[goddard_test] spawned metal can at (%.0f, %.0f, %.0f)\n",
                    can->x, can->y, can->z);
        }
    }
}

static void test_place_player_near_type(World *world, Entity *jim) {
    const char *type = getenv("JN_TEST_NEAR_TYPE");
    if (!type || strlen(type) < 4 || !world || !jim) return;

    Entity *best = NULL;
    float bestd2 = 1.0e30f;
    for (Entity *e = world->head; e; e = e->next) {
        if (!e->alive || e == jim) continue;
        if (strncmp(e->type, type, 4) != 0) continue;
        float dx = e->x - jim->x;
        float dz = e->z - jim->z;
        float d2 = dx * dx + dz * dz;
        if (d2 < bestd2) {
            bestd2 = d2;
            best = e;
        }
    }
    if (!best) {
        fprintf(stderr, "[test_near] no live %.4s target found\n", type);
        return;
    }

    float radius = env_float_default("JN_TEST_NEAR_RADIUS", 220.0f);
    float dx = jim->x - best->x;
    float dz = jim->z - best->z;
    float len = sqrtf(dx * dx + dz * dz);
    float ux = len > 1.0e-4f ? dx / len : sinf(best->ry);
    float uz = len > 1.0e-4f ? dz / len : cosf(best->ry);
    jim->x = best->x + ux * radius;
    jim->y = best->y;
    jim->z = best->z + uz * radius;
    jim->vx = jim->vy = jim->vz = 0.0f;
    fprintf(stderr, "[test_near] player near %s '%s' at (%.0f, %.0f, %.0f), radius=%.0f\n",
            best->type, best->tag, jim->x, jim->y, jim->z, radius);
}

int main(int argc, char **argv) {
    const char *start_level = "level1";
    int want_newgame = 0;
    int want_menu = 0;
    int want_nodamage = 0;
    int want_sandbox = 0;
    int headless = 0;
    long frame_limit = -1;
    unsigned long seed = 0;
    int seed_set = 0;
    const char *dump_png_dir = NULL;
    const char *dump_state_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (i < argc - 1 && strcmp(argv[i], "--level") == 0) {
            start_level = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--newgame") == 0) {
            want_newgame = 1;
        } else if (strcmp(argv[i], "--menu") == 0) {
            want_menu = 1;
        } else if (strcmp(argv[i], "--nodamage") == 0 ||
                   strcmp(argv[i], "--invincible") == 0) {
            want_nodamage = 1;
        } else if (strcmp(argv[i], "--sandbox") == 0) {
            want_sandbox = 1;
        } else if (strcmp(argv[i], "--headless") == 0) {
            headless = 1;
        } else if (i < argc - 1 && strcmp(argv[i], "--frames") == 0) {
            char *end = NULL;
            errno = 0;
            frame_limit = strtol(argv[++i], &end, 10);
            if (errno || !end || *end || frame_limit < 0) {
                fprintf(stderr, "Invalid --frames value '%s'\n", argv[i]);
                return 2;
            }
        } else if (i < argc - 1 && strcmp(argv[i], "--seed") == 0) {
            char *end = NULL;
            errno = 0;
            seed = strtoul(argv[++i], &end, 0);
            if (errno || !end || *end || seed > UINT_MAX) {
                fprintf(stderr, "Invalid --seed value '%s'\n", argv[i]);
                return 2;
            }
            seed_set = 1;
        } else if (i < argc - 1 && strcmp(argv[i], "--dump-png") == 0) {
            dump_png_dir = argv[++i];
        } else if (i < argc - 1 && strcmp(argv[i], "--dump-state") == 0) {
            dump_state_path = argv[++i];
        } else {
            fprintf(stderr, "Unknown or incomplete option '%s'\n", argv[i]);
            return 2;
        }
    }

    /* A frame grabber must never catch the controls card. Decided once,
       here, rather than at each help_show_timed() call site. */
    help_set_auto(!headless &&
                  getenv("JN_HEADLESS")   == NULL &&
                  getenv("JN_SCREENSHOT") == NULL &&
                  getenv("JN_CAPTURE")    == NULL);

    if (headless) {
        SDL_setenv("SDL_VIDEODRIVER", "offscreen", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
        SDL_setenv("JN_HEADLESS", "1", 1);
    }
    if (seed_set) srand((unsigned int)seed);
    if (dump_png_dir && !make_directory_tree(dump_png_dir)) {
        fprintf(stderr, "Cannot create --dump-png directory '%s': %s\n",
                dump_png_dir, strerror(errno));
        return 2;
    }

    /* --newgame: enter through the CTaskList task system (CJimmyGame /
       CMainMenu "New Game" route) instead of a direct level load. The task's
       gam_file becomes the start level; this turns on campaign mode (lives,
       mission, restart flow). Direct `--level X` keeps campaign mode OFF so the
       audit + screenshot harnesses render exactly as before Wave N5. */
    char newgame_gam[64] = {0};
    if (want_newgame) gamestate_new_game();   /* clears the picture stores */
    if (want_newgame &&
        game_flow_begin_task("NewGame", newgame_gam, sizeof(newgame_gam), NULL) &&
        newgame_gam[0])
        start_level = newgame_gam;

    LevelDesc current_desc;
    if (!level_desc_for(start_level, &current_desc)) {
        fprintf(stderr, "Failed to resolve level '%s'\n", start_level);
        return 1;
    }
    int fixture_mode = strcmp(current_desc.name, "fixture0") == 0;

    Window w;
    if (!window_init(&w, "JN Engine - Step 4: Textured Scene", 1280, 720))
        return 1;

    SDL_SetWindowSize(w.sdl_win, 1280, 720);
    w.width = 1280;
    w.height = 720;
    if (fixture_mode)
        fprintf(stderr, "[native_level] loading built-in fixture0\n");
    else
        fprintf(stderr, "[native_level] loading %s from %s\n",
                current_desc.name, current_desc.gam_path);

    const char *cam_path = getenv("JN_NATIVE_LEVEL1_CAMERA");
    char keyframe_path[192];
    const char *keyframe = getenv("JN_NATIVE_LEVEL1_KEYFRAME");
    if ((!cam_path || !cam_path[0]) && keyframe && keyframe[0]) {
        char keyframe_leaf[96];
        snprintf(keyframe_leaf, sizeof(keyframe_leaf),
                 "keyframe_cameras/%s.txt", keyframe);
        asset_path_join(keyframe_path, sizeof(keyframe_path),
                        asset_root(JN_ASSET_NATIVE), keyframe_leaf);
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

    if (!renderer_init(w.width, w.height)) {
        window_destroy(&w);
        return 1;
    }

    AseModel *sky_model = NULL;   /* faithful sky dome when the level declares one */
    unsigned int clouds_tex = 0;  /* real sky.png cloud texture for the dome */
    float sky_spin = 0.0f;        /* accumulated cloud-drift rotation (rad) */

    configure_level_sky(&current_desc, &sky_model, &clouds_tex);
    /* Hide multi-material slots whose canvas chain didn't resolve so SCHOOL's
       missing slots, foliage tree trunks with debug_flat branches, etc. don't
       render as dark slabs. */
    renderer_set_hide_untextured_groups(1);

    /* Phase 4: enable shader-side alpha cutout for capture-derived billboards
       (foliage, playground sand). The original game runs with ALPHATESTENABLE=1
       on >89% of draws -- we approximate by discarding fragments whose texture
       alpha is below the threshold, giving clean leaf cutouts without
       replicating the original's color-key machinery. */
    renderer_set_alpha_cutout(PHASE4_ALPHA_CUTOUT_ENABLED,
                              PHASE4_ALPHA_CUTOUT_THRESHOLD);

    if (!fixture_mode && !audio_init()) {
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
        input_destroy(); audio_destroy(); renderer_destroy();
        window_destroy(&w);
        return 0;
    }

    World world;
    world_init(&world);
    world_box_init();
    gamestate_init();
    /* CJimmyGame::InitGame — seed the mission layer (lives=5, value=100). For a
       --newgame entry this already ran inside game_flow_begin_task. */
    if (!game_flow_campaign_active())
        game_flow_init_game();

    /* Load all player poses with the shared real-Jimmy texture. All jim*.ASE
       reference jimmylast.bmp (which doesn't exist); jimycarl.png is the
       atlas that matches the model UVs (red shirt, atom emblem, backpack). */
    asset_cache_begin_level();
    unsigned int jim_tex = fixture_mode ? 0 : tex_cache_get("assets/png/jimycarl.png");
    int jim_poses_loaded = fixture_mode ? 0 : player_anim_init(jim_tex);
    int jim_model_ok = (jim_poses_loaded > 0);

    /* 2D HUD overlay. Disabled when a matched-camera override is active (the
       game-1 native-vs-capture faithfulness validators render through this same
       loop and must stay pixel-identical) or via JN_DISABLE_HUD. */
    int hud_enabled = !fixture_mode && !renderer_camera_override_active() &&
                      !env_enabled("JN_DISABLE_HUD");
    if (hud_enabled) hud_init();

    /* Game selector for the per-instance sprite tier: each game ships its own
       sprites.omt, so SpriteIndex must resolve against the right extraction.
       The default GAM root is JNBG; a root naming jnvsjn selects the sequel. */
    entity_visual_set_jnbg(
        strstr(asset_root(JN_ASSET_GAM), "jnvsjn") == NULL);

    /* Sandbox / verification mode (--sandbox, or ?sandbox=1 on the web): reveal
       the rideable C3DRocketShip and grant the combat tools so the player can
       actually exercise the Wave N2–N4 behaviors in-browser, where the
       JN_TEST_* env hooks aren't reachable and there's no inventory UI yet.
       Must precede load_level so the visual resolver sees the flag. */
    if (want_sandbox) {
        entity_visual_set_sandbox(1);
        gamestate_grant_tool("baseball", NULL);   /* enables the F throw */
        gamestate_grant_tool("bubble", NULL);
        gamestate_grant_tool("helmet", NULL);
        printf("[SANDBOX] rocketship revealed; baseball/bubble/helmet granted\n");
    }

    /* Key the collected-pickup table to this level before anything spawns.
       game_flow_enter_level() cannot serve here: it runs after
       entity_bind_vtables(), so at on_spawn time game_flow_current_level() is
       still the previous level (empty on the launch load). */
    gamestate_set_level(current_desc.name);
    /* The launch level was entered without a start point; a RETURN portal in
       it therefore has no departure point to read (see behavior_load.c). */
    gamestate_set_level_entry(current_desc.name, "");

    /* Cold entry: the launch level was jumped into, not walked into, so
       pre-grant the pictures it gates on but never awards (level1a/level1b/
       level1c/level4a). Campaign play is excluded -- there the player is meant
       to backtrack for them, which is why level1b gates on picture 14 and only
       level2/level2b award it. JN_NO_PREGRANT=1 turns it off so the refusal
       path stays testable. */
    if (!want_newgame && !env_enabled("JN_NO_PREGRANT"))
        gamestate_pregrant_pictures(current_desc.name);

    int n = load_level(&current_desc, &world);
    if (n < 0) {
        fprintf(stderr, "Failed to load %s\n", current_desc.gam_path);
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }
    {
        int missing = 0;
        for (int i = 0; i < world.placement_count; i++)
            if (!model_cache_get(world.placements[i].ase_path)) missing++;
        printf("placements_loaded=%d, missing_mesh=%d\n", world.placement_count, missing);
    }

    /* Browser demo / JN_DEMO_SPAWN=1: respawn Jimmy near the Retroville
       centre so the FollowCam frames the Phase 5 trees + Phase 5b textures
       instead of the unlit terrain at Jimmy's Level1.gam GAM-spawn (which
       looked black). Numbers picked to put him just south of the Phase-5b
       RampsNEW02 (OMT 10287, 62.6, 3257 → GL 10287, 62.6, -3257) so it's
       in frame plus several tree placements. */
    {
        int do_demo_spawn = 0;
#ifdef __EMSCRIPTEN__
        /* The hardcoded Retroville framing coords below are tuned for level1
           only. For every other level the browser build uses the level's own
           3JIM start so the camera lands in that level's actual content
           (gems, enemies, vehicles) instead of off-map empty terrain. */
        if (strcmp(current_desc.name, "level1") == 0) do_demo_spawn = 1;
#endif
        if (env_enabled("JN_DEMO_SPAWN")) do_demo_spawn = 1;
        /* Explicit spawn override "x,y,z" — QA helper to park the camera on a
           specific entity (gem, vehicle, NPC) in any level. Takes precedence. */
        const char *spawn_xyz = getenv("JN_DEMO_SPAWN_XYZ");
        if (spawn_xyz && spawn_xyz[0]) {
            float sx = 0, sy = 0, sz = 0, sry = 0;
            int n = sscanf(spawn_xyz, "%f,%f,%f,%f", &sx, &sy, &sz, &sry);
            if (n >= 3) {
                Entity *js = world_find_type(&world, "3JIM");
                if (js) {
                    js->x = sx; js->y = sy; js->z = sz;
                    /* optional 4th component: facing in radians, so QA
                       screenshots can re-aim the follow cam at a reported
                       entity (ry convention: forward = (sin ry, cos ry)). */
                    if (n == 4) js->ry = sry;
                    fprintf(stderr, "[demo_spawn] override 3JIM at (%.0f,%.0f,%.0f) ry=%.2f\n",
                            sx, sy, sz, js->ry);
                }
                do_demo_spawn = 0;
            }
        }
        if (do_demo_spawn) {
            Entity *jim_spawn = world_find_type(&world, "3JIM");
            if (jim_spawn) {
                /* Tuned via xvfb screenshot sweep: this position framed the
                   most green/textured pixels and the fewest near-black
                   pixels across a 6-spawn grid — i.e. the user sees the
                   Phase 5 trees + Phase 5b textures, not unlit ground. */
                jim_spawn->x = 10000.0f;
                jim_spawn->y = 200.0f;
                jim_spawn->z = -3000.0f;
                fprintf(stderr,
                        "[demo_spawn] respawned 3JIM at (%.0f, %.0f, %.0f)\n",
                        jim_spawn->x, jim_spawn->y, jim_spawn->z);
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

    if (getenv("JN_TEST_PICKUPS")) {
        Entity *jp = world_find_type(&world, "3JIM");
        if (jp) {
            const int ids[] = { 51, 162, 68, 191, 296, 87 };  /* apple,coin,diamond,gem,present,candy */
            const int N = (int)(sizeof(ids) / sizeof(ids[0]));
            for (int i = 0; i < N; i++) {
                Entity *e = world_add(&world);
                if (!e) break;
                memcpy(e->type, "3PIC", 4); e->type[4] = '\0';
                snprintf(e->tag, sizeof(e->tag), "test_pic_%d", ids[i]);
                e->sprite_index = ids[i];
                e->x = jp->x + (i - N / 2) * 70.0f;
                e->y = jp->y + 50.0f;
                e->z = jp->z - 170.0f;
            }
        }
    }

    if (getenv("JN_TEST_CLEAR")) {
        /* Force a one-item objective and complete it to exercise the HUD
           level-clear banner. */
        gamestate_item_added();
        gamestate_item_collected();
    }

    if (getenv("JN_TEST_COV")) {
        Entity *jp = world_find_type(&world, "3JIM");
        if (jp) {
            const char *kinds[] = { "3CIN","3HUG","3ULT","3FOW","3SPK","3BOT",
                                    "3NUM","3TES","3MOP","3SHU","3GRT","3TAR" };
            const int N = (int)(sizeof(kinds) / sizeof(kinds[0]));
            const float R = 300.0f;
            for (int i = 0; i < N; i++) {
                float ang = (6.28318f * i) / N;
                Entity *e = world_add(&world);
                if (!e) break;
                memcpy(e->type, kinds[i], 4); e->type[4] = '\0';
                snprintf(e->tag, sizeof(e->tag), "cov_%s", kinds[i]);
                e->x = jp->x + R * cosf(ang);
                e->y = jp->y + 60.0f;
                e->z = jp->z - 200.0f + R * sinf(ang);
                e->ry = ang + 3.14159f;
            }
        }
    }

    /* All eight gadgets. Four of them -- shrinkray, grappler, goddard and
       rocket -- have no pickup anywhere in the 35-level corpus; the original
       grants them through task/story logic nobody has recovered, so this is a
       stand-in for testing and free play rather than a claim about how the
       game hands them over. Icons are the pickups' own sprites where one
       exists (see plan section 18.1); -1 falls back to the name. */
    /* TEMPORARY (owner, 2026-08-21): every gadget in hand at the start of
       every level, so the whole set can be exercised anywhere without hunting
       for the four pickups that exist. Four of the eight have no pickup in the
       corpus at all, so there is no faithful acquisition to fall back on yet.
       Set JN_NO_GADGETS=1 to get the pickup-only behaviour back. */
    if (!env_enabled("JN_NO_GADGETS")) {
        gamestate_grant_gadget("jetpack",       99, INV_KIND_GADGET);
        gamestate_grant_gadget("shrinkray",     -1, INV_KIND_GADGET);
        gamestate_grant_gadget("bubble",        26, INV_KIND_GADGET);
        gamestate_grant_gadget("grappler",      -1, INV_KIND_GADGET);
        gamestate_grant_gadget("goddard",       -1, INV_KIND_GADGET);
        gamestate_grant_gadget("rocket",       136, INV_KIND_GADGET);
        gamestate_grant_gadget("scooter",      111, INV_KIND_PART | INV_KIND_GADGET);
        gamestate_grant_gadget("invisibility", 114, INV_KIND_PART | INV_KIND_GADGET);
        printf("[GADGET] all eight granted (temporary: every level)\n");
    }

    /* JN_TEST_TOOLS used to grant watergun/glasses/jetpack/wrench as well.
       The first three came from the speculative tool table the corpus-derived
       one replaced -- no .gam row is tagged for any of them and nothing reads
       those names -- and the jetpack is now a real gadget granted with the
       other seven. Only the baseball is left, because behavior_player really
       does gate the F throw on it. */
    if (getenv("JN_TEST_TOOLS"))
        gamestate_grant_tool("baseball", NULL);   /* enables the F throw */

    /* Wave N3: grant the baseball so the gated F-key throw (and the balloon-pop
       path) can be exercised without first walking to a PIC_NUMBER==6 pickup. */
    if (getenv("JN_TEST_BASEBALL"))
        gamestate_grant_tool("baseball", NULL);

    if (getenv("JN_TEST_GEMS")) {
        Entity *jp = world_find_type(&world, "3JIM");
        if (jp) {
            const char *grn[] = { "gemred.grn", "gemblue.grn", "gemyellow.grn" };
            for (int i = 0; i < 3; i++) {
                Entity *e = world_add(&world);
                if (!e) break;
                memcpy(e->type, "3GEM", 4); e->type[4] = '\0';
                snprintf(e->tag, sizeof(e->tag), "test_gem_%d", i);
                snprintf(e->grn_base, sizeof(e->grn_base), "%s", grn[i]);
                e->x = jp->x + (i - 1) * 80.0f;
                e->y = jp->y + 40.0f;
                e->z = jp->z - 160.0f;
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
            /* 3ASE is drawn by the dedicated per-object branch (ASEStop mesh),
               not the resolver, so it's not a placeholder when it has a mesh. */
            if (strcmp(e->type, "3ASE") == 0 && e->ase_file[0]) continue;
            /* Authored InitiallyVisible=0 never drew at boot in the original;
               an unresolved one renders nothing (no box), so don't count it. */
            if (e->has_initially_visible && e->initially_visible == 0) continue;
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

    /* Bind vtables + run on_spawn for every entity (must come after gam_load).
       Reset the cutscene shot list first so 3CAM on_spawn registers fresh. */
    cutscene_reset();
    if (!fixture_mode) entity_bind_vtables(&world);
    if (fixture_mode && !fixture_level_start_runtime(&world)) {
        fprintf(stderr, "fixture0: failed to spawn projectile\n");
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }

    /* Find JIM and frame the camera on him. JIM's spawn Y becomes the ground plane. */
    Camera *cam = renderer_camera();
    Entity *jim  = place_player(&world, NULL);
    if (jim) {
        test_place_player_near_type(&world, jim);
        world.ground_y = jim->y - jim->half_extents[1];
        gamestate_set_spawn(jim->x, jim->y, jim->z);
        printf("Player at (%.1f, %.1f, %.1f), ground_y=%.1f\n",
               jim->x, jim->y, jim->z, world.ground_y);
    }
    game_flow_enter_level(current_desc.name);
    goddard_reconcile_after_level_load(&world, current_desc.name);

    /* Headless SCENE-gate test (JN_TEST_SET_SCENE=<int>): seed the SCENE task
       state (loading a CTaskList store if a direct --level launch has none) so
       the SCENE-gated visibility leaves (3FOW/3YCA/3HUM) can be exercised at a
       chosen story value without a full campaign playthrough. No effect on a
       normal run. */
    {
        const char *s = getenv("JN_TEST_SET_SCENE");
        if (s && *s) {
            long v = strtol(s, NULL, 0);
            game_flow_test_seed_state("SCENE", v);
            printf("[JN_TEST_SET_SCENE] SCENE=0x%lx\n", v);
        }
    }

    /* Intro cutscene: play the level's scripted 3CAM shots on a real new-game
       (campaign) entry, or on demand via JN_CUTSCENE. A direct `--level X`
       launch (audit + matched-camera validators) leaves campaign mode OFF and
       JN_CUTSCENE unset, so the camera stays on the follow cam — rendering is
       unchanged from before Wave N5. */
    if (game_flow_campaign_active() || env_enabled("JN_CUTSCENE"))
        cutscene_request_intro();

    /* CMainMenu front-end: --menu opens the title/level-select over the loaded
       level as a backdrop; the player's choice routes into the level/task
       system (New Game -> NewGame.tsk -> level1b; VR items load directly). */
    help_show_timed(HELP_BOOT_SECONDS);   /* greet the first level too */
    if (want_nodamage) {
        gamestate_set_invulnerable(1);
        fprintf(stderr, "[nodamage] player damage disabled; kill plane still respawns\n");
    }
    if (want_menu)
        menu_open();
    if (fixture_mode) fixture_level_configure_floor(&world);
    else configure_safety_floor(&world, jim);

    /* Headless collision self-test seam (JN_TEST_COLLIDE=1): exercise the mesh
       CollisionWorld (ground-follow + wall clamp) and exit, mirroring the other
       JN_TEST_* hooks. */
    /* Headless CLoadLevel portal probe. Seed a departure point first if one
       was asked for, so a VR level's RETURN portals have somewhere to go. */
    if (getenv("JN_TEST_LOAD")) {
        const char *ret = getenv("JN_TEST_LOAD_RETURN");
        if (ret && *ret) {
            char lvl[64], sp[32];
            const char *colon = strchr(ret, ':');
            snprintf(lvl, sizeof lvl, "%.*s",
                     colon ? (int)(colon - ret) : (int)strlen(ret), ret);
            snprintf(sp, sizeof sp, "%s", colon ? colon + 1 : "");
            gamestate_set_level_entry(lvl, sp);
            gamestate_request_level_swap(lvl, sp);   /* promotes entry -> departure */
            gamestate_reset_for_new_level();         /* ...and drop the swap itself */
            gamestate_set_level_entry(current_desc.name, "");
            printf("[JN_TEST_LOAD] seeded departure point %s (spawn %s)\n",
                   gamestate_return_level(), gamestate_return_spawn());
        }
        int rc = load_portal_probe(&world, jim);
        ground_destroy();
        fixture_level_destroy();
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return rc;
    }

    if (getenv("JN_TEST_COLLIDE")) {
        int rc = collide_self_test(&world, jim);
        ground_destroy();
        fixture_level_destroy();
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return rc;
    }

    cam->fov = 1.047215f;
    cam->near_z = 20.0f;
    cam->far_z = 28000.0f;
    printf("[native_level] viewport=1280x720 fov_y=60.001 near=20 far=28000\n");

    FollowCam fcam;
    follow_cam_init(&fcam);
    camera_record_init_game();
    camera_record_set_ray_probe(camrec_world_probe);
    {
        /* Headless/QA hook: start the record-camera demo in a mode. */
        const char *cr = getenv("JN_CAMREC");
        if (cr && cr[0]) {
            if (!strcmp(cr, "follow") || !strcmp(cr, "1"))
                camera_record_set_mode(CAMREC_FOLLOW);
            else if (!strcmp(cr, "hold") || !strcmp(cr, "2"))
                camera_record_set_mode(CAMREC_HOLD);
        }
    }
    follow_cam_snap(&fcam, cam, jim);

    int mouse_down = 0, last_mx = 0, last_my = 0;

    printf("Controls: Up/Down (W/S) = forward/back  |  Left/Right (A/D) = turn  |  SPACE = jump  |  SHIFT = run  |  R = respawn  |  LMB drag = free-look  |  ESC = quit\n");
    printf("Entities: %d   Items: %d\n", world.count, gamestate_get()->items_total);
    int screenshot_taken = 0;
    int screenshot_mode  = getenv("JN_SCREENSHOT") != NULL;
    const char *screenshot_path = getenv("JN_SCREENSHOT_PATH");
    if (!screenshot_path || !screenshot_path[0])
        screenshot_path = "screenshot.png";
    /* In screenshot mode let a few physics ticks fire first so pending state
       (level swaps, animation transitions) actually flushes before capture. */
    int screenshot_warmup_ticks = 0;
    int screenshot_warmup_goal = 4;
    {
        const char *warmup = getenv("JN_SCREENSHOT_WARMUP_TICKS");
        if (warmup && *warmup) {
            int n = atoi(warmup);
            if (n > 0) screenshot_warmup_goal = n;
        }
    }
    if (env_enabled("JN_DEMO_AUTORUN")) {
        input_set_virtual_move(0.0f, 1.0f);
    } else if (getenv("JN_DEMO_MOVE_X") || getenv("JN_DEMO_MOVE_Y")) {
        input_set_virtual_move(env_float_default("JN_DEMO_MOVE_X", 0.0f),
                               env_float_default("JN_DEMO_MOVE_Y", 0.0f));
    }
    int demo_jump_enabled = env_enabled("JN_DEMO_JUMP");
    int demo_jump_sent = 0;
    int demo_jump_tick = demo_jump_enabled ? env_int_default("JN_DEMO_JUMP_TICK", 2) : 0;

    /* Headless combat test (Wave N2): at warmup tick JN_TEST_THROW, the player
       throws a baseball at the nearest Yokian so the enemy-defeat path can be
       exercised without keyboard input. Mirrors the other JN_TEST_* hooks. */
    int n2_throw_tick = -1;
    int n2_throw_done = 0;
    {
        const char *s = getenv("JN_TEST_THROW");
        if (s && *s) n2_throw_tick = atoi(s);
    }

    /* Headless vehicle test (Wave N4): at warmup tick JN_TEST_RIDE, board the
       nearest rocket (3ROC) and drive it forward+up via the virtual input so the
       flight path can be exercised without a keyboard. */
    int n4_ride_tick = -1, n4_ride_done = 0, n4_ride_climbed = 0;
    float n4_ride_board_y = 0.0f;
    {
        const char *s = getenv("JN_TEST_RIDE");
        if (s && *s) n4_ride_tick = atoi(s);
    }

    picture_sweep_if_requested(&world);

    /* Headless trigger test (Wave N2.x): at warmup tick JN_TEST_AITRIG, force the
       first eligible TouchActivated 3AIT through its activate core so the
       C3DAITrigger mission-wiring (hide/teleport/repoint/chain) can be exercised
       without walking the player through a volume. */
    int aitrig_test_tick = -1, aitrig_test_done = 0;
    {
        const char *s = getenv("JN_TEST_AITRIG");
        if (s && *s) aitrig_test_tick = atoi(s);
    }

    /* Headless SCENE-sequencer test (JN_TEST_SCENE=<ObjectTag>): a few ticks
       after warmup, force the AITrigger with this ObjectTag through its activate
       core so its story-progress SCENE write fires. Use with --newgame so a
       CTaskList store is loaded. Logs the SCENE transition. */
    int scene_test_tick = -1, scene_test_done = 0;
    const char *scene_test_tag = getenv("JN_TEST_SCENE");
    if (scene_test_tag && *scene_test_tag) scene_test_tick = 2;
    /* If the fired trigger targets Goddard, re-sample its pose ~60 ticks later so
       the scripted hold (stays put) vs patrol (walks the chain) is observable. */
    int goddard_script_sample_tick = -1, goddard_script_sampled = 0;

    /* Headless talk-reward test (JN_TEST_TALK=<friendTag>): a few ticks after
       warmup, force the friend NPC with this ObjectTag through its talk-reward so
       its per-character story-progress SCENE write fires (the friend half of the
       SCENE sequencer). Combine with JN_TEST_SET_SCENE=<gate> (or --newgame) so a
       CTaskList store is loaded at the friend's gate value. Logs the transition. */
    int talk_test_tick = -1, talk_test_done = 0;
    const char *talk_test_tag = getenv("JN_TEST_TALK");
    if (talk_test_tag && *talk_test_tag) talk_test_tick = 2;

    /* Headless Goddard/3MEP probe: when JN_TEST_GODDARD=1 is set, the level
       load path synthesizes C3DGoddard plus one nearby 3MEP can. Let the can's
       one-second beacon request mode 5, then report whether Goddard collected
       and released back to mode 2. */
    int goddard_test_tick = -1, goddard_test_done = 0;
    if (env_enabled("JN_TEST_GODDARD")) goddard_test_tick = 90;

    /* Headless visibility report (JN_TEST_VIS=<FourCC>): after warmup, print each
       matching entity's visible flag and the live SCENE — for SCENE-gate
       validation (3FOW/3YCA/3HUM). Fires once. */
    int vis_test_done = 0;
    const char *vis_test_type = getenv("JN_TEST_VIS");

    /* Headless swing-door test (3SWN): at warmup tick JN_TEST_SWING, force the
       nearest C3DSwingDoor through its on_trigger so the yaw-swing state machine
       can be exercised without walking the player into the doorway. */
    int swing_test_tick = -1, swing_test_done = 0;
    {
        const char *s = getenv("JN_TEST_SWING");
        if (s && *s) swing_test_tick = atoi(s);
    }

    /* CMainMenu: when the menu is up in a headless screenshot run, auto-confirm
       the default item (New Game) a few ticks after warmup so the menu route is
       exercised end-to-end in CI. JN_MENU_AUTO_TICK overrides the delay. */
    int menu_auto_tick = screenshot_warmup_goal + 2;
    {
        const char *s = getenv("JN_MENU_AUTO_TICK");
        if (s && *s) menu_auto_tick = atoi(s);
    }

    /* QA probe: JN_QA_PROBE="x,y" simulates a QA-mode click at window coords
       (x,y) once warmup settles, printing the pick JSON to stdout. Headless
       acceptance check for the annotation feature (docs/qa_annotate_plan.md):
       combine with JN_SCREENSHOT to also capture the selection highlight. */
    int qa_probe_armed = 0, qa_probe_x = 0, qa_probe_y = 0;
    {
        const char *probe = getenv("JN_QA_PROBE");
        if (probe && sscanf(probe, "%d,%d", &qa_probe_x, &qa_probe_y) == 2)
            qa_probe_armed = 1;
    }

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
    unsigned long fixed_steps = 0;
    int run_failed = 0;
    FILE *state_dump = NULL;
    if (dump_state_path) {
        state_dump = fopen(dump_state_path, "wb");
        if (!state_dump) {
            fprintf(stderr, "Cannot open --dump-state file '%s': %s\n",
                    dump_state_path, strerror(errno));
            run_failed = 1;
        }
    }

    while (!run_failed) {
        if (frame_limit >= 0) {
            if (fixed_steps >= (unsigned long)frame_limit) break;
        } else if (w.should_quit) {
            break;
        }
        Uint32 now = frame_limit >= 0
            ? (Uint32)((fixed_steps * 1000UL) / 60UL)
            : SDL_GetTicks();
        float frame_time = frame_limit >= 0 ? DT : (now - last_time) / 1000.0f;
        last_time = now;
        help_tick(frame_time);   /* fade the boot-time controls card */

        /* Cap frame time to prevent spiral of death (lag spike) */
        if (frame_time > 0.1f) frame_time = 0.1f;

        if (frame_limit >= 0) accumulator = DT;
        else accumulator += frame_time;
        if (screenshot_mode && screenshot_warmup_ticks < screenshot_warmup_goal) {
            accumulator = DT;   /* force one tick per render frame during warmup */
        }

        /* Update phase: fixed timestep */
        while (accumulator >= DT) {
            accumulator -= DT;
            fixed_steps++;

            input_update();
            if (demo_jump_enabled && !demo_jump_sent &&
                screenshot_warmup_ticks >= demo_jump_tick) {
                input_press_virtual_jump();
                demo_jump_sent = 1;
            }

            /* Front-end menu (CMainMenu): while open, gameplay is frozen and
               the player's selection routes into the level/task system. The
               currently-loaded level renders as a static backdrop. */
            help_input();
            /* The action menu (AMI). The original opens it from Jimmy's own
               input dispatcher; here it is Tab, and it refuses to stack on top
               of the front-end menu or the QA browser. */
            if (gadget_menu_is_open()) {
                gadget_menu_input();
            } else if (!menu_active() && !level_select_active() &&
                       !help_active() && input_just_pressed(SDL_SCANCODE_TAB)) {
                gadget_menu_open(0);
            }
            if (!menu_active() && !help_active() &&
                input_just_pressed(SDL_SCANCODE_M)) {
                if (level_select_active()) level_select_close();
                else                       level_select_open();
            }

            /* QA level browser. Separate from the menu above: that one is the
               certified CMainMenu routing table and must stay ten entries. */
            if (level_select_active()) {
                level_select_input();
                const char *ls_level = NULL;
                if (level_select_take_confirm(&ls_level) && ls_level) {
                    /* A browser jump is a cold entry -- pre-grant, unless this
                       is campaign play (where the backtracking is the design). */
                    if (!game_flow_campaign_active() &&
                        !env_enabled("JN_NO_PREGRANT"))
                        gamestate_pregrant_pictures(ls_level);
                    gamestate_request_level_swap(ls_level, "");
                    level_select_close();
                }
            }
            if (menu_active()) {
                menu_input();
                const char *sel_level = NULL;
                int sel_newgame = 0;
                int confirmed = menu_take_confirm(&sel_level, &sel_newgame);
                /* Headless: auto-confirm the default item (New Game) so the
                   menu route is exercised in screenshot/CI runs. */
                if (!confirmed && screenshot_mode &&
                    screenshot_warmup_ticks >= menu_auto_tick) {
                    menu_current(&sel_level, &sel_newgame);
                    confirmed = 1;
                }
                if (confirmed) {
                    if (!sel_level) {
                        w.should_quit = 1;   /* Quit item */
                    } else {
                        char ng[64] = {0};
                        if (sel_newgame) gamestate_new_game();
                        if (sel_newgame &&
                            game_flow_begin_task("NewGame", ng, sizeof ng, NULL) &&
                            ng[0])
                            sel_level = ng;   /* NewGame.tsk -> level1b */
                        gamestate_request_level_swap(sel_level, "");
                        menu_close();
                    }
                }
            }

          if (!menu_active() && !level_select_active() &&
              !gadget_menu_is_open()) {
            gamestate_tick(DT);   /* pickup-card decay */
            behavior_gadgets_update(&world, DT);
            s_arrow_clock += DT;  /* ShowArrow spin; once per frame */

            /* Per-entity behavior tick (player reads input, platforms move, etc.) */
            for (Entity *e = world.head; e; e = e->next) {
                entity_update(e, &world, DT);
            }
            /* Keep the rideable rocket in sync with the sandbox flag (handles
               runtime toggles + levels with no authored rocket), then clear any
               unhandled enter-vehicle tap so it can't leak into the next frame. */
            sandbox_reconcile(&world, jim);
            input_virtual_board_consume();

            /* Talk (C3DFriends): T talks to the nearest friend in range, running
               that friend's per-character SCENE talk-reward (the friend half of
               the SCENE sequencer). Interactive only; headless uses JN_TEST_TALK. */
            if (jim && input_just_pressed(SDL_SCANCODE_T)) {
                Entity *f = behavior_friend_talk_nearest(&world);
                if (f) printf("[TALK] player talked to '%s' (%.4s)\n", f->tag, f->type);
            }

            /* Headless combat test: throw a baseball at the nearest Yokian. */
            if (n2_throw_tick >= 0 && !n2_throw_done &&
                screenshot_warmup_ticks >= n2_throw_tick && jim) {
                n2_throw_done = 1;
                Entity *best = NULL; float bestd2 = 1e30f;
                for (Entity *en = world.head; en; en = en->next) {
                    if (!en->alive || en == jim) continue;
                    /* Yokians (N2) and balloons (N3) are both valid baseball
                       targets — picks the nearest so the test works on either. */
                    if (strncmp(en->type, "3SOL", 4) && strncmp(en->type, "3GUA", 4) &&
                        strncmp(en->type, "3SPY", 4) && strncmp(en->type, "3TUR", 4) &&
                        strncmp(en->type, "3BAL", 4)) continue;
                    float dx = en->x - jim->x, dz = en->z - jim->z;
                    float d2 = dx * dx + dz * dz;
                    if (d2 < bestd2) { bestd2 = d2; best = en; }
                }
                if (best) {
                    fprintf(stderr, "[n2_test] throwing baseball at %s '%s'\n",
                            best->type, best->tag);
                    float dx = best->x - jim->x, dy = best->y - jim->y,
                          dz = best->z - jim->z;
                    float dl = sqrtf(dx * dx + dy * dy + dz * dz);
                    float ux = dl > 1e-4f ? dx / dl : 0.0f;
                    float uy = dl > 1e-4f ? dy / dl : 0.0f;
                    float uz = dl > 1e-4f ? dz / dl : 1.0f;
                    /* Origin clear of the player's own SOLID AABB. */
                    projectile_spawn(&world,
                                     jim->x + ux * 80.0f, jim->y + 30.0f + uy * 80.0f,
                                     jim->z + uz * 80.0f,
                                     dx, dy, dz,
                                     PROJ_TEAM_PLAYER, 1200.0f, 100, 4.0f);
                }
            }

            /* Headless trigger test: force-fire an eligible C3DAITrigger. */
            if (aitrig_test_tick >= 0 && !aitrig_test_done &&
                screenshot_warmup_ticks >= aitrig_test_tick) {
                aitrig_test_done = 1;
                Entity *fired = behavior_ai_trigger_test_fire(&world);
                if (!fired)
                    fprintf(stderr, "[aitrig_test] no eligible 3AIT found\n");
            }

            /* Headless SCENE-sequencer test: fire the named story trigger. */
            if (scene_test_tick >= 0 && !scene_test_done &&
                screenshot_warmup_ticks >= scene_test_tick) {
                scene_test_done = 1;
                long before = game_flow_entity_state("SCENE");
                Entity *fired = behavior_ai_trigger_fire_tag(&world, scene_test_tag);
                long after = game_flow_entity_state("SCENE");
                fprintf(stderr, "[scene_test] tag='%s' fired=%s SCENE 0x%lx -> 0x%lx\n",
                        scene_test_tag, fired ? "yes" : "NO", before, after);
                Entity *g = behavior_goddard_get();
                if (fired && g) {
                    fprintf(stderr,
                            "[scene_test] goddard mode=%d pos=(%.0f,%.0f,%.0f) patrol='%s'\n",
                            behavior_goddard_mode(), g->x, g->y, g->z, g->patrol_point);
                    goddard_script_sample_tick = screenshot_warmup_ticks + 60;
                }
            }

            /* Re-sample Goddard after the scripted AITrigger so motion is visible:
               HOLD stays at the placed pose, PATROL walks toward the PatrolPoint. */
            if (goddard_script_sample_tick >= 0 && !goddard_script_sampled &&
                screenshot_warmup_ticks >= goddard_script_sample_tick) {
                goddard_script_sampled = 1;
                Entity *g = behavior_goddard_get();
                if (g)
                    fprintf(stderr,
                            "[scene_test] goddard +60t mode=%d pos=(%.0f,%.0f,%.0f) patrol='%s'\n",
                            behavior_goddard_mode(), g->x, g->y, g->z, g->patrol_point);
            }

            /* Headless talk-reward test: force the named friend's talk-reward. */
            if (talk_test_tick >= 0 && !talk_test_done &&
                screenshot_warmup_ticks >= talk_test_tick) {
                talk_test_done = 1;
                long before = game_flow_entity_state("SCENE");
                Entity *fired = behavior_friend_talk_tag(&world, talk_test_tag);
                long after = game_flow_entity_state("SCENE");
                fprintf(stderr, "[talk_test] tag='%s' fired=%s SCENE 0x%lx -> 0x%lx\n",
                        talk_test_tag, fired ? "yes" : "NO", before, after);
            }

            if (goddard_test_tick >= 0 && !goddard_test_done &&
                screenshot_warmup_ticks >= goddard_test_tick) {
                goddard_test_done = 1;
                Entity *g = behavior_goddard_get();
                Entity *can = NULL;
                for (Entity *e = world.head; e; e = e->next) {
                    if (strcasecmp(e->tag, "test_metal_can") == 0) {
                        can = e; break;
                    }
                }
                fprintf(stderr,
                        "[goddard_test] present=%s visible=%d mode=%d can_alive=%d target_is_can=%d\n",
                        g ? "yes" : "NO", g ? g->visible : 0,
                        behavior_goddard_mode(),
                        can ? can->alive : 0,
                        can ? behavior_goddard_target_is(can) : 0);
            }

            /* Headless visibility report for SCENE-gate validation. */
            if (vis_test_type && strlen(vis_test_type) >= 4 && !vis_test_done &&
                screenshot_warmup_ticks >= 3) {
                vis_test_done = 1;
                long scene = game_flow_entity_state("SCENE");
                int shown = 0, total = 0;
                for (Entity *v = world.head; v; v = v->next) {
                    if (!v->alive || strncmp(v->type, vis_test_type, 4) != 0) continue;
                    total++; shown += v->visible ? 1 : 0;
                    fprintf(stderr, "[vis_test] %.4s '%s' visible=%d\n",
                            v->type, v->tag, v->visible);
                }
                fprintf(stderr, "[vis_test] %.4s: %d/%d visible @ SCENE=0x%lx level='%s'\n",
                        vis_test_type, shown, total, scene, game_flow_current_level());
            }

            /* Headless swing-door test: fire the nearest 3SWN's trigger. */
            if (swing_test_tick >= 0 && !swing_test_done && jim &&
                screenshot_warmup_ticks >= swing_test_tick) {
                swing_test_done = 1;
                Entity *best = NULL; float bestd2 = 1e30f;
                for (Entity *d = world.head; d; d = d->next) {
                    if (!d->alive || strncmp(d->type, "3SWN", 4)) continue;
                    float dx = d->x - jim->x, dz = d->z - jim->z;
                    float d2 = dx * dx + dz * dz;
                    if (d2 < bestd2) { bestd2 = d2; best = d; }
                }
                if (best && best->vt && best->vt->on_trigger) {
                    fprintf(stderr, "[swing_test] firing '%s' ry=%.1f\n",
                            best->tag, best->ry);
                    best->vt->on_trigger(best, jim);
                } else {
                    fprintf(stderr, "[swing_test] no 3SWN found\n");
                }
            }

            /* Headless vehicle test: board the nearest rocket, then fly it. */
            if (n4_ride_tick >= 0 && jim) {
                if (!n4_ride_done && screenshot_warmup_ticks >= n4_ride_tick) {
                    n4_ride_done = 1;
                    Entity *best = NULL; float bestd2 = 1e30f;
                    for (Entity *rk = world.head; rk; rk = rk->next) {
                        if (!rk->alive || strncmp(rk->type, "3ROC", 4)) continue;
                        float dx = rk->x - jim->x, dz = rk->z - jim->z;
                        float d2 = dx * dx + dz * dz;
                        if (d2 < bestd2) { bestd2 = d2; best = rk; }
                    }
                    if (best) {
                        n4_ride_board_y = best->y;
                        behavior_vehicle_force_board(best);
                        fprintf(stderr, "[n4_test] boarded rocket '%s' at y=%.0f\n",
                                best->tag, best->y);
                    }
                }
                /* Once aboard, drive forward + up; log the first real climb. */
                if (behavior_vehicle_riding()) {
                    input_set_virtual_move(0.0f, 1.0f);
                    input_set_virtual_fly(1.0f);
                    Entity *rk = behavior_vehicle_current();
                    if (rk && !n4_ride_climbed && rk->y - n4_ride_board_y > 50.0f) {
                        n4_ride_climbed = 1;
                        fprintf(stderr, "[n4_test] rocket climbed to y=%.0f (+%.0f)\n",
                                rk->y, rk->y - n4_ride_board_y);
                    }
                }
            }

            /* Physics: gravity, AABB collision, trigger detection. */
            physics_step(&world, DT);
          }  /* end gameplay block — frozen while the menu or browser is up */

            /* Drain a pending level swap (also the menu's New Game / VR route).
               Done outside the entity-update iteration above so we don't mutate
               the list mid-walk, and outside the menu freeze so the confirmed
               selection actually loads. */
            const GameState *gs_pre = gamestate_get();
            if (gs_pre->swap_level[0] != '\0') {
                char level_buf[64];
                char spawn_buf[32];
                LevelDesc swap_desc;
                snprintf(level_buf, sizeof(level_buf), "%s", gs_pre->swap_level);
                snprintf(spawn_buf, sizeof(spawn_buf), "%s", gs_pre->swap_spawn);
                if (level_desc_for(level_buf, &swap_desc)) {
                    printf("[SWAP] loading %s (cache: %d tex, %d models)\n",
                           swap_desc.gam_path, asset_cache_tex_count(), asset_cache_model_count());
                    world_destroy(&world);
                    /* Every cached Entity* is dangling the instant the old
                       entities are freed. Clear them here, at the free site,
                       so nothing can read one before the new level re-arms
                       them on spawn. g_player matters most: it is armed
                       first-write-wins, so a stale value is permanent. */
                    jim = NULL;
                    behavior_player_reset();
                    behavior_vehicle_reset();
                    behavior_goddard_reset();
                    behavior_scooter_reset();
                    behavior_gadgets_reset();
                    behavior_moving_target_reset();
                    g_camrec_player = NULL;
                    s_sandbox_prepped_rocket = NULL;
                    world_init(&world);
                    gamestate_reset_for_new_level();
                    /* Re-key the collected-pickup table before the new level's
                       entities spawn (see the launch load for why). */
                    gamestate_set_level(swap_desc.name);
                    /* Where this level was entered, for a RETURN portal in it.
                       The departure pair was already promoted out of the
                       previous entry by gamestate_request_level_swap(). */
                    gamestate_set_level_entry(swap_desc.name, spawn_buf);
                    asset_cache_begin_level();
                    if (load_level(&swap_desc, &world) >= 0) {
                        current_desc = swap_desc;
                        configure_level_sky(&current_desc, &sky_model, &clouds_tex);
                        cutscene_reset();
                        entity_bind_vtables(&world);
                        /* Player textures + models + ground stay live across levels. */
                        tex_cache_get("assets/png/jimycarl.png");
                        tex_cache_get("assets/png/mud.png");
                        for (int pa = 0; pa < PA_COUNT; pa++)
                            (void)player_anim_model((PlayerAnim)pa);
                        jim = place_player(&world, spawn_buf);
                        if (jim) {
                            world.ground_y = jim->y - jim->half_extents[1];
                            gamestate_set_spawn(jim->x, jim->y, jim->z);
                        }
                        game_flow_enter_level(swap_desc.name);
                        behavior_goddard_reset();
                        behavior_scooter_reset();
                    behavior_gadgets_reset();
                    behavior_moving_target_reset();
                        goddard_reconcile_after_level_load(&world, swap_desc.name);
                        picture_sweep_if_requested(&world);
                        configure_safety_floor(&world, jim);
                        asset_cache_purge_stale();
                        printf("[SWAP] post-purge cache: %d tex, %d models\n",
                               asset_cache_tex_count(), asset_cache_model_count());
                        follow_cam_snap(&fcam, cam, jim);
                        help_show_timed(HELP_BOOT_SECONDS);   /* and every level swapped into */
                    } else {
                        fprintf(stderr, "[SWAP] gam_load failed for %s\n", swap_desc.gam_path);
                    }
                } else {
                    fprintf(stderr, "[SWAP] could not find %s under %s\n",
                            level_buf, asset_root(JN_ASSET_GAM));
                    gamestate_reset_for_new_level();
                }
                /* skip respawn/cam steps this tick — world just rebuilt */
                continue;
            }

            /* Lifecycle: manual respawn (R, no life cost), or a real death
               (health depleted by enemies, or fell below the kill plane).
               Deaths route through CJimmyGame's lives/restart flow
               (game_flow_player_died) before respawning + healing. */
            if (jim) {
                int manual_respawn = input_just_pressed(SDL_SCANCODE_R);
                float kill_y = world.safety_floor_enabled
                    ? world.safety_floor_y - 2000.0f
                    : world.ground_y - 2000.0f;
                int died = gamestate_player_is_down() || jim->y < kill_y;
                if (died) {
                    if (jim->y < kill_y)
                        printf("[DEATH] below kill plane (y=%.1f)\n", jim->y);
                    int respawn = game_flow_player_died();
                    gamestate_heal_player(gamestate_get()->health_max);
                    if (respawn) {
                        gamestate_respawn_player(jim);
                        follow_cam_snap(&fcam, cam, jim);
                    }
                } else if (manual_respawn) {
                    gamestate_respawn_player(jim);
                    follow_cam_snap(&fcam, cam, jim);
                }
            }

            /* Camera: a playing cutscene drives the pose directly; otherwise
               the follow camera tracks the player — unless a matched-camera
               override is installed (M7a), in which case the camera pose is
               fixed by the descriptor and begin_frame ignores the follow cam. */
            if (cutscene_active())
                cutscene_update(cam, &world, DT);
            else if (!renderer_camera_override_active()) {
                if (camera_record_mode() != CAMREC_OFF) {
                    /* Record-camera demo: the original DAT_00509a50
                       mechanism drives the pose (see camera_record.h). */
                    if (camera_record_mode() == CAMREC_FOLLOW) {
                        g_camrec_world = &world;
                        g_camrec_player = jim;
                        camera_record_follow_update(jim, DT);
                    }
                    camera_record_apply(cam);
                } else {
                    follow_cam_update(&fcam, cam, jim, &world, DT);
                }
            }
        }

        /* Event handling */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) w.should_quit = 1;
            /* An open controls card swallows the first ESC, the way any
               modal should; a second ESC quits as it always has. */
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                if      (help_active())         help_set(0);
                else if (level_select_active()) level_select_close();
                else                            w.should_quit = 1;
            }
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_c) g_show_coords = !g_show_coords;
            /* B ("bug"), not Q: in noclip Q is held-to-fly-down
               (behavior_player.c) and reporters fly constantly while
               annotating. Not an F-key: F2 never reached the emscripten
               build in Firefox, while letter keys are proven to arrive. */
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_b && !ev.key.repeat)
                qa_toggle();
            /* V cycles the record-camera demo: OFF -> FOLLOW -> HOLD. */
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_v && !ev.key.repeat)
                camera_record_cycle_mode();
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_1) audio_play(0);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_2) audio_play(1);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_3) audio_play(2);
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_4) audio_play(3);
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                w.width = ev.window.data1; w.height = ev.window.data2;
            }
            /* QA mode: the cursor annotates instead of steering the camera —
               clicks pick objects, motion drives the hover probe. */
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                if (qa_active()) qa_on_mouse_click(ev.button.x, ev.button.y);
                else { mouse_down = 1; last_mx = ev.button.x; last_my = ev.button.y; }
            }
            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) mouse_down = 0;
            /* Track the cursor even with QA off so toggling the mode on picks
               whatever is already under the resting pointer. */
            if (ev.type == SDL_MOUSEMOTION)
                qa_on_mouse_motion(ev.motion.x, ev.motion.y);
            if (ev.type == SDL_MOUSEMOTION && mouse_down && !qa_active()) {
                int dx = ev.motion.x - last_mx, dy = ev.motion.y - last_my;
                last_mx = ev.motion.x; last_my = ev.motion.y;
                /* Free-look: layer a temporary yaw offset (decays back behind
                   the player once he moves) + direct pitch control. */
                fcam.yaw_offset -= dx * 0.004f;
                fcam.pitch      -= dy * 0.004f;
                if (fcam.pitch >  0.6f) fcam.pitch =  0.6f;
                if (fcam.pitch < -1.2f) fcam.pitch = -1.2f;
            }
        }

        if (qa_probe_armed && screenshot_warmup_ticks >= 1) {
            if (!qa_active()) qa_toggle();
            qa_on_mouse_click(qa_probe_x, qa_probe_y);
            qa_probe_armed = 0;
        }

        /* Render phase: uncapped */
        Uint32 deterministic_now = frame_limit >= 0
            ? (Uint32)((fixed_steps * 1000UL) / 60UL)
            : SDL_GetTicks();
        capture_begin_frame(cap_seq, deterministic_now);
        renderer_begin_frame(w.width, w.height);

        /* Sky behind everything: full rotating cloud hemisphere first, then the
           faithful static painted-neighborhood backdrop on top at the horizon. */
        sky_spin += DT * 0.012f;   /* slow cloud drift */
        if (clouds_tex) renderer_draw_cloud_dome(clouds_tex, sky_spin);
        if (sky_model)  renderer_draw_sky_dome(sky_model, 0.0f);

        /* Procedural ground plane: drawn beneath the authored OMT level (the
           physics backstop is the separate, env-gated safety floor). Always
           drawn at safety_floor_y so toggling the physics gate never shifts the
           visible plane; retiring this plane is Phase 4 of the collision
           overhaul. */
        ground_draw(world.safety_floor_y);

        /* Pickable scene content: entities + placements (single enumeration
           path shared with the QA pick pass -- see draw_scene). */
        qa_begin_scene(0);
        draw_scene(&world, jim_model_ok);
        qa_end_scene();

        /* QA pick pass: re-enumerate the same scene into the offscreen ID
           buffer and resolve the object under the cursor. Throttled inside
           qa_want_pick; immediate when a click is pending. */
        if (qa_want_pick(deterministic_now) &&
            renderer_pick_begin(w.width, w.height)) {
            qa_begin_scene(1);
            draw_scene(&world, jim_model_ok);
            int qx, qy;
            qa_get_cursor(&qx, &qy);
            qa_pick_resolve(renderer_pick_end(qx, qy), current_desc.name,
                            cam->pos[0], cam->pos[1], cam->pos[2], cam->yaw);
        }

        /* 2D HUD overlay, drawn after all 3D geometry. */
        if (hud_enabled && !menu_active())
            hud_draw(w.width, w.height, gamestate_get());

        /* CMainMenu overlay (drawn over the backdrop scene when open). */
        menu_draw(w.width, w.height);
        gadget_menu_draw(w.width, w.height);
        level_select_draw(w.width, w.height);
        help_draw(w.width, w.height);

        /* Live coordinate readout (QA) — player draw-space pos + facing. */
        dbg_coords_overlay(w.width, w.height, jim);

        renderer_end_frame();
        capture_end_frame();
        cap_seq++;

        if (state_dump)
            dump_deterministic_state(state_dump, (unsigned int)(fixed_steps - 1), &world);

        if (dump_png_dir) {
            char png_path[PATH_MAX];
            if (snprintf(png_path, sizeof(png_path), "%s/frame_%06lu.png",
                         dump_png_dir, fixed_steps - 1) >= (int)sizeof(png_path)) {
                fprintf(stderr, "--dump-png path is too long\n");
                run_failed = 1;
            } else {
                glFinish();
                save_screenshot(png_path, w.width, w.height);
            }
        }

        /* Capture frame budget exhausted: break so the normal shutdown path
           (input/SDL teardown) runs. No-op in non-capture builds. */
        if (capture_should_exit()) break;

        if (screenshot_mode && !screenshot_taken) {
            if (screenshot_warmup_ticks >= screenshot_warmup_goal) {
                glFinish();
                dump_entity_positions(&world);
                save_screenshot(screenshot_path, w.width, w.height);
                screenshot_taken = 1;
                w.should_quit = 1;
            } else {
                screenshot_warmup_ticks++;
            }
        }

        /* HUD: update window title with FPS, items, and player state. */
        frame_count++;
        Uint32 now_fps = frame_limit >= 0 ? deterministic_now : SDL_GetTicks();
        if (frame_limit < 0 && now_fps - fps_time >= 500) {
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

    if (state_dump) fclose(state_dump);

    capture_shutdown();
    player_anim_destroy();
    world_box_destroy();
    ground_destroy();
    fixture_level_destroy();
    world_destroy(&world);
    asset_cache_destroy_all();
    input_destroy();
    audio_destroy();
    renderer_destroy();
    window_destroy(&w);
    return run_failed ? 1 : 0;
}
