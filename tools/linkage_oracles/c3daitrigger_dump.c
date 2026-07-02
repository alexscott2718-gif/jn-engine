/* Headless dumper for the C3DAITrigger linkage oracle
   (docs/linked_parity_plan.md L3). Links the real, unmodified
   behavior_ai_trigger.c + game_flow.c + task_loader.c + gam_loader.c and
   drives its public JN_TEST_SCENE-style test hook
   (behavior_ai_trigger_fire_tag) -- which bypasses the touch/arm gates and
   goes straight into the real aitrig_activate() mutation-and-dispatch core --
   over a set of synthetic scenarios covering the two pieces of
   ActivateAITrigger (FUN_0040c300) that are fully decompiled and
   independent of the deferred subsystems (AIState/AISpeed general C3DAI
   machine, AIAnim/ActivateByAnim, the ActivateObject0..4 state machine,
   reward/counter/menu side effects):

     1. Target mutation: AIHideObj show/hide, AINewPos teleport, AINewRotY,
        AIPatrol repoint, then ToggleObject and NextTrigger forwarding.
     2. ApplyAITriggerStoryProgress (FUN_0040caa0): the SCENE story-progress
        patch table (docs/decomp/_scene_sequencer.md).

   Stubs behavior_animated_spawn_base (spawn-time level gating, irrelevant to
   a synthetic already-alive entity) and behavior_goddard_apply_ai_state (a
   no-op for non-Goddard targets anyway, per the file's own comment; AIState/
   AISpeed are outside this aspect's certified scope) and
   physics_aabb_overlap (only reachable from aitrig_on_update, which this
   driver never calls -- fire_tag bypasses it entirely). world_add() is
   reproduced byte-for-byte from src/engine/world.c, same as the earlier
   gam_load-based oracles' dumpers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/game_flow.h"

Entity *g_player = NULL;

void behavior_animated_spawn_base(Entity *e) { (void)e; }
int behavior_goddard_apply_ai_state(Entity *e, int ai_state, int ai_speed) {
    (void)e; (void)ai_state; (void)ai_speed;
    return 0;
}
int physics_aabb_overlap(const Entity *a, const Entity *b) {
    (void)a; (void)b;
    return 0;
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

Entity *behavior_ai_trigger_test_fire(World *w);
Entity *behavior_ai_trigger_fire_tag(World *w, const char *tag);

static Entity *spawn(World *w, const char *type, const char *tag) {
    Entity *e = world_add(w);
    memcpy(e->type, type, 4);
    e->type[4] = '\0';
    snprintf(e->tag, sizeof(e->tag), "%s", tag);
    return e;
}

static void set_str_prop(Entity *e, const char *name, const char *val) {
    /* str_prop_bag_add is static in gam_loader.c; replicate its (documented,
       trivial) semantics here: skip "none"/empty, else append to the bag. */
    if (!val[0] || strcasecmp(val, "none") == 0) return;
    if (e->nstrs >= ENTITY_MAX_STRS) return;
    GamStr *s = &e->strs[e->nstrs++];
    snprintf(s->name, sizeof(s->name), "%s", name);
    snprintf(s->val, sizeof(s->val), "%s", val);
}

static void set_int_prop(Entity *e, const char *name, int val) {
    if (e->nprops >= ENTITY_MAX_PROPS) return;
    GamProp *p = &e->props[e->nprops++];
    snprintf(p->name, sizeof(p->name), "%s", name);
    p->f = (float)val;
    p->i = val;
}

static void set_float_prop(Entity *e, const char *name, float val) {
    if (e->nprops >= ENTITY_MAX_PROPS) return;
    GamProp *p = &e->props[e->nprops++];
    snprintf(p->name, sizeof(p->name), "%s", name);
    p->f = val;
    p->i = (int)val;
}

/* Trivial vt with an on_trigger that just counts invocations, so ToggleObject/
   NextTrigger forwarding is directly observable. */
