#include "gamestate.h"
#include "game_flow.h"
#include "entity_visual.h"
#include "../engine/world.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

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
        /* Death is resolved by the game-flow controller (lives/restart), which
           the main loop drives off gamestate_player_is_down(). */
        return;
    }
    printf("[HEALTH] player hit (-%d) -> %d / %d\n",
           amount, g_state.health, g_state.health_max);
}

int gamestate_player_is_down(void) {
    return g_state.health <= 0;
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

const char *gamestate_active_tool_tag(void) {
    if (g_state.inventory_count <= 0) return "";
    if (g_state.active_tool < 0 || g_state.active_tool >= g_state.inventory_count)
        g_state.active_tool = 0;
    return g_state.inventory[g_state.active_tool].tag;
}

void gamestate_cycle_active_tool(void) {
    if (g_state.inventory_count <= 0) { g_state.active_tool = 0; return; }
    g_state.active_tool = (g_state.active_tool + 1) % g_state.inventory_count;
    printf("[INVENTORY] active tool -> '%s' (slot %d)\n",
           g_state.inventory[g_state.active_tool].tag, g_state.active_tool + 1);
}

/* --- Sandbox / verification mode (web Use-Tool/vehicle/sandbox UI) -------
   Runtime toggle of the sandbox flag (entity_visual draws the otherwise-hidden
   rideable rocket when on; main.c spawns one where the level has none). Turning
   it on also grants the combat tools so the active-tool path has something to
   fire. Tools are left granted when toggled off (harmless for a QA aid). */
EMSCRIPTEN_KEEPALIVE
int gamestate_toggle_sandbox(void) {
    int on = !entity_visual_sandbox_enabled();
    entity_visual_set_sandbox(on);
    if (on) {
        gamestate_grant_tool("baseball", NULL);
        gamestate_grant_tool("bubble", NULL);
        gamestate_grant_tool("helmet", NULL);
    }
    printf("[SANDBOX] %s (runtime toggle)\n", on ? "ON" : "OFF");
    return on;
}

EMSCRIPTEN_KEEPALIVE
int gamestate_sandbox_enabled(void) {
    return entity_visual_sandbox_enabled();
}

/* Thin EMSCRIPTEN_KEEPALIVE wrappers so the web UI can cycle/read the active
   tool. (The plain functions above are also used by native code.) */
EMSCRIPTEN_KEEPALIVE
void gamestate_cycle_active_tool_web(void) { gamestate_cycle_active_tool(); }

EMSCRIPTEN_KEEPALIVE
const char *gamestate_active_tool_tag_web(void) { return gamestate_active_tool_tag(); }

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
            /* Feed the CJimmyGame win-condition layer. */
            game_flow_level_objective_met();
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
