#include "help_overlay.h"
#include "ui_text.h"
#include "../engine/input.h"
#include "../engine/renderer.h"
#include <stdio.h>

/* ---- Mechanix viewer palette -------------------------------------------
   Taken from the Hot Wheels Mechanix standalone viewer so the two tools read
   as one family: ink ground, panel cards, a flame accent, muted mono labels. */
#define INK_R    0.055f
#define INK_G    0.075f
#define INK_B    0.098f   /* #0E1319 */
#define PANEL_R  0.086f
#define PANEL_G  0.114f
#define PANEL_B  0.149f   /* #161D26 */
#define STRIPE_R 0.114f
#define STRIPE_G 0.153f
#define STRIPE_B 0.200f   /* #1D2733 */
#define LINE_R   0.165f
#define LINE_G   0.208f
#define LINE_B   0.259f   /* #2A3542 */
#define FLAME_R  0.941f
#define FLAME_G  0.333f
#define FLAME_B  0.169f   /* #F0552B */
#define TEXT_R   0.902f
#define TEXT_G   0.925f
#define TEXT_B   0.949f   /* #E6ECF2 */
#define MUTED_R  0.518f
#define MUTED_G  0.588f
#define MUTED_B  0.651f   /* #8496A6 */

typedef struct { const char *keys; const char *what; } HelpRow;
typedef struct { const char *label; const HelpRow *rows; int count; } HelpSection;

/* Every binding below was read off the source, not remembered:
   behavior_player.c (move, run, jump, noclip, tool), behavior_vehicle.c
   (board), and main.c (M, T, R). Keep this in step if the bindings move. */
static const HelpRow g_move[] = {
    { "W A S D", "Move" },
    { "Mouse",   "Look around" },
    { "Shift",   "Run" },
    { "Space",   "Jump or fly up" },
    { "Ctrl Q",  "Fly down in noclip" },
    { "E",       "Ride a vehicle" },
};
static const HelpRow g_act[] = {
    { "N", "Noclip on or off" },
    { "F", "Use the active tool" },
    { "T", "Talk to a friend" },
    { "R", "Respawn" },
    { "M", "Level select" },
    { "H", "Hide this card" },
};
static const HelpSection g_sections[] = {
    { "MOVEMENT", g_move, (int)(sizeof(g_move) / sizeof(g_move[0])) },
    { "ACTIONS",  g_act,  (int)(sizeof(g_act)  / sizeof(g_act[0]))  },
};
static const int g_section_count = (int)(sizeof(g_sections) / sizeof(g_sections[0]));

/* Rarely-needed keys get one muted footer line rather than rows of their own. */
static const char *g_footer = "C coordinates    V camera    B bug report    Esc quit";

static int   g_auto     = 1;   /* automatic level-boot greeting allowed */
static int   g_active   = 0;
static float g_timer    = 0.0f;   /* seconds left when auto-shown; 0 means pinned */
static const float g_fade_len = 1.5f;   /* length of the tail fade, in seconds */

void help_set(int on) {
    g_active = on ? 1 : 0;
    g_timer  = 0.0f;              /* an explicit show is pinned and never fades */
}

void help_toggle(void) {
    help_set(!g_active);
    printf("[HELP] %s\n", g_active ? "open (pinned)" : "closed");
}

int help_active(void) { return g_active; }

void help_set_auto(int enabled) { g_auto = enabled ? 1 : 0; }

void help_show_timed(float seconds) {
    if (!g_auto) return;   /* headless / screenshot / capture run */
    g_active = 1;
    g_timer  = seconds > 0.0f ? seconds : 0.0f;
}

void help_tick(float dt) {
    if (!g_active || g_timer <= 0.0f) return;   /* pinned, or already hidden */
    if (dt > 0.25f) dt = 0.25f;                 /* don't let a load hitch eat the timer */
    g_timer -= dt;
    if (g_timer <= 0.0f) {
        g_timer  = 0.0f;
        g_active = 0;
    }
}

