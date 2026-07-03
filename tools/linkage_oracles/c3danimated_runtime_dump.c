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
#include "../../src/game/behaviors/behavior_ai.h"
#include "../../src/game/behaviors/behavior_projectile.h"
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
static AseModel g_model_jim_fence;
static AseModel g_model_jim_splat;
static AseModel g_model_jim_hit;
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
    init_model(&g_model_jim_fence, 3, 10.0f);
    init_model(&g_model_jim_splat, 5, 10.0f);
    init_model(&g_model_jim_hit, 4, 10.0f);
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
    if (ends_ci(path, "jimfence.ASE")) return &g_model_jim_fence;
    if (ends_ci(path, "jimsplat.ASE")) return &g_model_jim_splat;
    if (ends_ci(path, "jimhit.ASE")) return &g_model_jim_hit;
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

static int g_damage_down;
static int g_damage_calls;

void gamestate_damage_player(int amount) {
    if (amount > 0) g_damage_calls++;
}

int gamestate_player_is_down(void) {
    return g_damage_down;
}

int physics_aabb_overlap(const Entity *a, const Entity *b) {
    (void)a; (void)b;
    return 1;
}

float world_query_segment(const World *w, const Entity *ignore,
                          float px, float py, float pz,
                          float qx, float qy, float qz) {
    (void)w; (void)ignore; (void)px; (void)py; (void)pz;
    (void)qx; (void)qy; (void)qz;
    return 1.0f;
}

Entity *world_add(World *w) {
    (void)w;
    return calloc(1, sizeof(Entity));
}

void behavior_ai_idle(Entity *e) { (void)e; }

BehaviorAIResult behavior_ai_seek_position(Entity *e, float x, float y, float z,
                                           float speed, float arrive_radius,
                                           float dt) {
    (void)e; (void)x; (void)y; (void)z; (void)speed; (void)arrive_radius; (void)dt;
    return BEHAVIOR_AI_SEEKING;
}

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

static void init_player_entity(Entity *e) {
    memset(e, 0, sizeof(*e));
    memcpy(e->type, "3JIM", 4);
    e->alive = 1;
    e->visible = 1;
    g_player = e;
    player_anim_bind_entity(e);
}

static void run_until_idle(Entity *e, PlayerAnim a) {
    for (int i = 0; i < 16; i++) {
        player_anim_advance_entity(e, a, 0.1f);
        if (e->user_flag == (int)PA_IDLE) {
            const char *alias = animated_dispatch_active_alias(e);
            if (alias && strcasecmp(alias, "STOP") == 0) break;
        }
    }
}

static void trigger_enemy_projectile(Entity *jim, int down) {
    World w;
    memset(&w, 0, sizeof(w));
    Entity bolt;
    memset(&bolt, 0, sizeof(bolt));
    memcpy(bolt.type, "PROJ", 4);
    bolt.alive = 1;
    bolt.visible = 1;
    bolt.user_flag = PROJ_TEAM_ENEMY;
    bolt.user_float = 1.0f;
    bolt.points = down ? 100 : 10;
    g_player = jim;
    g_damage_down = down;
    vt_projectile.on_update(&bolt, &w, 0.016f);
}

static void trigger_yokian_melee(Entity *jim) {
    World w;
    memset(&w, 0, sizeof(w));
    Entity y;
    memset(&y, 0, sizeof(y));
    memcpy(y.type, "3SOL", 4);
    y.alive = 1;
    y.visible = 1;
    y.x = jim->x;
    y.z = jim->z;
    g_player = jim;
    g_damage_down = 0;
    vt_yokian.on_update(&y, &w, 0.016f);
}

static void trigger_tesla_contact(Entity *jim) {
    Entity tesla;
    memset(&tesla, 0, sizeof(tesla));
    memcpy(tesla.type, "3TES", 4);
    tesla.alive = 1;
    tesla.visible = 1;
    tesla.user_flag = 1;
    tesla.x = jim->x + 100.0f;
    tesla.z = jim->z;
    g_player = jim;
    g_damage_down = 0;
    vt_tesla.on_trigger(&tesla, jim);
}

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
    (void)player_anim_init(0);
    init_player_entity(&jim);
    player_anim_advance_entity(&jim, PA_LADDER, 0.0f);
    player_anim_advance_entity(&jim, PA_LADDER, 0.1f);
    player_anim_advance_entity(&jim, PA_LADDER, 0.1f);
    print_dispatch("PLAYER", &jim);

    Entity fence;
    init_player_entity(&fence);
    (void)player_anim_start_entity_state(&fence, PA_FENCE);
    run_until_idle(&fence, PA_FENCE);
    print_dispatch("FENCE", &fence);

    Entity hit;
    init_player_entity(&hit);
    trigger_enemy_projectile(&hit, 0);
    print_dispatch("PROJECTILE_HIT", &hit);
    run_until_idle(&hit, PA_HIT);
    print_dispatch("PROJECTILE_HIT_END", &hit);

    Entity splat;
    init_player_entity(&splat);
    trigger_enemy_projectile(&splat, 1);
    print_dispatch("PROJECTILE_SPLAT", &splat);
    run_until_idle(&splat, PA_SPLAT);
    print_dispatch("PROJECTILE_SPLAT_END", &splat);

    Entity enemy_hit;
    init_player_entity(&enemy_hit);
    trigger_yokian_melee(&enemy_hit);
    print_dispatch("ENEMY_HIT", &enemy_hit);

    Entity tesla_hit;
    init_player_entity(&tesla_hit);
    trigger_tesla_contact(&tesla_hit);
    print_dispatch("TESLA_HIT", &tesla_hit);

    animated_dispatch_free_entity(&base);
    animated_dispatch_free_entity(&car);
    animated_dispatch_free_entity(&jim);
    animated_dispatch_free_entity(&fence);
    animated_dispatch_free_entity(&hit);
    animated_dispatch_free_entity(&splat);
    animated_dispatch_free_entity(&enemy_hit);
    animated_dispatch_free_entity(&tesla_hit);
    player_anim_destroy();
    return 0;
}
