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
    /* Cells 62..90 are punctuation. The atlas always had them; nothing mapped
       them, so every '-', '.' and ':' in a UI string rendered as a blank of
       the same advance -- silently turning "-824" into "824" on the QA card's
       coordinate lines. Order below is the atlas's, verified by rendering the
       alpha channel cell by cell (docs: the shape is in alpha, the atlas is a
       mask texture). Cells 91..108 are accented Latin-1 forms, unmapped. */
    switch (c) {
    case '(':  return 62;
    case ')':  return 63;
    case '.':  return 64;
    case ';':  return 65;
    case ',':  return 66;
    case '!':  return 67;
    case '?':  return 68;
    case ':':  return 69;
    case '{':  return 70;
    case '}':  return 71;
    case '+':  return 72;
    case '-':  return 73;
    case '=':  return 74;
    case '[':  return 75;
    case ']':  return 76;
    case '#':  return 77;
    case '@':  return 78;
    case '$':  return 81;
    case '"':  return 82;
    case '/':  return 83;
    case '>':  return 84;
    case '<':  return 85;
    case '\'': return 86;
    case '&':  return 87;
    case '%':  return 88;
    case '*':  return 89;   /* atlas carries a multiplication cross */
    default:   break;
    }
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
