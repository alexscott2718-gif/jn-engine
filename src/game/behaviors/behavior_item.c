#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <string.h>
#include <math.h>

#define ITEM_SPIN_RATE 2.4f

static void item_on_spawn(Entity *e, World *w) {
    behavior_trigger_spawn_base(e, 30.0f, 30.0f, 30.0f);
    e->user_flag = 0;             /* 0 = uncollected, 1 = collected */
    e->pickup_counted = 0;        /* not yet in the level item tally */
    int was_trigger = (e->runtime_flags & ENTITY_FLAG_TRIGGER) != 0;
    /* PostLoadPickupItem (00436200) / ResetPickupItemVisibility (00435b20):
       a pickup whose global state slot is set is not shown again, and one
       authored InitallyActive=0 does not start available. The first is what
       makes the save-global collected table mean anything -- without it,
       re-entering a level re-awards every picture in it. The second is what
       makes the vending-machine pairs work at all. */
    int gate = behavior_pickup_spawn_gate(e, w);
    /* An InitallyActive=0 product still counts toward the level's items --
       it is gated, not absent. Only one already collected on an earlier
       visit drops out of the tally. */
    if (gate != PICKUP_SPAWN_TAKEN && was_trigger && e->visible)
        gamestate_item_added();
    if (gate != PICKUP_SPAWN_AVAILABLE) return;
}

static void item_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;
    if (!behavior_animated_update_base(e, w, dt)) return;
    /* No bob. The original does not hover its pickups at all (owner comparison
       2026-08-20), so the item stays at its authored height. That earlier
       "the hover runs N times too fast" fix was treating the symptom of a
       mechanism that should not exist; it also dragged the ShowArrow marker up
       and down, since the arrow is drawn at e->y.

       The spin stays: a 3PIC resolves to a camera-facing billboard, so its yaw
       was never visible anyway, and the few mesh-drawn pickups keep the
       behaviour they had. */
    e->ry += ITEM_SPIN_RATE * dt;
}

/* Gadget and part pickups, read off the .gam corpus (scan of every 3PIC row in
   all 35 shipped levels, 2026-08-20).
 *
 * The table this replaces was mostly invention. Four of its nine tags --
 * watergun, jetpack, burpgun, glasses -- match no ObjectTag anywhere in the
 * corpus, and its comment claimed they "gate real progression in the native
 * port today". Nothing does: the only consumer of the inventory in the whole
 * tree is behavior_player.c, and it asks only for "baseball".
 *
 * Two rules separate a carried gadget from the other ~80 named pickup rows,
 * and both come from the data rather than from naming:
 *
 *   1. A row that awards or requires a picture is an economy item, not a
 *      gadget. wrench1 and wrench2 award PIC_NUMBER 18; hydrant and water2 in
 *      the same level require 18. The wrench is spent at the hydrant, not
 *      carried in a pocket -- so modelling it as a permanent tool was wrong in
 *      both directions. passcard (PIC_NUMBER 25) is the same shape.
 *
 *   2. A row drawn on sprites.omt chunk 106 is an invisible trigger volume,
 *      not an item. Every RequiredPicNum-gated machine in the corpus -- cmach,
 *      fmach, mach, mdiam, fp, cm, cjar, book, kitty, nest1/2 -- uses 106.
 *      That is what the vending machine actually is: a blank trigger sitting
 *      over the machine's model, which is also why its ShowArrow marker floats
 *      at the machine's mid height rather than over any visible item.
 *
 * What survives both rules -- visible, and outside the picture economy -- is
 * the list below. GADGET rows are the ones the action menu offers. PART rows
 * are carried quest items that the menu does not offer, distinguished by their
 * wiring rather than their names: sewerpart fires movegoddard on pickup, and
 * scooterpart is the scooter the AMI mode table already knows about.
 *
 * Deliberately absent: applepie x3 (level1c) and vertitem (level4) are visible
 * and picture-free but carry only a PointValue and no wiring at all, so there
 * is no evidence they are carried rather than simply scored. They stay out
 * until something says otherwise.
 *
 * The icon is the pickup's own sprite. The original draws its inventory from
 * C2DInGameMenu canvas records that the hud-draw certificate is still blocked
 * on, but SpriteIndex is right there in the row, so the art is authentic even
 * though the layout is ours. Resolution happens in hud.c, not here -- this
 * file is on the C3DPickupItem oracle's fixed file list and must not grow a
 * dependency on entity_visual.c. */
typedef struct {
    const char *tag;    /* .gam ObjectTag, matched case-insensitively, exact */
    const char *tool;   /* inventory identity */
    int         kind;   /* INV_KIND_GADGET (menu-selectable) or INV_KIND_PART */
} GadgetGrant;
static const GadgetGrant GADGET_GRANTS[] = {
    /* tag            tool            kind             level    sprite */
    { "shrinkray",    "shrinkray",    INV_KIND_GADGET },  /* level1b     99 */
    { "invisibility", "invisibility", INV_KIND_GADGET },  /* level5     114 */
    { "bubblepickup", "bubble",       INV_KIND_GADGET },  /* level7      26 */
    { "scooterpart",  "scooterpart",  INV_KIND_PART   },  /* level1c    111 */
    { "sewerpart",    "sewerpart",    INV_KIND_PART   },  /* level1a    134 */
    { "foil",         "foil",         INV_KIND_PART   },  /* level1b    183 */
    { "godphone",     "godphone",     INV_KIND_PART   },  /* level1     184 */
};

