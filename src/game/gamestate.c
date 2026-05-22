#include "gamestate.h"
#include "../engine/world.h"
#include <stdio.h>
#include <string.h>

static GameState g_state;

void gamestate_init(void) {
    memset(&g_state, 0, sizeof(g_state));
}

void gamestate_item_added(void) {
    g_state.items_total++;
}

void gamestate_item_collected(void) {
    g_state.items_collected++;
    printf("[ITEM] collected %d / %d\n", g_state.items_collected, g_state.items_total);
    if (g_state.items_total > 0 && g_state.items_collected >= g_state.items_total) {
        if (!g_state.level_done) {
            g_state.level_done = 1;
            printf("=== LEVEL CLEARED ===\n");
        }
    }
}

void gamestate_request_level_change(void) {
    if (!g_state.level_change_requested) {
        g_state.level_change_requested = 1;
        printf("[GAMESTATE] level change pending\n");
    }
}

void gamestate_request_level_swap(const char *level, const char *start_point) {
    if (g_state.swap_level[0] != '\0') return;  /* one swap at a time */
    snprintf(g_state.swap_level, sizeof(g_state.swap_level), "%s", level ? level : "");
    snprintf(g_state.swap_spawn, sizeof(g_state.swap_spawn), "%s", start_point ? start_point : "");
    g_state.level_change_requested = 1;
    printf("[GAMESTATE] swap requested -> '%s' (spawn '%s')\n",
           g_state.swap_level, g_state.swap_spawn);
}

void gamestate_reset_for_new_level(void) {
    g_state.items_total = 0;
    g_state.level_done = 0;
    g_state.level_change_requested = 0;
    g_state.swap_level[0] = '\0';
    g_state.swap_spawn[0] = '\0';
    g_state.spawn_set = 0;
    /* items_collected intentionally preserved (lifetime tally). */
}

void gamestate_set_spawn(float x, float y, float z) {
    g_state.spawn_x = x;
    g_state.spawn_y = y;
    g_state.spawn_z = z;
    g_state.spawn_set = 1;
}

void gamestate_respawn_player(Entity *player) {
    if (!player || !g_state.spawn_set) return;
    player->x = g_state.spawn_x;
    player->y = g_state.spawn_y;
    player->z = g_state.spawn_z;
    player->vx = player->vy = player->vz = 0.0f;
    player->on_ground = 0;
    printf("[RESPAWN] player -> (%.1f, %.1f, %.1f)\n",
           player->x, player->y, player->z);
}

const GameState *gamestate_get(void) {
    return &g_state;
}
