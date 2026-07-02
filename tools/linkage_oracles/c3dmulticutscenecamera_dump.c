/* Headless dumper for the C3DMultiCutSceneCamera linkage oracle
   (docs/linked_parity_plan.md L3). Pulls in the REAL, unmodified
   src/game/behaviors/behavior_cutscene.c via #include so this driver's main()
   shares its translation unit and can call the file's `static`
   cutscene_mca_local_offset directly -- the only way to reach it without
   changing its linkage. Same stub set as the CLoadLevel/C3DCutSceneCamera
   oracles' dumpers (audio/anim/spawn externs unrelated to this function). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/player_anim.h"

Entity *g_player = NULL;

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
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        char *save = NULL;
        char *tok = strtok_r(line, "|\n", &save);
        if (!tok || tok[0] != 'M') continue;

        char *idx = strtok_r(NULL, "|\n", &save);
        int camera_type = atoi(strtok_r(NULL, "|\n", &save));
        float t = bits_f32(strtok_r(NULL, "|\n", &save));

        float out[3];
        cutscene_mca_local_offset(camera_type, t, out);
        printf("M|%s|%08x|%08x|%08x\n", idx,
               f32_bits(out[0]), f32_bits(out[1]), f32_bits(out[2]));
    }
    return 0;
}
