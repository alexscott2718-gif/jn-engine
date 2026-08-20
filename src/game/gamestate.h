#ifndef GAMESTATE_H
#define GAMESTATE_H

struct Entity;

/* Fixed-size inventory. The original C3DPickupItem path logs "ERROR: OVER max
   pickup items" beyond a hard cap; 16 is a safe mirror until Ghidra pins the
   exact size. Each owned tool/key occupies one slot. */
#define INVENTORY_MAX 16

/* --- Picture-flag economy (CPickupType / C3DPickupItem) ------------------
   The original's pickup currency. CheckRequiredPicAndConsume (vtable 3 slot
   54, 00436830) reads a per-id count, consumes ReqPicNumAmount on success and
   clears the flag when the count reaches zero; HandlePickupCollection
   (00435ce0) awards PIC_NUMBER. Counts, not bits: 31 of the 62 authored
   gating rows ask for 2 or 3 (docs/picture_flag_wiring_plan.md).

   The corpus tops out at id 72; the headroom is for jnvsjn. */
#define PIC_ID_MAX 96

/* Collected-state table -- the native stand-in for the original's global
   pickup-state array DAT_004f8438. Keyed by (level, PickupIndex), NOT by index
   alone: the 456 distinct authored indices never repeat *within* a level, but
   22 of them collide *across* levels (1901..1907 in Level3 and level5a,
   2101..2106 in Level7 and level4b, 3401..3409 in VR04 and VR05), so a flat
   index-keyed table would mark level5a's pickups collected because the player
   took Level3's. Open-addressed set; 478 authored rows is the hard ceiling, so
   1024 slots never exceed half load. */
#define PICKUP_TAKEN_SLOTS 1024
#define PICKUP_LEVEL_MAX   16

typedef struct {
    char level[PICKUP_LEVEL_MAX];  /* lowercased level key; empty = free slot */
    int  index;                    /* PickupIndex */
    int  taken;                    /* the stored state; 0 once cleared */
} PickupTakenSlot;

/* What an inventory slot is for. A GADGET is offered by the action menu and
   can be made active; a PART is a carried quest item that the menu does not
   list. The split is read off the .gam corpus, not off the names -- see the
   GADGET_GRANTS table in behavior_item.c for the two rules that produce it. */
#define INV_KIND_PART   0
#define INV_KIND_GADGET 1

typedef struct {
    char        tag[24];     /* tool/key identity, e.g. "shrinkray" */
    const char *icon_path;   /* HUD icon (static string), NULL = use sprite */
    int         sprite;      /* the pickup's own SpriteIndex, -1 if none.
                                Resolved to a PNG in hud.c: gamestate must not
                                depend on entity_visual.c. */
    int         kind;        /* INV_KIND_* */
} InventorySlot;

typedef struct {
    int items_total;
    int items_collected;
    /* Bumped by every successful collection, including a repeat purchase
       from a re-armed vending machine, which items_collected deliberately
       does not count twice. Consumers want an edge, not a total. */
    /* Pickup notify card (the original's counter-popup hook,
       FUN_004061d0). One slot deep: a newer pickup replaces the card
       rather than queueing behind it. */
    int   popup_sprite;     /* SpriteIndex of the collected item, or -1.
                               The HUD resolves it to a PNG; the behavior
                               layer must not depend on entity_visual. */
    int   popup_id;         /* PIC_NUMBER, or -1 for a scoring-only pickup */
    int   popup_count;      /* how many of that picture are now held */
    float popup_timer;      /* seconds of card left */
    /* Typed counters (Step 3). */
    int gems_collected;      /* 3GEM pickups */
    int points;              /* accumulated GAM "Points" */
    /* Player status (Step 2 HUD). health is a 0..health_max bar. */
    int health;
    int health_max;
    int level_done;
    int level_change_requested;
    /* Cached respawn pose, set when the player is bound to a level. */
    float spawn_x, spawn_y, spawn_z;
    int   spawn_set;
    /* Pending swap, drained by main loop. Empty strings = no swap. */
    char  swap_level[64];
    char  swap_spawn[32];
    /* Inventory / tools (Step 4). */
    InventorySlot inventory[INVENTORY_MAX];
    int           inventory_count;
    /* Currently selected tool (index into inventory). The "use tool" action
       (F key / web Use-Tool button) dispatches on this slot's tag. */
    int           active_tool;
    /* Picture-flag economy. Both of these are save-global: they deliberately
       outlive a level swap (see gamestate_reset_for_new_level) because the
       corpus requires it -- level1b gates on picture 14, which is only awarded
       in level2 and level2b, so the original expects the player to come back. */
    int             pic_count[PIC_ID_MAX];
    PickupTakenSlot pickup_taken[PICKUP_TAKEN_SLOTS];
    int             pickup_taken_count;
    /* Level key the pickup table is written under. Set by the main loop before
       the level's entities spawn -- game_flow_enter_level() runs *after*
       entity_bind_vtables(), so game_flow_current_level() is still the previous
       level at on_spawn time and cannot be used here. */
    char            level[PICKUP_LEVEL_MAX];
} GameState;

void gamestate_init(void);
void gamestate_item_added(void);
void gamestate_item_collected(void);
/* Raise the pickup card. sprite_index < 0 means no icon; picture_id < 0
   means the pickup awards no picture, so no count shows. */
