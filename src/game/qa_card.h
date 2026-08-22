#ifndef GAME_QA_CARD_H
#define GAME_QA_CARD_H

/* Native QA card editor + JSON export.
 *
 * The web build gets this from shell.html: click an object in QA mode, fill in
 * a category + message, and the report joins a tag stack you can export as
 * markdown + JSON (docs/qa_annotate_plan.md M2/M3). Native builds had none of
 * it -- "native degraded mode" (M5) put the pick JSON on the clipboard and
 * stopped there, so a reporter on the Windows QA build could highlight an
 * object but could not say what was wrong with it.
 *
 * This is that missing half, in-engine. It reuses the same pieces level_select
 * already uses -- renderer_draw_screen_rect for the panels, ui_text for the
 * glyphs -- so it costs no new renderer surface.
 *
 * Font note: the message field accepts anything fontsmall.png can draw, which
 * since the ui_text punctuation mapping is the printable ASCII set. Characters
 * with no glyph are dropped on input rather than typed as an invisible
 * advance. Exported files are plain C strings either way.
 */

/* Open the card for a freshly picked object. Called from qa.c on a `pick`
 * event; every field is the same identity the web dialog shows. */
void qa_card_open(const char *kind, const char *name, const char *tag,
                  const char *asset, const char *level,
                  float px, float py, float pz,
                  float ax, float ay, float az,
                  float cam_x, float cam_y, float cam_z, float cam_yaw);

/* The player's current position, sampled per frame by main.c. Cards record it
 * because the collision reports this exists for are about where the *player*
 * was standing (or falling through), not only what was under the cursor. */
void qa_card_note_player(float x, float y, float z);

/* Dialog is up: main.c routes keys/text here and freezes gameplay input. */
int  qa_card_active(void);

/* SDL_KEYDOWN / SDL_TEXTINPUT while the dialog is up. */
void qa_card_key(int sdl_keycode);
void qa_card_text(const char *utf8);

/* Number of saved cards this session. */
int  qa_card_count(void);

/* Drop the most recent card (X undo). Returns 1 if one was removed. */
int  qa_card_undo(void);

/* Write jn-qa-report.json + jn-qa-report.md next to the running exe's working
 * directory and put the same payload on the clipboard. Returns cards written,
 * or -1 if the files could not be opened. */
int  qa_card_export(void);

/* Draw the dialog and the tag stack. Call in the overlay pass. */
void qa_card_draw(int viewport_w, int viewport_h);

#endif /* GAME_QA_CARD_H */
