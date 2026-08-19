#include "menu.h"
#include "ui_text.h"
#include "../engine/input.h"
#include "../engine/renderer.h"
#include <stdio.h>

/* Level routing table.
 *
 * The first ten entries are the faithful CMainMenu order from
 * docs/decomp/CMainMenu.md (.rdata:004ec71c): New Game, then the eight VR
 * challenges in their specced sequence, then Quit. That is what the original
 * front-end offers and it is not reordered here.
 *
 * Everything after the FAITHFUL_COUNT marker is a QA convenience this port
 * adds: the rest of the story levels, which the original reaches through play
 * and --level reaches directly. They are listed after the faithful set so the
 * specced routing stays first and recognisable. The list scrolls because it no
 * longer fits on screen.
 */
typedef struct {
    const char *label;
    const char *level;      /* normalized level to load, NULL for Quit */
    int         is_newgame;
    int         is_quit;
} MenuItem;

static const MenuItem g_items[] = {
    /* --- faithful CMainMenu routing ------------------------------------ */
    { "New Game", "level1b", 1, 0 },
    { "VR 01",    "vr01",    0, 0 },
    { "VR 03",    "vr03",    0, 0 },
    { "VR 02",    "vr02",    0, 0 },
    { "VR 08",    "vr08",    0, 0 },
    { "VR 06",    "vr06",    0, 0 },
    { "VR 07",    "vr07",    0, 0 },
    { "VR 05",    "vr05",    0, 0 },
    { "VR 04",    "vr04",    0, 0 },
    /* --- added: the story levels, for QA builds ------------------------ */
    { "Level 1",   "level1",   0, 0 },
    { "Level 1A",  "level1a",  0, 0 },
    { "Level 1B",  "level1b",  0, 0 },
    { "Level 1C",  "level1c",  0, 0 },
    { "Level 1D",  "level1d",  0, 0 },
    { "Level 1E",  "level1e",  0, 0 },
    { "Level 1F",  "level1f",  0, 0 },
    { "Level 2",   "level2",   0, 0 },
    { "Level 2A",  "level2a",  0, 0 },
    { "Level 2B",  "level2b",  0, 0 },
    { "Level 3",   "level3",   0, 0 },
    { "Level 3A",  "level3a",  0, 0 },
    { "Level 3C",  "level3c",  0, 0 },
    { "Level 3D",  "level3d",  0, 0 },
    { "Level 4",   "level4",   0, 0 },
    { "Level 4A",  "level4a",  0, 0 },
    { "Level 4B",  "level4b",  0, 0 },
    { "Level 4C",  "level4c",  0, 0 },
    { "Level 4D",  "level4d",  0, 0 },
    { "Level 5",   "level5",   0, 0 },
    { "Level 5A",  "level5a",  0, 0 },
    { "Level 5B",  "level5b",  0, 0 },
    { "Level 6",   "level6",   0, 0 },
    { "Level 6A",  "level6a",  0, 0 },
    { "Level 7",   "level7",   0, 0 },
    /* --- always last ---------------------------------------------------- */
    { "Quit",     NULL,      0, 1 },
};
static const int g_item_count = (int)(sizeof(g_items) / sizeof(g_items[0]));

/* How many rows fit on screen at once. The rest scroll. */
#define MENU_WINDOW 9

static int g_active = 0;
static int g_sel    = 0;
static int g_top    = 0;   /* index of the first visible row */

static void clamp_window(void) {
    if (g_sel < g_top)                    g_top = g_sel;
    if (g_sel >= g_top + MENU_WINDOW)     g_top = g_sel - MENU_WINDOW + 1;
    int max_top = g_item_count - MENU_WINDOW;
    if (max_top < 0) max_top = 0;
    if (g_top > max_top) g_top = max_top;
    if (g_top < 0)       g_top = 0;
}

void menu_open(void) {
    g_active = 1;
    g_sel = 0;
    g_top = 0;
    printf("[MAINMENU] open (%d items, window %d); selection='%s'\n",
           g_item_count, MENU_WINDOW, g_items[0].label);
}

void menu_close(void) { g_active = 0; }
int  menu_active(void) { return g_active; }

void menu_input(void) {
    if (!g_active) return;
    int moved = 0;

    if (input_just_pressed(SDL_SCANCODE_UP) || input_just_pressed(SDL_SCANCODE_W)) {
        g_sel = (g_sel - 1 + g_item_count) % g_item_count;
        moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_DOWN) || input_just_pressed(SDL_SCANCODE_S)) {
        g_sel = (g_sel + 1) % g_item_count;
        moved = 1;
    }
    /* Page a screenful at a time - the list is long enough to want it. */
    if (input_just_pressed(SDL_SCANCODE_PAGEUP) || input_just_pressed(SDL_SCANCODE_LEFT)) {
        g_sel -= MENU_WINDOW;
        if (g_sel < 0) g_sel = 0;
        moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_PAGEDOWN) || input_just_pressed(SDL_SCANCODE_RIGHT)) {
        g_sel += MENU_WINDOW;
        if (g_sel >= g_item_count) g_sel = g_item_count - 1;
        moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_HOME)) { g_sel = 0; moved = 1; }
    if (input_just_pressed(SDL_SCANCODE_END))  { g_sel = g_item_count - 1; moved = 1; }

    if (moved) {
        clamp_window();
        printf("[MAINMENU] selection='%s' (%d of %d)\n",
               g_items[g_sel].label, g_sel + 1, g_item_count);
    }
}

