/* Headless probe for the picture-flag economy in src/game/gamestate.c.
   Exercises the picture counts, the (level, PickupIndex) collected-state table,
   the persistence rules, and the generated cold-entry pre-grant table without
   linking the engine. Every assertion the checker makes is emitted as a
   "K|<name>|<value>" line; gamestate's own logging is ignored by the parser. */
#include <stdio.h>
#include "../src/game/gamestate.h"

/* gamestate.c reaches into the campaign and visual layers for the sandbox and
   level-clear bridges. None of that is under test here. */
void game_flow_level_objective_met(void) {}
int  game_flow_campaign_active(void) { return 0; }
void game_flow_end_campaign(void) {}
int  game_flow_begin_task(const char *task_name, char *out_gam, int gam_size,
                          float out_spawn[3]) {
    (void)task_name; (void)out_gam; (void)gam_size; (void)out_spawn;
    return 0;
}
int  entity_visual_sandbox_enabled(void) { return 0; }
void entity_visual_set_sandbox(int on) { (void)on; }

static void k(const char *name, int value) {
    printf("K|%s|%d\n", name, value);
}

int main(void) {
    gamestate_init();

    /* --- picture counts: award, consume, refuse, clamp ------------------- */
    gamestate_pic_award(5, 1);
    k("award1", gamestate_pic_count(5));            /* 1 */
    gamestate_pic_award(5, 2);
    k("award2", gamestate_pic_count(5));            /* 3 */
    k("consume_ok", gamestate_pic_consume(5, 2));   /* 1 */
    k("after_consume", gamestate_pic_count(5));     /* 1 */
    /* Short: refuse *and* leave the count untouched -- a partial consume would
       silently drain the player. */
    k("consume_short", gamestate_pic_consume(5, 2));
    k("after_short", gamestate_pic_count(5));       /* still 1 */
    k("consume_exact", gamestate_pic_consume(5, 1));
    k("after_exact", gamestate_pic_count(5));       /* 0 */
    k("consume_empty", gamestate_pic_consume(5, 1));

    /* ReqPicNumAmount < 1 means 1 (ctor 004358b0 default). */
    gamestate_pic_award(7, 1);
    k("consume_zero_amount", gamestate_pic_consume(7, 0));
    k("after_zero_amount", gamestate_pic_count(7)); /* 0 */

    /* Out-of-range ids are inert in both directions. */
    gamestate_pic_award(-1, 5);
    gamestate_pic_award(PIC_ID_MAX, 5);
    gamestate_pic_award(PIC_ID_MAX + 40, 5);
    k("neg_count", gamestate_pic_count(-1));
    k("over_count", gamestate_pic_count(PIC_ID_MAX));
    k("neg_consume", gamestate_pic_consume(-1, 1));
    k("over_consume", gamestate_pic_consume(PIC_ID_MAX, 1));
    /* Zero and the top legal id must both be usable. */
    gamestate_pic_award(0, 1);
    gamestate_pic_award(PIC_ID_MAX - 1, 1);
    k("id_zero", gamestate_pic_count(0));
    k("id_top", gamestate_pic_count(PIC_ID_MAX - 1));

    /* --- collected-state table: keyed on (level, PickupIndex) ------------ */
    /* 1901 is authored in BOTH Level3 and level5a. A flat index-keyed table
       marks level5a's pickup collected because the player took Level3's; this
       is the measured collision the table exists to survive. */
    gamestate_set_level("level3");
    k("level_echo", gamestate_level()[0] == 'l');
    gamestate_pickup_mark(gamestate_level(), 1901);
    k("l3_taken", gamestate_pickup_taken(gamestate_level(), 1901));
    gamestate_set_level("level5a");
    k("l5a_not_taken", gamestate_pickup_taken(gamestate_level(), 1901));
    gamestate_pickup_mark(gamestate_level(), 1901);
    k("l5a_taken", gamestate_pickup_taken(gamestate_level(), 1901));
    k("l3_still_taken", gamestate_pickup_taken("level3", 1901));
    k("other_level_clear", gamestate_pickup_taken("level1c", 1901));
    /* The level key is case-folded: .gam stems are mixed case (Level3.gam) but
       the runtime name is lowercased. */
    k("case_folded", gamestate_pickup_taken("LEVEL3", 1901));
    /* Neighbouring index must not alias. */
    k("neighbour_clear", gamestate_pickup_taken("level3", 1902));
    /* PickupIndex <= 0 is the original's non-table pickup: never recorded. */
    gamestate_pickup_mark("level3", 0);
    gamestate_pickup_mark("level3", -1);
    k("zero_index", gamestate_pickup_taken("level3", 0));
    k("neg_index", gamestate_pickup_taken("level3", -1));
    /* An empty level key cannot record anything. */
    gamestate_pickup_mark("", 4242);
    k("empty_level", gamestate_pickup_taken("", 4242));

    /* --- persistence: a level swap keeps both stores --------------------- */
    gamestate_pic_award(23, 4);
    gamestate_reset_for_new_level();
    k("swap_keeps_count", gamestate_pic_count(23));      /* 4 */
    k("swap_keeps_taken", gamestate_pickup_taken("level3", 1901));

    /* ...and a new game clears both. */
    gamestate_new_game();
    k("newgame_clears_count", gamestate_pic_count(23));
    k("newgame_clears_taken", gamestate_pickup_taken("level3", 1901));

    /* --- generated cold-entry pre-grant table ---------------------------- */
    k("pregrant_level1c", gamestate_pregrant_pictures("level1c"));
    k("pregrant_level1c_id23", gamestate_pic_count(23));
    gamestate_new_game();
    k("pregrant_level1b", gamestate_pregrant_pictures("LEVEL1B"));
    k("pregrant_level1b_id12", gamestate_pic_count(12));
    k("pregrant_level1b_id14", gamestate_pic_count(14));
    gamestate_new_game();
    k("pregrant_level1a", gamestate_pregrant_pictures("level1a"));
    k("pregrant_level4a", gamestate_pregrant_pictures("level4a"));
    /* Levels that can supply their own requirements get nothing: pre-granting
       there would disable the gate we just built. */
    k("pregrant_level1", gamestate_pregrant_pictures("level1"));
    k("pregrant_level2", gamestate_pregrant_pictures("level2"));
    k("pregrant_unknown", gamestate_pregrant_pictures("nosuchlevel"));
    k("pregrant_empty", gamestate_pregrant_pictures(""));

    /* --- clear: the vending-machine re-arm ------------------------------- */
    /* SetPickupItemState state 1 clears the product's collected flag so the
       machine can dispense it again. Clearing must not evict the slot: these
       are open-addressed probe chains, and a hole would strand every key that
       probed past it. */
    gamestate_new_game();
    gamestate_pickup_mark("level1a", 205);   /* cand, the product */
    gamestate_pickup_mark("level1a", 206);   /* flurp, a neighbour */
    k("clear_before", gamestate_pickup_taken("level1a", 205));
    gamestate_pickup_clear("level1a", 205);
    k("clear_after", gamestate_pickup_taken("level1a", 205));
    k("clear_leaves_neighbour", gamestate_pickup_taken("level1a", 206));
    gamestate_pickup_mark("level1a", 205);
    k("remark_after_clear", gamestate_pickup_taken("level1a", 205));
    /* Clearing a key that was never marked is a no-op, not a table write. */
    gamestate_pickup_clear("level1a", 999);
    k("clear_unmarked", gamestate_pickup_taken("level1a", 999));
    /* ...and clear is level-scoped exactly like mark. */
    gamestate_pickup_mark("level5", 205);
    gamestate_pickup_clear("level1a", 205);
    k("clear_is_level_scoped", gamestate_pickup_taken("level5", 205));
    k("clear_index_zero", (gamestate_pickup_clear("level1a", 0),
                           gamestate_pickup_taken("level1a", 0)));

    /* --- the table survives a full level's worth of marks ---------------- */
    gamestate_new_game();
    for (int i = 1; i <= 400; i++)
        gamestate_pickup_mark("level4c", 3000 + i);
    k("bulk_first", gamestate_pickup_taken("level4c", 3001));
    k("bulk_last", gamestate_pickup_taken("level4c", 3400));
    k("bulk_miss", gamestate_pickup_taken("level4c", 3401));
    k("bulk_other_level", gamestate_pickup_taken("level4d", 3001));
    return 0;
}
