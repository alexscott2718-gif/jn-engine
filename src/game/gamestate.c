#include "gamestate.h"
#include "../engine/world.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

static GameState g_state;

void gamestate_init(void) {
    memset(&g_state, 0, sizeof(g_state));
    g_state.health_max = 100;
    g_state.health     = 100;
}

void gamestate_gem_collected(void) {
    g_state.gems_collected++;
}

/* --- Player health / hit model (Wave N2) -------------------------------
   Minimal damage tracking so enemies can hurt the player. health is a
   0..health_max bar already shown by the HUD. At 0 the player is "down":
   we log it and refill so the demo loop keeps running (a real death/restart
   path lands with the Wave N5 game-flow controller). */
int gamestate_player_health(void) {
    return g_state.health;
}

void gamestate_damage_player(int amount) {
    if (amount <= 0) return;
    g_state.health -= amount;
    if (g_state.health <= 0) {
        g_state.health = 0;
        printf("[HEALTH] player down (-%d) -> 0\n", amount);
        g_state.health = g_state.health_max;  /* refill until Wave N5 death flow */
        printf("[HEALTH] refilled to %d\n", g_state.health);
        return;
    }
    printf("[HEALTH] player hit (-%d) -> %d / %d\n",
           amount, g_state.health, g_state.health_max);
}

void gamestate_heal_player(int amount) {
    if (amount <= 0) return;
    g_state.health += amount;
    if (g_state.health > g_state.health_max) g_state.health = g_state.health_max;
}

void gamestate_add_points(int points) {
    g_state.points += points;
}

int gamestate_has_tool(const char *tag) {
    if (!tag || !tag[0]) return 0;
    for (int i = 0; i < g_state.inventory_count; i++)
        if (strcasecmp(g_state.inventory[i].tag, tag) == 0) return 1;
    return 0;
}

int gamestate_grant_tool(const char *tag, const char *icon_path) {
    if (!tag || !tag[0]) return 0;
    if (gamestate_has_tool(tag)) return 0;
    if (g_state.inventory_count >= INVENTORY_MAX) {
        printf("[INVENTORY] ERROR: OVER max pickup items (%d)\n", INVENTORY_MAX);
        return 0;
    }
    InventorySlot *s = &g_state.inventory[g_state.inventory_count++];
    snprintf(s->tag, sizeof(s->tag), "%s", tag);
    s->icon_path = icon_path;
    printf("[INVENTORY] +tool '%s' (slot %d)\n", s->tag, g_state.inventory_count);
    return 1;
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
