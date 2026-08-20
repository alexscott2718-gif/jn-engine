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
 *   4. behavior_pickup_dispatch_state ActivateObject then ToggleObject, each
 *                                    handed the authored Toggle.
 *   5. behavior_pickup_award_pictures the PIC_NUMBER award.
 *   6. behavior_pickup_dispatch_next NextTrigger, after the award.
 *
 * plus behavior_pickup_spawn_gate, the load-time half. PostLoadPickupItem
 * (00436200) and ResetPickupItemVisibility (00435b20) both consult the pickup
 * state table and leave an already-collected pickup hidden, and PostLoad also
 * applies InitallyActive. Without the first, the save-global table would be
 * write-only -- re-entering a level would re-award every picture in it, and,
 * because the gate runs first, re-touching one would also drain the pictures it
 * costs. Without the second, the vending machines below do not exist.
 *
 * THE VENDING MACHINES. Twelve authored pairs across nine levels -- cmach/cand,
 * fmach/flurp, mdiam/diam, gdish/refill, piggy1/piggy2, cjar/coins2 -- are a
 * two-object exchange. The machine authors InitallyActive=1 and a
 * RequiredPicNum gate (typically 2 coins, picture 10); the product authors
 * InitallyActive=0 and awards a picture. Paying the machine fires its
 * ActivateObject/ToggleObject at the product with Toggle=1, which is
 * SetPickupItemState state 1: clear the product's collected flag and show it.
 * Collecting the product fires its ToggleObject back at the machine, re-arming
 * it. The loop is a real cycle in the authored graph and it terminates only
 * because the gate consumes -- every full pass is picture-negative (-2 coins,
 * +1 product). This is the strongest evidence that RequiredPicNum consumes
 * rather than thresholds.
 *
 * Not ported here (documented deferrals, not oversights):
 *   - PickedUpIndex: the original swaps the sprite to the replacement index on
 *     pickup state 2 instead of hiding. Native hides in both cases.
 *   - Every state slot except the pickup family's. ActivateObject/ToggleObject
 *     resolve to 3RCK, 3OMT, 3HYD, 3SWN and 3KIT targets too, and NextTrigger
 *     resolves overwhelmingly to cutscene cameras (3CAM 19, 3MCA 20). None of
 *     those classes has a recovered state body, so the dispatch reaches them
 *     and finds no slot. That is reported, not silently swallowed.
 */
#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* on_trigger carries no World, but every side effect needs one to resolve a
   tag. The spawn hook does carry it, runs for every pickup in the level, and
   re-runs on every level load, so it is the natural place to capture it. */
static World *s_world;

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

/* Make a pickup uncollectible without touching the collected-state table.
   `hide` is a separate decision from `disable` on purpose -- see the
   InitallyActive note in behavior_pickup_spawn_gate. */
static void pickup_deactivate(Entity *e, int hide) {
    e->user_flag = 1;
    e->runtime_flags &= ~(ENTITY_FLAG_SOLID | ENTITY_FLAG_TRIGGER);
    if (hide) {
        e->visible = 0;
        e->alive = 0;
    }
}

/* C3DPickupItem::SetPickupItemState (004360b0), reached through the state slot
   (vtable offset 0x428) that a C3DTriggerType's ActivateObject / ToggleObject
   dispatch calls with the authored Toggle.

   State 0 shows and enables the pickup. State 1 additionally clears its
   collected flag, which is what re-arms a vending-machine product that the
   player already took on an earlier pass. Any other value -- including the -1
   that 25 side-effect rows author -- has no recovered body, so it writes
   nothing rather than guessing. In the shipped corpus every row whose target is
   a pickup authors Toggle=1, so state 1 is the only path the data exercises. */
void behavior_pickup_set_state(Entity *e, int state) {
    if (!e) return;
    if (state != 0 && state != 1) return;

    if (state == 1)
        gamestate_pickup_clear(gamestate_level(), pickup_index_of(e));

    /* Both recovered states show the pickup. This is the only way an
       InitallyActive=0 product ever becomes visible. */
    e->pickup_inactive = 0;
    e->user_flag = 0;
    e->alive = 1;
    e->visible = 1;
    e->runtime_flags |= ENTITY_FLAG_TRIGGER;
    printf("[PICSTATE] level=%s index=%d tag='%s' -> state %d\n",
           gamestate_level(), pickup_index_of(e), e->tag, state);
}

