#include "menu.h"
#include "../engine/input.h"
#include "../engine/renderer.h"
#include <stdio.h>

/* Level routing table — the executable's level-file menu order
   (docs/decomp/CMainMenu.md): New Game first, then the 8 VR levels in their
   specced menu sequence, then Quit. New Game routes through the NewGame task
   (CTaskList -> level1b); VR items load their .gam directly. */
typedef struct {
    const char *label;     /* item identity (logged; no text renderer yet) */
    const char *level;     /* normalized level to load, NULL for Quit */
    int         is_newgame;
} MenuItem;

static const MenuItem g_items[] = {
    { "New Game", "level1b", 1 },
    { "VR 01",    "vr01",    0 },
    { "VR 03",    "vr03",    0 },
    { "VR 02",    "vr02",    0 },
    { "VR 08",    "vr08",    0 },
    { "VR 06",    "vr06",    0 },
    { "VR 07",    "vr07",    0 },
    { "VR 05",    "vr05",    0 },
    { "VR 04",    "vr04",    0 },
    { "Quit",     NULL,      0 },
};
static const int g_item_count = (int)(sizeof(g_items) / sizeof(g_items[0]));

static int g_active = 0;
static int g_sel    = 0;

void menu_open(void) {
    g_active = 1;
    g_sel = 0;
    printf("[MAINMENU] open (%d items); selection='%s'\n",
           g_item_count, g_items[0].label);
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
    if (moved)
        printf("[MAINMENU] selection='%s'\n", g_items[g_sel].label);
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

/* Current selection without requiring Enter — used by the headless screenshot
   path to auto-confirm New Game so the menu route is exercised in CI. */
void menu_current(const char **level_out, int *is_newgame_out) {
    const MenuItem *it = &g_items[g_sel];
    if (level_out)      *level_out = it->level;
    if (is_newgame_out) *is_newgame_out = it->is_newgame;
}

void menu_draw(int viewport_w, int viewport_h) {
    if (!g_active) return;
    renderer_begin_overlay(viewport_w, viewport_h);

    /* Dim the live scene behind the menu. */
    renderer_draw_screen_rect(viewport_w, viewport_h,
                              0, 0, (float)viewport_w, (float)viewport_h,
                              0.02f, 0.04f, 0.10f, 0.55f);

    /* A column of selectable bars; the current selection glows. With no text
       renderer yet, the highlighted bar + console log identify the item. */
    float bw = viewport_w * 0.40f;
    float bh = 34.0f;
    float gap = 12.0f;
    float total = g_item_count * (bh + gap) - gap;
    float x = (viewport_w - bw) * 0.5f;
    float y = (viewport_h - total) * 0.5f;

    /* Title band. */
    renderer_draw_screen_rect(viewport_w, viewport_h,
                              x, y - bh - gap, bw, bh,
                              0.85f, 0.70f, 0.15f, 0.9f);

    for (int i = 0; i < g_item_count; i++) {
        int on = (i == g_sel);
        float r = on ? 0.95f : 0.20f;
        float g = on ? 0.80f : 0.24f;
        float b = on ? 0.25f : 0.34f;
        float a = on ? 0.95f : 0.70f;
        renderer_draw_screen_rect(viewport_w, viewport_h,
                                  x, y + i * (bh + gap), bw, bh, r, g, b, a);
        if (on)  /* selection pip on the left */
            renderer_draw_screen_rect(viewport_w, viewport_h,
                                      x - bh - gap, y + i * (bh + gap), bh, bh,
                                      0.95f, 0.80f, 0.25f, 0.95f);
    }
}
