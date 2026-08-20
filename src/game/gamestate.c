#include "gamestate.h"
#include "game_flow.h"
#include "entity_visual.h"
#include "picture_pregrants_generated.h"
#include "../engine/world.h"
#include <ctype.h>
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

static int g_invulnerable = 0;

void gamestate_set_invulnerable(int on) { g_invulnerable = on ? 1 : 0; }
int  gamestate_invulnerable(void)       { return g_invulnerable; }

void gamestate_damage_player(int amount) {
    if (amount <= 0) return;
    if (g_invulnerable) return;   /* --nodamage */
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

static int inventory_add(const char *tag, const char *icon_path,
                         int sprite_index, int kind) {
    if (!tag || !tag[0]) return 0;
    if (gamestate_has_tool(tag)) return 0;
    if (g_state.inventory_count >= INVENTORY_MAX) {
        printf("[INVENTORY] ERROR: OVER max pickup items (%d)\n", INVENTORY_MAX);
        return 0;
    }
    InventorySlot *s = &g_state.inventory[g_state.inventory_count++];
    snprintf(s->tag, sizeof(s->tag), "%s", tag);
    s->icon_path = icon_path;
    s->sprite    = sprite_index;
    s->kind      = kind;
    printf("[INVENTORY] +%s '%s' (slot %d)\n",
           kind == INV_KIND_GADGET ? "gadget" : "part",
           s->tag, g_state.inventory_count);
    return 1;
}

int gamestate_grant_tool(const char *tag, const char *icon_path) {
    return inventory_add(tag, icon_path, -1, INV_KIND_GADGET);
}

int gamestate_grant_gadget(const char *tag, int sprite_index, int kind) {
    return inventory_add(tag, NULL, sprite_index, kind);
}

int gamestate_gadget_count(void) {
    int n = 0;
    for (int i = 0; i < g_state.inventory_count; i++)
        if (g_state.inventory[i].kind == INV_KIND_GADGET) n++;
    return n;
}

const InventorySlot *gamestate_gadget_at(int i) {
    if (i < 0) return NULL;
    for (int k = 0; k < g_state.inventory_count; k++) {
        if (g_state.inventory[k].kind != INV_KIND_GADGET) continue;
        if (i-- == 0) return &g_state.inventory[k];
    }
    return NULL;
}

const InventorySlot *gamestate_active_gadget(void) {
    if (g_state.active_tool < 0 || g_state.active_tool >= g_state.inventory_count)
        return NULL;
    const InventorySlot *s = &g_state.inventory[g_state.active_tool];
    return s->kind == INV_KIND_GADGET ? s : NULL;
}

void gamestate_set_active_gadget(int gadget_ordinal) {
    if (gadget_ordinal < 0) return;
    for (int k = 0; k < g_state.inventory_count; k++) {
        if (g_state.inventory[k].kind != INV_KIND_GADGET) continue;
        if (gadget_ordinal-- == 0) { g_state.active_tool = k; return; }
    }
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

/* --- Picture-flag economy (CPickupType) ---------------------------------
   The count store behind CheckRequiredPicAndConsume (00436830) and the
   PIC_NUMBER award in HandlePickupCollection (00435ce0). The original keeps a
   flag *and* a count per picture id and clears the flag when the count reaches
   zero; a single count carries both -- count > 0 is the flag. */
int gamestate_pic_count(int id) {
    if (id < 0 || id >= PIC_ID_MAX) return 0;
    return g_state.pic_count[id];
}

void gamestate_pic_award(int id, int n) {
    if (id < 0 || id >= PIC_ID_MAX) return;
    if (n < 1) return;
    g_state.pic_count[id] += n;
    printf("[PICTURE] award id=%d +%d -> %d\n", id, n, g_state.pic_count[id]);
}

int gamestate_pic_consume(int id, int n) {
    if (id < 0 || id >= PIC_ID_MAX) return 0;
    /* ReqPicNumAmount defaults to 1 in the 3PIC constructor (004358b0); the
       authored rows that leave it at -1 mean the same thing. */
    if (n < 1) n = 1;
    if (g_state.pic_count[id] < n) return 0;
    g_state.pic_count[id] -= n;
    printf("[PICTURE] consume id=%d -%d -> %d\n", id, n, g_state.pic_count[id]);
    return 1;
}

/* --- Collected-state table (DAT_004f8438) -------------------------------- */
static void pickup_level_key(const char *level, char *out, size_t out_size) {
    size_t i = 0;
    if (level)
        for (; level[i] && i + 1 < out_size; i++)
            out[i] = (char)tolower((unsigned char)level[i]);
    out[i] = '\0';
}

static unsigned pickup_hash(const char *key, int index) {
    unsigned h = 2166136261u;
    for (const char *p = key; *p; p++) {
        h ^= (unsigned char)*p;
        h *= 16777619u;
    }
    h ^= (unsigned)index;
    h *= 16777619u;
    return h;
}

/* Find the slot for (level, index). With create=0 a miss returns NULL; with
   create=1 a miss claims the first free slot (NULL only if the table is full,
   which 478 authored rows in 1024 slots cannot reach). */
static PickupTakenSlot *pickup_slot(const char *level, int index, int create) {
    char key[PICKUP_LEVEL_MAX];
    pickup_level_key(level, key, sizeof key);
    if (!key[0]) return NULL;

    unsigned h = pickup_hash(key, index);
    for (unsigned probe = 0; probe < PICKUP_TAKEN_SLOTS; probe++) {
        PickupTakenSlot *s =
            &g_state.pickup_taken[(h + probe) & (PICKUP_TAKEN_SLOTS - 1)];
        if (!s->level[0]) {
            if (!create) return NULL;
            snprintf(s->level, sizeof(s->level), "%s", key);
            s->index = index;
            g_state.pickup_taken_count++;
            return s;
        }
        if (s->index == index && strcmp(s->level, key) == 0) return s;
    }
    return NULL;
}

int gamestate_pickup_taken(const char *level, int pickup_index) {
    /* HandlePickupCollection only consults the table for PickupIndex > 0;
       index 0 is its special non-table pickup. */
    if (pickup_index <= 0) return 0;
    PickupTakenSlot *s = pickup_slot(level, pickup_index, 0);
    return s && s->taken;
}

void gamestate_pickup_mark(const char *level, int pickup_index) {
    if (pickup_index <= 0) return;
    PickupTakenSlot *s = pickup_slot(level, pickup_index, 1);
    if (!s) {
        printf("[PICKUP] ERROR: collected-state table full (%d slots)\n",
               PICKUP_TAKEN_SLOTS);
        return;
    }
    s->taken = 1;
}

void gamestate_pickup_clear(const char *level, int pickup_index) {
    if (pickup_index <= 0) return;
    /* create=0: nothing to clear if it was never marked. */
    PickupTakenSlot *s = pickup_slot(level, pickup_index, 0);
    if (s) s->taken = 0;
}

void gamestate_set_level(const char *level) {
    pickup_level_key(level, g_state.level, sizeof g_state.level);
}

const char *gamestate_level(void) {
    return g_state.level;
}

void gamestate_new_game(void) {
    memset(g_state.pic_count, 0, sizeof g_state.pic_count);
    memset(g_state.pickup_taken, 0, sizeof g_state.pickup_taken);
    g_state.pickup_taken_count = 0;
    printf("[PICTURE] new game: picture counts and collected pickups cleared\n");
}

int gamestate_pregrant_pictures(const char *level) {
    char key[PICKUP_LEVEL_MAX];
    int granted = 0;
    pickup_level_key(level, key, sizeof key);
    if (!key[0]) return 0;
    for (size_t i = 0; i < PICTURE_PREGRANT_COUNT; i++) {
        if (strcmp(PICTURE_PREGRANTS[i].level, key) != 0) continue;
        gamestate_pic_award(PICTURE_PREGRANTS[i].id, PICTURE_PREGRANTS[i].count);
        granted += PICTURE_PREGRANTS[i].count;
    }
    if (granted)
        printf("[PICTURE] pre-granted %d picture(s) for a cold jump into '%s'\n",
               granted, key);
    return granted;
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

/* Campaign toggle for the web build (the CLI uses --newgame). ON: begin the
   NewGame task (turns on campaign mode + seeds the SCENE store) and queue a
   runtime swap to its start level (level1b); the main loop's swap drain loads
   it. OFF: end the campaign and return to free-roam level1. Returns the new
   campaign-on state. */
EMSCRIPTEN_KEEPALIVE
int gamestate_toggle_campaign_web(void) {
    if (game_flow_campaign_active()) {
        game_flow_end_campaign();
        gamestate_request_level_swap("level1", "");
        return 0;
    }
    char ng[64] = {0};
    gamestate_new_game();   /* a campaign start is a new game */
    if (game_flow_begin_task("NewGame", ng, sizeof ng, NULL) && ng[0])
        gamestate_request_level_swap(ng, "");
    else
        gamestate_request_level_swap("level1b", "");  /* baked NewGame start */
    printf("[CAMPAIGN] ON (web toggle)\n");
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int gamestate_campaign_active_web(void) { return game_flow_campaign_active(); }

void gamestate_item_added(void) {
    g_state.items_total++;
}

#define PICKUP_POPUP_SECONDS 2.5f

void gamestate_notify_pickup(int sprite_index, int picture_id, int count) {
    g_state.popup_sprite = sprite_index;
    g_state.popup_id = picture_id;
    g_state.popup_count = count;
    g_state.popup_timer = PICKUP_POPUP_SECONDS;
}

void gamestate_tick(float dt) {
    if (g_state.popup_timer > 0.0f) {
        g_state.popup_timer -= dt;
        if (g_state.popup_timer < 0.0f) g_state.popup_timer = 0.0f;
    }
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
    /* items_collected intentionally preserved (lifetime tally).
       pic_count and pickup_taken are intentionally preserved too: the picture
       flags are save-global and outlive a level. level1b gates on picture 14,
       which is only awarded in level2/level2b, so clearing here would make the
       shipped corpus uncompletable. Only gamestate_new_game() clears them. */
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
