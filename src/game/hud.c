#include "hud.h"
#include "../engine/renderer.h"
#include "../engine/assets/asset_cache.h"
#include <string.h>

/* HUD art (assets/hud/). apple.png is a clean alpha icon; heart/toolchest/bars
   are full-frame palette panels from the original png/ dir. */
#define HUD_APPLE     "assets/hud/apple.png"
#define HUD_HEART     "assets/hud/heart.png"
#define HUD_TOOLCHEST "assets/hud/toolchest.png"

void hud_init(void) {
    /* Warm the cache so the first frame doesn't stall. Failures are non-fatal:
       the draw path guards on a 0 texture id. */
    (void)tex_cache_get(HUD_APPLE);
    (void)tex_cache_get(HUD_HEART);
    (void)tex_cache_get(HUD_TOOLCHEST);
}

/* ---- tiny 3x5 rect-based digit font (no font asset needed) ----
   Each glyph is 5 rows; the low 3 bits of each byte are the columns
   (bit 2 = left, bit 0 = right). */
static const unsigned char GLYPH_DIGIT[10][5] = {
    {7,5,5,5,7}, /* 0 */
    {2,6,2,2,7}, /* 1 */
    {7,1,7,4,7}, /* 2 */
    {7,1,7,1,7}, /* 3 */
    {5,5,7,1,1}, /* 4 */
    {7,4,7,1,7}, /* 5 */
    {7,4,7,5,7}, /* 6 */
    {7,1,2,2,2}, /* 7 */
    {7,5,7,5,7}, /* 8 */
    {7,5,7,1,7}, /* 9 */
};
static const unsigned char GLYPH_SLASH[5] = {1,1,2,4,4};

/* Minimal uppercase subset for the "LEVEL CLEAR" objective banner. */
static const unsigned char *glyph_for_letter(char c) {
    static const unsigned char L[5]={4,4,4,4,7}, E[5]={7,4,7,4,7},
        V[5]={5,5,5,5,2}, C[5]={7,4,4,4,7}, A[5]={7,5,7,5,5},
        R[5]={7,5,7,6,5}, S[5]={7,4,7,1,7}, P[5]={7,5,7,4,4},
        T[5]={7,2,2,2,2}, O[5]={7,5,5,5,7}, M[5]={5,7,7,5,5},
        N[5]={5,7,7,7,5}, I[5]={7,2,2,2,7}, G[5]={7,4,5,5,7};
    switch (c) {
        case 'L': return L; case 'E': return E; case 'V': return V;
        case 'C': return C; case 'A': return A; case 'R': return R;
        case 'S': return S; case 'P': return P; case 'T': return T;
        case 'O': return O; case 'M': return M; case 'N': return N;
        case 'I': return I; case 'G': return G;
        default:  return NULL;
    }
}

/* Draw one 3x5 glyph at (x,y) with the given pixel size. */
static void draw_glyph(int vw, int vh, float x, float y, float px,
                       const unsigned char rows[5],
                       float r, float g, float b, float a) {
    for (int row = 0; row < 5; row++) {
        unsigned char bits = rows[row];
        for (int col = 0; col < 3; col++) {
            if (bits & (1 << (2 - col)))
                renderer_draw_screen_rect(vw, vh,
                                          x + col * px, y + row * px, px, px,
                                          r, g, b, a);
        }
    }
}

/* Advance per character = 4*px (3 wide + 1 gap). */
static float glyph_advance(float px) { return px * 4.0f; }

/* Draw an uppercase ASCII string (letters/digits/space/'/'); unknown chars are
   rendered as blank space. Returns total pixel width drawn. */
static float draw_text(int vw, int vh, float x, float y, float px, const char *s,
                       float r, float g, float b, float a) {
    float x0 = x;
    for (const char *p = s; *p; p++) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= '0' && c <= '9')
            draw_glyph(vw, vh, x, y, px, GLYPH_DIGIT[c - '0'], r, g, b, a);
        else if (c == '/')
            draw_glyph(vw, vh, x, y, px, GLYPH_SLASH, r, g, b, a);
        else if (c != ' ') {
            const unsigned char *gl = glyph_for_letter(c);
            if (gl) draw_glyph(vw, vh, x, y, px, gl, r, g, b, a);
        }
        x += px * 4.0f;
    }
    return x - x0;
}

/* Draw an integer right-justified is overkill; draw left-to-right from x.
   Returns the x past the last glyph. */
static float draw_int(int vw, int vh, float x, float y, float px, int value,
                      float r, float g, float b, float a) {
    char buf[16];
    int n = 0;
    if (value < 0) value = 0;
    /* itoa without stdio */
    char tmp[16];
    int t = 0;
    do { tmp[t++] = (char)('0' + value % 10); value /= 10; } while (value && t < 15);
    while (t > 0) buf[n++] = tmp[--t];
    for (int i = 0; i < n; i++) {
        draw_glyph(vw, vh, x, y, px, GLYPH_DIGIT[buf[i] - '0'], r, g, b, a);
        x += glyph_advance(px);
    }
    return x;
}

/* Draw "a/b" with a slash glyph. */
static void draw_ratio(int vw, int vh, float x, float y, float px,
                       int a_val, int b_val,
                       float r, float g, float b_, float al) {
    x = draw_int(vw, vh, x, y, px, a_val, r, g, b_, al);
    draw_glyph(vw, vh, x, y, px, GLYPH_SLASH, r, g, b_, al);
    x += glyph_advance(px);
    draw_int(vw, vh, x, y, px, b_val, r, g, b_, al);
}