int menu_take_confirm(const char **level_out, int *is_newgame_out) {
    if (!g_active) return 0;
    if (!input_just_pressed(SDL_SCANCODE_RETURN) &&
        !input_just_pressed(SDL_SCANCODE_KP_ENTER) &&
        !input_just_pressed(SDL_SCANCODE_SPACE))
        return 0;
    const MenuItem *it = &g_items[g_sel];
    printf("[MAINMENU] activate '%s' -> %s%s\n", it->label,
           it->level ? it->level : "(quit)",
           it->is_newgame ? " (NewGame task)" : "");
    if (level_out)      *level_out = it->level;
    if (is_newgame_out) *is_newgame_out = it->is_newgame;
    return 1;
}

void menu_current(const char **level_out, int *is_newgame_out) {
    const MenuItem *it = &g_items[g_sel];
    if (level_out)      *level_out = it->level;
    if (is_newgame_out) *is_newgame_out = it->is_newgame;
}

void menu_draw(int viewport_w, int viewport_h) {
    if (!g_active) return;
    renderer_begin_overlay(viewport_w, viewport_h);

    renderer_draw_screen_rect(viewport_w, viewport_h,
                              0, 0, (float)viewport_w, (float)viewport_h,
                              0.02f, 0.04f, 0.10f, 0.55f);

    int shown = g_item_count < MENU_WINDOW ? g_item_count : MENU_WINDOW;
    float bw = viewport_w * 0.40f;
    float bh = 34.0f;
    float gap = 12.0f;
    float total = shown * (bh + gap) - gap;
    float x = (viewport_w - bw) * 0.5f;
    float y = (viewport_h - total) * 0.5f;

    const float text_scale = 2.0f;
    const float text_h = ui_text_line_height(text_scale);

    /* Title band. */
    renderer_draw_screen_rect(viewport_w, viewport_h,
                              x, y - bh - gap, bw, bh,
                              0.85f, 0.70f, 0.15f, 0.9f);
    ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                          y - bh - gap + (bh - text_h) * 0.5f,
                          text_scale, "JIMMY NEUTRON",
                          0.08f, 0.05f, 0.02f, 1.0f);

    for (int row = 0; row < shown; row++) {
        int i = g_top + row;
        if (i >= g_item_count) break;
        int on = (i == g_sel);
        float r = on ? 0.95f : 0.20f;
        float g = on ? 0.80f : 0.24f;
        float b = on ? 0.25f : 0.34f;
        float a = on ? 0.95f : 0.70f;
        float row_y = y + row * (bh + gap);
        renderer_draw_screen_rect(viewport_w, viewport_h, x, row_y, bw, bh, r, g, b, a);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              row_y + (bh - text_h) * 0.5f, text_scale, g_items[i].label,
                              on ? 0.08f : 0.95f,
                              on ? 0.05f : 0.95f,
                              on ? 0.02f : 1.00f, 1.0f);
        if (on)
            renderer_draw_screen_rect(viewport_w, viewport_h,
                                      x - bh - gap, row_y, bh, bh,
                                      0.95f, 0.80f, 0.25f, 0.95f);
    }

    /* Scrollbar - only when there is something to scroll. */
    if (g_item_count > MENU_WINDOW) {
        float track_x = x + bw + gap;
        float track_w = 8.0f;
        renderer_draw_screen_rect(viewport_w, viewport_h,
                                  track_x, y, track_w, total,
                                  0.16f, 0.19f, 0.26f, 0.85f);
        float frac = (float)MENU_WINDOW / (float)g_item_count;
        float thumb_h = total * frac;
        if (thumb_h < 22.0f) thumb_h = 22.0f;
        float max_top = (float)(g_item_count - MENU_WINDOW);
        float t = max_top > 0.0f ? (float)g_top / max_top : 0.0f;
        renderer_draw_screen_rect(viewport_w, viewport_h,
                                  track_x, y + t * (total - thumb_h), track_w, thumb_h,
                                  0.95f, 0.80f, 0.25f, 0.95f);

        /* Position readout and a hint, below the list. */
        char pos[48];
        snprintf(pos, sizeof(pos), "%d of %d", g_sel + 1, g_item_count);
        float hint_h = ui_text_line_height(1.3f);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              y + total + gap, 1.3f, pos,
                              0.72f, 0.78f, 0.88f, 1.0f);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              y + total + gap + hint_h + 5.0f, 1.3f,
                              "Up Down to move   PgUp PgDn to page   Enter to play",
                              0.52f, 0.58f, 0.68f, 1.0f);
    }
}
