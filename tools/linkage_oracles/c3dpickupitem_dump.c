/* Headless dumper for the C3DPickupItem linkage oracle
   (docs/linked_parity_plan.md L3). Loads a real shipped .gam file with the
   real, unmodified gam_load(), binds the real vt_item, and drives the real
   item_on_trigger() / behavior_pickup_* core over every placed 3PIC row,
   emitting an ordered event trace per row so the oracle can diff both the
   EFFECTS and the ORDER against C3DPickupItem::HandlePickupCollection
   (00435ce0).

   Ordering is the point. The recovered body runs CheckRequiredPicAndConsume
   BEFORE the collected-state check, fires ActivateObject/ToggleObject BEFORE
   the PIC_NUMBER award, and NextTrigger AFTER it. The engine's own
   [PICGATE] / [PICSTATE] / [PICFIRE] / [PICAWARD] lines already carry that
   interleaving in real execution order, so the oracle reads them directly
   rather than having this file paraphrase them -- one less place for the
   instrumentation to disagree with the behaviour. This dumper adds only what
   the engine cannot say for itself:

     R|<file-index>|<PickupIndex>   record separator: exact-currency probe
     B|<user_flag>|<visible>|<InitallyActive>   state straight after on_spawn
     N|<sound>                      pickup sound played (audio is stubbed)
     M|<sound>                      NeedMoreSound played on a refusal
     P|<delta>                      score awarded by this collection
     E|<user_flag>|<visible>|<alive>|<taken>   state after the call
     R2|<file-index>               record separator: one-short refusal probe
     C2|<count>                    currency left after the refusal
     R3|<file-index>               record separator: gate-before-taken probe
     C3|<count>                    currency left after re-touching a taken row

   Stubs: audio (it is the sound's *position in the sequence* that is certified
   here, not the mix), the level-gate/animation bookkeeping in behavior_base,
   g_player, and vt_creature -- the creature leaf is a different FourCC and
   outside this aspect. world_add() is reproduced byte-for-byte from
   src/engine/world.c, the same headless-without-renderer convention the other
   .gam-driven oracles use. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/behaviors/behaviors.h"
#include "../../src/game/behaviors/behavior_base.h"
#include "../../src/game/gamestate.h"
#include "../../src/engine/assets/gam_loader.h"

Entity *g_player = NULL;

/* vt_creature is only referenced by the JN_TEST_PICTURES sweep, which is not
   part of this aspect; a distinct dummy keeps the identity comparison honest. */
const EntityVTable vt_creature = { 0, 0, 0, 0, 0 };

/* --- stubs ------------------------------------------------------------- */
void behavior_animated_spawn_base(Entity *e) { (void)e; }
int behavior_animated_update_base(Entity *e, World *w, float dt) {
    (void)e; (void)w; (void)dt;
    return 1;
}
void behavior_trigger_spawn_base(Entity *e, float hx, float hy, float hz) {
    e->half_extents[0] = hx;
    e->half_extents[1] = hy;
    e->half_extents[2] = hz;
    e->runtime_flags = ENTITY_FLAG_TRIGGER;
}
int  entity_visual_sandbox_enabled(void) { return 0; }
void entity_visual_set_sandbox(int on) { (void)on; }
/* behavior_ai_trigger.c is linked for the shared tag dispatch only; these two
   belong to its own certified aspect and are stubbed here exactly as its own
   oracle's dumper stubs them. */
int behavior_goddard_apply_ai_state(Entity *e, int ai_state, int ai_speed) {
    (void)e; (void)ai_state; (void)ai_speed;
    return 0;
}
int physics_aabb_overlap(const Entity *a, const Entity *b) {
    (void)a; (void)b;
    return 0;
}