/* Exact, case-insensitive. Every tag in GADGET_GRANTS occurs exactly once in
   the corpus, so substring matching buys nothing and costs precision -- it is
   what let the old "wrench" entry swallow wrench1 and wrench2. */
static int tag_equals(const char *a, const char *b) {
    if (!a[0] || !b[0]) return 0;
    while (*a && *b && (*a | 0x20) == (*b | 0x20)) { a++; b++; }
    return *a == 0 && *b == 0;
}

static void item_grant_tool(const Entity *e) {
    for (size_t i = 0; i < sizeof(GADGET_GRANTS) / sizeof(GADGET_GRANTS[0]); i++) {
        if (tag_equals(e->tag, GADGET_GRANTS[i].tag)) {
            gamestate_grant_gadget(GADGET_GRANTS[i].tool, e->sprite_index,
                                   GADGET_GRANTS[i].kind);
            return;
        }
    }
}

/* C3DPickupItem::HandlePickupCollection (00435ce0). The order below is the
   decompiled order, which is where the observable behaviour lives:
 *
 *     if toucher != player            return
 *     if !CheckRequiredPicAndConsume  return          <- gate, BEFORE the
 *     if state[PickupIndex] != 0      return             collected check
 *     update state + visibility (or PickedUpIndex sprite)
 *     fire ActivateObject / ToggleObject
 *     award PIC_NUMBER + score
 *     fire NextTrigger
 *     play pickup sound
 *
 * The gate really does run before the collected-state check in the original;
 * that is safe there because a collected pickup is hidden and disabled at load
 * (PostLoadPickupItem) and so never re-collides. item_on_spawn reproduces that,
 * and the native per-entity user_flag guard below is a second belt for the same
 * thing -- without either, the gate would re-charge a player who walks back
 * into a pickup they already took. */
static void item_on_trigger(Entity *e, Entity *by) {
    (void)by;
    if (e->user_flag) return;

    if (!behavior_pickup_gate_allows(e)) return;
    if (behavior_pickup_taken(e)) return;

    e->user_flag = 1;
    behavior_pickup_mark_taken(e);
    e->visible = 0;
    e->alive = 0;

    behavior_pickup_dispatch_state(e);

    behavior_pickup_award_pictures(e);
    /* Typed counters: gems tally separately; all pickups award their Points. */
    if (strncmp(e->type, "3GEM", 4) == 0)
        gamestate_gem_collected();
    /* PointValue is -1 on rows that author no score: the format's unset
       convention (docs/decomp/C3DPickupItem.md records the range as -1..1000).
       Testing truthiness scored minus one point for every one of them. */
    if (e->points > 0)
        gamestate_add_points(e->points);
    item_grant_tool(e);
    /* A 3PIC that awards the baseball picture (PIC_NUMBER==6, the same id
       C3DBaseballPickup sets) grants the throw ability -- the faithful,
       in-level way to obtain the baseball (level1c / level2a / Level2b). */
    if (gam_prop_i(e, "PIC_NUMBER", -1) == 6)
        gamestate_grant_tool("baseball", NULL);

    behavior_pickup_dispatch_next(e);

    {
        int snd = gam_prop_i(e, "SoundIndex", -1);
        if (snd >= 0) {
            const char *db = e->sound_database[0] ? e->sound_database : "soundeffects.omt";
            audio_play_db(db, snd, 0, 128);
        }
    }
    /* The card shows on every collection; the level tally counts each pickup
       once -- a vending machine re-arms and can be bought repeatedly, and
       without that the lifetime tally runs past the level total and trips the
       win bridge ("collected 4 / 3"). */
    {
        /* The card shows what was just taken. SpriteIndex is the pickup's
           own canvas id, so no unrecovered icon mapping is involved; the
           HUD resolves it to an image. */
        int pic = gam_prop_i(e, "PIC_NUMBER", -1);
        gamestate_notify_pickup(e->sprite_index, pic,
                                pic >= 0 ? gamestate_pic_count(pic) : 0);
    }
    if (!e->pickup_counted) {
        e->pickup_counted = 1;
        gamestate_item_collected();
    }
}

/* The state slot (vtable offset 0x428) another trigger's ActivateObject or
   ToggleObject calls with its authored Toggle. Deliberately NOT on_trigger:
   on a pickup those mean opposite things -- on_trigger collects it, state 1
   re-arms it. */
static void item_on_set_state(Entity *e, int state) {
    behavior_pickup_set_state(e, state);
}

const EntityVTable vt_item = {
    .on_spawn = item_on_spawn,
    .on_update = item_on_update,
    .on_trigger = item_on_trigger,
    .on_set_state = item_on_set_state,
    .flags = ENTITY_FLAG_TRIGGER,
};
