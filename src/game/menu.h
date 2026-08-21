#ifndef MENU_H
#define MENU_H

/* Native port of CMainMenu (docs/decomp/CMainMenu.md) — the front-end menu
   game mode. CMainMenu is a thin CJimmyGame subclass that runs as a game mode
   like a level but, instead of gameplay, drives the title / level-select screen
   and routes the player's selection to the chosen level's .tsk/.gam. The
   routing targets are the executable's level-file table (.rdata:004ec71c):
   "New Game" -> NewGame.tsk -> Level 1 (level1b), and the eight VR challenge
   levels in their menu order (VR01, VR03, VR02, VR08, VR06, VR07, VR05, VR04).

   The original's clickable items are CMenuElement canvas objects and the screen
   graph lives in menu-manager code not decompiled in the spec; this port
   implements the specced routing table with keyboard navigation (Up/Down +
   Enter), drawn as a selectable list over the live scene as a backdrop. */

void menu_open(void);
void menu_close(void);
int  menu_active(void);

/* Per-frame input: reads Up/Down to move the selection. Call while active. */
void menu_input(void);

/* If Enter was pressed this frame, returns 1 and fills the selection:
   *level_out = normalized level name to load ("level1b", "vr01", ...) or NULL
   for the Quit item; *is_newgame_out = 1 when the item routes through the
   NewGame task. Returns 0 if no confirmation this frame. */
int  menu_take_confirm(const char **level_out, int *is_newgame_out);

/* Current selection without an Enter press (headless auto-confirm path). */
void menu_current(const char **level_out, int *is_newgame_out);

/* Move the selection onto the item routing to `level` (case-insensitive),
   as Up/Down would. 1 when the table has such an item. Headless seam: it
   lets a probe reach a VR item, which the auto-confirm path alone cannot --
   that one only ever takes item 0. The routing table is untouched. */
int  menu_select_level(const char *level);

/* Draw the menu overlay (call after the 3D scene, before end_frame). */
void menu_draw(int viewport_w, int viewport_h);

#endif
