#ifndef GAME_FLOW_H
#define GAME_FLOW_H

#include "task_loader.h"

/* Native port of CJimmyGame (docs/decomp/CJimmyGame.md) — the per-level
   game-mode controller and parent of every CLevel*Game / CLevelVR0N leaf and
   CMainMenu. The 40 CLevel*Game leaves are 0-owned-method subclasses that only
   bind a specific level's .gam, so the controller behavior lives here once and
   the per-level differences collapse into a data table (see game_flow.c).

   InitGame seeds the mission layer: mission_counter (lives) = 5, mission_value
   = 100, mission_active = 1, and owns a CTaskList (the .tsk) that supplies the
   spawn point and the initial mission entity-state table. This module replaces
   the ad-hoc level-clear logic in gamestate.c with a real
   objective/win-condition/lives layer, and the death/restart path documented
   in C2DInGameMenu (RestartLevel.tsk). */

/* Whether a real campaign/task is driving the session. Direct `--level X`
   launches (the audit + screenshot harnesses) run with this OFF, so the
   death/lives/restart flow and any campaign side-effects stay inert and
   rendering is unchanged from before Wave N5. A NewGame / menu entry turns it
   ON.

   NB: the RequiredLevel/ExactLevel visibility gate (behavior_base.c) stays
   env-driven (JN_PROGRESS_LEVEL) and is deliberately NOT auto-enabled here:
   the original's progress counter runs to ~550 and its exact per-objective
   increments aren't ground-truthed yet, so feeding it a small campaign ordinal
   would wrongly hide gated objects. Mapping campaign progress -> gate
   thresholds is a deferred open question (needs save-game ground truth). */
int  game_flow_campaign_active(void);

/* CJimmyGame::InitGame — seed the mission state for a fresh game mode. */
void game_flow_init_game(void);

/* Called once per level (re)load with the level's normalized name (e.g.
   "level1b"). Resolves the level's campaign index and, when in campaign mode,
   updates the progress level used by the RequiredLevel/ExactLevel gates. */
void game_flow_enter_level(const char *level_name);

/* Begin a campaign from a named task (e.g. "NewGame"): loads the .tsk via
   CTaskList, marks campaign mode active, and returns the start level .gam +
   spawn through the out-params (spawn may be NULL). Returns 1 on success. */
int  game_flow_begin_task(const char *task_name, char *out_gam, int gam_size,
                          float out_spawn[3]);

/* End the campaign: campaign mode OFF, task store unloaded (SCENE writes go
   inert again). Used by the web "Campaign" toggle to return to free-roam. */
void game_flow_end_campaign(void);

/* Current CTaskList (NULL until a task has been begun). */
const TaskList *game_flow_task(void);

/* Current mission state for a task tag (case-insensitive); -1 if absent. The
   CTaskList store is mutable at runtime, so this reflects SCENE-sequencer writes. */
long game_flow_entity_state(const char *tag);

/* Advance a mission tag's runtime state (the SCENE-sequencer write path: port of
   set_task_state / FUN_0045f990). Writes the live CTaskList store, so subsequent
   game_flow_entity_state reads observe it. Returns 1 if a task is loaded and the
   tag exists. The object-notify push and reward/objective side effects of the
   original are deferred (consumers poll). */
int  game_flow_set_entity_state(const char *tag, long value);

/* Headless test seam (JN_TEST_SET_SCENE): ensure a CTaskList store exists (load
   the baked NewGame table if none) and write a tag, WITHOUT entering campaign
   mode or swapping level — so a direct `--level X` launch can exercise the SCENE
   gates. Not used by normal play. */
void game_flow_test_seed_state(const char *tag, long value);

/* Campaign progress ordinal (the current level's campaign index). Data only —
   exposed for the HUD / future gate-threshold wiring; the visibility gate
   itself stays env-driven (see note on game_flow_campaign_active). */
int  game_flow_progress_level(void);

/* Normalized current level name set by game_flow_enter_level(), or "" before
   the first level enters. */
const char *game_flow_current_level(void);

/* Mission counters (CJimmyGame InitGame defaults: lives=5, value=100). */
int  game_flow_lives(void);
int  game_flow_mission_value(void);

/* Death / restart (C2DInGameMenu death path). Decrement a life; if lives
   remain, signals a respawn (RestartLevel.tsk semantics — reload the current
   level's spawn); at 0 lives, a game-over that (for the demo loop) resets
   lives. Returns 1 if the player should respawn this frame. */
int  game_flow_player_died(void);

/* Win-condition / objective layer (replaces gamestate's items heuristic).
   A level's objective is met -> mark it won so the controller may advance. */
void game_flow_level_objective_met(void);
int  game_flow_level_won(void);

/* Level table (CLevel*Game collapse). Campaign ordinal for a level name, or -1
   if unknown; is_vr flags the 8 VR challenge levels. */
int  game_flow_level_index(const char *level_name);
int  game_flow_level_is_vr(const char *level_name);

#endif