/* The sound is stubbed, but WHEN it is played is observable and certified. */
static int g_expect_need_more = 0;
int audio_play_db(const char *db, int handle, int loops, int gain) {
    (void)db; (void)loops; (void)gain;
    printf(g_expect_need_more ? "M|%d\n" : "N|%d\n", handle);
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

static void emit_state(const Entity *e, const char *level) {
    printf("E|%d|%d|%d|%d\n", e->user_flag, e->visible, e->alive,
           gamestate_pickup_taken(level, gam_prop_i(e, "PickupIndex", -1)));
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <gam-file> <level-key>\n", argv[0]);
        return 1;
    }
    World w;
    memset(&w, 0, sizeof w);
    gamestate_init();
    gamestate_set_level(argv[2]);

    int n = gam_load(&w, argv[1]);
    if (n < 0) {
        fprintf(stderr, "gam_load failed on %s\n", argv[1]);
        return 1;
    }

    /* w.head is push-front order; reverse to on-disk object order. */
    Entity **arr = malloc(sizeof(Entity *) * (size_t)(w.count > 0 ? w.count : 1));
    int i = 0;
    for (Entity *e = w.head; e; e = e->next) arr[i++] = e;

    /* Bind + spawn every 3PIC through the real vtable, so PostLoadPickupItem's
       load gates (already-collected, InitallyActive) are the real ones. */
    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        if (strncmp(e->type, "3PIC", 4) != 0) continue;
        e->vt = &vt_item;
        if (vt_item.on_spawn) vt_item.on_spawn(e, &w);
    }

    /* Snapshot every row's post-spawn state BEFORE any probe runs. The probes
       have real cross-row side effects -- a vending machine's state dispatch
       re-arms its product -- so reading this inside the probe loop would report
       a later row's spawn state after an earlier row had already touched it. */
    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        int file_idx = w.count - 1 - k;
        if (strncmp(e->type, "3PIC", 4) != 0) continue;
        printf("R|%d|%d\n", file_idx, gam_prop_i(e, "PickupIndex", -1));
        printf("B|%d|%d|%d\n", e->user_flag, e->visible,
               gam_prop_i(e, "InitallyActive", 1));
    }

    for (int k = w.count - 1; k >= 0; k--) {
        Entity *e = arr[k];
        int file_idx = w.count - 1 - k;
        if (strncmp(e->type, "3PIC", 4) != 0) continue;

        int index = gam_prop_i(e, "PickupIndex", -1);
        int need = gam_prop_i(e, "RequiredPicNum", -1);
        int amount = gam_prop_i(e, "ReqPicNumAmount", 1);
        if (amount < 1) amount = 1;

        printf("R1|%d\n", file_idx);

        /* Seed exactly what this row asks for, so the gate must pass, then
           drive the real collection path. */
        gamestate_new_game();
        gamestate_set_level(argv[2]);
        if (need >= 0) gamestate_pic_award(need, amount);
        int before_points = gamestate_get()->points;

        e->user_flag = 0;
        e->visible = 1;
        e->alive = 1;
        e->runtime_flags |= ENTITY_FLAG_TRIGGER;

        g_expect_need_more = 0;
        if (vt_item.on_trigger) vt_item.on_trigger(e, g_player);
        printf("P|%d\n", gamestate_get()->points - before_points);
        emit_state(e, argv[2]);

        /* Starve the gate and re-drive a fresh copy: the refusal path must
           return without collecting, and must play NeedMoreSound. */
        if (need >= 0) {
            gamestate_new_game();
            gamestate_set_level(argv[2]);
            if (amount > 1) gamestate_pic_award(need, amount - 1);  /* one short */
            e->user_flag = 0;
            e->visible = 1;
            e->alive = 1;
            e->runtime_flags |= ENTITY_FLAG_TRIGGER;
            g_expect_need_more = 1;
            printf("R2|%d\n", file_idx);
            if (vt_item.on_trigger) vt_item.on_trigger(e, g_player);
            g_expect_need_more = 0;
            printf("C2|%d\n", gamestate_pic_count(need));
            emit_state(e, argv[2]);
        }

        /* HandlePickupCollection runs the gate BEFORE the collected-state
           check. Prove it: mark this row collected, hand it enough currency,
           touch it again, and see whether the currency was taken. */
        if (need >= 0 && index > 0) {
            gamestate_new_game();
            gamestate_set_level(argv[2]);
            gamestate_pic_award(need, amount);
            gamestate_pickup_mark(argv[2], index);
            e->user_flag = 0;
            e->visible = 1;
            e->alive = 1;
            e->runtime_flags |= ENTITY_FLAG_TRIGGER;
            printf("R3|%d\n", file_idx);
            if (vt_item.on_trigger) vt_item.on_trigger(e, g_player);
            printf("C3|%d\n", gamestate_pic_count(need));
        }
    }

    free(arr);
    return 0;
}
