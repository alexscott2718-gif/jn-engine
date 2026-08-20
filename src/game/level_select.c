#include "level_select.h"
#include "ui_text.h"
#include "../engine/input.h"
#include "../engine/renderer.h"
#include <stdio.h>

/* Every level the engine can load. Order is the natural play order, not a
   spec: nothing here is certified, and nothing here should be -- see the
   header for why this list is not in menu.c. */
typedef struct { const char *label; const char *level; } LevelRow;

static const LevelRow g_rows[] = {
    { "Level 1",   "level1"  }, { "Level 1A",  "level1a" },
    { "Level 1B",  "level1b" }, { "Level 1C",  "level1c" },
    { "Level 1D",  "level1d" }, { "Level 1E",  "level1e" },
    { "Level 1F",  "level1f" }, { "Level 2",   "level2"  },
    { "Level 2A",  "level2a" }, { "Level 2B",  "level2b" },
    { "Level 3",   "level3"  }, { "Level 3A",  "level3a" },
    { "Level 3C",  "level3c" }, { "Level 3D",  "level3d" },
    { "Level 4",   "level4"  }, { "Level 4A",  "level4a" },
    { "Level 4B",  "level4b" }, { "Level 4C",  "level4c" },
    { "Level 4D",  "level4d" }, { "Level 5",   "level5"  },
    { "Level 5A",  "level5a" }, { "Level 5B",  "level5b" },
    { "Level 6",   "level6"  }, { "Level 6A",  "level6a" },
    { "Level 7",   "level7"  },
    { "VR 01",     "vr01"    }, { "VR 02",     "vr02"    },
    { "VR 03",     "vr03"    }, { "VR 04",     "vr04"    },
    { "VR 05",     "vr05"    }, { "VR 06",     "vr06"    },
    { "VR 07",     "vr07"    }, { "VR 08",     "vr08"    },
};
static const int g_count = (int)(sizeof(g_rows) / sizeof(g_rows[0]));

/* Rows visible at once; the rest scroll. */
#define LS_WINDOW 9

static int g_active = 0;
static int g_sel    = 0;
static int g_top    = 0;

static void clamp_window(void) {
    if (g_sel < g_top)                g_top = g_sel;
    if (g_sel >= g_top + LS_WINDOW)   g_top = g_sel - LS_WINDOW + 1;
    int max_top = g_count - LS_WINDOW;
    if (max_top < 0) max_top = 0;
    if (g_top > max_top) g_top = max_top;
    if (g_top < 0)       g_top = 0;
}

void level_select_open(void) {
    g_active = 1;
    clamp_window();
    printf("[LEVELSEL] open (%d levels, window %d); selection='%s'\n",
           g_count, LS_WINDOW, g_rows[g_sel].label);
}

void level_select_close(void) { g_active = 0; }
int  level_select_active(void) { return g_active; }

void level_select_input(void) {
    if (!g_active) return;
    int moved = 0;

    if (input_just_pressed(SDL_SCANCODE_UP) || input_just_pressed(SDL_SCANCODE_W)) {
        g_sel = (g_sel - 1 + g_count) % g_count; moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_DOWN) || input_just_pressed(SDL_SCANCODE_S)) {
        g_sel = (g_sel + 1) % g_count; moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_PAGEUP) || input_just_pressed(SDL_SCANCODE_LEFT)) {
        g_sel -= LS_WINDOW; if (g_sel < 0) g_sel = 0; moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_PAGEDOWN) || input_just_pressed(SDL_SCANCODE_RIGHT)) {
        g_sel += LS_WINDOW; if (g_sel >= g_count) g_sel = g_count - 1; moved = 1;
    }
    if (input_just_pressed(SDL_SCANCODE_HOME)) { g_sel = 0; moved = 1; }
    if (input_just_pressed(SDL_SCANCODE_END))  { g_sel = g_count - 1; moved = 1; }

    if (moved) {
        clamp_window();
        printf("[LEVELSEL] selection='%s' (%d of %d)\n",
               g_rows[g_sel].label, g_sel + 1, g_count);
    }
}

