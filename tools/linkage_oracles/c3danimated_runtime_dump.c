/* Runtime wiring dumper for C3DAnimated/event-animation-dispatch.

   This harness includes the real behavior_cutscene.c so it can call the static
   cutscene_apply_target_anim bridge, and links the real animated_dispatch.c,
   behavior_base.c, and player_anim.c. Asset/audio/game-flow dependencies are
   stubbed with deterministic fake models; the test is the runtime dispatch
   wiring, not filesystem asset loading. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/animated_dispatch.h"
#include "../../src/game/player_anim.h"
#include "../../src/engine/assets/asset_cache.h"

#ifndef BEHAVIOR_CUTSCENE_SOURCE
#define BEHAVIOR_CUTSCENE_SOURCE "../../src/game/behaviors/behavior_cutscene.c"
#endif

Entity *g_player = NULL;

static AseModel g_model_carl_stop;
static AseModel g_model_carl_walk;
static AseModel g_model_jim_stop;
static AseModel g_model_jim_ladder;
static AseModel g_model_default;

static void init_model(AseModel *m, int frames, float fps) {
    memset(m, 0, sizeof(*m));
    m->frame_count = frames;
    m->framespeed = fps;
}

static void init_models(void) {
    init_model(&g_model_carl_stop, 2, 5.0f);
    init_model(&g_model_carl_walk, 5, 5.0f);
    init_model(&g_model_jim_stop, 2, 10.0f);
    init_model(&g_model_jim_ladder, 2, 10.0f);
    init_model(&g_model_default, 3, 5.0f);
}

static int ends_ci(const char *s, const char *suffix) {
    size_t n = s ? strlen(s) : 0;
    size_t m = suffix ? strlen(suffix) : 0;
    return n >= m && strcasecmp(s + n - m, suffix) == 0;
}

AseModel *model_cache_get(const char *path) {
    if (!path) return NULL;
    if (ends_ci(path, "carlstop.ASE")) return &g_model_carl_stop;
    if (ends_ci(path, "carlwalk.ASE")) return &g_model_carl_walk;
    if (ends_ci(path, "jimstop.ase")) return &g_model_jim_stop;
    if (ends_ci(path, "jimladder.ASE")) return &g_model_jim_ladder;
    if (strstr(path, "assets/ase/jim") || strstr(path, "assets/ase/Jim"))
        return &g_model_default;
    return &g_model_default;
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

long game_flow_entity_state(const char *name) {
    (void)name;
    return -1;
}

float gam_prop_f(const Entity *e, const char *name, float def) {
    (void)e; (void)name;
    return def;
}

int gam_prop_i(const Entity *e, const char *name, int def) {
    (void)e; (void)name;
    return def;
}

const char *gam_str(const Entity *e, const char *name, const char *def) {
    (void)e; (void)name;
    return def;
}

#include BEHAVIOR_CUTSCENE_SOURCE

static void print_dispatch(const char *tag, Entity *e) {
    AnimatedDispatch *d = e->anim_dispatch;
    const AnimatedRecord *cur = animated_dispatch_current_record(e);
    AnimatedDispatchSample s;
    int has_sample = animated_dispatch_sample(e, &s);
    const char *alias = animated_dispatch_active_alias(e);
    printf("%s alias=%s idx=%d rec_fc=%d frame=%d sample=%d:%d:%.3f fires=%d loop=%d model=%s user=%d\n",
           tag,
           alias ? alias : "-",
           d ? d->current_index : -1,
           cur ? cur->frame_count : -1,
           d ? d->cur_frame : -99,
           has_sample ? s.frame_a : -1,
           has_sample ? s.frame_b : -1,
           has_sample ? s.lerp : -1.0f,
           d ? d->anim_ended_fires : -1,
           d ? d->play_loop : -1,
           e->cutscene_model[0] ? e->cutscene_model : "-",
           e->user_flag);
}

int main(void) {
    init_models();

    AnimatedClip base_clip = { 2, 64.0f };
    Entity base;
    memset(&base, 0, sizeof(base));
    base.alive = 1;
    base.visible = 1;
    animated_dispatch_set_key_strings("HI", "HI", "");
    (void)animated_dispatch_init_entity(&base);
    (void)animated_dispatch_create_record(&base, "HIEND", &base_clip);
    (void)animated_dispatch_set_by_name(&base, "END", 0);
    (void)behavior_animated_update_base(&base, NULL, 0.064f);
    (void)behavior_animated_update_base(&base, NULL, 0.064f);
    print_dispatch("BASE", &base);

    Entity car;
    memset(&car, 0, sizeof(car));
    memcpy(car.type, "3CAR", 4);
    car.alive = 1;
    car.visible = 1;
    cutscene_apply_target_anim(&car, "WALK", 0);
    (void)behavior_animated_update_base(&car, NULL, 0.2f);
    (void)behavior_animated_update_base(&car, NULL, 0.2f);
    print_dispatch("CUT", &car);

    Entity jim;
    memset(&jim, 0, sizeof(jim));
    memcpy(jim.type, "3JIM", 4);
    jim.alive = 1;
    jim.visible = 1;
    g_player = &jim;
    (void)player_anim_init(0);
    player_anim_bind_entity(&jim);
    player_anim_advance_entity(&jim, PA_LADDER, 0.0f);
    player_anim_advance_entity(&jim, PA_LADDER, 0.1f);
    player_anim_advance_entity(&jim, PA_LADDER, 0.1f);
    print_dispatch("PLAYER", &jim);

    animated_dispatch_free_entity(&base);
    animated_dispatch_free_entity(&car);
    animated_dispatch_free_entity(&jim);
    player_anim_destroy();
    return 0;
}
