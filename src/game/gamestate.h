#ifndef GAMESTATE_H
#define GAMESTATE_H

struct Entity;

/* Fixed-size inventory. The original C3DPickupItem path logs "ERROR: OVER max
   pickup items" beyond a hard cap; 16 is a safe mirror until Ghidra pins the
   exact size. Each owned tool/key occupies one slot. */
#define INVENTORY_MAX 16

typedef struct {
    char        tag[24];     /* tool/key identity, e.g. "watergun1" */
    const char *icon_path;   /* HUD icon (static string), NULL = generic */
} InventorySlot;

typedef struct {
    int items_total;
    int items_collected;
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
} GameState;

void gamestate_init(void);
void gamestate_item_added(void);
void gamestate_item_collected(void);
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
   cleared — caller will set the new one after loading. items_collected is
   intentionally preserved to act as a lifetime tally. */
void gamestate_reset_for_new_level(void);
/* Remember where to respawn the player for the current level. */
void gamestate_set_spawn(float x, float y, float z);
/* Restore the player to the cached spawn and zero its velocity. */
void gamestate_respawn_player(struct Entity *player);
const GameState *gamestate_get(void);

#endif
