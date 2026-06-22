#include "game_flow.h"
#include "gamestate.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* CJimmyGame::InitGame seeds these (docs/decomp/CJimmyGame.md §Per-Frame):
   mission_counter_a/b = 5 (lives/retries class default), mission_value = 100. */
#define JIMMYGAME_DEFAULT_LIVES        5
#define JIMMYGAME_DEFAULT_MISSION_VAL  100

typedef struct {
    int      campaign_active;
    int      mission_active;
    int      lives;
    int      mission_value;
    int      level_won;
    int      progress_index;     /* campaign ordinal of the current level */
    char     level_name[32];     /* normalized current level (e.g. "level1b") */
    TaskList task;
    int      task_loaded;
} GameFlow;

static GameFlow g_flow;

/* ---- Level table (CLevel*Game / CLevelVR0N collapse) --------------------
   The 40 level controllers are 0-owned-method leaves of CJimmyGame that only
   bind a .gam; their only per-level datum is identity + campaign position. This
   single table replaces hand-porting 40 near-identical controllers. Order is a
   best-effort campaign sequence (major world, then sub-level letter); the VR
   challenge levels are flagged and sit after the story levels. The exact
   in-game unlock order beyond this is a deferred open question. */
typedef struct { const char *name; int is_vr; } LevelTableEntry;

static const LevelTableEntry g_level_table[] = {
    { "level1",  0 }, { "level1a", 0 }, { "level1b", 0 }, { "level1c", 0 },
    { "level1d", 0 }, { "level1e", 0 }, { "level1f", 0 },
    { "level2",  0 }, { "level2a", 0 }, { "level2b", 0 },
    { "level3",  0 }, { "level3a", 0 }, { "level3b", 0 }, { "level3c", 0 },
    { "level3d", 0 }, { "level3e", 0 },
    { "level4",  0 }, { "level4a", 0 }, { "level4b", 0 }, { "level4c", 0 },
    { "level4d", 0 },
    { "level5",  0 }, { "level5a", 0 }, { "level5b", 0 },
    { "level6",  0 }, { "level6a", 0 },
    { "level7",  0 },
    { "vr01", 1 }, { "vr02", 1 }, { "vr03", 1 }, { "vr04", 1 },
    { "vr05", 1 }, { "vr06", 1 }, { "vr07", 1 }, { "vr08", 1 },
};
static const int g_level_table_count =
    (int)(sizeof(g_level_table) / sizeof(g_level_table[0]));

int game_flow_level_index(const char *level_name) {
    if (!level_name) return -1;
    for (int i = 0; i < g_level_table_count; i++)
        if (strcasecmp(g_level_table[i].name, level_name) == 0)
            return i;
    return -1;
}

int game_flow_level_is_vr(const char *level_name) {
    int idx = game_flow_level_index(level_name);
    return idx >= 0 ? g_level_table[idx].is_vr : 0;
}

/* ---- Controller lifecycle ----------------------------------------------- */

void game_flow_init_game(void) {
    /* Mirror CJimmyGame::InitGame's mission seed. */
    g_flow.mission_active = 1;
    g_flow.lives          = JIMMYGAME_DEFAULT_LIVES;
    g_flow.mission_value  = JIMMYGAME_DEFAULT_MISSION_VAL;
    g_flow.level_won      = 0;
    printf("[JIMMYGAME] InitGame: lives=%d mission_value=%d active=1\n",
           g_flow.lives, g_flow.mission_value);
}

void game_flow_enter_level(const char *level_name) {
    if (!level_name) level_name = "";
    snprintf(g_flow.level_name, sizeof(g_flow.level_name), "%s", level_name);
    g_flow.level_won = 0;
    int idx = game_flow_level_index(level_name);
    g_flow.progress_index = idx >= 0 ? idx : g_flow.progress_index;
    if (!g_flow.mission_active)
        game_flow_init_game();   /* first entry seeds the mission counters */
    printf("[JIMMYGAME] enter level '%s' (campaign idx %d, vr=%d)\n",
           level_name, idx, game_flow_level_is_vr(level_name));
}

int game_flow_begin_task(const char *task_name, char *out_gam, int gam_size,
                         float out_spawn[3]) {
    if (!task_name) return 0;
    if (!task_load(task_name, &g_flow.task)) {
        fprintf(stderr, "[JIMMYGAME] task '%s' not found\n", task_name);
        return 0;
    }
    g_flow.task_loaded     = 1;
    g_flow.campaign_active = 1;
    game_flow_init_game();
    printf("[JIMMYGAME] begin task '%s' -> %s (spawn %.1f,%.1f,%.1f, %d states)\n",
           task_name, g_flow.task.gam_file,
           g_flow.task.spawn[0], g_flow.task.spawn[1], g_flow.task.spawn[2],
           g_flow.task.entity_count);
    if (out_gam && gam_size > 0)
        snprintf(out_gam, (size_t)gam_size, "%s", g_flow.task.gam_file);
    if (out_spawn) {
        out_spawn[0] = g_flow.task.spawn[0];
        out_spawn[1] = g_flow.task.spawn[1];
        out_spawn[2] = g_flow.task.spawn[2];
    }
    return 1;
}

const TaskList *game_flow_task(void) {
    return g_flow.task_loaded ? &g_flow.task : NULL;
}

long game_flow_entity_state(const char *tag) {
    return g_flow.task_loaded ? task_entity_state(&g_flow.task, tag) : -1;
}

int game_flow_campaign_active(void) { return g_flow.campaign_active; }
int game_flow_progress_level(void)  { return g_flow.progress_index; }
int game_flow_lives(void)           { return g_flow.lives; }
int game_flow_mission_value(void)   { return g_flow.mission_value; }

/* ---- Death / restart (C2DInGameMenu death path) ------------------------- */

int game_flow_player_died(void) {
    /* Free-roam (`--level`) launches aren't a campaign: respawn without
       spending a campaign life, so the audit/screenshot harnesses are inert. */
    if (!g_flow.campaign_active) return 1;
    if (g_flow.lives > 0) g_flow.lives--;
    if (g_flow.lives > 0) {
        printf("[JIMMYGAME] player died -> %d lives left; RestartLevel\n",
               g_flow.lives);
        return 1;   /* respawn at the level's start (RestartLevel.tsk) */
    }
    /* Game over. The original returns to the menu / reloads; for the demo loop
       we reset the mission and respawn so play continues. */
    printf("[JIMMYGAME] GAME OVER (0 lives) -> reset mission, respawn\n");
    game_flow_init_game();
    return 1;
}

/* ---- Win condition ------------------------------------------------------ */

void game_flow_level_objective_met(void) {
    if (g_flow.level_won) return;
    g_flow.level_won = 1;
    printf("[JIMMYGAME] objective met -> level '%s' WON\n", g_flow.level_name);
}

int game_flow_level_won(void) { return g_flow.level_won; }
