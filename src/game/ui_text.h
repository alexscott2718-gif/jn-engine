#ifndef GAME_UI_TEXT_H
#define GAME_UI_TEXT_H

/* Shared menu/HUD text renderer over the shipped fontsmall.png atlas.
   The atlas's 110 fixed-pitch rows are A-Z, a-z, 0-9 (0..61), ASCII
   punctuation (62..90), and accented Latin-1 forms (91..108, unmapped). */
void  ui_text_init(void);
int   ui_text_glyph_index(char c);
float ui_text_measure(const char *text, float scale);
float ui_text_line_height(float scale);
void  ui_text_draw(int viewport_w, int viewport_h,
                   float x, float y, float scale, const char *text,
                   float r, float g, float b, float a);
void  ui_text_draw_centered(int viewport_w, int viewport_h,
                            float center_x, float y, float scale, const char *text,
                            float r, float g, float b, float a);

#endif