static int g_toggle_fired = 0;
static int g_next_fired = 0;
static void toggle_on_trigger(Entity *e, Entity *by) { (void)e; (void)by; g_toggle_fired++; }
static void next_on_trigger(Entity *e, Entity *by) { (void)e; (void)by; g_next_fired++; }
static const EntityVTable vt_toggle_probe = { NULL, NULL, toggle_on_trigger, 0 };
static const EntityVTable vt_next_probe   = { NULL, NULL, next_on_trigger, 0 };

static unsigned int f32_bits(float f) {
    unsigned int u;
    memcpy(&u, &f, 4);
    return u;
}

/* ---- Scenario 1: hide (AIHideObj=0) ------------------------------------- */
static void scenario_hide(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_HIDE");
    Entity *target = spawn(w, "3JIM", "TGT_HIDE");
    target->visible = 1;
    target->runtime_flags = ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER;
    set_str_prop(trig, "AITarget", "TGT_HIDE");
    set_int_prop(trig, "AIHideObj", 0);

    g_toggle_fired = g_next_fired = 0;
    behavior_ai_trigger_fire_tag(w, "T_HIDE");
    printf("S|hide|%d|%08x\n", target->visible, target->runtime_flags);
}

/* ---- Scenario 2: show (AIHideObj=1) ------------------------------------- */
static void scenario_show(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_SHOW");
    Entity *target = spawn(w, "3JIM", "TGT_SHOW");
    target->visible = 0;
    set_str_prop(trig, "AITarget", "TGT_SHOW");
    set_int_prop(trig, "AIHideObj", 1);

    behavior_ai_trigger_fire_tag(w, "T_SHOW");
    printf("S|show|%d\n", target->visible);
}

/* ---- Scenario 3: AIHideObj=-1 (no change) -------------------------------- */
static void scenario_hide_noop(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_HNOOP");
    Entity *target = spawn(w, "3JIM", "TGT_HNOOP");
    target->visible = 1;
    set_str_prop(trig, "AITarget", "TGT_HNOOP");
    set_int_prop(trig, "AIHideObj", -1);

    behavior_ai_trigger_fire_tag(w, "T_HNOOP");
    printf("S|hidenoop|%d\n", target->visible);
}

/* ---- Scenario 4: AINewPos teleport --------------------------------------- */
static void scenario_newpos(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_POS");
    Entity *target = spawn(w, "3JIM", "TGT_POS");
    Entity *marker = spawn(w, "3PAT", "MARKER_POS");
    target->x = 1.0f; target->y = 2.0f; target->z = 3.0f;
    marker->x = 111.5f; marker->y = 222.25f; marker->z = -333.75f;
    set_str_prop(trig, "AITarget", "TGT_POS");
    set_str_prop(trig, "AINewPos", "MARKER_POS");

    behavior_ai_trigger_fire_tag(w, "T_POS");
    printf("S|newpos|%08x|%08x|%08x\n", f32_bits(target->x), f32_bits(target->y), f32_bits(target->z));
}

/* ---- Scenario 5: AINewRotY -------------------------------------------- */
static void scenario_rot(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_ROT");
    Entity *target = spawn(w, "3JIM", "TGT_ROT");
    target->ry = 0.0f;
    set_str_prop(trig, "AITarget", "TGT_ROT");
    set_float_prop(trig, "AINewRotY", 90.0f);

    behavior_ai_trigger_fire_tag(w, "T_ROT");
    printf("S|rot|%08x\n", f32_bits(target->ry));
}

/* ---- Scenario 6: AINewRotY = -1 (no change) ----------------------------- */
static void scenario_rot_noop(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_ROTNOOP");
    Entity *target = spawn(w, "3JIM", "TGT_ROTNOOP");
    target->ry = 1.2345f;
    set_str_prop(trig, "AITarget", "TGT_ROTNOOP");
    /* AINewRotY left unauthored -> gam_prop_f default -1.0 -> no-op branch */

    behavior_ai_trigger_fire_tag(w, "T_ROTNOOP");
    printf("S|rotnoop|%08x\n", f32_bits(target->ry));
}

