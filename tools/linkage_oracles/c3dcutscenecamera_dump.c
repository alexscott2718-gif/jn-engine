/* Headless dumper for the C3DCutSceneCamera linkage oracle
   (docs/linked_parity_plan.md L3). Pulls in the REAL, unmodified
   src/game/behaviors/behavior_cutscene.c via #include so this driver's main()
   shares its translation unit and can call its `static` functions
   (cutscene_3cam_dist, cutscene_3cam_place) directly -- the only way to reach
   them without changing their linkage.

   behavior_cutscene.c also defines other cutscene machinery (audio dispatch,
   sequencing, player-anim overrides) this oracle doesn't exercise; those
   reference a handful of engine externs that are irrelevant to the pure
   placement/distance math under test here. Stub them minimally below so the
   translation unit links standalone -- same spirit as the CLoadLevel oracle's
   local world_add(). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/player_anim.h"

Entity *g_player = NULL;

/* gam_loader.c (linked in for the real gam_prop_f/gam_prop_i/gam_str that
   behavior_cutscene.c calls) needs world_add(); reproduced here byte-for-byte
   from src/engine/world.c, same as the CLoadLevel oracle's dumper. */
Entity *world_add(World *w) {
    Entity *e = calloc(1, sizeof(Entity));
    if (!e) return NULL;
    e->alive = 1;
    e->visible = 1;
    e->omt_index = -1;
    e->next = w->head;
    w->head = e;
    w->count++;
    return e;
}

int audio_play_db(const char *db, int handle, int loops, int gain) {
    (void)db; (void)handle; (void)loops; (void)gain;
    return -1;
}
float audio_duration_db(const char *db, int handle) {
    (void)db; (void)handle;
    return 0.0f;
}
void audio_channel_halt(int channel) { (void)channel; }
void player_anim_advance(PlayerAnim a, float dt) { (void)a; (void)dt; }
void behavior_animated_spawn_base(Entity *e) { (void)e; }

#include "../../src/game/behaviors/behavior_cutscene.c"

static unsigned int f32_bits(float f) {
    unsigned int u;
    memcpy(&u, &f, 4);
    return u;
}

static float bits_f32(const char *hex) {
    unsigned int u = (unsigned int)strtoul(hex, NULL, 16);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

int main(void) {
    char line[512];
    while (fgets(line, sizeof line, stdin)) {
        char *save = NULL;
        char *tok = strtok_r(line, "|\n", &save);
        if (!tok) continue;

        if (tok[0] == 'D') {
            char *idx = strtok_r(NULL, "|\n", &save);
            CutSceneShot s;
            memset(&s, 0, sizeof s);
            s.initial_dist = bits_f32(strtok_r(NULL, "|\n", &save));
            s.zoom_speed   = bits_f32(strtok_r(NULL, "|\n", &save));
            s.min_dist     = bits_f32(strtok_r(NULL, "|\n", &save));
            s.max_dist     = bits_f32(strtok_r(NULL, "|\n", &save));
            float t        = bits_f32(strtok_r(NULL, "|\n", &save));
            float dist = cutscene_3cam_dist(&s, t);
            printf("D|%s|%08x\n", idx, f32_bits(dist));
        } else if (tok[0] == 'S') {
            char *idx = strtok_r(NULL, "|\n", &save);
            CutSceneShot s;
            memset(&s, 0, sizeof s);
            s.camera_type      = atoi(strtok_r(NULL, "|\n", &save));
            s.view_from_camera = atoi(strtok_r(NULL, "|\n", &save));
            s.cam_pos[0] = bits_f32(strtok_r(NULL, "|\n", &save));
            s.cam_pos[1] = bits_f32(strtok_r(NULL, "|\n", &save));
            s.cam_pos[2] = bits_f32(strtok_r(NULL, "|\n", &save));
            s.offset[0]  = bits_f32(strtok_r(NULL, "|\n", &save));
            s.offset[1]  = bits_f32(strtok_r(NULL, "|\n", &save));
            s.offset[2]  = bits_f32(strtok_r(NULL, "|\n", &save));
            s.look_voffset = bits_f32(strtok_r(NULL, "|\n", &save));

            Entity target;
            memset(&target, 0, sizeof target);
            target.x  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.y  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.z  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.ry = bits_f32(strtok_r(NULL, "|\n", &save));

            float t = bits_f32(strtok_r(NULL, "|\n", &save));

            float cam[3], look[3];
            cutscene_3cam_place(&s, &target, t, cam, look);
            printf("S|%s|%08x|%08x|%08x|%08x|%08x|%08x\n", idx,
                   f32_bits(cam[0]), f32_bits(cam[1]), f32_bits(cam[2]),
                   f32_bits(look[0]), f32_bits(look[1]), f32_bits(look[2]));
        }
    }
    return 0;
}
