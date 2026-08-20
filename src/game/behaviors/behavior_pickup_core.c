/* behavior_pickup_core.c -- the CPickupType picture-flag economy.
 *
 * The half of C3DPickupItem::HandlePickupCollection (00435ce0) that is not
 * specific to the 3PIC leaf, factored out because two native vtables need it:
 * vt_item (3PIC, 218 awarding rows) and vt_creature (3FIS/3GIR/3DIN, 19 more).
 * Both FourCC families descend from CPickupType in the original, both author
 * PIC_NUMBER / PickupIndex, and the 3PIC rows additionally author the
 * RequiredPicNum / ReqPicNumAmount / NeedMoreSound gate.
 * See docs/decomp/C3DPickupItem.md and docs/picture_flag_wiring_plan.md.
 *
 * The pieces, in the order HandlePickupCollection uses them:
 *
 *   1. behavior_pickup_gate_allows   CheckRequiredPicAndConsume, vtable 3
 *                                    slot 54 (00436830). Runs BEFORE the
 *                                    collected-state check in the original.
 *   2. behavior_pickup_taken         the DAT_004f8438[PickupIndex] test.
 *   3. behavior_pickup_mark_taken    the DAT_004f8438 write.
 *   4. behavior_pickup_award_pictures the PIC_NUMBER award.
 *
 * plus behavior_pickup_restore_taken, which is the load-time half:
 * PostLoadPickupItem (00436200) and ResetPickupItemVisibility (00435b20) both
 * consult the same table and leave an already-collected pickup hidden. Without
 * that, the save-global table would be write-only -- re-entering a level would
 * re-award every picture in it, an infinite farm that trivially defeats the
 * gate -- and, because the gate runs first, re-touching one would also drain
 * the pictures it costs.
 *
 * Not ported here (documented deferrals, not oversights):
 *   - PickedUpIndex: the original swaps the sprite to the replacement index on
 *     pickup state 2 instead of hiding. Native hides in both cases.
 *   - ActivateObject / ToggleObject / NextTrigger dispatch: phase 4 of the
 *     plan, which lifts behavior_ai_trigger.c's existing tag-dispatch into a
 *     shared helper rather than writing a second copy. The call sites in
 *     item_on_trigger are marked.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include <stdio.h>
#include <string.h>

/* The original consults its pickup-state table only for PickupIndex > 0. */
static int pickup_index_of(const Entity *e) {
    return gam_prop_i(e, "PickupIndex", -1);
}

int behavior_pickup_taken(const Entity *e) {
    return gamestate_pickup_taken(gamestate_level(), pickup_index_of(e));
}

void behavior_pickup_mark_taken(const Entity *e) {
    gamestate_pickup_mark(gamestate_level(), pickup_index_of(e));
}

int behavior_pickup_restore_taken(Entity *e) {
    if (!behavior_pickup_taken(e)) return 0;
    e->user_flag = 1;               /* per-entity once-only guard, pre-armed */
    e->visible = 0;
    e->alive = 0;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    return 1;
}

/* CheckRequiredPicAndConsume (00436830): RequiredPicNum == -1 passes; otherwise
   consume ReqPicNumAmount of that picture, or play NeedMoreSound and refuse.
   This is the first thing in this build that can refuse a pickup. */
int behavior_pickup_gate_allows(const Entity *e) {
    int required = gam_prop_i(e, "RequiredPicNum", -1);
    if (required < 0) return 1;

    int amount = gam_prop_i(e, "ReqPicNumAmount", 1);
    if (amount < 1) amount = 1;      /* ctor 004358b0 default */

    if (gamestate_pic_consume(required, amount)) {
        printf("[PICGATE] level=%s index=%d need=%d amount=%d -> ok\n",
               gamestate_level(), pickup_index_of(e), required, amount);
        return 1;
    }

    printf("[PICGATE] level=%s index=%d need=%d amount=%d have=%d -> REFUSED\n",
           gamestate_level(), pickup_index_of(e), required, amount,
           gamestate_pic_count(required));

    int sound = gam_prop_i(e, "NeedMoreSound", -1);
    if (sound >= 0) {
        const char *db = e->sound_database[0] ? e->sound_database : "soundeffects.omt";
        audio_play_db(db, sound, 0, 128);
    }
    return 0;
}

void behavior_pickup_award_pictures(const Entity *e) {
    int id = gam_prop_i(e, "PIC_NUMBER", -1);
    if (id < 0) return;
    printf("[PICAWARD] level=%s index=%d id=%d\n",
           gamestate_level(), pickup_index_of(e), id);
    gamestate_pic_award(id, 1);
}

/* JN_TEST_PICTURES sweep. Force every picture-economy pickup row in the level
   through its own on_trigger, exactly as a player touch would, and report how
   many newly collected. Needed because two of the three award paths are
   otherwise unreachable headlessly: 3PIC needs the player to physically overlap
   it, and the 3FIS/3GIR/3DIN creatures are deliberately left non-trigger (see
   behavior_creature.c). Call it in a loop until it returns 0 -- one pass is not
   a fixpoint, because collecting a picture can unlock a gate the sweep already
   walked past.

   Scoped to vt_item and vt_creature on purpose. PickupIndex is a CPickupType
   field, so 3RED (70 rows) and 3ANI (6) carry one too, but neither authors
   PIC_NUMBER and neither behavior consults the collected-state table yet; left
   in, they would re-collect on every level entry and make the sweep's
   re-entry count look like a double-award. Every one of the 237 authored
   PIC_NUMBER rows is a 3PIC, 3FIS, 3GIR or 3DIN, so this loses no award. */
int behavior_pickup_sweep_collect(World *w) {
    int collected = 0;
    if (!w) return 0;
    for (Entity *e = w->head; e; e = e->next) {
        if (e->vt != &vt_item && e->vt != &vt_creature) continue;
        if (pickup_index_of(e) < 0) continue;          /* not a pickup row */
        if (!e->vt->on_trigger) continue;
        if (e->user_flag) continue;                    /* already collected */
        e->vt->on_trigger(e, g_player);
        if (e->user_flag) collected++;
    }
    return collected;
}
