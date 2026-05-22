#ifndef GAMESTATE_H
#define GAMESTATE_H

struct Entity;

typedef struct {
    int items_total;
    int items_collected;
    int level_done;
    int level_change_requested;
    /* Cached respawn pose, set when the player is bound to a level. */
    float spawn_x, spawn_y, spawn_z;
    int   spawn_set;
    /* Pending swap, drained by main loop. Empty strings = no swap. */
    char  swap_level[64];
    char  swap_spawn[32];
} GameState;

void gamestate_init(void);
void gamestate_item_added(void);
void gamestate_item_collected(void);
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
