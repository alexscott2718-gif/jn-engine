#include "ui_text.h"
#include "../engine/renderer.h"
#include "../engine/assets/asset_cache.h"

#define UI_FONT_PATH "assets/png/fontsmall.png"
#define UI_FONT_ATLAS_W 22.0f
#define UI_FONT_ATLAS_H 1109.0f
#define UI_FONT_SOURCE_X 3.0f
#define UI_FONT_SOURCE_W 13.0f
#define UI_FONT_CELL_H 10.0f
#define UI_FONT_ADVANCE 11.0f
#define UI_FONT_SPACE_ADVANCE 6.0f

void ui_text_init(void) {
    (void)tex_cache_get(UI_FONT_PATH);
}

int ui_text_glyph_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return 26 + c - 'a';
    if (c >= '0' && c <= '9') return 52 + c - '0';
    return -1;
}

static float ui_text_advance(char c, float scale) {
    return (c == ' ' ? UI_FONT_SPACE_ADVANCE : UI_FONT_ADVANCE) * scale;
}

float ui_text_measure(const char *text, float scale) {
    if (!text || scale <= 0.0f) return 0.0f;
    float width = 0.0f;
    for (; *text; text++) width += ui_text_advance(*text, scale);
    return width;
}

float ui_text_line_height(float scale) {
    return scale > 0.0f ? UI_FONT_CELL_H * scale : 0.0f;
}

void ui_text_draw(int viewport_w, int viewport_h,
                  float x, float y, float scale, const char *text,
                  float r, float g, float b, float a) {
    if (!text || scale <= 0.0f || viewport_w <= 0 || viewport_h <= 0) return;
    unsigned int tex = tex_cache_get(UI_FONT_PATH);
    if (!tex) return;

    /* Sample texel centers inside each cell. The atlas is linearly filtered;
       using cell boundaries would bleed the next row into glyph descenders. */
    const float u0 = (UI_FONT_SOURCE_X + 0.5f) / UI_FONT_ATLAS_W;
    const float u1 = (UI_FONT_SOURCE_X + UI_FONT_SOURCE_W - 0.5f) / UI_FONT_ATLAS_W;
    for (; *text; text++) {
        int glyph = ui_text_glyph_index(*text);
        if (glyph >= 0) {
            float source_y = (float)glyph * UI_FONT_CELL_H;
            float v_top = 1.0f - (source_y + 0.5f) / UI_FONT_ATLAS_H;
            float v_bottom = 1.0f -
                (source_y + UI_FONT_CELL_H - 0.5f) / UI_FONT_ATLAS_H;
            renderer_draw_mask_region_2d(tex, viewport_w, viewport_h,
                x, y, UI_FONT_SOURCE_W * scale, UI_FONT_CELL_H * scale,
                u0, v_top, u1, v_bottom, r, g, b, a);
        }
        x += ui_text_advance(*text, scale);
    }
}

void ui_text_draw_centered(int viewport_w, int viewport_h,
                           float center_x, float y, float scale, const char *text,
                           float r, float g, float b, float a) {
    ui_text_draw(viewport_w, viewport_h,
                 center_x - ui_text_measure(text, scale) * 0.5f,
                 y, scale, text, r, g, b, a);
}
