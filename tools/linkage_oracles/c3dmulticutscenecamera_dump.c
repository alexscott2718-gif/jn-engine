/* Headless dumper for the C3DMultiCutSceneCamera linkage oracle
   (docs/linked_parity_plan.md L3). Pulls in the REAL, unmodified
   src/game/behaviors/behavior_cutscene.c via #include so this driver's main()
   shares its translation unit and can call the file's `static`
   cutscene_mca_local_offset and entity_transform_local directly -- the only
   way to reach them without changing their linkage. Same stub set as the
   CLoadLevel/C3DCutSceneCamera oracles' dumpers (audio/anim/spawn externs
   unrelated to these functions). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/animated_dispatch.h"
#include "../../src/engine/assets/asset_cache.h"
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
void player_anim_advance_entity(Entity *e, PlayerAnim a, float dt) {
    (void)e; (void)a; (void)dt;
}
int player_anim_start_entity_state(Entity *e, PlayerAnim a) {
    (void)e; (void)a;
    return 0;
}
void behavior_animated_spawn_base(Entity *e) { (void)e; }
AseModel *model_cache_get(const char *path) { (void)path; return NULL; }
void animated_dispatch_clip_from_ase(AnimatedClip *out, const AseModel *model) {
    (void)model;
    if (out) memset(out, 0, sizeof(*out));
}
void animated_dispatch_set_key_strings(const char *suffix_base,
                                       const char *suffix_alt,
                                       const char *prefix) {
    (void)suffix_base; (void)suffix_alt; (void)prefix;
}
AnimatedDispatch *animated_dispatch_init_entity(Entity *e) {
    (void)e;
    return NULL;
}
AnimatedRecord *animated_dispatch_create_record(Entity *e, const char *name,
                                                const AnimatedClip *clip) {
    (void)e; (void)name; (void)clip;
    return NULL;
}
const AnimatedRecord *animated_dispatch_find_record(const Entity *e,
                                                    const char *name) {
    (void)e; (void)name;
    return NULL;
}
AnimatedDispatchResult animated_dispatch_set_by_name(Entity *e,
                                                     const char *name,
                                                     int loop) {
    (void)e; (void)name; (void)loop;
    return ANIMATED_DISPATCH_NOT_FOUND;
}

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
        if (!tok || (tok[0] != 'M' && tok[0] != 'W')) continue;

        char *idx = strtok_r(NULL, "|\n", &save);
        int camera_type = atoi(strtok_r(NULL, "|\n", &save));
        float t = bits_f32(strtok_r(NULL, "|\n", &save));

        float out[3];
        cutscene_mca_local_offset(camera_type, t, out);
        if (tok[0] == 'M') {
            printf("M|%s|%08x|%08x|%08x\n", idx,
                   f32_bits(out[0]), f32_bits(out[1]), f32_bits(out[2]));
        } else {
            Entity target;
            memset(&target, 0, sizeof target);
            target.x  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.y  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.z  = bits_f32(strtok_r(NULL, "|\n", &save));
            target.rx = bits_f32(strtok_r(NULL, "|\n", &save));
            target.ry = bits_f32(strtok_r(NULL, "|\n", &save));
            target.rz = bits_f32(strtok_r(NULL, "|\n", &save));
            float look_voffset = bits_f32(strtok_r(NULL, "|\n", &save));

            float world[3];
            entity_transform_local(&target, out, world);
            printf("W|%s|%08x|%08x|%08x|%08x|%08x|%08x\n", idx,
                   f32_bits(world[0]), f32_bits(world[1]), f32_bits(world[2]),
                   f32_bits(target.x), f32_bits(target.y + look_voffset - 60.0f),
                   f32_bits(target.z));
        }
    }
    return 0;
}