/* PostLoadPickupItem (00436200): capture the world, then apply the two load
   time gates -- already-collected, and InitallyActive. Returns 1 when the
   pickup ends up unavailable, so the caller can skip the rest of its spawn
   work (notably the level item tally). */
int behavior_pickup_spawn_gate(Entity *e, World *w) {
    if (w) s_world = w;
    /* Already collected: hidden, and the decomp says so -- PostLoad only shows
       the canvas when the pickup-state slot is 0. */
    if (behavior_pickup_taken(e)) {
        pickup_deactivate(e, 1);
        return PICKUP_SPAWN_TAKEN;
    }
    /* InitallyActive == 0 (note the misspelling: it matches the executable
       string and the .gam schema). 28 rows author it -- the vending-machine
       products, which must not be collectible until their machine has been
       paid.

       Disabled but NOT hidden, deliberately. That an inactive pickup cannot be
       collected is well supported: the field is the "initial active state", the
       vending data only works that way, and ActivateObject names the transition
       out of it. That it is also *invisible* is not: the recovered slot-266
       body describes states 0 and 1, both of which show, and never says what
       inactive looks like. Hiding these rows was tried and changed the level1
       golden by ~0.05% of the frame, which is the project telling us we were
       guessing at a visual. So the gameplay half lands and the visual half
       waits for evidence. */
    if (gam_prop_i(e, "InitallyActive", 1) == 0) {
        pickup_deactivate(e, 0);
        /* The renderer reads this, not the authored property, so activating
           the pickup can actually reveal it. */
        e->pickup_inactive = 1;
        return PICKUP_SPAWN_INACTIVE;
    }
    e->pickup_inactive = 0;
    return PICKUP_SPAWN_AVAILABLE;
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

/* One dispatch, with enough reporting to tell the three outcomes apart:
   the tag did not resolve, it resolved but the target class has no native
   entry point for this kind, or it fired. */
static void pickup_report(const Entity *e, const char *kind, const char *tag,
                          const Entity *target, int fired) {
    printf("[PICFIRE] level=%s index=%d kind=%s tag='%s' target=%s outcome=%s\n",
           gamestate_level(), pickup_index_of(e), kind, tag,
           target ? target->type : "-",
           !target ? "unresolved" : (fired ? "fired" : "no-native-slot"));
}

/* HandlePickupCollection fires ActivateObject and then ToggleObject, each
   through the target's state slot with the authored Toggle. */
void behavior_pickup_dispatch_state(const Entity *e) {
    int toggle = gam_prop_i(e, "Toggle", -1);
    static const char *const FIELDS[2] = { "ActivateObject", "ToggleObject" };
    for (int i = 0; i < 2; i++) {
        const char *tag = gam_str(e, FIELDS[i], "none");
        if (!tag[0] || strcasecmp(tag, "none") == 0) continue;
        Entity *t = behavior_trigger_set_state_tag(s_world, tag, toggle);
        pickup_report(e, FIELDS[i], tag, t,
                      t && t->vt && t->vt->on_set_state ? 1 : 0);
    }
}

/* ...and NextTrigger after the award, as a trigger-chain forward rather than a
   state write -- the same dispatch C3DAITrigger uses. */
void behavior_pickup_dispatch_next(const Entity *e) {
    const char *tag = gam_str(e, "NextTrigger", "none");
    if (!tag[0] || strcasecmp(tag, "none") == 0) return;
    Entity *t = behavior_trigger_fire_tag(s_world, tag, g_player);
    pickup_report(e, "NextTrigger", tag, t,
                  t && t->vt && t->vt->on_trigger ? 1 : 0);
}

/* JN_TEST_PICTURES sweep. Force every picture-economy pickup row in the level
   through its own on_trigger, exactly as a player touch would, and report how
   many newly collected. Needed because two of the three award paths are
   otherwise unreachable headlessly: 3PIC needs the player to physically overlap
   it, and the 3FIS/3GIR/3DIN creatures are deliberately left non-trigger (see
   behavior_creature.c). Call it in a loop until it returns 0 -- one pass is not
   a fixpoint, because collecting a picture can unlock a gate the sweep already
   walked past, and paying a vending machine reveals a product that was inactive
   when the sweep started.

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