int level_select_take_confirm(const char **level_out) {
    if (!g_active) return 0;
    if (!input_just_pressed(SDL_SCANCODE_RETURN) &&
        !input_just_pressed(SDL_SCANCODE_KP_ENTER) &&
        !input_just_pressed(SDL_SCANCODE_SPACE))
        return 0;
    printf("[LEVELSEL] load '%s' -> %s\n", g_rows[g_sel].label, g_rows[g_sel].level);
    if (level_out) *level_out = g_rows[g_sel].level;
    return 1;
}

void level_select_draw(int viewport_w, int viewport_h) {
    if (!g_active) return;
    renderer_begin_overlay(viewport_w, viewport_h);

    renderer_draw_screen_rect(viewport_w, viewport_h, 0, 0,
                              (float)viewport_w, (float)viewport_h,
                              0.02f, 0.04f, 0.10f, 0.55f);

    const float text_scale = 2.0f;          /* whole numbers keep glyphs on the
                                               pixel grid -- see ui_text.c */
    const float hint_scale = 1.0f;
    const float text_h = ui_text_line_height(text_scale);

    int shown = g_count < LS_WINDOW ? g_count : LS_WINDOW;
    float bw  = viewport_w * 0.40f;
    float bh  = 34.0f;
    float gap = 12.0f;
    float total = shown * (bh + gap) - gap;
    float x = (float)(int)((viewport_w - bw) * 0.5f);
    float y = (float)(int)((viewport_h - total) * 0.5f);

    /* Title band. */
    renderer_draw_screen_rect(viewport_w, viewport_h, x, y - bh - gap, bw, bh,
                              0.85f, 0.70f, 0.15f, 0.9f);
    ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                          y - bh - gap + (float)(int)((bh - text_h) * 0.5f),
                          text_scale, "LEVEL SELECT",
                          0.08f, 0.05f, 0.02f, 1.0f);

    for (int row = 0; row < shown; row++) {
        int i = g_top + row;
        if (i >= g_count) break;
        int on = (i == g_sel);
        float row_y = y + row * (bh + gap);
        renderer_draw_screen_rect(viewport_w, viewport_h, x, row_y, bw, bh,
                                  on ? 0.95f : 0.20f,
                                  on ? 0.80f : 0.24f,
                                  on ? 0.25f : 0.34f,
                                  on ? 0.95f : 0.70f);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              row_y + (float)(int)((bh - text_h) * 0.5f),
                              text_scale, g_rows[i].label,
                              on ? 0.08f : 0.95f,
                              on ? 0.05f : 0.95f,
                              on ? 0.02f : 1.00f, 1.0f);
        if (on)
            renderer_draw_screen_rect(viewport_w, viewport_h,
                                      x - bh - gap, row_y, bh, bh,
                                      0.95f, 0.80f, 0.25f, 0.95f);
    }

    /* Scrollbar, position readout and key hint. */
    if (g_count > LS_WINDOW) {
        float track_x = x + bw + gap;
        renderer_draw_screen_rect(viewport_w, viewport_h, track_x, y, 8.0f, total,
                                  0.16f, 0.19f, 0.26f, 0.85f);
        float thumb_h = total * ((float)LS_WINDOW / (float)g_count);
        if (thumb_h < 22.0f) thumb_h = 22.0f;
        float max_top = (float)(g_count - LS_WINDOW);
        float t = max_top > 0.0f ? (float)g_top / max_top : 0.0f;
        renderer_draw_screen_rect(viewport_w, viewport_h,
                                  track_x, y + t * (total - thumb_h), 8.0f, thumb_h,
                                  0.95f, 0.80f, 0.25f, 0.95f);

        char pos[48];
        snprintf(pos, sizeof(pos), "%d of %d", g_sel + 1, g_count);
        float hint_h = ui_text_line_height(hint_scale);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              y + total + gap, hint_scale, pos,
                              0.72f, 0.78f, 0.88f, 1.0f);
        ui_text_draw_centered(viewport_w, viewport_h, x + bw * 0.5f,
                              y + total + gap + hint_h + 5.0f, hint_scale,
                              "Up Down to move   PgUp PgDn to page   Enter to play   M to close",
                              0.52f, 0.58f, 0.68f, 1.0f);
    }
}
