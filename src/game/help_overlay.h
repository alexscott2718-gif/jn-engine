#ifndef GAME_HELP_OVERLAY_H
#define GAME_HELP_OVERLAY_H

/* In-game controls card, styled after the Hot Wheels Mechanix viewer's HUD:
   dark panel over a dimmed stage, a flame accent rail down the left edge,
   uppercase section labels, and one filled badge per key.

   It shows itself for a few seconds at the start of every level and fades out;
   H (or F1) brings it back and pins it until dismissed.

   The shipped fontsmall.png atlas only carries A-Z, a-z and 0-9, so every
   label here is letters, digits and spaces -- no slashes or punctuation. */

void help_toggle(void);
void help_set(int on);
int  help_active(void);

/* Show for `seconds`, then fade out on its own. Called at each level boot. */
void help_show_timed(float seconds);

/* Advance the auto-hide timer. Pass the real frame delta, in seconds. */
void help_tick(float dt);

/* Per-frame input: H or F1 toggles (and pins). Safe to call every frame. */
void help_input(void);

/* Draw the card (call in the overlay pass, after the HUD). */
void help_draw(int viewport_w, int viewport_h);

#endif
