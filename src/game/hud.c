#include "hud.h"
#include "../engine/renderer.h"
#include "../engine/assets/asset_cache.h"
#include "hud_layout_generated.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Faithful HUD reconstructed from the original game's captured D3D7 stream
   (accepted Level-1 ground-truth frame 8881). Static art (atom, gauge, gadget)
   comes straight from the captured textures at their measured positions; the
   two numeric counters are drawn live from GameState using the game's own
   chrome digit font, harvested from the capture (tools/harvest_hud_digits.py).
   Target image: assets/capture/level1_hudfix/hud/hud_reconstruction.png.

   Authored in a 640x480 reference space; we scale by window-height/480 and
   anchor each element to the screen corner its capture position implies, so the
   4:3 HUD never distorts on a wider window. */

#define HUD_REF_W 640.0f
#define HUD_REF_H 480.0f

/* Map a normalized 640x480-ref rect to window pixels: uniform scale + corner
   anchoring (each widget hugs the screen edge it was authored at). */
static void rect_to_screen(float nx, float ny, float nw, float nh,
                           int vw, int vh,
                           float *x, float *y, float *w, float *h) {
    float s = (float)vh / HUD_REF_H;
    *w = nw * HUD_REF_W * s;
    *h = nh * HUD_REF_H * s;
    *x = (nx + nw * 0.5f < 0.5f) ? nx * HUD_REF_W * s
                                 : (float)vw - (1.0f - nx) * HUD_REF_W * s;
    *y = (ny + nh * 0.5f < 0.5f) ? ny * HUD_REF_H * s
                                 : (float)vh - (1.0f - ny) * HUD_REF_H * s;
}

/* Draw `value` right-justified into a counter's digit slots using the chrome
   font. Slots run left-to-right from (c->nx); the value's least-significant
   digit lands in the rightmost slot. Missing glyphs (3,4,6 pending XP
   recapture) draw a faint placeholder rather than a gap. */
static void draw_counter(int vw, int vh, int value, const HudCounter *c) {
    if (c->slots <= 0) return;
    if (value < 0) value = 0;
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%d", value);
    if (n < 1) return;

    for (int i = 0; i < n; i++) {
        int slot = c->slots - 1 - i;          /* fill from the right */
        if (slot < 0) break;                   /* value wider than field */
        int digit = buf[n - 1 - i] - '0';
        float nx = c->nx + (float)slot * c->cell_nw;
        float x, y, w, h;
        rect_to_screen(nx, c->ny, c->cell_nw, c->cell_nh, vw, vh, &x, &y, &w, &h);

        char path[64];
        snprintf(path, sizeof(path), HUD_DIGIT_PATH, digit);
        unsigned int tex = tex_cache_get(path);
        if (tex)
            renderer_draw_sprite_2d(tex, vw, vh, x, y, w, h, 1, 1, 1, 1);
        else  /* glyph not yet harvested: dim chrome-tinted placeholder */
            renderer_draw_screen_rect(vw, vh, x, y, w, h, 0.45f, 0.35f, 0.65f, 0.55f);
    }
}

void hud_init(void) {
    for (int i = 0; i < HUD_LAYOUT_COUNT; i++)
        (void)tex_cache_get(HUD_LAYOUT[i].texture);
    for (int d = 0; d < 10; d++) {
        char path[64];
        snprintf(path, sizeof(path), HUD_DIGIT_PATH, d);
        (void)tex_cache_get(path);
    }
}

void hud_draw(int vw, int vh, const GameState *gs) {
    if (!gs || vw <= 0 || vh <= 0) return;

    /* Static art: everything except the counter clusters (their digits are
       drawn live below; their decorative separators are omitted for a clean
       dynamic number). */
    for (int i = 0; i < HUD_LAYOUT_COUNT; i++) {
        const HudLayoutElem *e = &HUD_LAYOUT[i];
        if (strncmp(e->role, "counter", 7) == 0)  /* digits drawn live below */
            continue;
        unsigned int tex = tex_cache_get(e->texture);
        if (!tex) continue;
        float x, y, w, h;
        rect_to_screen(e->nx, e->ny, e->nw, e->nh, vw, vh, &x, &y, &w, &h);
        renderer_draw_sprite_2d(tex, vw, vh, x, y, w, h, 1, 1, 1, 1);
    }

    /* Live counters from the item/score state. JN_HUD_TEST="items,score"
       forces values for QA against the captured frame (17 / 250). */
    int items = gs->items_collected, score = gs->points;
    const char *t = getenv("JN_HUD_TEST");
    if (t) sscanf(t, "%d,%d", &items, &score);
    draw_counter(vw, vh, items, &HUD_COUNTER_ITEMS);
    draw_counter(vw, vh, score, &HUD_COUNTER_SCORE);
}