void help_input(void) {
    if (input_just_pressed(SDL_SCANCODE_H) || input_just_pressed(SDL_SCANCODE_F1))
        help_toggle();
}

/* Opacity for this frame: solid while pinned or holding, easing out at the tail. */
static float help_alpha(void) {
    if (!g_active) return 0.0f;
    if (g_timer <= 0.0f) return 1.0f;
    if (g_timer >= g_fade_len) return 1.0f;
    return g_timer / g_fade_len;
}

/* Badge metrics, shared by the measure and draw passes. */
#define BADGE_PAD_X 5.0f
#define BADGE_PAD_Y 2.0f
#define BADGE_GAP   4.0f

/* One flame badge per space-separated token, after the viewer's key chips.
   With draw=0 nothing is emitted and only the total width is returned, so the
   panel can size its key column to the widest row instead of a guessed value. */
static float key_badges(int vw, int vh, float x, float y,
                        const char *keys, float scale, float a, int draw) {
    const float line_h = ui_text_line_height(scale);
    const float x0 = x;

    const char *p = keys;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        char tok[24];
        int n = 0;
        while (*p && *p != ' ' && n < (int)sizeof(tok) - 1) tok[n++] = *p++;
        tok[n] = 0;

        float bw = ui_text_measure(tok, scale) + BADGE_PAD_X * 2.0f;
        float bh = line_h + BADGE_PAD_Y * 2.0f;
        if (draw) {
            renderer_draw_screen_rect(vw, vh, x, y, bw, bh,
                                      FLAME_R, FLAME_G, FLAME_B, 0.95f * a);
            ui_text_draw(vw, vh, x + BADGE_PAD_X, y + BADGE_PAD_Y, scale, tok,
                         1.0f, 1.0f, 1.0f, a);
        }
        x += bw + BADGE_GAP;
    }
    return x - x0;
}

