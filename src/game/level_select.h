#ifndef GAME_LEVEL_SELECT_H
#define GAME_LEVEL_SELECT_H

/* QA level browser: every level the engine can load, in a scrolling list.
 *
 * Deliberately NOT part of menu.c. CMainMenu's routing table is certified by
 * tools/linkage_oracles/CMainMenu.py, which pins it to exactly ten entries --
 * New Game, the eight VR challenges in the executable's order, then the
 * no-route terminator -- and proves the count by probing past the end and
 * requiring a wrap to index 0. Appending story levels there silently broke
 * that certificate. A QA browser is not the front-end, so it lives in its own
 * file with its own state and its own key (M).
 *
 * menu.c stays the faithful stand-in for CMainMenu; this is the convenience.
 */

void level_select_open(void);
void level_select_close(void);
int  level_select_active(void);

/* Per-frame input: Up/Down to move, PgUp/PgDn or Left/Right to page,
   Home/End to jump. Call while active. */
void level_select_input(void);

/* If Enter/Space was pressed this frame, returns 1 and sets *level_out to the
   normalized level name ("level3", "vr01", ...). Returns 0 otherwise. */
int  level_select_take_confirm(const char **level_out);

/* Draw the browser (call in the overlay pass). */
void level_select_draw(int viewport_w, int viewport_h);

#endif