void hud_draw(int vw, int vh, const GameState *gs) {
    if (!gs || vw <= 0 || vh <= 0) return;

    const float pad = 16.0f;

    /* --- Item / apple counter (top-left) --- */
    unsigned int apple = tex_cache_get(HUD_APPLE);
    float icon = 40.0f;
    if (apple)
        renderer_draw_sprite_2d(apple, vw, vh, pad, pad, icon, icon,
                                1.0f, 1.0f, 1.0f, 1.0f);
    /* Drop shadow then white digits for legibility over any scene. */
    float tx = pad + icon + 12.0f;
    float ty = pad + 6.0f;
    float dpx = 6.0f;
    draw_ratio(vw, vh, tx + 2.0f, ty + 2.0f, dpx,
               gs->items_collected, gs->items_total, 0.0f, 0.0f, 0.0f, 0.6f);
    draw_ratio(vw, vh, tx, ty, dpx,
               gs->items_collected, gs->items_total, 1.0f, 1.0f, 1.0f, 1.0f);

    /* --- Gem counter (below items), only when relevant --- */
    if (gs->gems_collected > 0) {
        float gy = pad + icon + 12.0f;
        /* small gem swatch */
        renderer_draw_screen_rect(vw, vh, pad + 6.0f, gy, 26.0f, 26.0f,
                                  0.20f, 0.85f, 0.30f, 1.0f);
        draw_int(vw, vh, tx + 2.0f, gy + 4.0f + 2.0f, dpx, gs->gems_collected,
                 0.0f, 0.0f, 0.0f, 0.6f);
        draw_int(vw, vh, tx, gy + 4.0f, dpx, gs->gems_collected,
                 0.4f, 1.0f, 0.5f, 1.0f);
    }

    /* --- Health bar + heart icon (bottom-left) --- */
    unsigned int heart = tex_cache_get(HUD_HEART);
    float hy = vh - pad - 44.0f;
    if (heart)
        renderer_draw_sprite_2d(heart, vw, vh, pad, hy, 44.0f, 44.0f,
                                1.0f, 1.0f, 1.0f, 1.0f);
    float bar_x = pad + 44.0f + 12.0f;
    float bar_y = hy + 12.0f;
    float bar_w = 220.0f;
    float bar_h = 20.0f;
    int hmax = gs->health_max > 0 ? gs->health_max : 100;
    float frac = (float)gs->health / (float)hmax;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    /* frame + dark backing */
    renderer_draw_screen_rect(vw, vh, bar_x - 2.0f, bar_y - 2.0f,
                              bar_w + 4.0f, bar_h + 4.0f, 0.0f, 0.0f, 0.0f, 0.6f);
    renderer_draw_screen_rect(vw, vh, bar_x, bar_y, bar_w, bar_h,
                              0.12f, 0.12f, 0.12f, 0.85f);
    /* fill: green when high, red when low */
    float fr = 1.0f - frac, fg = frac;
    renderer_draw_screen_rect(vw, vh, bar_x, bar_y, bar_w * frac, bar_h,
                              fr * 0.9f + 0.1f, fg * 0.85f + 0.1f, 0.12f, 1.0f);

    /* --- Inventory tool icons (bottom row, right of the health bar) --- */
    if (gs->inventory_count > 0) {
        float slot = 40.0f;
        float ix = bar_x + bar_w + 20.0f;
        float iy = vh - pad - slot;
        /* Toolchest affordance heads the row. */
        unsigned int chest = tex_cache_get(HUD_TOOLCHEST);
        if (chest) {
            renderer_draw_sprite_2d(chest, vw, vh, ix, iy, slot, slot,
                                    1.0f, 1.0f, 1.0f, 1.0f);
            ix += slot + 16.0f;
        }
        for (int i = 0; i < gs->inventory_count; i++) {
            const InventorySlot *s = &gs->inventory[i];
            unsigned int t = s->icon_path ? tex_cache_get(s->icon_path) : 0;
            /* backing plate */
            renderer_draw_screen_rect(vw, vh, ix - 2.0f, iy - 2.0f,
                                      slot + 4.0f, slot + 4.0f,
                                      0.0f, 0.0f, 0.0f, 0.5f);
            if (t)
                renderer_draw_sprite_2d(t, vw, vh, ix, iy, slot, slot,
                                        1.0f, 1.0f, 1.0f, 1.0f);
            else
                renderer_draw_screen_rect(vw, vh, ix, iy, slot, slot,
                                          0.3f, 0.5f, 0.8f, 0.9f);
            ix += slot + 10.0f;
        }
    }

    /* --- Objective banner: "LEVEL CLEAR" when all items are collected --- */
    if (gs->level_done) {
        const char *msg = "LEVEL CLEAR";
        float px = 8.0f;
        float text_w = (float)((int)strlen(msg)) * px * 4.0f;
        float bx = (vw - text_w) * 0.5f;
        float by = vh * 0.16f;
        /* translucent green plate behind the text */
        renderer_draw_screen_rect(vw, vh, bx - 24.0f, by - 16.0f,
                                  text_w + 48.0f, px * 5.0f + 32.0f,
                                  0.05f, 0.35f, 0.10f, 0.7f);
        draw_text(vw, vh, bx + 3.0f, by + 3.0f, px, msg, 0.0f, 0.0f, 0.0f, 0.7f);
        draw_text(vw, vh, bx, by, px, msg, 0.55f, 1.0f, 0.55f, 1.0f);
    }
}