void help_draw(int viewport_w, int viewport_h) {
    float a = help_alpha();
    if (a <= 0.0f) return;
    renderer_begin_overlay(viewport_w, viewport_h);

    /* Whole-number scales only. A glyph advances UI_FONT_ADVANCE * scale, so a
       fractional scale lands glyphs on fractional pixels and the atlas's linear
       filter smears them into ragged word spacing. 1, 2 and 3 stay on the grid.
       Small uppercase labels over larger rows is the viewer's hierarchy too. */
    const float title_s = 3.0f;
    const float label_s = 1.0f;
    const float row_s   = 2.0f;
    const float key_s   = 2.0f;

    const float title_h = ui_text_line_height(title_s);
    const float label_h = ui_text_line_height(label_s);
    const float row_h   = ui_text_line_height(row_s);
    const float key_h   = ui_text_line_height(key_s) + BADGE_PAD_Y * 2.0f;

    const float pad     = 22.0f;
    const float rail_w  = 3.0f;
    const float row_gap = 6.0f;
    const float sec_gap = 14.0f;
    const float gutter  = 22.0f;   /* between the key column and the descriptions */

    /* Size the panel from the content: the key column is as wide as the widest
       badge group, and the panel as wide as the longest description after it.
       Measuring beats a guessed constant -- a longer label can't overflow. */
    float key_col = 0.0f;
    float desc_w  = 0.0f;
    for (int s = 0; s < g_section_count; s++) {
        for (int i = 0; i < g_sections[s].count; i++) {
            float kw = key_badges(0, 0, 0, 0, g_sections[s].rows[i].keys, key_s, 0.0f, 0);
            if (kw > key_col) key_col = kw;
            float dw = ui_text_measure(g_sections[s].rows[i].what, row_s);
            if (dw > desc_w) desc_w = dw;
        }
    }
    key_col += gutter;

    float content_w = key_col + desc_w;
    float title_w   = ui_text_measure("CONTROLS", title_s);
    float footer_w  = ui_text_measure(g_footer, label_s);
    if (title_w  > content_w) content_w = title_w;
    if (footer_w > content_w) content_w = footer_w;

    float panel_w = content_w + rail_w + pad * 2.0f;
    if (panel_w > viewport_w * 0.94f) panel_w = viewport_w * 0.94f;

    float body_h = 0.0f;
    for (int s = 0; s < g_section_count; s++) {
        body_h += label_h + 6.0f;
        body_h += g_sections[s].count * (key_h + row_gap);
        if (s + 1 < g_section_count) body_h += sec_gap;
    }
    float footer_h = ui_text_line_height(label_s);
    float panel_h  = pad + title_h + 12.0f + 1.0f + 14.0f + body_h + 10.0f + footer_h + pad;

    /* Snap the panel to whole pixels so the text inside stays on the grid. */
    float px = (float)(int)((viewport_w - panel_w) * 0.5f);
    float py = (float)(int)((viewport_h - panel_h) * 0.5f);
    if (py < 8.0f) py = 8.0f;

    /* Dim the stage, a hairline border behind the panel, then the panel itself. */
    renderer_draw_screen_rect(viewport_w, viewport_h, 0, 0,
                              (float)viewport_w, (float)viewport_h,
                              INK_R, INK_G, INK_B, 0.70f * a);
    renderer_draw_screen_rect(viewport_w, viewport_h, px - 1.0f, py - 1.0f,
                              panel_w + 2.0f, panel_h + 2.0f,
                              LINE_R, LINE_G, LINE_B, 0.95f * a);
    renderer_draw_screen_rect(viewport_w, viewport_h, px, py, panel_w, panel_h,
                              PANEL_R, PANEL_G, PANEL_B, 0.97f * a);
    /* Flame rail down the left edge: the viewer's level-button signature. */
    renderer_draw_screen_rect(viewport_w, viewport_h, px, py, rail_w, panel_h,
                              FLAME_R, FLAME_G, FLAME_B, a);

    float cx = px + rail_w + pad;          /* content left edge */
    float cw = panel_w - rail_w - pad * 2.0f;
    float y  = py + pad;

    ui_text_draw(viewport_w, viewport_h, cx, y, title_s, "CONTROLS",
                 FLAME_R, FLAME_G, FLAME_B, a);
    y += title_h + 12.0f;
    renderer_draw_screen_rect(viewport_w, viewport_h, cx, y, cw, 1.0f,
                              LINE_R, LINE_G, LINE_B, a);
    y += 14.0f;

    for (int s = 0; s < g_section_count; s++) {
        ui_text_draw(viewport_w, viewport_h, cx, y, label_s, g_sections[s].label,
                     MUTED_R, MUTED_G, MUTED_B, a);
        y += label_h + 6.0f;

        for (int i = 0; i < g_sections[s].count; i++) {
            const HelpRow *rw = &g_sections[s].rows[i];
            if (i & 1)   /* zebra stripe, so a long row stays easy to track across */
                renderer_draw_screen_rect(viewport_w, viewport_h,
                                          cx - 6.0f, y - 2.0f, cw + 12.0f, key_h + 4.0f,
                                          STRIPE_R, STRIPE_G, STRIPE_B, 0.65f * a);
            key_badges(viewport_w, viewport_h, cx, y, rw->keys, key_s, a, 1);
            ui_text_draw(viewport_w, viewport_h, cx + key_col,
                         y + (float)(int)((key_h - row_h) * 0.5f), row_s, rw->what,
                         TEXT_R, TEXT_G, TEXT_B, a);
            y += key_h + row_gap;
        }
        if (s + 1 < g_section_count) y += sec_gap;
    }

    y += 10.0f;
    ui_text_draw(viewport_w, viewport_h, cx, y, label_s, g_footer,
                 MUTED_R, MUTED_G, MUTED_B, 0.9f * a);
}
