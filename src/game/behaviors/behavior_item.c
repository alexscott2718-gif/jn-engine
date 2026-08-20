#include "behaviors.h"
#include "behavior_base.h"
#include "../gamestate.h"
#include "../../engine/audio.h"
#include <stddef.h>
#include <string.h>
#include <math.h>

#define ITEM_BOB_AMP   12.0f
#define ITEM_BOB_FREQ  1.2f
#define ITEM_SPIN_RATE 2.4f

static void item_on_spawn(Entity *e, World *w) {
    (void)w;
    behavior_trigger_spawn_base(e, 30.0f, 30.0f, 30.0f);
    e->user_flag = 0;             /* 0 = uncollected, 1 = collected */
    e->user_float = e->y;         /* base y for bob */
    /* PostLoadPickupItem (00436200) / ResetPickupItemVisibility (00435b20):
       a pickup whose global state slot is set is not shown again. This is what
       makes the save-global collected table mean anything -- without it,
       re-entering a level re-awards every picture in it. */
    if (behavior_pickup_restore_taken(e)) return;
    if (e->visible && (e->runtime_flags & ENTITY_FLAG_TRIGGER))
        gamestate_item_added();
}

static void item_on_update(Entity *e, World *w, float dt) {
    (void)w;
    if (!e->alive) return;
    if (!behavior_animated_update_base(e, w, dt)) return;
    static float t = 0.0f;
    t += dt;
    e->y  = e->user_float + ITEM_BOB_AMP * sinf(6.28318f * ITEM_BOB_FREQ * t + e->x * 0.01f);
    e->ry += ITEM_SPIN_RATE * dt;
}

/* Tool-granting pickups. Matched as a case-insensitive substring of the GAM
   ObjectTag so variants (pickupwatergun / WATERGUN1 / activatewatergun) all map
   to one inventory tool. Icons are authentic sprites pulled from sprites.omt.
   Kept alongside the picture economy, not replaced by it: the watergun,
   jetpack and keys gate real progression in the native port today, so the
   picture path is additive. */
typedef struct { const char *tag; const char *tool; const char *icon; } ToolGrant;
static const ToolGrant TOOL_GRANTS[] = {
    { "watergun", "watergun", "assets/hud/tool_watergun.png" },
    { "glasses",  "glasses",  "assets/hud/tool_glasses.png"  },
    { "jetpack",  "jetpack",  "assets/hud/tool_jetpack.png"  },
    { "wrench",   "wrench",   "assets/hud/tool_wrench.png"   },
    { "megaburp", "burpgun",  "assets/hud/tool_burpgun.png"  },
    { "burpgun",  "burpgun",  "assets/hud/tool_burpgun.png"  },
    { "tools04",  "tools",    "assets/hud/tool_wrench.png"   },
    { "fowlkey",  "fowlkey",  "assets/hud/tool_key.png"      },
    { "key",      "key",      "assets/hud/tool_key.png"      },
};

/* Case-insensitive substring search (needle in haystack). */
static int tag_contains(const char *haystack, const char *needle) {
    if (!haystack[0] || !needle[0]) return 0;
    size_t nl = strlen(needle);
    for (const char *p = haystack; *p; p++) {
        size_t i = 0;
        while (i < nl && p[i] &&
               (p[i] | 0x20) == (needle[i] | 0x20)) i++;
        if (i == nl) return 1;
    }
    return 0;
}

static void item_grant_tool(const Entity *e) {
    for (size_t i = 0; i < sizeof(TOOL_GRANTS) / sizeof(TOOL_GRANTS[0]); i++) {
        if (tag_contains(e->tag, TOOL_GRANTS[i].tag)) {
            gamestate_grant_tool(TOOL_GRANTS[i].tool, TOOL_GRANTS[i].icon);
            return;  /* first match wins */
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

    /* PHASE 4 goes here: ActivateObject then ToggleObject, via the shared tag
       dispatch lifted out of behavior_ai_trigger.c:182-190. */

    behavior_pickup_award_pictures(e);
    /* Typed counters: gems tally separately. The score award below is
       currently dead: gam_loader.c maps a property named "Points", which no
       row in the corpus authors -- the authored score field is PointValue
       (383 rows, decomp offset 0x620). Left as-is on purpose; fixing it is a
       loader/scoring change, not part of this economy. See the plan, 8.5. */
    if (strncmp(e->type, "3GEM", 4) == 0)
        gamestate_gem_collected();
    if (e->points)
        gamestate_add_points(e->points);
    item_grant_tool(e);
    /* A 3PIC that awards the baseball picture (PIC_NUMBER==6, the same id
       C3DBaseballPickup sets) grants the throw ability -- the faithful,
       in-level way to obtain the baseball (level1c / level2a / Level2b). */
    if (gam_prop_i(e, "PIC_NUMBER", -1) == 6)
        gamestate_grant_tool("baseball", NULL);

    /* PHASE 4 goes here: NextTrigger dispatch. */

    {
        int snd = gam_prop_i(e, "SoundIndex", -1);
        if (snd >= 0) {
            const char *db = e->sound_database[0] ? e->sound_database : "soundeffects.omt";
            audio_play_db(db, snd, 0, 128);
        }
    }
    gamestate_item_collected();
}

const EntityVTable vt_item = {
    .on_spawn = item_on_spawn,
    .on_update = item_on_update,
    .on_trigger = item_on_trigger,
    .flags = ENTITY_FLAG_TRIGGER,
};
