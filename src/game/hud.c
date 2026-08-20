#include "hud.h"
#include "ui_text.h"
#include "../engine/renderer.h"
#include "../engine/assets/asset_cache.h"
#include "hud_layout_generated.h"
#include "entity_visual.h"
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

/* Picture-store readout (docs/picture_flag_wiring_plan.md phase 5).

   NATIVE CHROME, NOT A PARITY CLAIM. DrawHud (00406690) is decoded to four
   literal counter positions and formats, but which gameplay value feeds each is
   an open question in docs/decomp/C2DInGameMenu.md -- the capture gives
   positions, not producers -- and the C2DInGameMenu/hud-draw certificate is
   linked-blocked precisely so nobody reconstructs a counter from one frame.
   So this uses the shipped menu font at a position of our own choosing, like
   the LEVEL CLEAR banner below, and makes no claim about DAT_004f83c0 or
   this[0x13f..0x141].

   Drawn only while something is held. That keeps it out of the way when the
   economy is idle, and it is also why the level1 golden is unaffected: at the
   capture pose the store is empty, so nothing is emitted. */
#define HUD_PIC_MAX_LISTED 6

static void hud_draw_pictures(int vw, int vh, const GameState *gs) {
    int total = 0, distinct = 0;
    for (int id = 0; id < PIC_ID_MAX; id++) {
        if (gs->pic_count[id] <= 0) continue;
        total += gs->pic_count[id];
        distinct++;
    }
    if (total <= 0) return;

    char line[96];
    int n = snprintf(line, sizeof(line), "PICTURES %d", total);
    int listed = 0;
    for (int id = 0; id < PIC_ID_MAX && listed < HUD_PIC_MAX_LISTED; id++) {
        if (gs->pic_count[id] <= 0) continue;
        int room = (int)sizeof(line) - n;
        if (room <= 1) break;
        n += snprintf(line + n, (size_t)room, "  %d x%d", id, gs->pic_count[id]);
        listed++;
    }
    /* The shipped atlas is A-Z a-z 0-9 only, so an ellipsis would render as
       three blanks -- say it in letters instead. */
    if (listed < distinct && n < (int)sizeof(line) - 6)
        snprintf(line + n, sizeof(line) - (size_t)n, " more");

    /* Top-right, right-aligned, in the same drop-shadow idiom as the
       level-clear banner. That corner is the one the extracted capture layout
       leaves empty -- atom, status icons and gauge sit top/mid-left, the gadget
       cluster bottom-left, the score counter bottom-right -- so native chrome
       there cannot be mistaken for one of the four decoded counters. */
    float scale = (float)vh / HUD_REF_H;
    if (scale < 1.0f) scale = 1.0f;
    float margin = 12.0f * scale;
    float x = (float)vw - ui_text_measure(line, scale) - margin;
    float y = margin;
    if (x < margin) x = margin;
    ui_text_draw(vw, vh, x + scale, y + scale, scale, line, 0.0f, 0.0f, 0.0f, 0.7f);
    ui_text_draw(vw, vh, x, y, scale, line, 0.95f, 0.85f, 0.35f, 1.0f);
}

/* Pickup card (docs/picture_flag_wiring_plan.md phase 5, owner request
   2026-08-20): the item just collected, with how many of it are now held in
   the top-right corner of the card.

   The HOOK is decomp-supported -- C3DPickupItem's collection path calls the
   picture/inventory service whose FUN_004061d0 docs/decomp/_scene_sequencer.md
   names as the on-screen "+counter" notify queue -- but the LAYOUT is not.
   The original draws it from C2DInGameMenu canvas records that the hud-draw
   certificate is still linked-blocked on, and assets/omt/inventory.omt's
   icons cannot be tied to picture ids without the unrecovered reward-grid
   table. So this is native chrome showing the pickup's own sprite, which we do
   know: SpriteIndex resolves through the generated chunk map.

   It fades out over its last half second and is drawn under the picture
   readout, which is the only other thing in that corner. */
static void hud_draw_pickup_card(int vw, int vh, const GameState *gs) {
    if (gs->popup_timer <= 0.0f) return;

    float alpha = gs->popup_timer < 0.5f ? gs->popup_timer / 0.5f : 1.0f;
    float s = (float)vh / HUD_REF_H;
    if (s < 1.0f) s = 1.0f;

    const float card = 56.0f * s;          /* square card, 640x480-ref units */
    const float pad  = 6.0f * s;
    float margin = 12.0f * s;
    float x = (float)vw - card - margin;
    float y = margin + ui_text_line_height(s) + 8.0f * s;   /* under the readout */

    /* Card body + a lighter inner well for the icon. */
    renderer_draw_screen_rect(vw, vh, x, y, card, card,
                              0.06f, 0.07f, 0.12f, 0.78f * alpha);
    renderer_draw_screen_rect(vw, vh, x + pad, y + pad,
                              card - pad * 2.0f, card - pad * 2.0f,
                              0.16f, 0.18f, 0.26f, 0.70f * alpha);

    const char *icon = (gs->popup_sprite > 0 &&
                        !sprite_chunk_is_hidden(gs->popup_sprite))
                     ? sprite_chunk_path(gs->popup_sprite) : NULL;
    if (icon) {
        unsigned int tex = tex_cache_get(icon);
        if (tex)
            renderer_draw_sprite_2d(tex, vw, vh, x + pad, y + pad,
                                    card - pad * 2.0f, card - pad * 2.0f,
                                    1.0f, 1.0f, 1.0f, alpha);
    }

    /* The count, top-right of the card. Scoring-only pickups award no picture,
       so they get the card without a number rather than a misleading zero. */
    if (gs->popup_id >= 0) {
        char n[16];
        snprintf(n, sizeof(n), "%d", gs->popup_count);
        float ts = s;
        float tw = ui_text_measure(n, ts);
        float tx = x + card - tw - pad;
        float ty = y + pad * 0.5f;
        ui_text_draw(vw, vh, tx + ts, ty + ts, ts, n, 0.0f, 0.0f, 0.0f, 0.8f * alpha);
        ui_text_draw(vw, vh, tx, ty, ts, n, 1.0f, 0.95f, 0.45f, alpha);
    }
}

void hud_init(void) {
    ui_text_init();
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

    hud_draw_pictures(vw, vh, gs);
    hud_draw_pickup_card(vw, vh, gs);

    /* The original functional HUD path exposed a level-clear message. Draw it
       with the shipped menu font now that the shared atlas renderer exists. */
    if (gs->level_done) {
        float scale = 3.0f * (float)vh / HUD_REF_H;
        float y = (float)vh * 0.18f;
        ui_text_draw_centered(vw, vh, (float)vw * 0.5f + scale, y + scale,
                              scale, "LEVEL CLEAR", 0.0f, 0.0f, 0.0f, 0.75f);
        ui_text_draw_centered(vw, vh, (float)vw * 0.5f, y,
                              scale, "LEVEL CLEAR", 0.25f, 1.0f, 0.35f, 1.0f);
    }
}
