#include "../engine/window.h"
#include "../engine/glad.h"
#include "../engine/renderer.h"
#include "../engine/world.h"
#include "../engine/audio.h"
#include "../engine/input.h"
#include "../engine/physics.h"
#include "../engine/ground.h"
#include "../engine/canon_data.h"   /* Phase 12: measured ground footprint/topography */
#include "../engine/capture.h"
#include "../engine/assets/gam_loader.h"
#include "../engine/assets/ase_loader.h"
#include "../engine/assets/tex_loader.h"
#include "../engine/assets/asset_cache.h"
#include "../engine/assets/placement_loader.h"
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

int main(void) {
    Window w;
    if (!window_init(&w, "JN Engine - Step 4: Textured Scene", 1280, 720))
        return 1;

    if (!renderer_init(w.width, w.height)) {
        window_destroy(&w);
        return 1;
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

    World world;
    world_init(&world);
    world_box_init();
    gamestate_init();

    /* Load all player poses with the shared real-Jimmy texture. All jim*.ASE
       reference jimmylast.bmp (which doesn't exist); jimycarl.png is the
       atlas that matches the model UVs (red shirt, atom emblem, backpack). */
    asset_cache_begin_level();
    unsigned int jim_tex = tex_cache_get("assets/png/jimycarl.png");
    int jim_poses_loaded = player_anim_init(jim_tex);
    int jim_model_ok = (jim_poses_loaded > 0);

    int n = gam_load(&world, "assets/gam/Level1.gam");
    if (n < 0) {
        fprintf(stderr, "Failed to load Level1.gam\n");
        world_destroy(&world);
        renderer_destroy();
        window_destroy(&w);
        return 1;
    }
    /* Static city geometry: OMT chunk centers from level1.omt → world placements. */
    placements_load(&world, "assets/ase/omt/level1_placements.txt");
    {
        int missing = 0;
        for (int i = 0; i < world.placement_count; i++)
            if (!model_cache_get(world.placements[i].ase_path)) missing++;
        printf("placements_loaded=%d, missing_mesh=%d\n", world.placement_count, missing);
    }

    /* Level1 has no ITEM entities; synthesize a small ring around the player
       spawn so the Phase 4 pickup/win loop is exercisable. */
    {
        Entity *jim_pre = world_find_type(&world, "3JIM");
        if (jim_pre) {
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
    if (jim) {
        world.ground_y = jim->y - jim->half_extents[1];
        gamestate_set_spawn(jim->x, jim->y, jim->z);
        printf("Player at (%.1f, %.1f, %.1f), ground_y=%.1f\n",
               jim->x, jim->y, jim->z, world.ground_y);
    }
    cam->fov    = 1.0472f;
    cam->far_z  = 80000.0f;

    /* Ground: a real level (placements present) supplies its own ground/street/
       terrain and water meshes (GROUND.ASE, ncwater*, etc.) at their authored
       world positions -- the faithful representation. We do NOT lay a synthetic
       ground over them (an earlier heightfield was a stand-in that buried the
       real geometry, including the stream). The synthetic flat floor is kept
       ONLY for empty test scenes with no level geometry. */
    if (world.placement_count == 0) {
        unsigned int ground_tex = tex_cache_get(CANON_GROUND_TEXTURE);
        ground_init(ground_tex, 20000.0f, 20000.0f,
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
                        if (strncasecmp(level_buf, "level1", 6) == 0)
                            placements_load(&world,
                                            "assets/ase/omt/level1_placements.txt");
                        entity_bind_vtables(&world);
                        /* Player textures + models + ground stay live across levels. */
                        tex_cache_get("assets/png/jimycarl.png");
                        tex_cache_get("assets/png/mud.png");
                        for (int pa = 0; pa < PA_COUNT; pa++)
                            (void)player_anim_model((PlayerAnim)pa);
                        asset_cache_purge_stale();
                        printf("[SWAP] post-purge cache: %d tex, %d models\n",
                               asset_cache_tex_count(), asset_cache_model_count());
                        jim = place_player(&world, spawn_buf);
                        if (jim) {
                            world.ground_y = jim->y - jim->half_extents[1];
                            gamestate_set_spawn(jim->x, jim->y, jim->z);
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

        /* Static world geometry from level1.omt (Phase 8). Translation uses
           (x, 0, z) because the exporter bakes height into vertices. */
        for (int pi = 0; pi < world.placement_count; pi++) {
            const WorldPlacement *pl = &world.placements[pi];
            AseModel *pm = model_cache_get(pl->ase_path);
            if (!pm) continue;
            renderer_draw_model(pm, 0, pl->x, 0.0f, pl->z, 0.0f, 1.0f);
        }

        renderer_end_frame();
        capture_end_frame();
        cap_seq++;

        /* Capture frame budget exhausted: break so the normal shutdown path
           (input/SDL teardown) runs. No-op in non-capture builds. */
        if (capture_should_exit()) break;

        if (screenshot_mode && !screenshot_taken) {
            if (screenshot_warmup_ticks >= SCREENSHOT_WARMUP_GOAL) {
                glFinish();
                save_screenshot("screenshot.png", w.width, w.height);
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