void gamestate_notify_pickup(int sprite_index, int picture_id, int count);
/* Per-frame decay for the card. Called once per tick by the main loop. */
void gamestate_tick(float dt);
/* Typed pickups (Step 3). */
void gamestate_gem_collected(void);
void gamestate_add_points(int points);
/* Player health / hit model (Wave N2). Enemies call damage; HUD reads health. */
int  gamestate_player_health(void);
/* --nodamage: make gamestate_damage_player() a no-op. The kill plane still
   respawns, so falling out of the world behaves normally. */
void gamestate_set_invulnerable(int on);
int  gamestate_invulnerable(void);
void gamestate_damage_player(int amount);
void gamestate_heal_player(int amount);
/* 1 when health has hit 0 (death pending). The game-flow controller resolves
   it into a life loss + respawn; gamestate no longer auto-refills. */
int  gamestate_player_is_down(void);
/* Inventory (Step 4). Grant a tool slot identified by tag (deduped); icon_path
   is a static string used by the HUD (may be NULL). Returns 1 if newly added. */
int  gamestate_grant_tool(const char *tag, const char *icon_path);
/* As grant_tool, but records the pickup's sprite id and its gadget/part kind.
   grant_tool is the thin wrapper: no sprite, INV_KIND_GADGET. */
int  gamestate_grant_gadget(const char *tag, int sprite_index, int kind);
/* Menu-selectable gadgets only, in inventory order. count() is what the action
   menu lists; at(i) returns the slot or NULL. */
int  gamestate_gadget_count(void);
const InventorySlot *gamestate_gadget_at(int i);
/* The gadget the action menu has selected, or NULL when the active slot is a
   part or the inventory is empty. set() takes an ordinal into the gadget list
   (what gadget_at indexes), not an inventory index. */
const InventorySlot *gamestate_active_gadget(void);
void gamestate_set_active_gadget(int gadget_ordinal);
int  gamestate_has_tool(const char *tag);
/* Active-tool selection. tag() returns the selected slot's tag ("" if none);
   cycle() advances to the next owned slot (wraps). Exported to the web UI. */
const char *gamestate_active_tool_tag(void);
void        gamestate_cycle_active_tool(void);
/* Runtime sandbox toggle (web UI). toggle() flips + grants tools on enable and
   returns the new state; enabled() reports it. */
int  gamestate_toggle_sandbox(void);
int  gamestate_sandbox_enabled(void);
void gamestate_request_level_change(void);
/* Stash a target level path + spawn-point name for the main loop to drain. */
void gamestate_request_level_swap(const char *level, const char *start_point);
/* Reset per-level state (items, level_done, pending requests). spawn is also
   cleared -- caller will set the new one after loading. items_collected is
   intentionally preserved to act as a lifetime tally, and so are the picture
   counts and the collected-pickup table (only a new game clears those). */
void gamestate_reset_for_new_level(void);
/* Remember where to respawn the player for the current level. */
void gamestate_set_spawn(float x, float y, float z);
/* Restore the player to the cached spawn and zero its velocity. */
void gamestate_respawn_player(struct Entity *player);
const GameState *gamestate_get(void);

/* --- Picture-flag economy ------------------------------------------------
   pic_count(id)      how many of picture `id` are held (0 for out-of-range).
   pic_award(id, n)   award n (n >= 1); out-of-range ids are ignored.
   pic_consume(id, n) CheckRequiredPicAndConsume's arithmetic: consume n and
                      return 1, or leave the count untouched and return 0 when
                      short. n < 1 is normalised to 1 (the 3PIC constructor at
                      004358b0 defaults ReqPicNumAmount to 1, and the authored
                      rows that leave it -1 mean the same thing). */
int  gamestate_pic_count(int id);
void gamestate_pic_award(int id, int n);
int  gamestate_pic_consume(int id, int n);

/* Collected-state table (DAT_004f8438). All three no-op for pickup_index <= 0,
   mirroring HandlePickupCollection's `PickupIndex > 0` table gate -- the
   original treats index 0 as a special non-table pickup.

   clear() is the write in SetPickupItemState state 1 (004360b0): a vending
   machine re-arms its product by clearing the product's collected flag. It is
   deliberately not a table *deletion* -- the slot stays claimed and flips back
   to 0 -- so the open-addressed probe chain is never broken. */
int  gamestate_pickup_taken(const char *level, int pickup_index);
void gamestate_pickup_mark(const char *level, int pickup_index);
void gamestate_pickup_clear(const char *level, int pickup_index);

/* The level key the collected-state table is written under. Set this before the
   level's entities spawn. */
void        gamestate_set_level(const char *level);
const char *gamestate_level(void);

/* Clear the picture counts and the collected-pickup table. A level swap must
   not call this; only starting a new game does. */
void gamestate_new_game(void);

/* Selector pre-grant. Entering `level` cold from the level browser (or as the
   launch level) grants the pictures that level's gates require but no row in it
   awards, so a jump straight into level1a/level1b/level1c/level4a does not
   dead-end on content the player would normally arrive holding. Derived from
   the corpus by tools/gen_picture_pregrants.py; a no-op for every other level,
   and deliberately not applied to campaign play or in-level portal swaps.
   Returns the number of pictures granted. */
int gamestate_pregrant_pictures(const char *level);

#endif