/* ---- Scenario 7: AIPatrol repoint --------------------------------------- */
static void scenario_patrol(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_PAT");
    Entity *target = spawn(w, "3JIM", "TGT_PAT");
    set_str_prop(trig, "AITarget", "TGT_PAT");
    set_str_prop(trig, "AIPatrol", "NEWNODE");

    behavior_ai_trigger_fire_tag(w, "T_PAT");
    printf("S|patrol|%s|%s\n", target->patrol_point, target->start_point);
}

/* ---- Scenario 8: ToggleObject + NextTrigger dispatch --------------------- */
static void scenario_dispatch(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_DISP");
    Entity *toggle = spawn(w, "3SWI", "TOGGLE_DISP");
    Entity *next = spawn(w, "TRIG", "NEXT_DISP");
    toggle->vt = &vt_toggle_probe;
    next->vt = &vt_next_probe;
    set_str_prop(trig, "ToggleObject", "TOGGLE_DISP");
    set_str_prop(trig, "NextTrigger", "NEXT_DISP");

    g_toggle_fired = g_next_fired = 0;
    Entity *fired = behavior_ai_trigger_fire_tag(w, "T_DISP");
    printf("S|dispatch|%d|%d|%d|%d\n", g_toggle_fired, g_next_fired,
           fired ? fired->user_flag : -1, trig->user_flag);
}

/* ---- Scenario 9: trigger_count increments across repeated fires --------- */
static void scenario_count(World *w) {
    Entity *trig = spawn(w, "3AIT", "T_COUNT");
    behavior_ai_trigger_fire_tag(w, "T_COUNT");
    behavior_ai_trigger_fire_tag(w, "T_COUNT");
    behavior_ai_trigger_fire_tag(w, "T_COUNT");
    printf("S|count|%d\n", trig->user_flag);
}

/* ---- Story-progress scenarios (ApplyAITriggerStoryProgress, FUN_0040caa0)
   Each case gets its own throwaway World so re-using a real ObjectTag (e.g.
   testing "teleportexplanation" at both its gate values) never collides with
   an earlier scenario's entity of the same tag. game_flow's task-state store
   is a process-global, explicitly re-seeded per case, so this is safe. */
static void story_case(const char *label, const char *object_tag,
                       long seed_scene) {
    World sw;
    memset(&sw, 0, sizeof sw);
    spawn(&sw, "3AIT", object_tag);
    game_flow_test_seed_state("SCENE", seed_scene);
    behavior_ai_trigger_fire_tag(&sw, object_tag);
    long got = game_flow_entity_state("SCENE");
    printf("T|%s|%ld\n", label, got);
}

static void story_kitty_case(const char *label, const char *object_tag,
                             const char *kitty_tag) {
    World sw;
    memset(&sw, 0, sizeof sw);
    spawn(&sw, "3AIT", object_tag);
    game_flow_test_seed_state(kitty_tag, 0);
    behavior_ai_trigger_fire_tag(&sw, object_tag);
    long got = game_flow_entity_state(kitty_tag);
    printf("T|%s|%ld\n", label, got);
}

int main(void) {
    World w;
    memset(&w, 0, sizeof w);

    scenario_hide(&w);
    scenario_show(&w);
    scenario_hide_noop(&w);
    scenario_newpos(&w);
    scenario_rot(&w);
    scenario_rot_noop(&w);
    scenario_patrol(&w);
    scenario_dispatch(&w);
    scenario_count(&w);

    /* Story-progress: gate hit vs. gate miss (SCENE not at the expected value
       -> no write) for the same tag, plus the KITTY-family special case. */
    story_case("teleportexplanation_hit", "teleportexplanation", 0x1e);
    story_case("teleportexplanation_hit_altgate", "teleportexplanation", 0x1f);
    story_case("teleportexplanation_miss", "teleportexplanation", 0x99);
    story_case("CARLOUT_hit", "CARLOUT", 0x46);
    story_case("CARLOUT_miss", "CARLOUT", 0x99);
    story_case("GIVEKEY_hit", "GIVEKEY", 0xcd);
    story_kitty_case("KITEND1", "KITEND1", "KITTY1");

    return 0;
}
